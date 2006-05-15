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

import gtk

from gazpacho import util

_ = gettext.gettext

class Popup(object):
    """This class defines a popup menu and its behaviour
    
    This popup menu has entries for selecting the widget, cut/copy/paste it
    and deleting it.
    
    It also shows a submenu for every parent widget in the hierarchy.
    """
    def __init__(self, command_manager, widget):
        self._command_manager = command_manager
        self._widget = widget
        self._popup_menu = self._create_menu(widget)
        
    def pop(self, event):
        """Call this method if you want the menu to show up"""
        if event:
            button = event.button
            event_time = event.time
        else:
            button = 0
            event_time = 0

        self._popup_menu.popup(None, None, None, button, event_time)
        return self._popup_menu

    # callbacks for the actions
    def _select_cb(self, item, widget):
        if widget:
            widget.project.selection_set(widget.gtk_widget, True)
    
    def _cut_cb(self, item, widget):
        self._command_manager.cut(widget)

    def _copy_cb(self, item, widget):
        self._command_manager.copy(widget)

    def _paste_cb(self, item, widget):
        gwidget = util.get_parent(widget)
        self._command_manager.paste(widget, gwidget.project)

    def _delete_cb(self, item, widget):
        self._command_manager.delete(widget)
        
    def _box_insert_before_cb(self, item, widget):
        parent_pos = self._find_parent_box_and_pos(widget)
        if parent_pos is None:
            return

        (parent, pos) = parent_pos
        self._command_manager.box_insert_placeholder(parent, pos)

    def _box_insert_after_cb(self, item, widget):
        parent_pos = self._find_parent_box_and_pos(widget)
        if parent_pos is None:
            return

        (parent, pos) = parent_pos
        # Adjust to insert _after_
        pos += 1
        self._command_manager.box_insert_placeholder(parent, pos)

    # aux methods
    def _create_menu(self, widget, add_children=True):
        from gazpacho.placeholder import Placeholder

        popup_menu = gtk.Menu()

        self._append_item(popup_menu, None,
                          _('Select')+' '+widget.name,
                          True,
                          self._select_cb, widget)
        self._append_item(popup_menu, gtk.STOCK_CUT, None, True,
                          self._cut_cb, widget)
        self._append_item(popup_menu, gtk.STOCK_COPY, None, True,
                          self._copy_cb, widget)
        if isinstance(widget, Placeholder):
            self._append_item(popup_menu, gtk.STOCK_PASTE, None, True,
                              self._paste_cb, widget)
        self._append_item(popup_menu, gtk.STOCK_DELETE, None, True,
                          self._delete_cb, widget)

        if isinstance(widget.gtk_widget, gtk.Box):
            separator = gtk.MenuItem()
            separator.show()
            popup_menu.append(separator)

            self._append_item(popup_menu, _('Insert before'), None, True,
                              self._box_insert_before_cb, widget)

            self._append_item(popup_menu, _('Insert after'), None, True,
                              self._box_insert_after_cb, widget)

        if add_children and not widget.is_toplevel():
            parent = widget.get_parent()
            self._populate_children(popup_menu, parent)

        return popup_menu

    def _append_item(self, menu, stock, label, sensitive, callback, data):
        if stock:
            menu_item = gtk.ImageMenuItem(stock, None)
        else:
            menu_item = gtk.MenuItem(label=label)

        menu_item.connect('activate', callback, data)

        menu_item.set_sensitive(sensitive)
        menu_item.show()
        menu.append(menu_item)

    def _populate_children(self, popup_menu, parent):
        while parent:
            separator = gtk.MenuItem()
            separator.show()
            popup_menu.append(separator)

            child = gtk.MenuItem(parent.name)
            child_menu = self._create_child_menu(parent)
            child.set_submenu(child_menu)
            child.show()
            popup_menu.append(child)

            parent = parent.get_parent()
        
    def _create_child_menu(self, parent):
        return Popup._create_menu(self, parent, False)

    # Used for finding the parent box to insert placeholder in
    def _find_parent_box_and_pos(self, widget):
        from gazpacho import placeholder
        if isinstance(self._widget, placeholder.Placeholder):
            child_widget = self._widget
            parent = util.get_parent(self._widget)
        else:
            child_widget = self._widget.gtk_widget
            parent = self._widget.get_parent()

        while parent:
            if parent == widget:
                break

            child_widget = parent.gtk_widget
            parent = parent.get_parent()

        if parent is not None and child_widget is not None:
            pos = parent.gtk_widget.get_children().index(child_widget)

            return parent, pos
    

class PlaceholderPopup(Popup):
    def _delete_placeholder_cb(self, item, placeholder):
        self._command_manager.delete_placeholder(placeholder)

    """In the case we want a popup over a Placeholder we don't
    want entries for cut/copy/paste or remove it since this has
    no sense"""
    def _create_menu(self, placeholder, add_children=False):
        popup_menu = gtk.Menu()

        self._append_item(popup_menu, gtk.STOCK_PASTE, None, True,
                          self._paste_cb, placeholder)

        parent = util.get_parent(placeholder)
        if parent:
            # Special-case boxes to make it possible to remove
            # placeholders from them.
            if isinstance(parent.gtk_widget, gtk.Box):
                self._append_item(popup_menu, gtk.STOCK_DELETE, None, True,
                                  self._delete_placeholder_cb, placeholder)

            self._populate_children(popup_menu, parent)
        
        return popup_menu

