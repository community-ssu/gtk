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
import gobject

from gazpacho import util
from gazpacho.gaction import GAction, GActionGroup
from gazpacho.widget import Widget

from gazpacho.widget import copy_widget
from gazpacho.widgetregistry import widget_registry
from gazpacho.dndhandlers import DND_POS_TOP, DND_POS_BOTTOM, DND_POS_LEFT

_ = gettext.gettext

class CommandManager(object):
    """This class is the entry point accesing the commands.
    Every undoable action in Gazpacho is wrapped into a command.
    The stack of un/redoable actions is stored in the Project class
    so each project is independent on terms of undo/redo.
    
    The CommandManager knows how to perform the undo and redo actions.
    There is also a method for every type of command supported so
    you don't have to worry about creating the low level command
    objects.
    """
    def __init__(self, app):
        self.app = app
        
    def undo(self, project):
        """Undo the last command if there is such a command"""
        if project.undo_stack.has_undo():
            cmd = project.undo_stack.pop_undo()
            cmd.undo()
    
    def redo(self, project):
        """Redo the last undo command if there is such a command"""
        if project.undo_stack.has_redo():
            cmd = project.undo_stack.pop_redo()
            cmd.redo()

    #
    # for every possible command we have a method here
    #

    def delete(self, widget_instance):
        # internal children cannot be deleted. Should we notify the user?
        if widget_instance.internal_name is not None:
            return
        description = _("Delete %s") % widget_instance.name
        cmd = CommandCreateDelete(widget_instance, None,
                                  widget_instance.get_parent(), False,
                                  description, self.app)
        cmd.execute()
        widget_instance.project.undo_stack.push_undo(cmd)

    def create(self, adaptor, placeholder, project, parent=None,
               interactive=True):
        if placeholder:
            parent = util.get_parent(placeholder)
            if parent is None:
                return

        if project is None:
            project = parent.project

        widget_instance = Widget(adaptor, project)
        widget_instance.create_gtk_widget(interactive)
        if widget_instance is None:
            return

        description = _("Create %s") % widget_instance.name

        cmd = CommandCreateDelete(widget_instance, placeholder, parent, True,
                                  description, self.app)
        cmd.execute()
        widget_instance.project.undo_stack.push_undo(cmd)

        if not self.app.palette.persistent_mode:
            self.app.palette.unselect_widget()

    def delete_placeholder(self, placeholder):
        parent = util.get_parent(placeholder)
        if len(parent.gtk_widget.get_children()) == 1:
            return
        
        description = _("Delete placeholder")
        cmd = CommandDeletePlaceholder(placeholder, parent, description,
                                       self.app)
        cmd.execute()
        parent.project.undo_stack.push_undo(cmd)
        
    def box_insert_placeholder(self, box, pos):
        description = _("Insert after")

        cmd = CommandInsertPlaceholder(box, pos, description, self.app)
        cmd.execute()
        box.project.undo_stack.push_undo(cmd)

    def set_property(self, prop, value):
        dsc = _('Setting %s of %s') % (prop.name, prop.get_object_name())
        cmd = CommandSetProperty(prop, value, dsc, self.app)
        cmd.execute()
        project = prop.get_project()
        project.undo_stack.push_undo(cmd)
        
    def set_translatable_property(self, prop, value, engineering_english, comment,
                                  translatable, has_context):
        gwidget = prop.widget
        dsc = _('Setting %s of %s') % (prop.name, gwidget.name)
        cmd = CommandSetTranslatableProperty(prop, value, engineering_english,
                                             comment, translatable,
                                             has_context,
                                             dsc, self.app)
        cmd.execute()
        project = prop.get_project()
        project.changed = True
        project.undo_stack.push_undo(cmd)

    def add_signal(self, widget_instance, signal):
        dsc = _('Add signal handler %s') % signal['handler']
        cmd = CommandAddRemoveSignal(True, signal, widget_instance, dsc,
                                     self.app)
        cmd.execute()
        widget_instance.project.undo_stack.push_undo(cmd)
        
    def remove_signal(self, widget_instance, signal):
        dsc = _('Remove signal handler %s') % signal['handler']
        cmd = CommandAddRemoveSignal(False, signal, widget_instance, dsc,
                                     self.app)
        cmd.execute()
        widget_instance.project.undo_stack.push_undo(cmd)

    def change_signal(self, widget_instance, old_signal, new_signal):
        dsc = _('Change signal handler for signal "%s"') % old_signal['name']
        cmd = CommandChangeSignal(widget_instance, old_signal, new_signal,
                                  dsc, self.app)
        cmd.execute()
        widget_instance.project.undo_stack.push_undo(cmd)

    def copy(self, widget_instance):
        """Add a copy of the widget to the clipboard.

        Note that it does not make sense to undo this operation
        """
        clipboard = self.app.get_clipboard()
        clipboard.add_widget(widget_instance)
        
    def cut(self, widget_instance):
        dsc = _('Cut widget %s into the clipboard') % widget_instance.name
        clipboard = self.app.get_clipboard()
        clipboard.add_widget(widget_instance)
        cmd = CommandCutPaste(widget_instance, widget_instance.project, None,
                              CommandCutPaste.CUT, dsc, self.app)
        cmd.execute()
        widget_instance.project.undo_stack.push_undo(cmd)
        
    def paste(self, placeholder, project):
        if project is None:
            raise ValueError("No project has been specified. Cannot paste "
                             "the widget")

        clipboard = self.app.get_clipboard()
        gwidget = clipboard.get_selected_widget(project)

        dsc = _('Paste widget %s from the clipboard') % gwidget.name
        cmd = CommandCutPaste(gwidget, project, placeholder,
                              CommandCutPaste.PASTE, dsc, self.app)
        cmd.execute()
        project.undo_stack.push_undo(cmd)

    def add_action(self, values, parent, project):
        gact = GAction(parent, values['name'], values['label'],
                       values['tooltip'], values['stock_id'],
                       values['callback'], values['accelerator'])
        dsc = _('Add action %s') % gact.name
        cmd = CommandAddRemoveAction(parent, gact, True, dsc, self.app)
        cmd.execute()
        project.undo_stack.push_undo(cmd)
    
    def remove_action(self, gact, project):
        dsc = _('Remove action %s') % gact.name
        cmd = CommandAddRemoveAction(gact.parent, gact, False, dsc, self.app)
        cmd.execute()
        project.undo_stack.push_undo(cmd)
    
    def edit_action(self, gact, new_values, project):
        dsc = _('Edit action %s') % gact.name
        cmd = CommandEditAction(gact, new_values, project, dsc, self.app)
        cmd.execute()
        project.undo_stack.push_undo(cmd)

    def add_action_group(self, name, project):
        gaction_group = GActionGroup(name)
        dsc = _('Add action group %s') % gaction_group.name
        cmd = CommandAddRemoveActionGroup(gaction_group, project, True,
                                          dsc, self.app)
        cmd.execute()
        project.undo_stack.push_undo(cmd)
    
    def remove_action_group(self, gaction_group, project):
        dsc = _('Remove action group %s') % gaction_group.name
        cmd = CommandAddRemoveActionGroup(gaction_group, project, False,
                                          dsc, self.app)
        cmd.execute()
        project.undo_stack.push_undo(cmd)
        
    def edit_action_group(self, gaction_group, new_name, project):
        dsc = _('Edit action group %s') % gaction_group.name
        cmd = CommandEditActionGroup(gaction_group, new_name, project, 
                                     dsc, self.app)
        cmd.execute()
        project.undo_stack.push_undo(cmd)

    def set_button_contents(self, gwidget, stock_id=None, notext=False,
                            label=None, image_path=None, position=-1):
        dsc = _("Setting button %s contents") % gwidget.name
        cmd = CommandSetButtonContents(gwidget, stock_id, notext,
                                       label, image_path, position,
                                       dsc, self.app)
        cmd.execute()
        gwidget.project.undo_stack.push_undo(cmd)

    def execute_drag_drop(self, source_widget, target_placeholder):
        """Execute a drag and drop action, i.e. move a widget from one
        place to another."""
        dsc = "Drag and Drop widget %s" % source_widget.name
        cmd = CommandDragDrop(dsc, self.app)
        cmd.set_source(source_widget)

        cmd.set_target(source_widget, target_placeholder)
        cmd.execute()
        source_widget.project.undo_stack.push_undo(cmd)

    def execute_drag(self, source_widget):
        """Execute a drag action, i.e. remove the widget. The drop
        will be handled elsewhere."""
        dsc = "Drag widget %s" % source_widget.name
        cmd = CommandDragDrop(dsc, self.app)
        cmd.set_source(source_widget)
        cmd.execute()
        source_widget.project.undo_stack.push_undo(cmd)

    def execute_drop(self, widget_instance, target_placeholder):
        """Execute a drop action, i.e. add a widget. The drag
        operation will be handled elsewhere."""
        dsc = "Drop widget %s" % widget_instance.name
        cmd = CommandDragDrop(dsc, self.app)
        cmd.set_target(widget_instance, target_placeholder)
        cmd.execute()
        widget_instance.project.undo_stack.push_undo(cmd)

    def execute_drag_extend(self, source_widget, target_widget, location, keep_source):
        cmd = DragExtendCommand(source_widget, target_widget, location,
                                keep_source, "Drag - extend", self.app)
        cmd.execute()
        target_widget.project.undo_stack.push_undo(cmd)
        
    def execute_drag_append(self, source_widget, target_box, pos, keep_source):
        cmd = DragAppendCommand(source_widget, target_box, pos, keep_source,
                                "Drag - append", self.app)
        cmd.execute()
        target_box.project.undo_stack.push_undo(cmd)
            
