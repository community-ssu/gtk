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
import os.path

import gtk
import gobject

from gazpacho import util
from gazpacho.context import Context
from gazpacho.loader.loader import ObjectBuilder
from gazpacho.loader.custom import ButtonAdapter, WindowAdapter, str2bool, \
     adapter_registry
from gazpacho.placeholder import Placeholder
from gazpacho.uim import GazpachoUIM
from gazpacho.widget import Widget, load_widget_from_gtk_widget
from gazpacho.widgets.base.custom import Custom
from gazpacho.filewriter import XMLWriter
from gazpacho.pomanager import PoManager
from gazpacho.celllayouteditor import LayoutRegistry
from gazpacho.treemodelregistry import TreeModelRegistry

_ = gettext.gettext

class GazpachoObjectBuilder(ObjectBuilder):
    def __init__(self, **kwargs):
        self._app = kwargs.pop('app', None)
        kwargs['placeholder'] = self.create_placeholder
        kwargs['custom'] = Custom
        ObjectBuilder.__init__(self, **kwargs)

    def create_placeholder(self, name):
        return Placeholder(self._app)

    def show_windows(self):
        pass

class GazpachoButtonAdapter(ButtonAdapter):
    def add(self, parent, child, properties):
        # This is a hack, remove when the button saving/loading code
        # is improved
        for cur_child in parent.get_children():
            parent.remove(cur_child)
        ButtonAdapter.add(self, parent, child, properties)
        
    
class GazpachoWindowAdapter(WindowAdapter):
    object_type = gtk.Window
    def prop_set_visible(self, window, value):
        window.set_data('gazpacho::visible', str2bool(value))
adapter_registry.register_adapter(GazpachoWindowAdapter)

