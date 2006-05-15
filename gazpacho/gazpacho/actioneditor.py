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

import gettext

import gtk
import gobject

from gazpacho.gaction import GAction, GActionGroup
from gazpacho.util import unselect_when_clicked_on_empty_space, select_iter

_ = gettext.gettext

(COL_OBJECT,) = range(1)

class GActionsView(gtk.ScrolledWindow):
    """A GActionsView is basically a TreeView where the actions
    and action groups are shown.
    
    The data it shows is always a two level hierarchy tree, where
    toplevel nodes are GActionGroups and leaves are GActions.
    """
    __gsignals__ = {
        'selection-changed' : (gobject.SIGNAL_RUN_LAST, None, (object,))
        }

    def __init__(self):
        gtk.ScrolledWindow.__init__(self)
        self.set_shadow_type(gtk.SHADOW_IN)
        
        self.project = None

        self._model = gtk.TreeStore(object)
        self._treeview = gtk.TreeView(self._model)
        self._treeview.set_headers_visible(False)
        self._treeview.connect('row-activated', self._row_activated_cb)
        self._treeview.connect('button-press-event',
                               unselect_when_clicked_on_empty_space)

        self._treeview.connect('button-press-event', self._button_press_cb)
        self._treeview.connect('key-press-event', self._key_press_cb)
        self._treeview.get_selection().connect('changed', self._selection_changed_cb)

        column = gtk.TreeViewColumn()
        renderer1 = gtk.CellRendererPixbuf()
        column.pack_start(renderer1, expand=False)
        column.set_cell_data_func(renderer1, self._draw_action, 0)

        renderer2 = gtk.CellRendererText()
        column.pack_start(renderer2, expand=True)
        column.set_cell_data_func(renderer2, self._draw_action, 1)

        self._treeview.append_column(column)

        self.add(self._treeview)

    def set_project(self, project):
        if self.project is not None:
            self.project.disconnect(self._add_action_id)
            self.project.disconnect(self._remove_action_id)
            self.project.disconnect(self._action_name_changed_id)
            
        self.project = project
        self._model.clear()
        if self.project is not None:
            self._add_action_id = self.project.connect('add-action',
                                                       self._add_action_cb)
            self._remove_action_id = self.project.connect('remove-action',
                                                          self._remove_action_cb)
            self._action_name_changed_id = self.project.connect('action-name-changed',
                                                                self._action_name_changed_cb)
            self._fill_model()

    def add_action(self, gaction):
        """Create an action and add it to the selected action
        group. This method will not create the action directly but
        delegate to the command manager."""
        if isinstance(gaction, GActionGroup):
            # create an action with the selected action group
            # as the parent
            parent = gaction
        else:
            # create a brother action of the selected action
            parent = gaction.parent

        dialog = GActionDialog(self.project._app.window, None)
        if dialog.run() == gtk.RESPONSE_OK:
            values = dialog.get_values()
            self.project._app.command_manager.add_action(values, parent, self.project)
        dialog.destroy()

    def add_action_group(self):
        """Create and add an action group. This method will not create
        the action group directly but delegate to the command manager."""
        # nothing is selected, we create an action group
        dialog = GActionGroupDialog(self.project._app.window, None)
        if dialog.run() == gtk.RESPONSE_OK:
            name = dialog.get_action_group_name()
            self.project._app.command_manager.add_action_group(name, self.project)
        dialog.destroy()

    def remove_action(self, gaction):
        """Remove the action group. This method will not remove the
        action group directly but delegate to the command manager."""
        if isinstance(gaction, GActionGroup):
            self.project._app.command_manager.remove_action_group(gaction, self.project)
        else:
            self.project._app.command_manager.remove_action(gaction, self.project)

    def edit_action(self, gaction):
        """Edit the action or action group. This method will not edit
        it directly but delegate to the command manager."""
        if isinstance(gaction, GAction):
            dialog = GActionDialog(self.project._app.window, gaction)
            if dialog.run() == gtk.RESPONSE_OK:
                new_values = dialog.get_values()
                self.project._app.command_manager.edit_action(gaction, new_values,
                                                                self.project)
            dialog.destroy()
        else:
            dialog = GActionGroupDialog(self.project._app.window, gaction)
            if dialog.run() == gtk.RESPONSE_OK:
                new_name = dialog.get_action_group_name()
                self.project._app.command_manager.edit_action_group(gaction, new_name,
                                                        self.project)
            dialog.destroy()
        
    def _fill_model(self):
        for gaction_group in self.project.uim.action_groups:
            parent_iter = self._model.append(None, (gaction_group,))
            for gaction in gaction_group.get_actions():
                self._model.append(parent_iter, (gaction,))
                
    def _selection_changed_cb(self, selection, data=None):
        """Callback for selection changes in the tree view."""
        model, it = selection.get_selected()
        item = None
        if it:
            item = model[it][COL_OBJECT]
        self.emit('selection-changed', item)

    def _add_action_cb(self, project, action):
        new_iter = None
        if isinstance(action, GActionGroup):
            new_iter = self._model.append(None, (action,))
        elif isinstance(action, GAction):
            parent = action.parent
            ag_iter = self._find_action_group(parent)
            if ag_iter is not None:
                new_iter = self._model.append(ag_iter, (action,))
        
        if new_iter:
            select_iter(self._treeview, new_iter)
    
    def _find_action_group(self, action_group):
        model = self._model
        for row in model:
            if row[COL_OBJECT] == action_group:
                return row.iter

    def _find_action(self, parent_iter, gaction):
        model = self._model
        for row in model[parent_iter].iterchildren():
            if row[COL_OBJECT] == gaction:
                return row.iter
            
    def _remove_action_cb(self, project, gaction):
        if isinstance(gaction, GActionGroup):
            action_group_iter = self._find_action_group(gaction)
            if action_group_iter is not None:
                del self._model[action_group_iter]
        elif isinstance(gaction, GAction):
            parent_iter = self._find_action_group(gaction.parent)
            if parent_iter is not None:
                child_iter = self._find_action(parent_iter, gaction)
                if child_iter is not None:
                    del self._model[child_iter]

    def _action_name_changed_cb(self, project, gaction):
        action_iter = None
        if isinstance(gaction, GAction):
            parent_iter = self._find_action_group(gaction.parent)
            action_iter = self._find_action(parent_iter, gaction)
        elif isinstance(gaction, GActionGroup):
            action_iter = self._find_action_group(gaction)
            
        if action_iter is not None:
            path = self._model[action_iter].path
            self._model.row_changed(path, action_iter)            
    
    def get_selected_action(self):
        selection = self._treeview.get_selection()
        model, model_iter = selection.get_selected()
        if model_iter:
            return model[model_iter][COL_OBJECT]

    def get_selected_action_group(self):
        selection = self._treeview.get_selection()
        model, model_iter = selection.get_selected()
        if model_iter:
            gaction = model[model_iter][COL_OBJECT]
            if isinstance(gaction, GActionGroup):
                return gaction
    
    def _draw_action(self, column, cell, model, iter, model_column):
        action = model[iter][COL_OBJECT]
        if action is None:
          return
        if model_column == 0:
            prop = 'stock-id'
            if isinstance(action, GActionGroup):
                data = 'gtk-execute'
            else:
                if action is not None:
                  data = action.stock_id or ''
                else:
                  data = ''
        elif model_column == 1:
            prop = 'text'
            data = action.name
            
        cell.set_property(prop, data)

    def _row_activated_cb(self, treeview, path, column):
        gaction = self._model[path][COL_OBJECT]
        self.edit_action(gaction)

    # Keybord interaction callback
    def _key_press_cb(self, view, event):
        """Callback for handling key press events. Right now it's only
        used for deleting actions and action groups."""
        if event.keyval in [gtk.keysyms.Delete, gtk.keysyms.KP_Delete]:
            model, model_iter = view.get_selection().get_selected()
            if model_iter:
                gaction = model[model_iter][COL_OBJECT]
                if isinstance(gaction, GActionGroup):
                    self.project._app.command_manager.remove_action_group(gaction, self.project)
                else:
                    self.project._app.command_manager.remove_action(gaction, self.project)

    # Popup menu for the action editor
    def _create_popup(self, path):
        """Create a context menu for the action tree. It will be
        filled with different items depending the selection."""
        gaction = None
        if path:
            gaction = self._model[path][COL_OBJECT]
         
        menu = gtk.Menu()
 
        # Add action
        add_action_item = gtk.MenuItem("_Add Action...", True)
        add_action_item.connect_object('activate', self._popup_add_action_cb, gaction)
        menu.append(add_action_item)
 
        # Add action group
        add_group_item = gtk.MenuItem("Add Action _Group...", True)
        add_group_item.connect_object('activate', self._popup_add_action_group_cb, None)
        menu.append(add_group_item)

        # Edit action or group
        edit_item = gtk.ImageMenuItem(gtk.STOCK_EDIT, None)
        edit_item.connect_object('activate', self._popup_edit_action_cb, gaction)
        menu.append(edit_item)
            
        # Delete action or group
        delete_item = gtk.ImageMenuItem(gtk.STOCK_DELETE, None)
        delete_item.connect_object('activate', self._popup_delete_action_cb, gaction)
        menu.append(delete_item)

        if not gaction:
            add_action_item.set_property('sensitive', False)
            edit_item.set_property('sensitive', False)
            delete_item.set_property('sensitive', False)
 
        menu.show_all()
        return menu

    def _popup_edit_action_cb(self, gaction):
        """Edit an action or action group. It's used by the context
        menu."""
        self.edit_action(gaction)
 
    def _popup_delete_action_cb(self, gaction):
        """Callback for deleting a certain action or action
        group. It's used by the context menu."""
        self.remove_action(gaction)
 
    def _popup_add_action_cb(self, gaction):
        """Callback that will create and add a new action. It's used
        by the context menu."""
        self.add_action(gaction)
 
    def _popup_add_action_group_cb(self, data=None):
        """Callback that will create and add a new action group. It's
        used by the context menu."""
        self.add_action_group()
 
    def _button_press_cb(self, view, event):
        """Callback for handling mouse clicks. It is used to show a
        context menu."""
        if event.button != 3:
            return False
 
        # No need for a context menu if there is no project
        if not self.project:
            return False
         
        result = view.get_path_at_pos(int(event.x), int(event.y))
        path = None
        if result:
            path = result[0]
            view.set_cursor(path)
            view.grab_focus()
         
        popup = self._create_popup(path)
        popup.popup(None, None, None, event.button, event.time)
        return True