class Command(object):
    """A command is the minimum unit of undo/redoable actions.

    It has a description so the user can see it on the Undo/Redo
    menu items.

    Every Command subclass should implement the 'execute' method in
    the following way:
        - After a command is executed, if the execute method is called
        again, their effects will be undone.
        - If we keep calling the execute method, the action will
        be undone, then redone, then undone, etc...
        - To acomplish this every command constructor will probably
        need to gather more data that it may seems necessary.

    After you execute a command in the usual way you should put that
    command in the command stack of that project and that's what
    the push_undo method does. Otherwise no undo will be available.
   
    Some commands unifies themselves. This means that if you execute
    several commands of the same type one after the other, they will be
    treated as only one big command in terms of undo/redo. In other words,
    they will collapse. For example, every time you change a letter on a
    widget's name it is a command but all these commands unifies to one
    command so if you undo that all the name will be restored to the old one.
    """
    def __init__(self, description=None, app=None):
        self.description = description
        self._app = app

    def __repr__(self):
        return self.description

    def execute(self):
        """ This is the main method of the Command class.
        Note that it does not have any arguments so all the
        necessary data should be provided in the constructor.
        """
        pass

    def undo(self):
        """Convenience method that just call execute"""
        self.execute()
    
    def redo(self):
        """Convenience method that just call redo"""
        self.execute()
    
    def unifies(self, other):
        """True if self unifies with 'other'
        Unifying means that both commands can be treated as they
        would be only one command
        """
        return False
    
    def collapse(self, other):
        """Combine self and 'other' to form only one command.
        'other' should unifies with self but this method does not
        check that.
        """
        return False