class Project(gobject.GObject):

    __gsignals__ = {
        'add_widget':          (gobject.SIGNAL_RUN_LAST, None, (object,)),
        'remove_widget':       (gobject.SIGNAL_RUN_LAST, None, (object,)),
        'widget_name_changed': (gobject.SIGNAL_RUN_LAST, None, (object,)),
        'project_changed':     (gobject.SIGNAL_RUN_LAST, None, ()),
        'selection_changed':   (gobject.SIGNAL_RUN_LAST, None, ()),
        'add_action':          (gobject.SIGNAL_RUN_LAST, None, (object,)),
        'remove_action':       (gobject.SIGNAL_RUN_LAST, None, (object,)),
        'action_name_changed': (gobject.SIGNAL_RUN_LAST, None, (object,)),
        }
    
    project_counter = 1
    
    def __init__(self, untitled, app, loader=None):
        gobject.GObject.__init__(self)
        self._widget_cache = {}

        self._app = app
        self._loader = loader
        
        self.name = None
       
        self.extra_strings = {}
        self.extra_plural_strings = {}

        self.po_manager = PoManager()

        if untitled:
            # The name of the project like network-conf
            self.name = _('Untitled %d') % Project.project_counter
            Project.project_counter += 1

        # The full path of the xml file for this project
        self.path = None

        # The menu entry in the /Project menu
        self.entry = None
        
        # A list of GtkWidgets that make up this project. The widgets are
        # stored in no particular order.
        self.widgets = []

        # We need to keep the selection in the project because we have multiple
        # projects and when the user switchs between them, he will probably
        # not want to loose the selection. This is a list of GtkWidget items.
        self.selection = []

        # widget -> old name of the widget
        self._widget_old_names = {}
        self.tooltips = gtk.Tooltips()

        # A flag that is set when a project has changes if this flag is not set
        # we don't have to query for confirmation after a close or exit is
        # requested
        self._changed = False

        # There is a UIManager in each project which holds the information
        # about menus and toolbars
        self.uim = GazpachoUIM(self)
        
        # This is the id for the menuitem entry on Gazpacho main menu for this
        # project
        self.uim_id = -1

        # Contains the undo (and redo) commands
        self.undo_stack = UndoRedoStack()
        self.undo_stack.connect('changed', self._undo_stack_changed_cb)
        
        # create our context
        self.context = Context(self)

        # The CellLayout registry
        self.layout_registry = LayoutRegistry()

        # The TreeModel registry
        self.treemodel_registry = TreeModelRegistry()

    def _undo_stack_changed_cb(self, stack):
        """Callback for the undo stack's 'changed' signal."""
        self.changed = True

    def get_app(self):
        return self._app
    
    def get_id(self):
        return "project%d" % id(self)

    def get_loader(self):
        return self._loader
    
    def set_loader(self, loader):
        self._loader = loader

    def get_loader(self):
        return self._loader
    
    def set_loader(self, loader):
        self._loader = loader
    
    def get_changed(self):
        return self._changed

    def set_changed(self, value):
        if self._changed != value:
            self._changed = value
            self.emit('project_changed')

    changed = property(get_changed, set_changed)
    
    def selection_changed(self):
        self.emit('selection_changed')

    def _on_widget_notify_name(self, gwidget, pspec):
        old_name = self._widget_old_names[gwidget]
        self.widget_name_changed(gwidget, old_name)
        self._widget_old_names[gwidget] = gwidget.name

        if isinstance(gwidget.gtk_widget,
                      (gtk.MenuBar, gtk.Toolbar)):
            self.uim.update_widget_name(gwidget)
        
    def _on_widget_notify_project(self, gwidget, pspec):
        self.remove_widget(gwidget.gtk_widget)

    def add_widget(self, gtk_widget):
        # we don't list placeholders
        if isinstance(gtk_widget, Placeholder):
            return

        # If it's a container, add the children as well
        if isinstance(gtk_widget, gtk.Container):
            for child in gtk_widget.get_children():
                self.add_widget(child)
        
        gwidget = Widget.from_widget(gtk_widget)

        # The internal widgets (e.g. the label of a GtkButton) are handled
        # by gtk and don't have an associated GladeWidget: we don't want to
        # add these to our list. It would be nicer to have a flag to check
        # (as we do for placeholders) instead of checking for the associated
        # GladeWidget, so that we can assert that if a widget is _not_
        # internal, it _must_ have a corresponding GladeWidget...
        # Anyway this suffice for now.
        if not gwidget:
            return



        gwidget.project = self
        self._widget_old_names[gwidget] = gwidget.name

        gwidget.connect('notify::name', self._on_widget_notify_name)
        gwidget.connect('notify::project', self._on_widget_notify_project)
        self.widgets.append(gtk_widget)

        self.emit('add_widget', gwidget)

    def remove_widget(self, gtk_widget):
        if isinstance(gtk_widget, Placeholder):
            return

        if isinstance(gtk_widget, gtk.Container):
            for child in gtk_widget.get_children():
                self.remove_widget(child)

        gwidget = Widget.from_widget(gtk_widget)
        if not gwidget:
            return

        if gtk_widget in self.selection:
            self.selection_remove(gtk_widget, False)
            self.selection_changed()

        self.release_widget_name(gwidget, gwidget.name)
        self.widgets.remove(gtk_widget)

        self.emit('remove_widget', gwidget)

    def add_action_group(self, action_group):
        self.uim.add_action_group(action_group)

        self.emit('add_action', action_group)
    
    def remove_action_group(self, action_group):
        self.uim.remove_action_group(action_group)
        self.emit('remove_action', action_group)
        
    def _add_action_cb(self, action_group, action):
        self.emit('add_action', action)
    
    def _remove_action_cb(self, action_group, action):
        self.emit('remove_action', action)
    
    def change_action_name(self, action):
        self.emit('action_name_changed', action)
         
    def release_widget_name(self, gwidget, name):
        pass # XXX TODO

    def widget_name_changed(self, gwidget, old_name):
        self.release_widget_name(gwidget, old_name)
        self.emit('widget_name_changed', gwidget)
    
    def selection_clear(self, emit_signal):
        if not self.selection:
            return
        
        for gtk_widget in self.selection:
            util.remove_nodes(gtk_widget)
            
        self.selection = []
        
        if emit_signal:
            self.selection_changed()

    def selection_remove(self, gtk_widget, emit_signal):
        """ Remove the widget from the current selection """
        if not util.has_nodes(gtk_widget):
            return

        util.remove_nodes(gtk_widget)

        self.selection.remove(gtk_widget)
        if emit_signal:
            self.selection_changed()

    def selection_add(self, gtk_widget, emit_signal):
        if util.has_nodes(gtk_widget):
            return

        util.add_nodes(gtk_widget)

        self.selection.insert(0, gtk_widget)
        if emit_signal:
            self.selection_changed()
        
    def selection_set(self, gtk_widget, emit_signal):
        # If the widget is already selected we don't have to bother
        if util.has_nodes(gtk_widget):
            # But we need to emit the selection_changed signal if the
            # widget isn't in the active project
            if self._app._project != self and emit_signal:
                self.selection_changed()
            return

        self.selection_clear(False)
        self.selection_add(gtk_widget, emit_signal)

    def delete_selection(self):
        """ Delete (remove from the project) the widgets in the selection """
        newlist = list(self.selection)
        for gtk_widget in newlist:
            gwidget = Widget.from_widget(gtk_widget)
            if gwidget is not None:
                self._app.command_manager.delete(gwidget)
            elif isinstance(gtk_widget, Placeholder):
                self._app.command_manager.delete_placeholder(gtk_widget)

    def new_widget_name(self, base_name, blacklist=[]):
        """Create a new name based on the base_name. The name should not
        exist neither in the project neither in the blacklist"""
        i = 0
        while True:
            i += 1
            name = '%s%d' % (base_name, i)
            if self._widget_cache.has_key(name):
                continue

            if name in blacklist:
                continue
            
            if not self.get_widget_by_name(name):
                return name

    def get_widget_by_name(self, name):
        for w in self.widgets:
            gw = Widget.from_widget(w)
            if gw.name == name:
                self._widget_cache[name] = gw
                return gw

    def _create_placeholder(self, name):
        return Placeholder(self._app)
        
    def open(path, app, buffer=None):
        loader = GazpachoObjectBuilder(filename=path, buffer=buffer, app=app)
        
        project = Project(False, app, loader)
        
        # Load the widgets
        for gtk_widget in loader.toplevels:
            
            # Grab the TreeModels
            if isinstance(gtk_widget, (gtk.TreeStore, gtk.ListStore)):
                project.treemodel_registry.add(gtk_widget,
                                          gtk_widget.get_data("gazpacho::name"))
            
            # GObjects are not displayed in the widget tree
            if not isinstance(gtk_widget, gtk.Widget):
                continue

            load_widget_from_gtk_widget(gtk_widget, project)
            project.add_widget(gtk_widget)

        if project is not None:
            project.path = path
            project.name = os.path.basename(path)
            project.changed = False

        project.extra_strings = loader.extra_strings
        project.extra_plural_strings = loader.extra_plural_strings

        return project

    open = staticmethod(open)
    
    def save(self, path):
        self.path = path
        self.name = os.path.basename(path)

        xw = XMLWriter(project=self)
        retval = xw.write(path, self.widgets, self.uim)
        
        self.changed = False
        return retval

    def serialize(self):
        xw = XMLWriter(project=self)
        return xw.serialize_widgets(self.widgets, self.uim)
    
    def set_po_headers(self, headers):
        self.po_manager.set_headers(headers)
    
    def generate_po_file(self, filename):
        self.po_manager.generate_po(self.serialize()[:-1], filename)
    
    def import_po_file(self, filename):
        self.po_manager.import_po(self, filename)
    
    def get_po_headers(self):
        return self.po_manager.get_headers()

