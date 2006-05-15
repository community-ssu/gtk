# -*- test-case-name: gazpacho.unittest.test_uim.py -*-
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
from xml.dom import minidom

import gtk

from gazpacho import util
from gazpacho.gaction import GAction, GActionGroup
from gazpacho.loader import tags
from gazpacho.widget import Widget

_ = gettext.gettext

default_actions = [
    # name, label, tooltip, stock_id, callback, accelerator
    (_('FileMenu'), _('_File'),'',
     None, '', ''),
    (_('New'), _('_New'), _('Create a new file'),
     gtk.STOCK_NEW, '', '<control>N'),
    (_('Open'), _('_Open'), _('Open a file'),
     gtk.STOCK_OPEN, '', '<control>O'),
    (_('Save'), _('_Save'), _('Save a file'),
     gtk.STOCK_SAVE, '', '<control>S'),
    (_('SaveAs'), _('Save _As'), _('Save with a different name'),
     gtk.STOCK_SAVE_AS, '', ''),
    (_('Quit'), _('_Quit'), _('Quit the program'),
     gtk.STOCK_QUIT, '', '<control>Q'),
    (_('EditMenu'), _('_Edit'), '',
     None, '', ''),
    (_('Copy'), _('_Copy'), _('Copy selected object into the clipboard'),
     gtk.STOCK_COPY, '', '<control>C'),
    (_('Cut'), _('Cu_t'), _('Cut selected object into the clipboard'),
     gtk.STOCK_CUT, '', '<control>X'),
    (_('Paste'), _('_Paste'), _('Paste object from the Clipboard'),
     gtk.STOCK_PASTE, '', '<control>V'),
    ]

