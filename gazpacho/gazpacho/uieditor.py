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
import xml.dom.minidom

import gtk
import gobject

from gazpacho.editor import PropertyEditorDialog
from gazpacho.util import unselect_when_clicked_on_empty_space, select_iter
from gazpacho.util import unselect_when_press_escape, xml_filter_nodes

_ = gettext.gettext

class UIEditor(PropertyEditorDialog):
    def __init__(self):
        PropertyEditorDialog.__init__(self)
        self.uim = None
    
    # public methods (utility)
    def add_managed_type (self, managed_type):
        
        if managed_type not in self.uim.managed_types:
            self.uim.managed_types.append(managed_type)
    
    # overridable methods    
    def set_widget(self, widget, proxy):
        """Derivated editors should override this and add their type
           with add_managed_type() to get the UIManager saved"""
        super(UIEditor, self).set_widget(widget, proxy)
        self.uim = self.gwidget.project.uim
        
        self.load_ui()

    def _create_widgets(self):
        """Build the dialog window interface widgets
           Override if you want to create your own"""
        #XXX: Could this method be called build_editor_dialog?
        hbox = gtk.HBox(spacing=6)

        sw = gtk.ScrolledWindow()
        sw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        hbox.pack_start(sw, True, True)

        self.model = gtk.TreeStore(object)
        self.treeview = gtk.TreeView(self.model)
        self.treeview.connect('button-press-event',
                              unselect_when_clicked_on_empty_space)
        self.treeview.connect('key-press-event',
                              unselect_when_press_escape)
        selection = self.treeview.get_selection()
        selection.connect('changed', self._on_selection_changed)
        self.treeview.set_size_request(300, 300)
        sw.add(self.treeview)
        self.treeview.set_headers_visible(False)
        renderer = gtk.CellRendererText()
        column = gtk.TreeViewColumn()
        self.treeview.append_column(column)
        column.pack_start(renderer)
        column.set_cell_data_func(renderer, self._cell_data_function)
        
        self.v_button_box = gtk.VButtonBox()
        self.v_button_box.set_layout(gtk.BUTTONBOX_START)
        self.up = gtk.Button(stock=gtk.STOCK_GO_UP)
        self.up.connect('clicked', self._on_up_clicked)
        self.v_button_box.pack_start(self.up)
        self.down = gtk.Button(stock=gtk.STOCK_GO_DOWN)
        self.down.connect('clicked', self._on_down_clicked)
        self.v_button_box.pack_start(self.down)
 
        