class CommandCreateDelete(Command):
    def __init__(self, widget_instance=None, placeholder=None, parent=None,
                 create=True, description=None, application=None):
        Command.__init__(self, description, application)

        self._widget_instance = widget_instance
        self._placeholder = placeholder
        self._parent = parent
        self._create = create
        self._initial_creation = create
        self._ui_defs = {}
        
    def _create_execute(self):
        from gazpacho import placeholder

        widget_instance = self._widget_instance

        # if _delete_execute was called on a toolbar/menubar the uim removed
        # its ui definitions so we need to add them again
        if self._ui_defs:
            widget_instance.project.uim.add_widget(widget_instance,
                                                   self._ui_defs)
            self._ui_defs = {}
        
        if not widget_instance.gtk_widget.flags() & gtk.TOPLEVEL:
            if self._placeholder is None:
                for child in self._parent.gtk_widget.get_children():
                    if isinstance(child, placeholder.Placeholder):
                        self._placeholder = child
                        break
            Widget.replace(self._placeholder,
                           widget_instance.gtk_widget,
                           self._parent)

        self._widget_instance.project.add_widget(widget_instance.gtk_widget)
        self._widget_instance.project.selection_set(widget_instance.gtk_widget,
                                                    True)

        if isinstance(widget_instance.gtk_widget, gtk.Widget):
            widget_instance.gtk_widget.show_all()

        if widget_instance.is_toplevel():
            # we have to attach the accelerators groups so key shortcuts
            # keep working when this window has the focus. Only do
            # this the first time when creating a window, not when
            # redoing the creation since the accel group is already
            # set by then
            if self._initial_creation:
                for ag in self._app.get_accel_groups():
                    widget_instance.gtk_widget.add_accel_group(ag)
                self._initial_creation = False
                
            # make window management easier by making created windows
            # transient for the editor window
