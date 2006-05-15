# Copyright (C) 2004,2005 by SICEm S.L. and Imendio AB
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

import gettext

import gobject
import gtk

from gazpacho import util
from gazpacho.properties import prop_registry, CustomProperty, \
     PropertyCustomEditor, TransparentProperty, StringType, \
     PropType
from gazpacho.widget import Widget, load_widget_from_gtk_widget
from gazpacho.widgets.base.base import ContainerAdaptor
from gazpacho.editor import EditorPropertyText
_ = gettext.gettext

(COL_STOCK_ID,
 COL_STOCK_LABEL) = range(2)


# Adaptors for Button
class ButtonContentsEditor(PropertyCustomEditor):
    translatable = False
    wide_editor = True
    
    def __init__(self):
        self.loading = False
        self.gwidget = None
        self.application_window = None
        self.input = self.create()

    def get_editor_widget(self):
        return self.input
    
    def _create_stock_list(self):
        "Create the list with the stock icons"
        liststore = gtk.ListStore(str, str)
        stocks = gtk.stock_list_ids()
        stocks.sort()
        for stock in stocks:
            info = gtk.stock_lookup(stock)
            if info is None:
                label = "* %s" % stock
            else:
                label = info[1]
            liststore.append((stock, label.replace('_', '')))

        treeview = gtk.TreeView(liststore)
        treeview.set_headers_visible(False)
        column = gtk.TreeViewColumn()
        cell1 = gtk.CellRendererPixbuf()
        column.pack_start(cell1)
        column.add_attribute(cell1, 'stock-id', COL_STOCK_ID)
        cell2 = gtk.CellRendererText()
        column.pack_start(cell2)
        column.add_attribute(cell2, 'text', COL_STOCK_LABEL)
        treeview.append_column(column)
        treeview.set_search_column(COL_STOCK_LABEL)
        treeview.set_enable_search(True)

        self.stock_list = treeview

        treeview.get_selection().connect('changed',
                                         self._on_stock_list_selection_changed)

        sw = gtk.ScrolledWindow()
        sw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        sw.set_shadow_type(gtk.SHADOW_IN)
        sw.add(treeview)

        return sw

    def _create_check_entry_button(self, table, row, check_text, button_text,
                                   entry_handler, check_handler,
                                   button_handler):
        check = gtk.CheckButton(check_text)
        table.attach(check, 0, 1, row, row + 1,
                     xoptions=gtk.FILL, yoptions=gtk.FILL)
        hbox = gtk.HBox()
        entry = gtk.Entry()
        entry.connect('changed', entry_handler)
        hbox.pack_start(entry)
        button = gtk.Button(button_text)
        button.connect('clicked', button_handler)
        hbox.pack_start(button, False, False)
        table.attach(hbox, 1, 2, row, row + 1, xoptions=gtk.EXPAND|gtk.FILL,
                     yoptions=gtk.EXPAND|gtk.FILL)
        check.connect('toggled', check_handler)
        return check, entry, button
        
    def _create_label(self, table):
        "Create the widgets needed to set the label of a button"
        c, e, b = self._create_check_entry_button(table, 0, _('Label:'), '...',
                                                  self._on_entry_label_changed,
                                                  self._on_check_label_toggled,
                                                  self._on_button_label_clicked)
        self.check_label, self.entry_label, self.button_label = c, e, b


    def _create_image(self, table):
        "Create the widgets needed to set the image of a button"
        c, e, b = self._create_check_entry_button(table, 1, _('Image:'), '...',
                                                  self._on_entry_image_changed,
                                                  self._on_check_image_toggled,
                                                  self._on_button_image_clicked)
        self.check_image, self.entry_image, self.button_image = c, e, b


    def _create_position(self, table):
        """Create the widgets needed to set the combo with the position
        of the image"""
        label = gtk.Label(('Image position:'))
        label.set_sensitive(False)
        table.attach(label, 0, 1, 2, 3, xoptions=gtk.FILL, yoptions=gtk.FILL)

        liststore = gtk.ListStore(str, int)
        liststore.append((_('Left'), gtk.POS_LEFT))
        liststore.append((_('Right'), gtk.POS_RIGHT))
        liststore.append((_('Top'), gtk.POS_TOP))
        liststore.append((_('Bottom'), gtk.POS_BOTTOM))
        combo = gtk.ComboBox(liststore)
        renderer = gtk.CellRendererText()
        combo.pack_start(renderer)
        combo.add_attribute(renderer, 'text', 0)

        combo.set_sensitive(False)
        table.attach(combo, 1, 2, 2, 3, xoptions=gtk.EXPAND|gtk.FILL,
                     yoptions=gtk.EXPAND|gtk.FILL)

        combo.connect('changed', self._on_combo_position_changed)
        self.combo_position = combo
        self.label_position = label

    def _create_stock_widgets(self):
        "Create the widgets needed to setup a stock button"
        alignment = gtk.Alignment(1.0, 0.0, 1.0, 1.0)
        alignment.set_padding(0, 12, 48, 0)
        vbox = gtk.VBox()
        vbox.pack_start(self._create_stock_list())
        self.check_notext = gtk.CheckButton(_('No text'))
        self.check_notext.connect('toggled', self._on_check_notext_toggled)
        vbox.pack_start(self.check_notext, False, False)
        alignment.add(vbox)
        return alignment

    def _create_custom_widgets(self):
        "Create the widgets needed to setup a custom button"
        alignment = gtk.Alignment(1.0, 0.0, 1.0, 1.0)
        alignment.set_padding(0, 12, 48, 0)
        table = gtk.Table(rows=3, columns=2)
        table.set_col_spacings(6)
        table.set_row_spacings(2)
        
        self._create_label(table)

        self._create_image(table)
        
        self._create_position(table)
        
        alignment.add(table)

        return alignment
    
    def create(self):
        "Create the whole editor with all the widgets inside"
        
        mainvbox = gtk.VBox()

        # widgets to setup a custom button
        self.radio_custom = gtk.RadioButton(None, _('Use custom button:'))
        mainvbox.pack_start(self.radio_custom, False, False)

        self.custom_widgets = self._create_custom_widgets()
        mainvbox.pack_start(self.custom_widgets, True, True)

        # widgets to setup a stock button
        self.radio_stock = gtk.RadioButton(self.radio_custom,
                                           _('Use stock button:'))
        self.radio_stock.connect('toggled', self._on_radio_toggled)
        mainvbox.pack_start(self.radio_stock, False, False)
        self.stock_widgets = self._create_stock_widgets()
        mainvbox.pack_start(self.stock_widgets, True, True)

        # use an extra alignment to add some padding to the top
        al = gtk.Alignment(0.0, 0.0, 1.0, 1.0)
        al.set_padding(12, 0, 0, 0)
        al.add(mainvbox)
        al.show_all()
        return al

    def _on_radio_toggled(self, button):
        if self.loading: return
        
        # this is emitted when the stock radio is toggled
        value = self.radio_stock.get_active()
        self.stock_widgets.set_sensitive(value)
        self.custom_widgets.set_sensitive(not value)

        if value:
            # if there is nothing selected in the icon list select the first
            # item
            selection = self.stock_list.get_selection()
            model, sel_iter = selection.get_selected()
            if not sel_iter:
                # this is calling set_stock as a side effect
                selection.select_iter(model.get_iter_first())
            else:
                self.set_stock()
        else:
            self.set_custom()

    def _on_check_label_toggled(self, button):
        if self.loading: return
        
        value = self.check_label.get_active()
        self.entry_label.set_sensitive(value)
        self.button_label.set_sensitive(value)
        self._set_position_sensitive(value and self.check_image.get_active())

        self.set_custom()
        
    def _on_check_image_toggled(self, button):
        if self.loading: return
        
        value = self.check_image.get_active()
        self.entry_image.set_sensitive(value)
        self.button_image.set_sensitive(value)
        self._set_position_sensitive(value and self.check_label.get_active())

        self.set_custom()

    def _set_position_sensitive(self, value):
        self.label_position.set_sensitive(value)
        self.combo_position.set_sensitive(value)

    def _on_stock_list_selection_changed(self, selection):
        if not self.loading:
            self.set_stock()

    def _on_check_notext_toggled(self, button):
        if not self.loading:
            self.set_stock()
        
    def _on_entry_label_changed(self, entry):
        if not self.loading:
            self.set_custom()

    def _on_entry_image_changed(self, entry):
        if not self.loading:
            self.set_custom()

    def _on_button_label_clicked(self, button):
        pass
    
    def _on_button_image_clicked(self, button):
         dialog = gtk.FileChooserDialog(_('Open image ...'),
                                        self.application_window,
                                        gtk.FILE_CHOOSER_ACTION_OPEN,
                                        (gtk.STOCK_CANCEL,
                                         gtk.RESPONSE_CANCEL,
                                         gtk.STOCK_OPEN,
                                         gtk.RESPONSE_OK))
         file_filter = gtk.FileFilter()
         file_filter.set_name("Images")
         file_filter.add_mime_type("image/png")
         file_filter.add_mime_type("image/jpeg")
         file_filter.add_mime_type("image/gif")
         file_filter.add_mime_type("image/x-xpixmap")
         file_filter.add_pattern("*.png")
         file_filter.add_pattern("*.jpg")
         file_filter.add_pattern("*.gif")
         file_filter.add_pattern("*.xpm")

         dialog.add_filter(file_filter)
    
         response = dialog.run()
         filename = dialog.get_filename()
         if response == gtk.RESPONSE_OK and filename:
             self.entry_image.set_text(filename)

         dialog.destroy()

    def _on_combo_position_changed(self, combo):
        if not self.loading:
            self.set_custom()
            
    def set_stock(self):
        if not self.gwidget:
            return
        
        model, sel_iter = self.stock_list.get_selection().get_selected()
        if sel_iter:
            stock_id = model[sel_iter][COL_STOCK_ID]
        else:
            stock_id = None

        notext = self.check_notext.get_active()

        self.command_manager.set_button_contents(self.gwidget,
                                                 stock_id=stock_id,
                                                 notext=notext)

    def set_custom(self):
        if not self.gwidget:
            return
        
        label = None
        if self.check_label.get_active():
            label = self.entry_label.get_text() or None

        image = None
        if self.check_image.get_active():
            image = self.entry_image.get_text() or None
            
        position = None
        ac_iter = self.combo_position.get_active_iter()
        if ac_iter:
            model = self.combo_position.get_model()
            position = model[ac_iter][1]
            
        self.command_manager.set_button_contents(self.gwidget,
                                                 label=label,
                                                 image_path=image,
                                                 position=position)
    
        
    def _set_combo_active_position(self, pos):
        for row in self.combo_position.get_model():
            if row[1] == pos:
                self.combo_position.set_active_iter(row.iter)
                break

    def _set_stock_list_active_stock(self, stock_id):
        selection = self.stock_list.get_selection()
        for row in self.stock_list.get_model():
            if row[COL_STOCK_ID] == stock_id:
                selection.select_iter(row.iter)
                self.stock_list.scroll_to_cell(row.path, None,
                                               True, 0.5, 0.0)
                break
            
    def update(self, context, gtk_widget, proxy):
        self.command_manager = context.get_command_manager()
        self.gwidget = Widget.from_widget(gtk_widget)
        self.application_window = context.get_application_window()

        self.loading = True
        
        # default values for some widgets
        self.check_notext.set_active(False) 
        self.check_label.set_active(True)
        self.entry_label.set_text("")
        self.check_image.set_active(False)
        self.entry_image.set_text("")
        self.combo_position.set_active(0)
        self.combo_position.set_sensitive(False)

        (stock_id, notext, label,
         image_path, position) = util.get_button_state(gtk_widget)

        if stock_id:
            self.stock_widgets.set_sensitive(True)
            self.custom_widgets.set_sensitive(False)
            self.radio_stock.set_active(True)
            
            self._set_stock_list_active_stock(stock_id)
            self.check_notext.set_active(notext)

        else:
            self.stock_widgets.set_sensitive(False)
            self.custom_widgets.set_sensitive(True)
            self.radio_custom.set_active(True)
            
            self.check_label.set_active(True)
            self.entry_label.set_sensitive(True)
            self.button_label.set_sensitive(True)
            if label is not None:
                self.entry_label.set_text(label)

            if image_path is not None:
                self.check_image.set_active(True)
                self.entry_image.set_sensitive(True)
                self.entry_image.set_text(image_path)
            else:
                self.entry_image.set_sensitive(False)
                self.button_image.set_sensitive(False)

            if label and image_path:
                self.combo_position.set_sensitive(True)
                self._set_combo_active_position(position)
            else:
                self.combo_position.set_sensitive(False)

        self.loading = False