#        hbox.pack_start(v_button_box, False, False)

        self.vbox.pack_start(hbox, True, True)

        h_button_box = gtk.HButtonBox()
        h_button_box.set_spacing(6)
        h_button_box.set_layout(gtk.BUTTONBOX_START)
        self.add = gtk.Button(stock=gtk.STOCK_ADD)
        self.add.connect('clicked', self._on_add_clicked)
        h_button_box.pack_start(self.add)
        self.remove = gtk.Button(stock=gtk.STOCK_REMOVE)
        self.remove.connect('clicked', self._on_remove_clicked)
        h_button_box.pack_start(self.remove)
        self.vbox.pack_start(h_button_box, False, False)

        label = gtk.Label()
        label.set_markup('<small>%s</small>' % \
                         _('Press Escape to unselected an item'))
        self.vbox.pack_start(label, False, False)

    def create_empty_xml(self):
        """Create a new empty XML document for this UI
           Override and set the root node for derived editors"""
        self.xml_doc = xml.dom.getDOMImplementation().createDocument(None,
                                                                     None,
                                                                     None)

    def create_xml_node(self, parent, typename, name=None):
        """Create a node of the specified type
           Override if you need special actions on node types"""
        element = self.xml_doc.createElement(typename)
        if typename != 'separator':
            element.setAttribute('name', name)
            element.setAttribute('action', name)
        return parent.appendChild(element)
    
    def set_buttons_sensitiviness(self, typename):
        "Set edit buttons sensitiveness depending on class"
        
    def set_ui(self):
        """Apply the UI xml to the widget"""
        ui_string = self.root_node.toxml()
        self.uim.update_ui(self.gwidget, ui_string, 'initial-state')

    def load_ui(self):
        """Loads the UI to the editor
           Override if you don't keep a treemodel with the items around"""
        # XXX right now we only allow to edit the initial state
        ui = self.uim.get_ui(self.gwidget, 'initial-state')

        self.model.clear()
        if ui:
            xml_string, merge_id = ui
            self.xml_doc = xml.dom.minidom.parseString(xml_string)
            self.xml_doc.normalize()
            self.root_node = self.xml_doc.documentElement
            typename = self.root_node.tagName
        else:
            # create empty XML document
            self.create_empty_xml()
            
        for xml_node in xml_filter_nodes(self.root_node.childNodes,
                                         self.root_node.ELEMENT_NODE):
            self._load_node(xml_node, None)
            
        self.set_buttons_sensitiviness(typename)
        
    # Default implementation specific (private) methods


    def _load_node(self, xml_node, parent_iter):
        new_iter = self.model.append(parent_iter, (xml_node,))
        for child_node in xml_filter_nodes(xml_node.childNodes,
                                           xml_node.ELEMENT_NODE):
            self._load_node(child_node, new_iter)    

    def _cell_data_function(self, column, cell, model, current_iter):
        xml_node = model.get_value(current_iter, 0)
        if not xml_node:
          return
        typename = xml_node.tagName
        name = xml_node.getAttribute('name')
        if typename == 'separator':
            name = '<separator>'
        
        cell.set_property('text', name)

    def _get_selected_iter(self):
        selection = self.treeview.get_selection()
        model, selected_iter = selection.get_selected()
        return selected_iter

    # callbacks
    def _on_selection_changed(self, selection):
        "Get selected element and set edit buttons sensitiveness for it"
        name = None
        
        model, selected_iter = selection.get_selected()
        if selected_iter is not None:
            xml_node = model.get_value(selected_iter, 0)
            if xml_node:
              name = xml_node.tagName
        else:
            name = self.root_node.tagName
        
        if name:
          self.set_buttons_sensitiviness(name)

    def create_empty_xml(self):
        "Create a new empty XML document for this UI"
        self.xml_doc = xml.dom.getDOMImplementation().createDocument(None,
                                                                     None,
                                                                     None)
        
    def _on_add_clicked(self, button):
        selected_iter = self._get_selected_iter()

        dialog = self.create_add_dialog()

        if dialog.run() == gtk.RESPONSE_OK:
            name = dialog.get_name()
            parent_node = dialog.parent_node
            typename = dialog.typename
            if dialog.use_separator():
                typename = 'separator'

            node = self.create_xml_node(parent_node, typename, name)
            new_iter = self.model.append(selected_iter, (node,))
            select_iter(self.treeview, new_iter)
            self.set_ui()
                
        dialog.destroy()
        
    def _on_remove_clicked(self, button):
        selected_iter = self._get_selected_iter()
        if selected_iter is None:
            return
        
        xml_node = self.model.get_value(selected_iter, 0)
        self.model.remove(selected_iter)
        
        parent_node = xml_node.parentNode
        parent_node.removeChild(xml_node)

        self.set_ui()

    def _on_up_clicked(self, button):
        selected_iter = self._get_selected_iter()
        if selected_iter is None:
            return
        
        xml_node = self.model.get_value(selected_iter, 0)
        parent_node = xml_node.parentNode
        previous_sibling = xml_node.previousSibling
        if previous_sibling is not None:
            print previous_sibling
            tmp_node = parent_node.removeChild(xml_node)
            parent_node.insertBefore(tmp_node, previous_sibling)
            # not update the TreeView
            selected_path = self.model.get_path(selected_iter)
            print selected_path, type(selected_path)
            sibling_iter = self.model.get_iter((selected_path[0] - 1,))
            self.model.move_before(selected_iter, sibling_iter)
            
            self.set_ui()

    def _on_down_clicked(self, button):
        print 'not implemented yet'

    def _on_left_clicked(self, button):
        print 'not implemented yet'

    def _on_right_clicked(self, button):
        print 'not implemented yet'
    
class MenuBarUIEditor(UIEditor):
    def set_widget(self, widget, proxy):
        super(MenuBarUIEditor, self).set_widget(widget, proxy)
        self.add_managed_type (gtk.MenuBar)

    def _create_widgets(self):
        super(MenuBarUIEditor, self)._create_widgets()

        self.left = gtk.Button(stock=gtk.STOCK_GO_BACK)
        self.left.connect('clicked', self._on_left_clicked)
        self.right = gtk.Button(stock=gtk.STOCK_GO_FORWARD)
        self.right.connect('clicked', self._on_right_clicked)
        self.v_button_box.pack_start(self.left)
        self.v_button_box.pack_start(self.right)

    def set_buttons_sensitiviness(self, typename):
        super(MenuBarUIEditor, self).set_buttons_sensitiviness(typename)

        buttons = (self.add, self.remove, self.up, self.down, self.left,
                   self.right)
        sensitivity = {
            'menubar': (True, False, False, False, False, False),
            'menu': (True, True, True, True, False, True),
            'menuitem': (False, True, True, True, True, False),
            'separator': (False, True, True, True, False, False),
            }
        for i, s in enumerate(sensitivity[typename]):
            buttons[i].set_sensitive(s)

    def create_add_dialog(self):
        selected_iter = self._get_selected_iter()
        if selected_iter is None:
            # we are adding a toplevel
            dialog = AddMenuDialog(self.uim.action_groups, self)
            dialog.typename = 'menu'
            dialog.parent_node = self.root_node
        else:
            # we are adding a child menuitem for a menu
            dialog = AddMenuitemDialog(self.uim.action_groups, self)
            dialog.typename = 'menuitem'
            dialog.parent_node = self.model.get_value(selected_iter, 0)
        return dialog

    def create_empty_xml(self):
        super(MenuBarUIEditor, self)._create_empty_xml()

        self.root_node = self.create_xml_node(self.xml_doc, 'menubar',
                                              self.gwidget.name)