class GazpachoUIM(object):
    """Wrapper for the UIManager.

    Provides additional functionality to the standard UIManager
    """

    def __init__(self, project):

        self.project = project

        # List of widget types that need the ui manager to be saved
        self.managed_types = []

        self.reset()
        
    def reset(self):
        self.uim = gtk.UIManager()

        # this dictionary maps gwidgets with their ui definitions. Example:
        # { gwidget1: {'initial-state' : (XML1, merge-id1)},
        #   gwidget2: {'initial-state' : (XML2, merge-id2),
        #              'special-mode' : (XML3, merge-id3)},
        # }
        self.uis = {}

        self.action_groups = []

        self.loaded_uis = {}
        
    def add_ui(self, gwidget, xml_string, ui_name='initial-state'):
        "Add a ui definition for gwidget with the name ui_name"
        if not self.uis.has_key(gwidget):
            self.uis[gwidget] = {}

        # add it to the uimanager
        merge_id = self.uim.add_ui_from_string(xml_string)
        
        self.uis[gwidget][ui_name] = (xml_string, merge_id)

    def get_ui(self, gwidget, ui_name=None):
        "Return a ui definition for gwidget or all of them if ui_name is None"
        if ui_name is None:
            return self.uis.get(gwidget)
        else:
            uis = self.uis.get(gwidget)
            if uis:
                return uis[ui_name]
        
    def update_ui(self, gwidget, xml, ui_name='initial-state'):
        """Update the ui definition of gwidget with name ui_name"""
        
        # save the parent and the packing props before changing the uim
        old_gtk_widget = gwidget.gtk_widget
        parent = old_gtk_widget.get_parent()
        if parent:
            props = {}
            for pspec in gtk.container_class_list_child_properties(type(parent)):
                props[pspec.name] = parent.child_get_property(old_gtk_widget,
                                                              pspec.name)

            parent.remove(old_gtk_widget)

        # update the ui manager
        old_merge_id = self.uis[gwidget][ui_name][1]
        self.uim.remove_ui(old_merge_id)
        new_merge_id = self.uim.add_ui_from_string(xml)
        
        self.uis[gwidget][ui_name] = (xml, new_merge_id)

        # update the gtk widget of gwidget
        new_gtk_widget = self.get_gtk_widget(gwidget)

        # copy the packing properties to the new widget
        if parent:
            parent.add(new_gtk_widget)

            for name, value in props.items():
                parent.child_set_property(new_gtk_widget, name, value)

        # finish widget configuration
        gwidget.setup_widget(new_gtk_widget)
        #gwidget.apply_properties()
        
        gwidget.project.widgets.remove(old_gtk_widget)
        gwidget.project.widgets.append(new_gtk_widget)
        gwidget.project.changed = True

    def get_gtk_widget(self, gwidget):
        self.uim.ensure_update()
        return self.uim.get_widget('/%s' % gwidget.name)

    def add_action_group(self, action_group):
        if self.uim is None:
            self.uim = gtk.UIManager()

        self.action_groups.append(action_group)
        id1 = action_group.connect('add-action', self.project._add_action_cb)
        id2 = action_group.connect('remove-action', self.project._remove_action_cb)
        action_group.set_data('add_action_id', id1)
        action_group.set_data('remove_action_id', id2)

        action_group.create_gtk_action_group(self.uim)
        
    def remove_action_group(self, action_group):
        self.action_groups.remove(action_group)
        id1 = action_group.get_data('add_action_id')
        id2 = action_group.get_data('remove_action_id')
        action_group.disconnect(id1)
        action_group.disconnect(id2)

        action_group.destroy_gtk_action_group()

    def get_action_group(self, action_group_name):
        """Return an action group given its name or None if no action group
        with that name was found"""
        for gag in self.action_groups:
            if gag.get_name() == action_group_name:
                return gag

    def create_default_actions(self):
        """Create a set of default action groups inside a DefaultActions
        action group in the project this uim belongs to.
        Don't add them if they are already there.
        """
        # create the 'DefaultActions' action group
        gaction_group = None
        for gag in self.action_groups:
            if gag.name == 'DefaultActions':
                gaction_group = gag
                break
        # if not found, create it
        else:
            gag = GActionGroup('DefaultActions')
            self.project.add_action_group(gag)
            gaction_group = self.action_groups[-1]

        gaction_group.add_actions(default_actions)

    def update_widget_name(self, gwidget):
        """Update the ui definitions of gwidget with the new widget name"""
        if self.uim is None:
            raise RuntimeError("No UIManager has been created yet. Can not "
                               "update the name of the widget %s" % \
                               gwidget.name)

        gwidget.gtk_widget.set_property('name', gwidget.name)
        gwidget.properties['name']._value = gwidget.name
        for ui_name, (xml_string, merge_id) in self.uis[gwidget].items():
            new_xml = self._change_ui_definition(xml_string, gwidget.name)
            self.update_ui(gwidget, new_xml, ui_name)

    def _change_ui_definition(self, xml_string, new_name):
        "Change a ui definition of a widget to use a new_name for the widget" 
        doc = minidom.parseString(xml_string)
        doc.documentElement.setAttribute('action', new_name)
        doc.documentElement.setAttribute('name', new_name)
        return doc.documentElement.toxml()
    
    def load(self, loader):
        """Get the UIManager of the loader and copy its content into
        this uimanager.

        Load also its ui definition spliting them into the widgets. When
        this uim is loaded the widgets it defines (menubars, toolbars, ...)
        are not loaded yet so we can't add the ui definitions into the
        self.uis dictionary. We use a temporary one until we load the
        widgets."""

        objs = [obj for obj in loader.get_widgets()
                        if isinstance(obj, gtk.UIManager)]
        if not objs:
            return
        
        if len(objs) >= 2:
            print 'Warning: multiple uimanagers is not supported yet'
            
        gaction_callbacks = {}
        uimanager = objs[0]
        for widget, signal, handler, after, sig_object in loader.get_signals():
            if signal == "activate" and isinstance(widget, gtk.Action):
                gaction_callbacks[widget] = handler

        # read the action groups
        for ag in uimanager.get_action_groups():
            # only add the action group if we don't have it
            gag = self.get_action_group(ag.get_name())
            if not gag:
                gag = GActionGroup.new(ag)
                self.add_action_group(gag)
            for action in ag.list_actions():
                ga = GAction.new(action, gag)
                # only add the action if the action group does not have it
                if not gag.get_action(ga.name):
                    ga.callback = gaction_callbacks.get(action, '') 
                    gag.add_action(ga)

        # load the ui definitions
        for ui_name, xml_string in loader.get_ui_definitions():
            # we need to split this ui definition into several widgets
            doc = minidom.parseString(xml_string)
            self._split_ui(doc, ui_name)

    def _split_ui(self, document, ui_name):
        """Split a XML tree into several ones, each one with only a widget
        definition"""
        root = document.documentElement
        for node in util.xml_filter_nodes(root.childNodes, root.ELEMENT_NODE):
            xml_string = node.toxml()
            name = node.getAttribute(tags.XML_TAG_NAME)
            if not self.loaded_uis.has_key(name):
                self.loaded_uis[name] = []
            
            self.loaded_uis[name].append((ui_name, xml_string))

    def load_widget(self, gwidget, old_name):
        """Load the gwidget replacing its gtk_widget by the one the uimanager
        has.
        
        old_name is the name the widget had before creating the gwidget from
        it and we are inside a paste operation it will be different to the
        name the gwidget has now.

        Also moves the dictionary self.loaded_uis into the self.uis"""
        
        # we have to load the ui manager first
        if not self.is_loading():
            self.load(self.project.get_loader())

        if not self.loaded_uis.has_key(old_name):
            return
        
        self.uis[gwidget] = {}
        for ui_name, xml_string in self.loaded_uis[old_name]:
            if gwidget.name != old_name:
                # we need to change the xml_string to change the widget name
                xml_string = self._change_ui_definition(xml_string,
                                                        gwidget.name)
            
            self.add_ui(gwidget, xml_string, ui_name)

        del self.loaded_uis[old_name]
        
        gtk_widget = self.get_gtk_widget(gwidget)

        old_gtk_widget = gwidget.gtk_widget
        parent = old_gtk_widget.get_parent()
        if parent:

            props = {}
            for pspec in gtk.container_class_list_child_properties(type(parent)):
                props[pspec.name] = parent.child_get_property(old_gtk_widget,
                                                              pspec.name)


            parent.remove(old_gtk_widget)
            parent.add(gtk_widget)

            for name, value in props.items():
                parent.child_set_property(gtk_widget, name, value)

        gwidget.setup_widget(gtk_widget)

    def is_loading(self):
        return len(self.loaded_uis.keys()) > 0

    def remove_widget(self, gwidget):
        "Remove all the uidefinitions of gwidget on the uimanager"
        for ui_name, (xml_string, merge_id) in self.uis[gwidget].items():
            self.uim.remove_ui(merge_id)

        del self.uis[gwidget]

    def add_widget(self, gwidget, definitions):
        """Add an existing widget with its definitions to the uim.

        Also, update the gtk_widget of gwidget after this change
        """
        self.uis[gwidget] = {}
        for ui_name, (xml_string, old_merge_id) in definitions.items():
            merge_id = self.uim.add_ui_from_string(xml_string)
            self.uis[gwidget][ui_name] = (xml_string, merge_id)

        gtk_widget = self.get_gtk_widget(gwidget)
        gwidget.setup_widget(gtk_widget)

    def save(self, document, ui_gwidgets):
        """Create a XML node with the information of this UIManager

        Only the information about the ui_gwidgets of this UIManager is
        actually saved.
        """
        uim_node = document.createElement(tags.XML_TAG_OBJECT)
        uim_node.setAttribute(tags.XML_TAG_CLASS, "GtkUIManager")
        uim_node.setAttribute(tags.XML_TAG_ID, "uimanager")

        # first save action groups
        for action_group in self.action_groups:
            # XXX don't save all the action groups, only those ones
            # that gwidgeta use
            child_node = document.createElement(tags.XML_TAG_CHILD)
            uim_node.appendChild(child_node)
            ag_node = action_group.write(document)
            child_node.appendChild(ag_node)

        nodes = self.save_ui_definitions(document, ui_gwidgets)

        for ui_node in nodes:
            uim_node.appendChild(ui_node)

        return uim_node
    
    def save_ui_definitions(self, document, ui_gwidgets):
        """Return a list of XML nodes with the ui definitions of the
        ui_gwidgets we need to save
        """
        uis = self._group_ui_definitions_by_name(ui_gwidgets)

        uis = self._get_required_uis(uis, ui_gwidgets)

        nodes = []
        for ui_name, xml_list in uis.items():
            ui_node = document.createElement(tags.XML_TAG_UI)
            ui_node.setAttribute(tags.XML_TAG_ID, ui_name)

            uidef_node = document.createElement(tags.XML_TAG_UI)
            for xml_string in xml_list:
                tmp = minidom.parseString(xml_string)
                uidef_node.appendChild(tmp.documentElement)

            text = document.createCDATASection(uidef_node.toxml())
            ui_node.appendChild(text)
            nodes.append(ui_node)

        return nodes

    def _get_ui_widgets(self, gtk_widget):
        """Get a list of all the widgets inside the hierarchy that are widgets
        related to a UI Manager (e.g. Toolbars and Menubars).
        """

        result = []
        for widget_type in self.managed_types:
            if isinstance(gtk_widget, widget_type):
                print "Saving UI for type", str (widget_type)
                result.append(gtk_widget)
                break

        if isinstance(gtk_widget, gtk.Container):
            for child in gtk_widget.get_children():
                result += self._get_ui_widgets(child)

        return result

    def save_widget(self, widget, root, xml_document):
        gtk_widget = widget.gtk_widget

        ui_widgets = self._get_ui_widgets(gtk_widget)
        if ui_widgets:
            ui_gwidgets = map(Widget.from_widget, ui_widgets)
            uim_node = self.save(xml_document, ui_gwidgets)
            root.appendChild(uim_node)
        
    def _group_ui_definitions_by_name(self, ui_gwidgets):
        """Group the ui definitions of this UI Manager by name.

        Only group those that belong to any of ui_gwidgets
        In the UIManager they are usually stored by widget.
        """
        uis = {}
        for gwidget, ui_def in self.uis.items():
            if gwidget not in ui_gwidgets:
                continue
            for ui_name, (xml_string, merge_id) in ui_def.items():
                if uis.has_key(ui_name):
                    uis[ui_name].append(xml_string)
                else:
                    uis[ui_name] = [xml_string]
        return uis

    def _get_required_uis(self, ui_definitions, ui_gwidgets):
        """Return the ui definitions that the ui_gwidgets are using"""
        # In this dict we decide if we need to save this ui or not
        # Initialy we don't save any ui
        uis_to_save = dict([(k, False) for k in ui_definitions.keys()])

        # now, let's loop through the widgets we need to save to see
        # if we need to save their uis
        for gwidget in ui_gwidgets:
            gwidget_uis = self.get_ui(gwidget)
            if not gwidget_uis:
                continue
            
            uis_names = gwidget_uis.keys()
            for ui_name in uis_names:
                if ui_name in uis_to_save.keys():
                    uis_to_save[ui_name] = True

        # so let's save the ui definitions
        for ui_name, save in uis_to_save.items():
            if not save:
                del ui_definitions[ui_name]

        return ui_definitions

