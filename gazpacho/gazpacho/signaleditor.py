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
import pango

_ = gettext.gettext

(COLUMN_SIGNAL,
 COLUMN_HANDLER,
 COLUMN_AFTER,
 COLUMN_AFTER_VISIBLE,
 COLUMN_HANDLER_EDITABLE,
 COLUMN_SLOT,
 COLUMN_BOLD) = range(7)

SLOT_MESSAGE = _("<Type the signal's handler here>")

class SignalEditor(gtk.VBox):
    """ The GladeSignalEditor is used to house the signal editor interface and
    associated functionality """

    def __init__(self, editor, app):
        gtk.VBox.__init__(self)

        self._editor = editor
        
        self._app = app

        scroll = gtk.ScrolledWindow()
        scroll.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        
        self._model = None
        self._signals_list = self._construct_signal_list()

        scroll.add(self._signals_list)
        self.pack_start(scroll)
        self.show_all()
        
        self._widget = None
        self._adaptor = None

    def _construct_signal_list(self):
        self._model = gtk.TreeStore(str, str, bool, bool, bool, bool, bool)
        view = gtk.TreeView(self._model)
        view.connect('row-activated', self._row_activated)
        view.connect('key-press-event', self._key_press_cb)
        view.connect('button-press-event', self._button_press_cb)
        
        # signal column
        renderer = gtk.CellRendererText()
        renderer.set_property('weight', pango.WEIGHT_BOLD)
        column = gtk.TreeViewColumn(_('Signal'), renderer, text=COLUMN_SIGNAL,
                                    weight_set=COLUMN_BOLD)
        view.append_column(column)

        # handler column
        renderer = gtk.CellRendererText()
        renderer.set_property('style', pango.STYLE_ITALIC)
        renderer.set_property('foreground', 'Gray')
        renderer.connect('edited', self._cell_edited)
        column = gtk.TreeViewColumn(_('Handler'), renderer,
                                    text=COLUMN_HANDLER,
                                    style_set=COLUMN_SLOT,
                                    foreground_set=COLUMN_SLOT,
                                    editable=COLUMN_HANDLER_EDITABLE)
        view.append_column(column)

        # after column
        renderer = gtk.CellRendererToggle()
        renderer.connect('toggled', self._after_toggled)
        column = gtk.TreeViewColumn(_('After'), renderer,
                                    active=COLUMN_AFTER,
                                    visible=COLUMN_AFTER_VISIBLE)
        view.append_column(column)
        return view

    def load_widget(self, widget):
        self._model.clear()
        self._widget = widget
        self._adaptor = widget and widget.adaptor or None

        if not widget: return

        last_type = ""
        parent_class = None
        for signal_name, signal_type in self._adaptor.list_signals():
            if signal_type != last_type:
                parent_class = self._model.append(None)
                self._model.set(parent_class,
                                COLUMN_SIGNAL, signal_type,
                                COLUMN_AFTER_VISIBLE, False,
                                COLUMN_HANDLER_EDITABLE, False,
                                COLUMN_SLOT, False,
                                COLUMN_BOLD, False)
                last_type = signal_type

            parent_signal = self._model.append(parent_class)
            signals = widget.list_signal_handlers(signal_name)

            if not signals:
                self._model.set(parent_signal,
                                COLUMN_SIGNAL, signal_name,
                                COLUMN_HANDLER, SLOT_MESSAGE,
                                COLUMN_AFTER, False,
                                COLUMN_HANDLER_EDITABLE, True,
                                COLUMN_AFTER_VISIBLE, False,
                                COLUMN_SLOT, True)
            else:
                widget_signal = signals[0]
                self._model.set(parent_signal,
                                COLUMN_BOLD, True,
                                COLUMN_SIGNAL, signal_name,
                                COLUMN_HANDLER, widget_signal['handler'],
                                COLUMN_AFTER, widget_signal['after'],
                                COLUMN_AFTER_VISIBLE, True,
                                COLUMN_HANDLER_EDITABLE, True,
                                COLUMN_SLOT, False)
                path_parent_class = self._model.get_path(parent_class)
                self._signals_list.expand_row(path_parent_class, False)

                for widget_signal in signals[1:]:
                    model_iter = self._model.append(parent_signal)
                    self._model.set(model_iter,
                                    COLUMN_HANDLER, widget_signal['handler'],
                                    COLUMN_AFTER, widget_signal['after'],
                                    COLUMN_AFTER_VISIBLE, True,
                                    COLUMN_HANDLER_EDITABLE, True,
                                    COLUMN_SLOT, False)

                # add the <Type...> slot
                model_iter = self._model.append(parent_signal)
                self._model.set(model_iter,
                                COLUMN_HANDLER, SLOT_MESSAGE,
                                COLUMN_AFTER, False,
                                COLUMN_AFTER_VISIBLE, False,
                                COLUMN_HANDLER_EDITABLE, True,
                                COLUMN_SLOT, True)

        self._signals_list.expand_row((0,), False)
                                
    def _after_toggled(self, cell, path_str, data=None):
        model_iter = self._model.get_iter(path_str)
        signal_name, handler, after = self._model.get(model_iter,
                                                      COLUMN_SIGNAL,
                                                      COLUMN_HANDLER,
                                                      COLUMN_AFTER)
        if signal_name is None:
            iter_parent = self._model.iter_parent(model_iter)
            signal_name = self._model.get_value(iter_parent, COLUMN_SIGNAL)

        old_signal = {'name': signal_name, 'handler': handler, 'after': after}
        new_signal = {'name': signal_name, 'handler': handler,
                      'after': not after}

        self._app.command_manager.change_signal(self._widget,
                                                old_signal,
                                                new_signal)

        self._model.set(model_iter, COLUMN_AFTER, not after)

    def _is_valid_identifier(self, text):
        if text is None:
            return False

        return True

    def _append_slot(self, iter_signal):
        iter_new_slot = self._model.append(iter_signal)
        self._model.set(iter_new_slot,
                        COLUMN_HANDLER, SLOT_MESSAGE,
                        COLUMN_AFTER, False,
                        COLUMN_AFTER_VISIBLE, False,
                        COLUMN_HANDLER_EDITABLE, True,
                        COLUMN_SLOT, True)
        iter_class = self._model.iter_parent(iter_signal)
        # mark the signal and class name as bold
        self._model.set(iter_signal, COLUMN_BOLD, True)
        self._model.set(iter_class, COLUMN_BOLD, True)

    def _remove_row(self, model_iter):
        "Remove a row from the signal tree"

        # if this isn't the first row we can just remove it
        signal_name = self._model.get_value(model_iter, COLUMN_SIGNAL)
        if signal_name is None:
            self._model.remove(model_iter)
            return

        # We cannot delete the first row so we have to copy the next
        # row and delete that instead
        next_iter = self._model.iter_nth_child(model_iter, 0)
        if not next_iter:
            return

        (next_handler,
         next_after,
         next_slot,
         next_visible,
         next_editable) = self._model.get(next_iter, COLUMN_HANDLER, COLUMN_AFTER,
                                          COLUMN_SLOT, COLUMN_AFTER_VISIBLE,
                                          COLUMN_HANDLER_EDITABLE)
        self._model.set(model_iter,
                        COLUMN_HANDLER, next_handler,
                        COLUMN_AFTER, next_after,
                        COLUMN_AFTER_VISIBLE, next_visible,
                        COLUMN_HANDLER_EDITABLE, next_editable,
                        COLUMN_SLOT, next_slot)

        next_iter = self._model.remove(next_iter)

        # if last signal handler was removed, the parents has to be updated
        # to reflect this
        if not next_iter:
            self._model.set_value(model_iter, COLUMN_BOLD, False)

            # We need to go through all children of the signal class
            # and see if any of them are bold and update the class node
            # to reflect this
            iter_class = self._model.iter_parent(model_iter)
            signal_iter = self._model.iter_children(iter_class)
            class_bold = False
            while signal_iter:
                class_bold = self._model.get_value(signal_iter, COLUMN_BOLD)
                if class_bold:
                    break
                signal_iter = self._model.iter_next(signal_iter)

            self._model.set_value(iter_class, COLUMN_BOLD, class_bold)


    def _is_void_signal_handler(self, signal_handler):
        return signal_handler is None or \
               signal_handler.strip() == '' or \
               signal_handler == SLOT_MESSAGE

    def _cell_edited(self, cell, path_str, new_handler, data=None):
        model_iter = self._model.get_iter(path_str)
        signal_name, old_handler, after, slot = self._model.get(model_iter,
                                                                COLUMN_SIGNAL,
                                                                COLUMN_HANDLER,
                                                                COLUMN_AFTER,
                                                                COLUMN_SLOT)
        if signal_name is None:
            iter_signal = self._model.iter_parent(model_iter)
            signal_name = self._model.get_value(iter_signal, COLUMN_SIGNAL)
        else:
            iter_signal = model_iter

        # false alarm
        if slot and self._is_void_signal_handler(new_handler):
            return

        # we are adding a new handler
        if slot and not self._is_void_signal_handler(new_handler):
            signal = {'name': signal_name, 'handler': new_handler,
                      'after': False}
            self._app.command_manager.add_signal(self._widget, signal)
            self._model.set(model_iter, COLUMN_HANDLER, new_handler,
                            COLUMN_AFTER_VISIBLE, True,
                            COLUMN_SLOT, False)
            # append a <Type...> slot
            self._append_slot(iter_signal)

        # we are removing a signal handler
        if not slot and self._is_void_signal_handler(new_handler):
            old_signal = {'name': signal_name, 'handler': old_handler,
                          'after': after}
            self._app.command_manager.remove_signal(self._widget, old_signal)
            self._remove_row(model_iter)

        # we are changing a signal handler
        if not slot and not self._is_void_signal_handler(new_handler):
            old_signal = {'name': signal_name, 'handler': old_handler,
                          'after': after}
            new_signal = {'name': signal_name, 'handler': new_handler,
                          'after': after}
            self._app.command_manager.change_signal(self._widget,
                                                    old_signal,
                                                    new_signal)
            self._model.set(model_iter, COLUMN_HANDLER, new_handler,
                            COLUMN_AFTER_VISIBLE, True,
                            COLUMN_SLOT, False)

    def _row_activated(self, view, path, column, data=None):
        view.set_cursor(path, column, True)
        view.grab_focus()

    def _remove_signal_handler(self, model_iter):
        """Remove signal handler both from the treeview and the widget."""
        signal_name, old_handler, after = self._model.get(model_iter,
                                                          COLUMN_SIGNAL,
                                                          COLUMN_HANDLER,
                                                          COLUMN_AFTER)
        if signal_name is None:
            iter_signal = self._model.iter_parent(model_iter)
            signal_name = self._model.get_value(iter_signal, COLUMN_SIGNAL)

        old_signal = {'name': signal_name, 'handler': old_handler,
                      'after': after}
        self._app.command_manager.remove_signal(self._widget, old_signal)
        self._remove_row(model_iter)

    def _key_press_cb(self, view, event):
        if event.keyval in (gtk.keysyms.Delete, gtk.keysyms.KP_Delete):
            model, model_iter = view.get_selection().get_selected()
            if self._is_row_deletable(model_iter):
                self._remove_signal_handler(model_iter)

    def _create_popup(self, item_iter):
        menu = gtk.Menu()
        menu_item = gtk.ImageMenuItem(gtk.STOCK_DELETE, None)
        menu_item.connect('activate', self._popup_delete_cb, item_iter)
        menu_item.show()
        menu.append(menu_item)
        return menu

    def _popup_delete_cb(self, item, item_iter):
        self._remove_signal_handler(item_iter)

    def _button_press_cb(self, view, event):
        if event.button != 3:
            return False
        
        result = view.get_path_at_pos(int(event.x), int(event.y))
        if not result:
            return False
        path = result[0]
        item_iter = self._model.get_iter(path)
        
        if self._is_row_deletable(item_iter):
            view.set_cursor(path)
            view.grab_focus()            
            popup = self._create_popup(item_iter)
            popup.popup(None, None, None, event.button, event.time)
            return True

        return False

    def _is_row_deletable(self, model_iter):
        """Check whether a row can be deleted or not."""
        editable, slot = self._model.get(model_iter,
                                         COLUMN_HANDLER_EDITABLE,
                                         COLUMN_SLOT)        
        return editable and not slot

gobject.type_register(SignalEditor)
