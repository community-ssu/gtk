# Copyright (C) 2004,2005 by SICEm S.L.
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

from gazpacho.popup import Popup
from gazpacho.widget import Widget
from gazpacho.util import select_iter, get_all_children

(WIDGET_COLUMN,
 N_COLUMNS) = range(2)
COLUMN_TYPES = [object]

class WidgetTreeView(gtk.ScrolledWindow):
    def __init__(self, app):
        gtk.ScrolledWindow.__init__(self)

        self._app = app

        self.set_shadow_type(gtk.SHADOW_IN)
        # The selected GladeWidget for this view. Selection should really be a
        # list not GladeWidget since we should support multiple selections.
        self._selected_widget = None
        
        self._project = None

        # We need to know the signal id which we are connected to so that when
        # the project changes we stop listening to the old project
        self._add_widget_signal_id = 0
        
        # The model
        self._model = gtk.TreeStore(*COLUMN_TYPES)
        self._populate_model()

        # The view
        self._tree_view = gtk.TreeView(self._model)
        self._tree_view.set_headers_visible(False)
        self._add_columns()
        
        self._connect_callbacks()

        # True when we are going to set the project selection. So that we don't
        # recurse cause we are also listening for the project changed selection
        # signal
        self._updating_selection = False

        self.add(self._tree_view)

    def _connect_callbacks(self):
        self._tree_view.connect('row-activated', self._row_activated_cb)
        self._tree_view.connect('key-press-event', self._key_press_cb)
        #self._tree_view.connect('motion-notify-event', self._motion_notify_cb)
        self._tree_view.connect('button-press-event', self._button_press_cb)
        self._tree_selection = self._tree_view.get_selection()
        self._tree_selection.connect('changed', self._selection_changed_cb)
        
        self._remove_widget_signal_id = 0
        self._widget_name_changed_signal_id = 0
        self._selection_changed_signal_id = 0
        
    # some properties
    def get_project(self): return self._project
    def set_project(self, value):
        if value == self._project:
            return
        # Thie view stops listening to the old project(if there was one)
        if self._project is not None:
            self._project.disconnect(self._add_widget_signal_id)
            self._project.disconnect(self._remove_widget_signal_id)
            self._project.disconnect(self._widget_name_changed_signal_id)
            self._project.disconnect(self._selection_changed_signal_id)

            # Set to null while we remove all the items from the store, because
            # we are going to trigger selection_changed signal emisions on the
            # View. By setting the project to None, _selection_changed_cb
            # will just return
        self._project = None

        self._model.clear()

        # if we were passed None we are done
        if value is None:
            return

        self._project = value
        self._populate_model()

        # Here we connect to all the signals of the project that interest us
        self._add_widget_signal_id = self._project.connect('add_widget',
                                                           self._add_widget_cb)
        self._remove_widget_signal_id = self._project.connect('remove_widget',
                                                              self._remove_widget_cb)
        self._widget_name_changed_signal_id = self._project.connect('widget_name_changed', self._widget_name_changed_cb)
        self._selection_changed_signal_id = self._project.connect('selection_changed', self._selection_update_cb)
        
    project = property(get_project, set_project)

    def _populate_model(self):
        if not self._project: return

        toplevels = [w for w in self._project.widgets
                           if w.flags() & gtk.TOPLEVEL]
        # add the widgets and recurse
        self._populate_model_real(toplevels, None, True)

    def _populate_model_real(self, widgets, parent_iter, add_children):
        for w in widgets:
            iter = parent_iter
            gw = Widget.from_widget(w)
            if gw:
                iter = self._model.append(parent_iter, (gw,))
            if add_children and isinstance(w, gtk.Container):
                children = get_all_children(w)
                self._populate_model_real(children, iter, True)
                
    def _add_columns(self):
        column = gtk.TreeViewColumn()
        renderer = gtk.CellRendererPixbuf()
        column.pack_start(renderer, False)
        column.set_cell_data_func(renderer, self._view_cell_function, True)

        renderer = gtk.CellRendererText()
        column.pack_start(renderer)
        column.set_cell_data_func(renderer, self._view_cell_function, False)

        self._tree_view.append_column(column)

    def _view_cell_function(self, tree_column, cell, model, iter, is_icon):
        widget = model.get_value(iter, WIDGET_COLUMN)
        if not widget: return

        if is_icon:
            if widget.adaptor.pixbuf:
                cell.set_property('pixbuf', widget.adaptor.pixbuf)
            else:
                cell.set_property('pixbuf', None)
        else:
            cell.set_property('text', widget.name)

    def _add_widget_cb(self, project, widget):
        self.add_item(widget)

    def _remove_widget_cb(self, project, widget):
        self.remove_item(widget)

    def _widget_name_changed_cb(self, project, widget):
        iter = self._find_iter_by_widget(widget)
        if not iter:
            return
 
        path = self._model.get_path(iter)
        self._model.row_changed(path, iter)

    def _selection_update_cb(self, project):
        if self._updating_selection:
            return
        self.selection_update(project)

    def _find_iter(self, iter, findme):
        next = iter
        while next:
            widget = self._model.get_value(next, WIDGET_COLUMN)
            if widget is None:
                print ('Could not get the glade widget from the model')

            # Check for widget wrapper and real widgets
            if widget == findme or widget.gtk_widget == findme:
                return next

            if self._model.iter_has_child(next):
                child = self._model.iter_children(next)
                retval = self._find_iter(child, findme)
                if retval is not None:
                    return retval
                
            next = self._model.iter_next(next)

        return None
    
    def _find_iter_by_widget(self, widget):
        root = self._model.get_iter_root()
        return self._find_iter(root, widget)

    def _add_children(self, parent, parent_iter):
        """ This function is needed because sometimes when adding a widget
        to the tree their children are not added.

        Please note that not all GtkWidget have a GWidget associated with them
        so there are some special cases we need to handle."""
        if not isinstance(parent, gtk.Container):
            return
        
        children = parent.get_children()
        
        gwidget = Widget.from_widget(parent)

        if gwidget is not None:
            children += gwidget.internal_children

        for child in children:
            gchild = Widget.from_widget(child)
            if gchild is None:
                if isinstance(child, gtk.Container):
                    self._add_children(child, parent_iter)
                else:
                    continue

            elif self._find_iter(parent_iter, gchild) is None:
                child_iter = self._model.append(parent_iter, (gchild,))
                self._add_children(child, child_iter)
                    
    def add_item(self, widget):
        parent = widget.get_parent()
        parent_iter = None
        if parent is not None:
            parent_iter = self._find_iter_by_widget(parent)

        # When creating a widget with internal children (e.g. GtkDialog) the
        # children are added first so they do have a GladeWidget parent but
        # don't have a parent_iter (since the parent has not being added yet
        if parent_iter is None and parent is not None:
            # we need to add this widget when adding the parent
            return
        
        widget_iter = self._model.append(parent_iter, (widget,))

        # now check for every children of this widget if it is already on the
        # tree. If not we add it.
        self._add_children(widget.gtk_widget, widget_iter)

        self._updating_selection = True
        widget.project.selection_set(widget.gtk_widget, True)
        self._updating_selection = False

        select_iter(self._tree_view, widget_iter)
                          
    def remove_item(self, widget):
        iter = self._find_iter_by_widget(widget)
        if iter:
            self._model.remove(iter)

    def selection_update(self, project):
        """ The project selection has changed, update our state to reflect the
        changes """

        self._tree_selection.unselect_all()
        
        selection = project.selection
        if not selection:
            return
        
        for widget in selection:
            widget_iter = self._find_iter_by_widget(widget)
            if widget_iter is not None:
                select_iter(self._tree_view, widget_iter)
        
    def _key_press_cb(self, view, event):
        if event.keyval in (gtk.keysyms.Delete, gtk.keysyms.KP_Delete):
            model, iter = view.get_selection().get_selected()
            if iter:
                widget = model.get_value(iter, WIDGET_COLUMN)
                self._app.command_manager.delete(widget)