#            app_window = self._app.get_window()
#            widget_instance.gtk_widget.set_transient_for(app_window)

    def _delete_execute(self):
        from gazpacho import placeholder
        widget_instance = self._widget_instance
        
        if self._parent:
            if self._placeholder is None:
                self._placeholder = placeholder.Placeholder(self._app)

            Widget.replace(widget_instance.gtk_widget,
                           self._placeholder, self._parent)

            #widget_adaptor = widget_instance.adaptor
            #self._parent.set_default_gtk_packing_properties(self._placeholder,
            #                                                widget_adaptor)
        
        widget_instance.gtk_widget.hide()
        widget_instance.project.remove_widget(widget_instance.gtk_widget)

        # if we are removing a toolbar or a menubar we need to update the
        # uimanager
        if isinstance(widget_instance.gtk_widget, (gtk.Toolbar, gtk.MenuBar)):
            d = widget_instance.project.uim.get_ui(widget_instance)
            if d:
                self._ui_defs = dict(d)
                widget_instance.project.uim.remove_widget(widget_instance)        
        return True

    def execute(self):
        if self._create:
            retval = self._create_execute()
        else:
            retval = self._delete_execute()

        self._create = not self._create

        return retval

class CommandDeletePlaceholder(Command):
    def __init__(self, placeholder=None, parent=None, description=None,
                 application=None):
        Command.__init__(self, description, application)

        self._placeholder = placeholder
        self._parent = parent
        self._create = False

        children = self._parent.gtk_widget.get_children()
        self._pos = children.index(placeholder)
        
    def _create_execute(self):
        from gazpacho import placeholder
        # create a placeholder and insert it at self._pos
        self._placeholder = placeholder.Placeholder(self._app)
        self._parent.gtk_widget.add(self._placeholder)
        self._parent.gtk_widget.reorder_child(self._placeholder, self._pos)

    def _delete_execute(self):
        self._placeholder.destroy()
        self._placeholder = None
        return True

    def execute(self):
        if self._create:
            retval = self._create_execute()
        else:
            retval = self._delete_execute()

        self._create = not self._create

        return retval

class CommandInsertPlaceholder(Command):
    def __init__(self, box, pos, description=None, application=None):
        Command.__init__(self, description, application)

        self._box = box
        self._pos = pos
        self._placeholder = None
        self._insert = True
        

    def _insert_execute(self):
        from gazpacho import placeholder
        # create a placeholder and insert it at self._pos
        self._placeholder = placeholder.Placeholder(self._app)
        self._box.gtk_widget.add(self._placeholder)
        self._box.gtk_widget.reorder_child(self._placeholder, self._pos)

    def _delete_execute(self):
        self._placeholder.destroy()
        self._placeholder = None
        return True

    def execute(self):
        if self._insert:
            retval = self._insert_execute()
        else:
            retval = self._delete_execute()

        self._insert = not self._insert

        return retval

class CommandSetProperty(Command):
    def __init__(self,  property=None, value=None, description=None,
                 application=None):
        Command.__init__(self, description, application)
        self._property = property
        self._value = value

    def execute(self):
        new_value = self._value
        # store the current value for undo
        self._value = self._property.value
        self._property.set(new_value)
        # if the property is the name, we explicitily set the name of the
        # gwidget to trigger the notify signal so several parts of the
        # interface get updated
        if self._property.name == 'name':
            self._property.widget.name = new_value

        return True

    def unifies(self, other):
        if isinstance(other, CommandSetProperty):
            return self._property == other._property
        return False

    def collapse(self, other):
        self.description = other.description
        other.description = None