gobject.type_register(GActionsView)

class GActionDialog(gtk.Dialog):
    """This Dialog allows the creation and edition of a GAction."""
    def __init__(self, toplevel=None, action=None):
        gtk.Dialog.__init__(self,
                            title='',
                            parent=toplevel,
                            flags=gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT)
        
        # If we are currently editing the accelerator
        self._accel_editing = False

        self.add_button(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL)
        if action is None:
            self.set_title(_('Add Action'))
            self.add_button(gtk.STOCK_ADD, gtk.RESPONSE_OK)
        else:
            self.set_title(_('Edit Action'))
            self.add_button(gtk.STOCK_OK, gtk.RESPONSE_OK)

        self.set_border_width(6)
        self.set_resizable(False)
        self.set_has_separator(False)
        self.vbox.set_spacing(6)        

        self._create_widgets()

        self.action = action

        self.set_default_response(gtk.RESPONSE_OK)

        # Set mnemonic callback for the labels in the buttons.
        for widget in self.action_area.get_children():
            hbox = widget.child.child
            label = hbox.get_children()[1]
            keyval = label.get_property('mnemonic-keyval')
            label.connect('mnemonic-activate', self._on_mnemonic_activate, keyval)
        
        self.vbox.show_all()


    def _create_widgets(self):

        size_group = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)

        # id
        hbox = gtk.HBox(spacing=6)
        label = self._create_label('_Id', size_group, hbox)
        self.id = gtk.Entry()
        self.id.set_activates_default(True)
        label.set_mnemonic_widget(self.id)
        hbox.pack_start(self.id)
        self.vbox.pack_start(hbox)

        # label
        hbox = gtk.HBox(spacing=6)
        label = self._create_label(_('_Label'), size_group, hbox)
        self.label = gtk.Entry()
        self.label.set_activates_default(True)
        label.set_mnemonic_widget(self.label)
        hbox.pack_start(self.label)
        self.vbox.pack_start(hbox)

        # stock
        hbox = gtk.HBox(spacing=6)
        label = self._create_label(_('_Stock'), size_group, hbox)
        self.stock_enabled = gtk.CheckButton()
        self.stock_enabled.connect('toggled', self._on_stock_check_toggled)
        label.set_mnemonic_widget(self.stock_enabled)
        hbox.pack_start(self.stock_enabled, False, False)

        stock_model = gtk.ListStore(str)
        stocks = gtk.stock_list_ids()
        stocks.sort()

        for stock in stocks:
            stock_model.append((stock,))

        self.stock = gtk.ComboBox(stock_model)
        renderer = gtk.CellRendererPixbuf()
        renderer.set_property('xalign', 0.0)
        self.stock.pack_start(renderer)
        self.stock.add_attribute(renderer, 'stock-id', 0)
        renderer = gtk.CellRendererText()
        renderer.set_property('xalign', 0.0)
        self.stock.pack_start(renderer)
        self.stock.add_attribute(renderer, 'text', 0)

        self.stock.set_wrap_width(3)
        
        self.stock.set_active(0)
        self.stock.set_sensitive(False)
        hbox.pack_start(self.stock)
        self.vbox.pack_start(hbox)

        # accelerator
        hbox = gtk.HBox(spacing=6)
        label = self._create_label(_('Accele_rator'), size_group, hbox)
        self.accelerator = gtk.Entry()
        self.accelerator.set_activates_default(True)
        self.accelerator.set_text('Press a key combination')
        self.accelerator.connect('key-press-event',
                                        self._on_accelerator_entry_key_press)
        self.accelerator.connect('button-press-event', self._on_accelerator_entry_button_press)
        self.accelerator.connect('focus-in-event', self.on_accelerator_focus_in)
        self.accelerator.connect('focus-out-event', self.on_accelerator_focus_out)
        label.set_mnemonic_widget(self.accelerator)
        hbox.pack_start(self.accelerator)
        
        self.vbox.pack_start(hbox)

        # tooltip
        hbox = gtk.HBox(spacing=6)
        label = self._create_label(_('_Tooltip'), size_group, hbox)
        self.tooltip = gtk.Entry()
        self.tooltip.set_activates_default(True)
        label.set_mnemonic_widget(self.tooltip)
        hbox.pack_start(self.tooltip)
        self.vbox.pack_start(hbox)

        # callback
        hbox = gtk.HBox(spacing=6)
        label = self._create_label(_('Call_back'), size_group, hbox)
        self.callback = gtk.Entry()
        self.callback.set_activates_default(True)
        label.set_mnemonic_widget(self.callback)
        hbox.pack_start(self.callback)
        self.vbox.pack_start(hbox)

    def _create_label(self, text, size_group, hbox):
        label = gtk.Label()
        label.set_markup_with_mnemonic(text+':')
        label.set_alignment(0.0, 0.5)

        keyval = label.get_property('mnemonic-keyval')
        label.connect('mnemonic-activate', self._on_mnemonic_activate, keyval)

        size_group.add_widget(label)
        hbox.pack_start(label, False, False)
        return label

    def on_accelerator_focus_in(self, widget, event):
        """
        Callback for when the accelerator entry gets focus.
        """
        self._accel_editing = True

    def on_accelerator_focus_out(self, widget, event):
        """
        Callback for when the accelerator entry looses focus.
        """
        self._accel_editing = False

    def _on_mnemonic_activate(self, widget, group_cycling, keyval):
        """
        Callback for the mnemonic activate signal. If we're currently
        editing the accelerator we have to disable the usual mnemonic
        action and use the keyval for as an accelerator value.
        """
        if self._accel_editing:
            key = gtk.accelerator_name(keyval, self.mnemonic_modifier)
            self.accelerator.set_text(key)
            return True

        return False

    def _on_stock_check_toggled(self, button):
        active = button.get_active()
        self.stock.set_sensitive(active)

    def _on_accelerator_entry_key_press(self, entry, event):
        """Callback for handling key events in the accelerator
        entry. Accelerators are added by pressing Ctrl, Shift and/or
        Alt in combination with the desired key. Delete and Backspace
        can be used to clear the entry. No other types of editing is
        possible."""
        
        # Tab must be handled as normal. Otherwise we can't move from
        # the entry.
        if event.keyval == gtk.keysyms.Tab:
            return False

        modifiers = event.get_state() & gtk.accelerator_get_default_mod_mask()
        modifiers = int(modifiers)

        # Check if we should clear the entry
        clear_keys = [gtk.keysyms.Delete,
                      gtk.keysyms.KP_Delete,
                      gtk.keysyms.BackSpace]
        
        if modifiers == 0:
            if event.keyval in clear_keys:
                entry.set_text('')
            return True

        # Check if the accelerator is valid and add it to the entry
        if gtk.accelerator_valid(event.keyval, modifiers):
            accelerator = gtk.accelerator_name(event.keyval, modifiers)
            entry.set_text(accelerator)

        return True

    def _on_accelerator_entry_button_press(self, entry, event):
        """Callback for handling mouse clicks on the accelerator
        entry. It is used to show a context menu with a clear item."""
        if event.button != 3:
            return False
 
        clear_item = gtk.MenuItem(_('_Clear'), True)
        if entry.get_text():
            clear_item.connect_object('activate',
                                      lambda entry: entry.set_text(''),
                                      entry)
        else:
            clear_item.set_property('sensitive', False)

        menu = gtk.Menu()
        menu.append(clear_item)
        menu.show_all()
        menu.popup(None, None, None, event.button, event.time)
        return True

    def _clear_widgets(self):
        self.id.set_text('')
        self.label.set_text('')
        self.stock_enabled.set_active(False)
        self.stock.set_active(0)
        self.stock.set_sensitive(False)
        
        self.accelerator.set_text('')
        self.tooltip.set_text('')
        self.callback.set_text('')
        
    def _load_widgets(self):
        self.id.set_text(self._action.name)
        self.label.set_text(self._action.label)
        if self._action.stock_id is None:
            self.stock_enabled.set_active(False)
            self.stock.set_active(0)
            self.stock.set_sensitive(False)
        else:
            self.stock_enabled.set_active(True)
            model = self.stock.get_model()
            for row in model:
                if row[COL_OBJECT] == self._action.stock_id:
                    self.stock.set_active_iter(row.iter)
                    break
        self.accelerator.set_text(self._action.accelerator or '')
        self.tooltip.set_text(self._action.tooltip or '')
        self.callback.set_text(self._action.callback or '')
        
    def _fill_action(self):
        self._action.name = self.id.get_text()
        self._action.label = self.label.get_text()
        if self.stock_enabled.get_active():
            model = self.stock.get_model()
            stock = model[self.stock.get_active_iter()][COL_OBJECT]
            self._action.stock_id = stock
        else:
            self._action.stock_id = None
        self._action.accelerator = self.accelerator.get_text()
        self._action.tooltip = self.tooltip.get_text()
        self._action.callback = self.callback.get_text()
        
    def get_action(self):
        self._fill_action()
        return self._action
    
    def set_action(self, gaction):
        if gaction is None:
            self._clear_widgets()
            self._action = GAction(None)
        else:
            self._action = gaction
            self._load_widgets()

    action = property(get_action, set_action)
    
    def get_values(self):
        values = {}
        values['name'] = self.id.get_text()
        values['label'] = self.label.get_text()
        values['accelerator'] = self.accelerator.get_text()
        values['tooltip'] = self.tooltip.get_text()
        values['callback'] = self.callback.get_text()
        if self.stock_enabled.get_active():
            model = self.stock.get_model()
            stock = model[self.stock.get_active_iter()][COL_OBJECT]
            values['stock_id'] = stock
        else:
            values['stock_id'] = None

        return values
    