# This is not working anyway because the current button_press_event code
# is not creating the widget when you click on a parent having a palette
# button selected
#    def _motion_notify_cb(self, view, event):
#        if pw.project_window is None:
#            cursor.set_for_widget_adaptor(event.window, None)
#        else:
#            cursor.set_for_widget_adaptor(event.window,
#                                          pw.project_window.add_class)
#
#        return False

    def _button_press_cb(self, view, event):
        if self._project is None:
            return False

        if event.button != 3:
            return False
        
        result = view.get_path_at_pos(int(event.x), int(event.y))
        if not result:
            return False
        path = result[0]
        item_iter = self._model.get_iter(path)
        widget = self._model[path][WIDGET_COLUMN]
        select_iter(self._tree_view, item_iter)
        
        mgr = self._app.get_command_manager()
        popup = Popup(mgr, widget)
        popup.pop(event)
        return True

    def _get_selected_widget(self):
        if self._project is None:
            return

        # There are no cells selected
        model, iter = self._tree_selection.get_selected()
        if iter is None:
            return

        return self._model.get_value(iter, WIDGET_COLUMN)
    
    def _selection_changed_cb(self, selection):
        widget = self._get_selected_widget() 
        # The cell exists, but no widget has been associated with it
        if widget is None:
            return

        self._updating_selection = True
        widget.project.selection_set(widget.gtk_widget, True)
        self._updating_selection = False

    def _row_activated_cb(self, treeview, path, column):
        widget = self._get_selected_widget()
        if widget is not None:
            toplevel = widget.gtk_widget.get_toplevel()
            toplevel.show_all()
            toplevel.present()

gobject.type_register(WidgetTreeView)