class CommandSetTranslatableProperty(Command):
    def __init__(self,  property=None, value=None, engineering_english=None, comment=None,
                 translatable=False, has_context=False, description=None,
                 application=None):
        Command.__init__(self, description, application)

        self._property = property
        self._value = value
        self._comment = comment
        self._engineering_english = engineering_english
        self._translatable = translatable
        self._has_context = has_context

    def execute(self):
        new_value = self._value
        new_comment = self._comment
        new_engineering_english = self._engineering_english
        new_translatable = self._translatable
        new_has_context = self._has_context

        # store the current value for undo
        self._value = self._property.value
        self._comment = self._property.i18n_comment
        self._engineering_english = self._property.engineering_english
        self._translatable = self._property.translatable
        self._has_context = self._property.has_i18n_context

        self._property.set(new_value)
        self._property.translatable = new_translatable
        self._property.i18n_comment = new_comment
        self._property.engineering_english = new_engineering_english
        self._property.has_i18n_context = new_has_context

        return True

    def unifies(self, other):
        return False

    def collapse(self, other):
        return False

class CommandAddRemoveSignal(Command):
    def __init__(self, add=True, signal=None, widget_instance=None,
                 description=None, application=None):
        Command.__init__(self, description, application)
        self._add = add
        self._signal = signal
        self._widget_instance = widget_instance

    def execute(self):
        if self._add:
            self._widget_instance.add_signal_handler(self._signal)
        else:
            self._widget_instance.remove_signal_handler(self._signal)

        self._add = not self._add
        return True

class CommandChangeSignal(Command):
    def __init__(self, widget_instance, old_signal_handler, new_signal_handler,
                 description=None, application=None):
        Command.__init__(self, description, application)
        self._widget_instance = widget_instance        
        self._old_handler = old_signal_handler
        self._new_handler = new_signal_handler

    def execute(self):
        self._widget_instance.change_signal_handler(self._old_handler, self._new_handler)
        self._old_handler, self._new_handler = self._new_handler, self._old_handler

class CommandCutPaste(Command):

    CUT, PASTE = range(2)
    
    def __init__(self, widget_instance, project, placeholder, operation,
                 description=None, application=None):
        Command.__init__(self, description, application)

        self._project = project
        self._placeholder = placeholder
        self._operation = operation
        self._widget_instance = widget_instance
        self._ui_defs = {}

    def execute(self):
        if self._operation == CommandCutPaste.CUT:
            self._execute_cut()
            self._operation = CommandCutPaste.PASTE
        else:
            self._execute_paste()
            self._operation = CommandCutPaste.CUT
        
    def _execute_cut(self):
        from gazpacho import placeholder
        widget_instance = self._widget_instance
        
        if not widget_instance.is_toplevel():
            parent = widget_instance.get_parent()

            if not self._placeholder:
                self._placeholder = placeholder.Placeholder(self._app)

            Widget.replace(widget_instance.gtk_widget,
                           self._placeholder, parent)
            #parent.set_default_gtk_packing_properties(self._placeholder,
            #                                          widget_instance.adaptor)

        widget_instance.gtk_widget.hide()
        widget_instance.project.remove_widget(widget_instance.gtk_widget)

        # if we are removing a ui managed widget we need to update the
        # uimanager
        wtype = widget_instance.gtk_widget.__class__
        if wtype in widget_instance.project.uim.managed_types:
            d = widget_instance.project.uim.get_ui(widget_instance)
            if d is not None:
                self._ui_defs = dict(d)
                widget_instance.project.uim.remove_widget(widget_instance)

    def _execute_paste(self):
        # if we did cut a toolbar/menubar the uim removed
        # its ui definitions so we need to add them again
        if len(self._ui_defs.keys()) > 0:
            self._widget_instance.project.uim.add_widget(self._widget_instance,
                                                         self._ui_defs)
            self._ui_defs = {}

        if self._widget_instance.is_toplevel():
            project = self._project
        else:
            parent = util.get_parent(self._placeholder)
            project = parent.project
            Widget.replace(self._placeholder,
                           self._widget_instance.gtk_widget,
                           parent)
            
        project.add_widget(self._widget_instance.gtk_widget)
        project.selection_set(self._widget_instance.gtk_widget, True)
        
        self._widget_instance.gtk_widget.show_all()

        # We need to store the project of a toplevel widget to use
        # when undoing the cut.
        self._project = project

