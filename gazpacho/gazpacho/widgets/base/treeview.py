# Copyright (C) 2005 by SICEm S.L. and Async Open Source
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

import gtk
import gobject
import pango

from gazpacho.properties import prop_registry, TransparentProperty, PropertyCustomEditor, CustomProperty, StringType
from gazpacho.widgets.base.base import ContainerAdaptor
from gazpacho.celllayouteditor import LayoutEditor

class TreeViewAdaptor(ContainerAdaptor):

    def fill_empty(self, context, widget):
        pass

# Disable headers-clickable, see bug #163851
prop_registry.override_simple('GtkTreeView::headers-clickable', disabled=True)

# A little trinket to change the column
# TODO:
#       - allow dnd
#       - drop scale inheritance, it brings conflicting semantics
class BlockScale(gtk.Scale):
    def __init__(self):
        gtk.Scale.__init__(self)
    
    def do_button_press_event(self, event):
        width = self.allocation.width
        adj = self.get_adjustment()
        items = int(adj.upper)
        item_width = int(width / items)
        x = 0
        for i in range(int(adj.lower), int(adj.upper)+1):
            if event.x < x + item_width:
                self.set_value(i)
                break
            x = x + item_width
        
    
    def do_expose_event(self, event):
        gc = self.window.new_gc()
        x = self.allocation.x + 2
        y = self.allocation.y + 2
        width = self.allocation.width - 4
        height = self.allocation.height - 4
        
        adj = self.get_adjustment()
        items = int(adj.upper)
        current = self.get_value()
        
        item_width = int(width / items)

        pad = width - item_width * items

        if pad > 1:
          pad = pad / 2

        for i in range(int(adj.lower), int(adj.upper)+1):

            if self.state == gtk.STATE_INSENSITIVE:
                state = gtk.STATE_INSENSITIVE
            elif i == current:
                state = gtk.STATE_SELECTED
            else:
                state = gtk.STATE_NORMAL

            gc.foreground = self.style.mid[state]
            self.window.draw_rectangle(gc, True, pad+x, y, item_width-1, height-1)
            gc.foreground = self.style.dark[state]
            self.window.draw_rectangle(gc, False, pad+x, y, item_width, height)

            layout = pango.Layout(self.get_pango_context())
            layout.set_text("Column %i" % i)
            (lw, lh) = layout.get_size()

            if int(lw / pango.SCALE) > item_width:
              layout.set_text("%i" % i)
              (lw, lh) = layout.get_size()

            gc.foreground = self.style.fg[state]
            self.window.draw_layout(gc,
                    pad + x + int((item_width/2) - int((lw / pango.SCALE) / 2)),
                    y + int(height/2) - (int((lh / pango.SCALE) / 2)),
                    layout)
            
            x = x + item_width
            
        return 0

gobject.type_register(BlockScale)

# The celllayout editor adapted to treeview
class TreeLayoutEditor(LayoutEditor):
    view = None
    current_column = 0

    # Chains up to the base editor class
    # Used also when the column is changed
    def update(self, context, gtk_widget, proxy):
        self.view = gtk_widget

        columns = gtk_widget.get_columns()

        if len(columns) > 0:
            for column in columns:
                super (TreeLayoutEditor, self).update(context,
                                                      column,
                                                      None)
                self.title_entry.set_text(self.layoutproxy.get_title())
        else:
            super (TreeLayoutEditor, self).update(context,
                                                  gtk.TreeViewColumn(),
                                                  None)
            gtk_widget.append_column(self.edited_widget)

        gtk_widget.set_headers_visible(True)
        self.expand_checkbutton.set_active(
                                      self.edited_widget.get_property("expand"))
        self.current_column = len(columns)
        self.update_scale(len(columns))
        self.scale.set_value(self.current_column)
        
    # The base class assumes the model is in the edited_widget
    def get_model_name(self):
        return self.view.get_data("gazpacho::named_treemodel")

    def set_model_name(self, name):
        self.view.set_data("gazpacho::named_treemodel", name)

    def get_model(self):
        return self.view.get_model()

    def set_model(self, model):
        self.view.set_model(model)
        self.project.changed = True

    # Add the treeview-specific bits
    def _create_widgets(self):
        super(TreeLayoutEditor, self)._create_widgets()

        hbox = gtk.HBox()

        self.scale = BlockScale()
        self.scale.set_digits(0)
        self.scale.set_range(1,2)
        self.scale.set_value(1)
        self.scale.set_sensitive(False)
        self.scale.connect("value-changed", self.on_value_changed)
        
        hbox.pack_start(self.scale, True, True, 0)
        
        vbbox = gtk.VButtonBox()
        hbox.pack_end(vbbox, False, False, 6)
        
        button = gtk.Button(stock=gtk.STOCK_ADD)
        button.connect("clicked", self.add_button_clicked)
        vbbox.add(button)
        self.add_button = button

        button = gtk.Button(stock=gtk.STOCK_REMOVE)
        button.set_sensitive(False)
        button.connect("clicked", self.remove_button_clicked)
        vbbox.add(button)
        self.remove_button = button

        self.expand_checkbutton = self.ob.get_widget("expand_checkbutton")
        self.expand_checkbutton.connect("toggled", self.expand_toggled)

        hbox.show_all()
        self.editor.add(hbox)
        self.editor.reorder_child(hbox, 0)
    
    def set_ui_visibility(self):
        self.title_box.show()