gobject.type_register(GActionDialog)

class GActionGroupDialog(gtk.Dialog):
    """This Dialog allows the creation and edition of a GActionGroup."""
    def __init__(self, toplevel=None, action_group=None):
        gtk.Dialog.__init__(self,
                            title='',
                            parent=toplevel,
                            flags=gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT)

        self.add_button(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL)
        if action_group is None:
            self.set_title(_('Add Action Group'))
            self.add_button(gtk.STOCK_ADD, gtk.RESPONSE_OK)
        else:
            self.set_title(_('Edit Action Group'))
            self.add_button(gtk.STOCK_OK, gtk.RESPONSE_OK)

        self.set_border_width(6)
        self.set_resizable(False)
        self.set_has_separator(False)
        self.vbox.set_spacing(6)

        # id
        hbox = gtk.HBox(spacing=6)
        label = gtk.Label()
        label.set_markup_with_mnemonic(_('_Id')+':')
        label.set_alignment(0.0, 0.5)
        hbox.pack_start(label, False, False)
        self.id = gtk.Entry()
        self.id.set_activates_default(True)
        label.set_mnemonic_widget(self.id)        
        hbox.pack_start(self.id)
        self.vbox.pack_start(hbox)

        self.action_group = action_group

        self.set_default_response(gtk.RESPONSE_OK)
        
        self.vbox.show_all()

    def _clear_widgets(self):
        self.id.set_text('')

    def _load_widgets(self):
        self.id.set_text(self._action_group.name)
        
    def _fill_action_group(self):
        self._action_group.name = self.id.get_text()
        
    def get_action_group(self):
        self._fill_action_group()
        return self._action_group
    
    def set_action_group(self, action_group):
        if action_group is None:
            self._clear_widgets()
            self._action_group = GActionGroup(None)
        else:
            self._action_group = action_group
            self._load_widgets()
            
    action_group = property(get_action_group, set_action_group)
    
    def get_action_group_name(self):
        return self.id.get_text()

gobject.type_register(GActionGroupDialog)