class CommandAddRemoveAction(Command):
    def __init__(self, parent, gact, add=True,
                 description=None, application=None):
        Command.__init__(self, description, application)

        self.add = add
        self.gact = gact
        self.parent = parent
        
    def execute(self):
        if self.add:
            self._add_execute()
        else:
            self._remove_execute()

        self.add = not self.add
    
    def _add_execute(self):
        self.parent.add_action(self.gact)
    
    def _remove_execute(self):
        self.parent.remove_action(self.gact)

class CommandEditAction(Command):
    def __init__(self, gact, new_values, project,
                 description=None, application=None):
        Command.__init__(self, description, application)
        
        self.new_values = new_values
        self.gact = gact
        self.project = project

    def execute(self):
        old_values = {
            'name' : self.gact.name,
            'label': self.gact.label,
            'stock_id': self.gact.stock_id,
            'tooltip': self.gact.tooltip,
            'accelerator': self.gact.accelerator,
            'callback': self.gact.callback,
            }
        self.gact.name = self.new_values['name']
        self.gact.label = self.new_values['label']
        self.gact.stock_id = self.new_values['stock_id']
        self.gact.tooltip = self.new_values['tooltip']
        self.gact.accelerator = self.new_values['accelerator']
        self.gact.callback = self.new_values['callback']
        self.gact.parent.update_action(self.gact, old_values['name'])
        self.new_values = old_values
        self.project.change_action_name(self.gact)


class CommandAddRemoveActionGroup(Command):
    def __init__(self, gaction_group, project, add=True,
                 description=None, application=None):
        Command.__init__(self, description, application)

        self.add = add
        self.project = project
        self.gaction_group = gaction_group
        self.gactions = self.gaction_group.get_actions()
        
    def execute(self):
        if self.add:
            self._add_execute()
        else:
            self._remove_execute()

        self.add = not self.add
    
    def _add_execute(self):
        self.project.add_action_group(self.gaction_group)
        for gaction in self.gactions:
            self.gaction_group.add_action(gaction)
    
    def _remove_execute(self):
        self.project.remove_action_group(self.gaction_group)

class CommandEditActionGroup(Command):
    def __init__(self, gaction_group, new_name, project,
                 description=None, application=None):
        Command.__init__(self, description, application)
        
        self.new_name = new_name
        self.gaction_group = gaction_group
        self.project = project

    def execute(self):
        old_name = self.gaction_group.name
        self.gaction_group.name = self.new_name
        self.new_name = old_name
        self.project.change_action_name(self.gaction_group)

class CommandSetButtonContents(Command):
    def __init__(self, gwidget, stock_id=None, notext=False,
                 label=None, image_path=None, position=-1,
                 description=None, application=None):
        Command.__init__(self, description, application)

        self.gwidget = gwidget
        self.stock_id = stock_id
        self.notext = notext
        self.label = label
        self.image_path = image_path
        self.position = position

    def execute(self):
        widget = self.gwidget
        button = self.gwidget.gtk_widget
        use_stock = widget.get_prop('use-stock')
        label = widget.get_prop('label')
        
        self._clear_button(button)

        state = util.get_button_state(button)
        
        if self.stock_id:
            # stock button
            
            if self.notext:
                image = gtk.Image()
                image.set_from_stock(self.stock_id, gtk.ICON_SIZE_BUTTON)
                image.show()
                button.add(image)
            else:
                use_stock.set(True)
                label.set(self.stock_id)
        else:
            # custom button. 3 cases:
            # 1) only text, 2) only image or 3) image and text
            if self.label and not self.image_path:
                # only text
                label.set(self.label)
            elif not self.label and self.image_path:
                # only image
                image = gtk.Image()
                image.set_from_file(self.image_path)
                image.set_data('image-file-name', self.image_path)
                image.show()
                button.add(image)
            elif self.label and self.image_path:
                # image and text
                align = gtk.Alignment(0.5, 0.5, 1.0, 1.0)
                if self.position in (gtk.POS_LEFT, gtk.POS_RIGHT):
                    box = gtk.HBox()
                else:
                    box = gtk.VBox()
                align.add(box)
                image = gtk.Image()
                image.set_from_file(self.image_path)
                image.set_data('image-file-name', self.image_path)
                label = gtk.Label(self.label)
                if '_' in self.label:
                    label.set_use_underline(True)
                    
                if self.position in (gtk.POS_LEFT, gtk.POS_TOP):
                    box.pack_start(image)
                    box.pack_start(label)
                else:
                    box.pack_start(label)
                    box.pack_start(image)
                    
                align.show_all()
                button.add(align)

        # save the state for undoing purposes
        (self.stock_id, self.notext, self.label,
         self.image_path, self.position) = state
        
    def _clear_button(self, button):
        "Clear the button and set default values for its properties"
        button.set_use_stock(False)
        button.set_use_underline(False)
        button.set_label('')

        child = button.get_child()
        if child:
            button.remove(child)