# Callbacks 
        
    def add_button_clicked(self, button):
        self.add_column()

    def remove_button_clicked(self, button):
        self.remove_column()

    def expand_toggled(self, button):
        self.edited_widget.set_property("expand", self.expand_checkbutton.get_active())
        self.project.changed = True

    def on_value_changed(self, scale):
        columns = self.view.get_columns()
        if columns is not None:
            layout = scale.get_layout()
            text = layout.get_text()
            layout.set_text("Column %s" % text)
            self.current_column = int(scale.get_value())-1
            super (TreeLayoutEditor, self).update(self.context,
                                                    columns[self.current_column],
                                                    None)
            self.title_entry.set_text(self.layoutproxy.get_title())
            self.expand_checkbutton.set_active(self.edited_widget.get_property("expand"))

# Utility
        
    def prev_column(self):
        if self.current_column > 0:
            columns = self.view.get_columns()
            if columns is not None:
                self.current_column = self.current_column - 1
                super (TreeLayoutEditor, self).update(self.context,
                                                        columns[self.current_column],
                                                        None)
                self.title_entry.set_text(self.layoutproxy.get_title())

 
    def add_column(self):
        super (TreeLayoutEditor, self).update(self.context,
                                                gtk.TreeViewColumn(),
                                                None)
        self.view.append_column(self.edited_widget)
        columns = len(self.view.get_columns())
        self.current_column = columns
        self.layoutproxy.set_title("Column %i" % self.current_column)
        self.title_entry.set_text("Column %i" % self.current_column)

        self.update_scale(columns)
        self.project.changed = True

    def remove_column(self):
        self.view.remove_column(self.edited_widget)
        column_list = self.view.get_columns()
        columns = len(column_list)
        if self.current_column >= columns:
            self.current_column = columns - 1
        if self.current_column == -1:
            self.add_column()
            return
        super (TreeLayoutEditor, self).update(self.context,
                                                column_list[self.current_column],
                                                None)
        self.edited_widget = column_list[self.current_column]
        self.title_entry.set_text(self.layoutproxy.get_title())

        self.update_scale(columns)
        self.project.changed = True
        
    def next_column(self):
        columns = self.view.get_columns()
        if self.current_column < len(columns)-1:
            self.current_column = self.current_column + 1
            self.edited_widget = columns[self.current_column]
            super (TreeLayoutEditor, self).update(self.context,
                                                    columns[self.current_column],
                                                    None)
            self.edited_widget.set_property("expand", self.expand_checkbutton.get_active())
            self.title_entry.set_text(self.layoutproxy.get_title())

    def update_scale(self, columns):
        if columns == 0:
            self.remove_button.set_sensitive(False)
        elif columns == 1:
            self.scale.set_sensitive(False)
            self.scale.set_value(columns)
        else:
            self.scale.set_range(1, columns)
            self.scale.set_value(columns)
            self.scale.set_sensitive(True)
            self.remove_button.set_sensitive(True)


class TreeViewLayoutAdaptor(TransparentProperty):
    editor = TreeLayoutEditor

prop_registry.override_property('GtkTreeView::columns', TreeViewLayoutAdaptor)

class ModelPropertyEditor(PropertyCustomEditor):
    combo = None
    
    def update(self, context, gtk_widget, proxy):
        self.context = context
        self.project = context.get_project()
        self.proxy = proxy
        self.widget = gtk_widget

        names = self.project.treemodel_registry.list_names()
        for name in names:
            treeiter = self.models.append([name])
            if name == self.project.treemodel_registry.lookup_name(self.widget.get_model()):
                self.combo.set_active_iter(treeiter)
            

    def get_editor_widget(self):
        if not self.combo:
            self._create_widgets()
            
        return self.combo
        
    def _create_widgets(self):
        self.combo = gtk.ComboBox()
        
        renderer = gtk.CellRendererText()
        self.combo.pack_start(renderer)
        self.combo.add_attribute(renderer, 'text', 0)
        
        self.combo.connect("changed", self.changed)
        
        self.models = gtk.ListStore(str)
        self.combo.set_model(self.models)
        
        return self.combo

    def changed(self, combo):
        iter = combo.get_active_iter()
        model = self.project.treemodel_registry.lookup_model(self.models[iter][0])
        #self.widget.set_model(model)
        # Emit the updated signal so connected widgets will refresh
        # automatically.
        self.project.treemodel_registry.emit("model-updated",
                                             self.models[iter][0],
                                             model)
 
        
class TreeViewModelAdaptor(CustomProperty, StringType):
    editor = ModelPropertyEditor
    label = "Data model"
    translatable = False
    object = None
    
    def set(self, value):
        pass
        
    def get(self):
        return self.get_project().treemodel_registry.lookup_name(self.object.get_model())

    def save(self):
        name = None
        if self.object:
            model = self.object.get_model()
            name = self.get_project().treemodel_registry.lookup_name()
        return name

prop_registry.override_property('GtkTreeView::model_edit', TreeViewModelAdaptor)

# The CellView
class CellViewAdaptor(ContainerAdaptor):

    def fill_empty(self, context, widget):
        pass

class CellViewLayoutAdaptor(TransparentProperty):
    editor = LayoutEditor

prop_registry.override_property('GtkCellView::columns', CellViewLayoutAdaptor)