gobject.type_register(MenuBarUIEditor)

class ToolbarUIEditor(UIEditor):
    def set_widget(self, widget, proxy):
        super(ToolbarUIEditor, self).set_widget(widget, proxy)
        self.add_managed_type (gtk.Toolbar)

    def set_buttons_sensitiviness(self, typename):
        buttons = (self.add, self.remove, self.up, self.down)
        sensitivity = {
            'toolbar':(True, False, False, False),
            'toolitem': (False, True, True, True),
            'separator': (False, True, True, True),
            }
        for i, s in enumerate(sensitivity[typename]):
            buttons[i].set_sensitive(s)

    def create_add_dialog(self):
        selected_iter = self._get_selected_iter()
        dialog = AddToolitemDialog(self.uim.action_groups, self)
        dialog.typename = 'toolitem'
        dialog.parent_node = self.root_node
        return dialog

    def create_empty_xml(self):
        super(ToolbarUIEditor, self).create_empty_xml()

        self.root_node = self.create_xml_node(self.xml_doc, 'toolbar',
                                              self.gwidget.name)

gobject.type_register(ToolbarUIEditor)


class ChooseActionDialog(gtk.Dialog):
    """Dialog to choose an action from a list"""
    def __init__(self, action_groups=[], toplevel=None):
        gtk.Dialog.__init__(self,
                            title='',
                            parent=toplevel,
                            flags=gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
                            buttons=(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                                     gtk.STOCK_OK, gtk.RESPONSE_OK))
        self.set_border_width(6)
        self.set_resizable(False)
        self.set_has_separator(False)
        self.vbox.set_spacing(6)

        self.label = gtk.Label(_('Choose an action')+':')
        self.vbox.pack_start(self.label, False)
        
        # create a combo with all the actions
        model = gtk.ListStore(str)
        self.actions = gtk.ComboBox(model)
        renderer = gtk.CellRendererText()
        self.actions.pack_start(renderer)
        self.actions.add_attribute(renderer, 'text', 0)
        
        for action_group in action_groups:
            for action in action_group.get_actions():
                model.append(('%s/%s' % (action_group.name, action.name),))
        
        self.actions.set_active(0)
        
        self.vbox.pack_start(self.actions)
        
        self.set_default_response(gtk.RESPONSE_OK)

    def get_selected_action_name(self):
        model = self.actions.get_model()
        active_iter = self.actions.get_active_iter()
        if active_iter is not None:
            name = model.get_value(active_iter, 0)
            action_name = name.split('/')[1]
            return action_name
        
    def use_separator(self):
        """Subclasses will probably override this"""
        return False

gobject.type_register(ChooseActionDialog)

class AddMenuDialog(ChooseActionDialog):
    def __init__(self, action_groups=[], toplevel=None):
        ChooseActionDialog.__init__(self, action_groups, toplevel)

        self.set_title(_('Adding a menu'))
        self.vbox.show_all()
        
    def get_name(self):
        return self.get_selected_action_name()

gobject.type_register(AddMenuDialog)

class AddToolitemDialog(AddMenuDialog):
    def __init__(self, action_groups=[], toplevel=None):
        AddMenuDialog.__init__(self, action_groups, toplevel)
        
        # put the combo in other place
        self.vbox.remove(self.actions)
        self.vbox.remove(self.label)
        
        hbox = gtk.HBox(spacing=6)
        self.select_action = gtk.RadioButton(None, _('Action'))
        self.select_separator = gtk.RadioButton(self.select_action, _('Separator'))

        hbox.pack_start(self.select_action, False, False)
        hbox.pack_start(self.actions)
        
        self.vbox.pack_start(hbox, False, False)
        self.vbox.pack_start(self.select_separator, False, False)
            
        self.set_title(_('Adding a toolitem'))
        self.vbox.show_all()

    def use_separator(self):
        return self.select_separator.get_active()
        
gobject.type_register(AddToolitemDialog)

class AddMenuitemDialog(AddToolitemDialog):
    def __init__(self, action_groups=[], toplevel=None):
        AddToolitemDialog.__init__(self, action_groups, toplevel)
        
        self.set_title(_('Adding a menuitem'))
        self.vbox.show_all()
    
gobject.type_register(AddMenuitemDialog)