gobject.type_register(Project)

class UndoRedoStack(gobject.GObject):
    """Class representing an undo/redo stack."""
    
    __gsignals__ = {
        'changed': (gobject.SIGNAL_RUN_LAST, None, ())
        }

    def __init__(self):
        gobject.GObject.__init__(self)
                
        #  A stack with the last executed commands
        self._commands = []

        #  Points to current undo command
        self._current_undo_command = -1
        
    def has_undo(self):
        """Check if there are any commands in the undo stack."""
        return self._current_undo_command != -1

    def has_redo(self):
        """Check if there are any commands in the redo stack."""
        return self._current_undo_command + 1 != len(self._commands)

    def get_undo_info(self):
        """Get the current undo command description."""
        if self.has_undo():
            return self._commands[self._current_undo_command].description
        return None

    def get_redo_info(self):
        """Get the current redo command description."""
        if self.has_redo():
            return self._commands[self._current_undo_command + 1].description
        return None

    def pop_undo(self):
        """Return the current undo command and move it to the redo stack."""
        if self.has_undo():
            cmd = self._commands[self._current_undo_command]
            self._current_undo_command -= 1
            self.emit('changed')
            return cmd
        return None

    def pop_redo(self):
        """Return the current redo command and move it to the undo stack."""
        if self.has_redo():
            cmd =  self._commands[self._current_undo_command + 1]
            self._current_undo_command += 1
            self.emit('changed')
            return cmd
        return None
        
    def push_undo(self, command):
        """Put the command in the project command stack
        It tries to collapse the command with the previous command
        """
        # If there are no "redo" items, and the last "undo" item unifies with
        # us, then we collapse the two items in one and we're done
        if self.has_undo() and not self.has_redo():
            cmd = self._commands[self._current_undo_command]
            if cmd.unifies(command):
                cmd.collapse(command)
                self.emit('changed')
                return

        # We should now free all the 'redo' items
        self._commands = self._commands[:self._current_undo_command + 1]

        # and then push the new undo item
        self._commands.append(command)
        self._current_undo_command += 1

        self.emit('changed')

    def get_undo_commands(self):
        """Get a list of all undo commands."""
        return self._commands[:self._current_undo_command + 1]

    def get_redo_commands(self):
        """Get a list of all redo commands."""
        return self._commands[self._current_undo_command + 1:]

gobject.type_register(UndoRedoStack)