class CommandStackView(gtk.ScrolledWindow):
    """This class is just a little TreeView that knows how
    to show the command stack of a project.
    It shows a plain list of all the commands performed by
    the user and also it mark the current command that
    would be redone if the user wanted so.
    Older commands are under newer commands on the list.
    """
    def __init__(self):
        gtk.ScrolledWindow.__init__(self)
        self._project = None

        self._model = gtk.ListStore(bool, str)
        self._treeview = gtk.TreeView(self._model)
        self._treeview.set_headers_visible(False)
        
        column = gtk.TreeViewColumn()
        renderer1 = gtk.CellRendererPixbuf()
        column.pack_start(renderer1, expand=False)
        column.set_cell_data_func(renderer1, self._draw_redo_position)

        renderer2 = gtk.CellRendererText()
        column.pack_start(renderer2, expand=True)
        column.add_attribute(renderer2, 'text', 1)
        
        self._treeview.append_column(column)

        self.add(self._treeview)

    def set_project(self, project):
        self._project = project
        self.update()
        
    def update(self):
        self._model.clear()
        if self._project is None:
            return

        undos = self._project.undo_stack.get_undo_commands()
        if undos:
            for cmd in undos[:-1]:
                self._model.insert(0, (False, cmd.description))
            self._model.insert(0, (True, undos[-1].description))
            
        for cmd in self._project.undo_stack.get_redo_commands():
            self._model.insert(0, (False, cmd.description))

    def _draw_redo_position(self, column, cell, model, iter):
        is_the_one = model.get_value(iter, 0)

        if is_the_one:
            stock_id = gtk.STOCK_JUMP_TO
        else:
            stock_id = None

        cell.set_property('stock-id', stock_id)
        
gobject.type_register(CommandStackView)


class CommandDragDrop(Command):
    """Command for executing a drag and drop action. Depending on
    whether source and/or target is specified the command will do a
    Drag, Drop or a Drag and Drop."""
    
    def __init__(self, description=None, app=None):
        Command.__init__(self, description, app)

        self._undo = False
        self._drag_cmd = None
        self._drop_cmd = None
        
    def set_source(self, source_widget):
        """Specify the widget which is to be draged."""
        self._drag_cmd = CommandCutPaste(source_widget,
                                         source_widget.project,
                                         None,
                                         CommandCutPaste.CUT,
                                         None,
                                         self._app)

    def set_target(self, widget_instance, target_placeholder):
        """Specify the target for the drop as well as the widget which
        is to be added there."""
        self._drop_cmd = CommandCutPaste(widget_instance,
                                         widget_instance.project,
                                         target_placeholder,
                                         CommandCutPaste.PASTE,
                                         None,
                                         self._app)

    def execute(self):
        if self._undo:
            if self._drop_cmd:
                self._drop_cmd.execute()
            if self._drag_cmd:
                self._drag_cmd.execute()                    
        else:
            if self._drag_cmd:
                self._drag_cmd.execute()
            if self._drop_cmd:
                self._drop_cmd.execute()

        self._undo = not self._undo


