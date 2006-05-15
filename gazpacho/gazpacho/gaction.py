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

from gettext import dgettext
import gtk
import gobject

from gazpacho.util import xml_create_string_prop_node
from gazpacho.loader import tags

class GAction(object):
    """A GAction has the same information that a GtkAction but it
    is easier to work with. The menubar/toolbar editor code will
    convert this GActions to GtkActions and viceversa.
    """
    def __init__(self, parent, name='', label='', tooltip='', stock_id=None,
                 callback='', accel=''):
        # parent GActionGroup
        self.parent = parent 
        
        self.name = name
        self.label = label
        self.tooltip = tooltip
        self.stock_id = stock_id
        self.callback = callback
        self.accelerator = accel

    def new(gtk_action, parent):
        """Create a new GAction from a GtkAction and a GtkActionGroup"""
        # This code is similar to code in the loader, investigate
        # if we can use more code reusage
        name = gtk_action.get_name()
        label = gtk_action.get_property('label')
        tooltip = gtk_action.get_property('tooltip')
        stock_id = gtk_action.get_property('stock-id') or None
        gaction = GAction(parent, name, label, tooltip, stock_id)

        # check if it has accelerator
        accel_entry = gtk.accel_map_lookup_entry('<Actions>/%s/%s' %
                                                 (parent.name, name))
        if accel_entry:
            key, modifier = accel_entry
            if key != 0:
                try:
                    key_string = chr(key)
                except ValueError:
                    key_string = key

                table = {gtk.gdk.CONTROL_MASK: '<control>',
                         gtk.gdk.MOD1_MASK: '<alt>'}

                accelerator = '%s%s' % (table[modifier], key_string)
                gaction.accelerator = accelerator

        # check if it has signal handler
        callback = gtk_action.get_data(tags.SIGNAL_HANDLER)
        if callback:
            gaction.callback = callback

        return gaction
    new = staticmethod(new)
    
    def __str__(self):
        return "%s %s %s %s %s" % (self.name, self.label, self.stock_id,
                                   self.accelerator, self.tooltip)

    def get_name(self):
        return self.name
    
    def write(self, xml_doc):
        # XXX: Use XMLWriter
        node = xml_doc.createElement(tags.XML_TAG_OBJECT)
        node.setAttribute(tags.XML_TAG_CLASS, "GtkAction")
        node.setAttribute(tags.XML_TAG_ID, self.name)

        default_label = None
        default_key = 0
        default_mask = None
        domain = None
        if self.stock_id:
            (stock_id, default_label, default_mask,
             default_key, domain) = gtk.stock_lookup(self.stock_id)

        node.appendChild(xml_create_string_prop_node(xml_doc, 'name', self.name))

        # default_label is translated, so compare against the translated version
        if default_label != dgettext(domain, self.label):
            label_node = xml_create_string_prop_node(xml_doc, 'label', self.label)
            label_node.setAttribute(tags.XML_TAG_TRANSLATABLE, tags.YES)
            node.appendChild(label_node)
        
        if self.tooltip:
            node.appendChild(xml_create_string_prop_node(xml_doc, 'tooltip',
                                                         self.tooltip))
        if self.stock_id:
            node.appendChild(xml_create_string_prop_node(xml_doc, 'stock_id',
                                                         self.stock_id))
        if self.callback:
            signalnode = xml_doc.createElement(tags.XML_TAG_SIGNAL)
            signalnode.setAttribute(tags.XML_TAG_HANDLER, self.callback)
            signalnode.setAttribute(tags.XML_TAG_NAME, 'activate')
            node.appendChild(signalnode)

        if self.accelerator:
            key, mask = gtk.accelerator_parse(self.accelerator)
            if (key != default_key and
                mask != default_mask):
                node.appendChild(xml_create_string_prop_node(xml_doc, 'accelerator',
                                                             self.accelerator))
        return node