class ButtonContentsAdaptor(TransparentProperty, StringType):
    """This adaptor allow the user the choose between a stock button and
    a custom button.

    A stock button is a button with a stock-icon. The text is optional
    A custom button allow the user to add a label, an image and the position
    of the image with respect to the label.
    """
    editor = ButtonContentsEditor

    def get(self):
        return util.get_button_state(self._object)

    def save(self):
        return None

prop_registry.override_property('GtkButton::contents', ButtonContentsAdaptor)

class LabelAdaptor(CustomProperty, StringType):

    editable = False

    def is_translatable(self):
        # If a stock id is set, do not make it translatable,
        # because we don't want to translate stock-id labels!
        stock_id = util.get_button_state(self._object)[0]
        if stock_id:
            return False

        return False
#        return True

    def save(self):
        (stock_id, notext, label,
         image_path, position) = util.get_button_state(self._object)
        if ((stock_id and notext) 
            or (not label and image_path)
            or (label and image_path)):
            return
        else:
            
            return self._object.get_property('label')
        
prop_registry.override_property('GtkButton::label', LabelAdaptor)

class ButtonAdaptor(ContainerAdaptor):
    def save(self, context, gwidget):
        """Create GazpachoWidgets for the possible children of
        this widget so they are saved with the button"""
        gtk_button = gwidget.gtk_widget
        
        (stock_id, notext, label,
         image_path, position) = util.get_button_state(gtk_button)

        child = gtk_button.get_child()
        project = context.get_project()

        if ((stock_id and notext)         # case 1: stock item without text
            or (not label and image_path) # case 2: only image
            or (label and image_path)):   # case 3: text and image

            self._create_names_for_internal_widgets(child, project)
            load_widget_from_gtk_widget(child, project)
            
            # case 4: stock item with text (nothing to do)
            # case 5: only text (nothing to do)

        # FIXME: Under some circumstances we end up with a GtkAlignment
        #        when we don't really need it.
        #        Find out when it's not needed and remove it

    def _create_names_for_internal_widgets(self, gtk_widget, project):
        class_name = gobject.type_name(gtk_widget)
        name = project.new_widget_name(class_name)
        gtk_widget.set_name(name)
        if isinstance(gtk_widget, gtk.Container):
            for child in gtk_widget.get_children():
                self._create_names_for_internal_widgets(child, project)

# GtkButton
prop_registry.override_simple('GtkButton::use-stock', editable=False)
#prop_registry.override_property('GtkButton::response-id', default="0", name="Response ID")

# GtkToggleButton
prop_registry.override_simple('GtkToggleButton::label', default="toggle button")

# GtkCheckButton
prop_registry.override_simple('GtkCheckButton::draw-indicator', default=True)
prop_registry.override_simple('GtkCheckButton::label', default="toggle button")

# GtkRadioButton
class RadioGroupProp(PropType):
    editable = True
    readable = True

    # We need to override get_default, since
    # the standard mechanism to get the initial value
    # is calling get_property, but ::group is not marked as readable
    def get_default(self, gobj):
        return
    
    def get(self):
        group_name = ''
        for group in self._object.get_group():
            if group.get_active():
                group_name = group.get_name()
        return group_name
    
    def set(self, group):
        if not group in self._object.get_group():
            self._object.set_property('group', group)

    def save(self):
        value = self.get()
        if value == self._object.get_name():
            return
        return value
        
prop_registry.override_property('GtkRadioButton::group', RadioGroupProp)