class DragExtendCommand(Command):
    """
    Command for replacing a target widget with a box containing a copy
    of both the target and the source.
    """

    def __init__(self, source_widget, target_widget, location, keep_source,
                 description=None, application=None):
        """
        Initialize the command.
        
        @param source_widget: the widget to add 
        @type source_widget:
        @param target_widget: the widget to replace
        @type target_widget:
        @param location: Where to put the source in relation to the target
        @type location: (constant value)        
        @param keep_source: if true we should not remove the source
        widget, if false we should.
        @type keep_source: bool
        """
        Command.__init__(self, description, application)

        self._placeholder = None
        self._undo = False
        self._project = target_widget.project
        self._gtk_target = target_widget.gtk_widget
        self._remove_cmd = None 
        
        # Create the box with a copy of the target and source widgets
        target_copy = copy_widget(target_widget, target_widget.project)
        source_copy = copy_widget(source_widget, target_widget.project)

        # XXX is this correct?
        target_copy.name = target_widget.name
        if not keep_source:
            source_copy.name = source_widget.name
        
        self._gtk_box = self._create_box(source_copy.gtk_widget,
                                         target_copy.gtk_widget, location)

        # Reuse the Cut command for removing the widget
        if not keep_source:
            self._remove_cmd = CommandCutPaste(source_widget,
                                               source_widget.project,
                                               None,
                                               CommandCutPaste.CUT,
                                               None,
                                               application)
        
    def _create_box(self, gtk_source, gtk_target, location):
        """
        Create a Box containing the widgets.
        
        @param gtk_source: the gtk  widget to add 
        @type gtk_source: gtk.Widget
        @param gtk_target: the gtk widget to replace
        @type gtk_target: gtk.Widget
        @param location: Where to put the source in relation to the target
        @type location: (constant value)
        """
        # Create a Box with size 2
        if location in [DND_POS_TOP, DND_POS_BOTTOM]:
            box_type = 'GtkVBox'
        else:
            box_type = 'GtkHBox'
        adaptor = widget_registry.get_by_name(box_type)
        box_widget = Widget(adaptor, self._project)
        box_widget.create_gtk_widget(interactive=False)
        box_widget.get_prop('size').set(2)

        # Add the source and target widgets
        children = box_widget.gtk_widget.get_children()
        if location in [DND_POS_TOP, DND_POS_LEFT]:
            source_placeholder, target_placeholder = children[0], children[1]
        else:
            source_placeholder, target_placeholder = children[1], children[0]

        Widget.replace(source_placeholder, gtk_source, box_widget)
        Widget.replace(target_placeholder, gtk_target, box_widget)

        return box_widget.gtk_widget

    def _replace(self, target, source):
        """
        Replace the target widget with the source widget.

        @param target: the target gtk widget
        @type target: gtk.Widget
        @param source: the source gtk widget
        @type source: gtk.Widget
        """
        parent = util.get_parent(target)
        Widget.replace(target, source, parent)

        # Remove old widget
        target.hide()
        self._project.remove_widget(target)

        # Add new widget
        self._project.add_widget(source)
        self._project.selection_set(source, True)
        source.show_all()

        # XXX we also need some UIM hack similar to the
        # CommandCutPaste and CommandCreateDelete. I'd rather see that
        # it would be fixed elsewhere though...

    def execute(self):
        if self._undo:
            self._replace(self._gtk_box, self._gtk_target)

        if self._remove_cmd:
            self._remove_cmd.execute()

        if not self._undo:
            self._replace(self._gtk_target, self._gtk_box)
            
        self._undo = not self._undo

class DragAppendCommand(Command):
    def __init__(self, source_widget, box, pos, keep_source,
                 description=None, application=None):
        Command.__init__(self, description, application)

        self._undo = False
        self._keep_source = keep_source

        self._box = box.gtk_widget
        self._pos = pos

        widget_copy = copy_widget(source_widget, box.project)
        self._source_copy = widget_copy.gtk_widget

        self._project = box.project

        # Reuse the Cut command for removing the widget
        self._remove_cmd = None
        if not keep_source:
            self._remove_cmd = CommandCutPaste(source_widget,
                                               source_widget.project,
                                               None,
                                               CommandCutPaste.CUT,
                                               None,
                                               application)

    def _insert_execute(self):
        self._box.add(self._source_copy)
        self._box.reorder_child(self._source_copy, self._pos)

        self._project.add_widget(self._source_copy)
        self._project.selection_set(self._source_copy, True)
        self._source_copy.show_all()


    def _delete_execute(self):
        self._box.remove(self._source_copy)
        self._source_copy.hide()
        self._project.remove_widget(self._source_copy)
        
    def execute(self):
        if self._undo:
            self._delete_execute()

        if self._remove_cmd:
            self._remove_cmd.execute()

        if not self._undo:
            self._insert_execute()
            
        self._undo = not self._undo