class GActionGroup(gobject.GObject):
    """A GActionGroup is just a list of GActions with a name.
    Associated with each GActionGroup there is a GtkActionGroup with
    is keep synchronized with the GActionGroup:
        
        - As we can't change the name of a GtkActionGroup, if the
        user change the name of a GActionGroup we have to regenerate
        the GtkActionGroup again, copying the actions from the old one
        to the new one.
        
        - When an action is added or removed, we create or destroy the
        GtkAction associated with it. If an action is only changed, we
        regenerate the GtkAction and tell the UIManager to update itself
        by removing the GtkActionGroup and then addind it again.
        Anyone knows a better way to do so?
    """
    __gsignals__ = {
        'add_action':    (gobject.SIGNAL_RUN_LAST, None, (object,)),
        'remove_action': (gobject.SIGNAL_RUN_LAST, None, (object,)),
        }

    def __init__(self, name=''):
        gobject.GObject.__init__(self)
        self._name = name
        self._actions = {}
        self._uimanager = None

    def new(action_group):
        return GActionGroup(action_group.get_name())
    new = staticmethod(new)
    
    def create_gtk_action_group(self, uimanager):
        self._uimanager = uimanager
        gtk_action_group = gtk.ActionGroup(self.name)
        gtk_action_group.set_data('gaction_group', self)
        self.set_data('gtk_action_group', gtk_action_group)
        self._uimanager.insert_action_group(gtk_action_group, 0)
        return gtk_action_group
        
    def destroy_gtk_action_group(self):
        gtk_action_group = self.get_data('gtk_action_group')
        self.set_data('gtk_action_group', None)
        self._uimanager.remove_action_group(gtk_action_group)
    
    def get_name(self):
        return self._name
    
    def set_name(self, new_name):
        """GtkAction groups can't change their name

        That's why we need to copy the old one into a new one
        """
        self._name = new_name
        
        if self._uimanager is None:
            return
        
        gtk_action_group = self.get_data('gtk_action_group')
        self._uimanager.remove_action_group(gtk_action_group)
        new_gtk_action_group = self.create_gtk_action_group(self._uimanager)

        for action in gtk_action_group.list_actions():
            gtk_action_group.remove_action(action)
            new_gtk_action_group.add_action(action)

    name = property(get_name, set_name)

    def add_action(self, action):
        self._actions[action.get_name()] = action

        self._add_gtk_action(action)
        
        self.emit('add-action', action)

    def add_actions(self, actions):
        # now create the default actions
        for name, label, tooltip, stock_id, callback, acc in actions:
            # check if the action already exists
            if not self._actions.has_key(name):
                ga = GAction(self, name, label, tooltip,
                             stock_id, callback, acc)
                self.add_action(ga)
            # XXX maybe we still want to add the action if its contents
            # are not exactly the same as the old one
        
    def _add_gtk_action(self, action):
        gtk_action_group = self.get_data('gtk_action_group')
        gtk_action_group.add_actions((
            (action.name, action.stock_id, action.label, action.accelerator,
             action.tooltip),
            ))
        
    def remove_action(self, action):
        del self._actions[action.get_name()]
        self._remove_gtk_action(action.name)
        self.emit('remove-action', action)

    def _remove_gtk_action(self, name):
        gtk_action_group = self.get_data('gtk_action_group')
        gtk_action = gtk_action_group.get_action(name)
        gtk_action_group.remove_action(gtk_action)
        
    def update_action(self, action, old_name):
        self._remove_gtk_action(old_name)
        self._add_gtk_action(action)
        del self._actions[old_name]
        self._actions[action.name] = action
        if self._uimanager is not None:
            # we need to remove it and then add it to the uimanager
            # so all the proxies are updated
            gtk_action_group = self.get_data('gtk_action_group')
            self._uimanager.remove_action_group(gtk_action_group)
            self._uimanager.insert_action_group(gtk_action_group, 0)

    def get_action(self, action_name):
        return self._actions.get(action_name)

    def get_actions(self):
        return self._actions.values()
    
    def write(self, xml_doc):
        node = xml_doc.createElement(tags.XML_TAG_OBJECT)
        node.setAttribute(tags.XML_TAG_CLASS, "GtkActionGroup")
        node.setAttribute(tags.XML_TAG_ID, self.name)

        # Sort all children, to keep the file format stabler
        keys = self._actions.keys()
        keys.sort()
        
        for key in keys:
            child_node = xml_doc.createElement(tags.XML_TAG_CHILD)
            node.appendChild(child_node)
            action = self._actions[key]
            action_node = action.write(xml_doc)
            child_node.appendChild(action_node)
            
        return node

gobject.type_register(GActionGroup)

