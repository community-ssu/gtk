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
import os
import sys
import urllib
import urlparse

import gtk
import gobject

from gazpacho import dialogs, util
from gazpacho.actioneditor import GActionsView
from gazpacho.catalog import get_all_catalogs
from gazpacho.clipboard import Clipboard, ClipboardWindow
from gazpacho.command import CommandManager, CommandStackView
from gazpacho.config import GazpachoConfig
from gazpacho.editor import Editor
from gazpacho.environ import environ
from gazpacho.loader.loader import ParseError
from gazpacho.palette import Palette
from gazpacho.placeholder import Placeholder
from gazpacho.project import Project
from gazpacho.widget import Widget
from gazpacho.widgetview import WidgetTreeView
from gazpacho.widgetregistry import widget_registry
from gazpacho.treemodeleditor import TreeModelEditor

from gazpacho.loader.loader import ObjectBuilder

try:
   from gazpacho.gazuiserver import GazUIServer
   use_server = True
except ImportError:
   use_server = False

try:
   from gazui.gazpachoglue import gazui_publish
   have_gazui = True
except ImportError:
   have_gazui = False

_ = gettext.gettext
__all__ = ['Application', '__version__']
__program_name__ = "Gazpacho"
__version__ = "0.6.2"

def get_child_by_name(container, name):
   if not isinstance(container, gtk.Container):
       return None

   for child in container.get_children():
       if child.name == name:
           return child
       elif isinstance(child, gtk.Container):
           found = get_child_by_name(child, name)
           if found:
               return found
   return None

def get_children_by_name(container, name):
   if not isinstance(container, gtk.Container):
       return None

   lst = []
   for child in container.get_children():
       if child.name == name:
           lst.append(child)
       elif isinstance(child, gtk.Container):
           lst += get_children_by_name(child, name)
   return lst

def register_stock_icons():
    
    # Register icons
    icon_factory = gtk.IconFactory()
    icon_set = gtk.IconSet()
    
    # "icon missing"
    icon_source = gtk.IconSource()
    icon_source.set_size_wildcarded(True)
    icon_source.set_filename(environ.find_pixmap('gazpacho-icon.png'))
    
    icon_set.add_source(icon_source)
   
    icon_factory.add('gazpacho-missing-icon', icon_set)
    
    icon_factory.add_default()

class Application(object):

    # DND information
    TARGET_TYPE_URI = 100
    targets = [('text/uri-list', 0, TARGET_TYPE_URI)]
    
    def __init__(self):
        self._config = GazpachoConfig()
        self._catalogs = get_all_catalogs()

        # The WidgetAdaptor that we are about to add to a container. None if no
        # class is to be added. This also has to be in sync with the depressed
        # button in the Palette
        self._add_class = None

        # This is our current project
        self._project = None

        # debugging windows
        self._command_stack_window = None
        self._clipboard_window = None

        self._clipboard = Clipboard(self)
        
        # here we put views that should update when changing the project
        self._project_views = []

        self._window = self._application_window_create()

        self._active_view = None

        self._projects = []
        
        self._command_manager = CommandManager(self)

        self._show_structure = False
        
        if use_server:
          gtk.threads_init()
          self._server = GazUIServer()
          self._server_address = self._server._address          
        else:
          self._server = None
          self._server_address = None
         
        register_stock_icons()

        l10n_dialogs = environ.find_glade('l10n-dialogs.glade')
        if not l10n_dialogs:
            raise IOError
        self.l10n_param_window_wt = ObjectBuilder(l10n_dialogs)
        self.l10n_param_window = self.l10n_param_window_wt.get_widget('l10n_param_window')
        
        # set the labels on the notebook pages 
        notebook = self.l10n_param_window_wt.get_widget('l10n_param_notebook')
        child = notebook.get_nth_page(0)
        notebook.set_tab_label_text(child, 'Extra Strings')
        child = notebook.get_nth_page(1)
        notebook.set_tab_label_text(child, 'L10N Headers')
                
        self.l10n_entries = {}

        for widget in self.l10n_param_window_wt.get_widgets():
            name = widget.get_name()
            
            if '_entry' in name:
                self.l10n_entries[name] = widget

        self.l10n_param_window_wt.signal_autoconnect(self)

    def _add_extrastr_fields(self, str=None, eng_english=None, plural=False, strings=None):
        str_field = gtk.Entry()
        str_field.set_name('str_field')
        if str:
            str_field.set_text(str)
        
        eng_english_field = gtk.Entry()
        eng_english_field.set_name('eng_english_field')
        if eng_english:
            eng_english_field.set_text(eng_english)

        remove_button = gtk.Button('gtk-remove')
        remove_button.set_use_stock(True)
       
        str_label = gtk.Label('UI String:')
        vbox = gtk.VBox()
        vbox.set_spacing(5)
        
        if plural:
            add_button = gtk.Button('gtk-add')
            add_button.set_use_stock(True)
            eng_english_label = gtk.Label('UI String (Plural):')
            vbox.set_name('plural')
        else:
            eng_english_label = gtk.Label('Engineering English:')
            vbox.set_name('singular')

        vbox.add(gtk.HSeparator())
        hbox = gtk.HBox()
        hbox.set_spacing(5)
        hbox.add(str_label)
        hbox.add(str_field)
        hbox.add(remove_button)
        hbox.set_child_packing(remove_button, False, False, 0, gtk.PACK_START)
        vbox.add(hbox)
        
        hbox = gtk.HBox()
        hbox.set_spacing(5)
        hbox.add(eng_english_label)
        hbox.add(eng_english_field)
        if plural:
            hbox.add(add_button)
            hbox.set_child_packing(add_button, False, False, 0, gtk.PACK_START)
            add_button.connect('clicked', self._add_extrastr_plural_box, vbox)
        vbox.add(hbox)
        vbox.set_data('key_field', str_field)
        vbox.set_data('num_strings', 0)

        remove_button.connect('clicked', self._remove_extrastr_vbox, vbox)
        
        if plural and strings:
            for string in strings:
                self._add_extrastr_plural_box(None, vbox, string)

        grid_vbox = self.l10n_param_window_wt.get_widget('grid_vbox')
        grid_vbox.add(vbox)
        grid_vbox.set_child_packing(vbox, False, False, 0, gtk.PACK_START)

        return True

    def _on_extrastr_add_singular(self, button):
        self._add_extrastr_fields()
        self.l10n_param_window.show_all()
    
    def _on_extrastr_add_plural(self, button):
        self._add_extrastr_fields(plural=True)
        self.l10n_param_window.show_all()
        pass

    def _on_l10n_param_close(self, button, data=None):
        grid_vbox = self.l10n_param_window_wt.get_widget('grid_vbox')

        for vbox in grid_vbox.get_children():
            field = get_child_by_name(vbox, 'str_field')
            id = field.get_text()
            field = get_child_by_name(vbox, 'eng_english_field')
            eng_english = field.get_text()

            if id and eng_english:
                if vbox.name == 'singular':
                    self._project.extra_strings[id] = eng_english
                elif vbox.name == 'plural':
                    lst = [eng_english]
                    for field in get_children_by_name(vbox, 'string_field'):
                        lst.append(field.get_text())
                    self._project.extra_plural_strings[id] = lst

                grid_vbox.remove(vbox)

        # get the headers
        project_name_entry = self.l10n_entries['project_name_entry']
        project_version_entry = self.l10n_entries['project_version_entry']
        bugee_entry = self.l10n_entries['bugee_entry']
        last_translator_entry = self.l10n_entries['last_translator_entry']
        language_team_entry = self.l10n_entries['language_team_entry']
        mime_version_entry = self.l10n_entries['mime_version_entry']
        content_type_entry = self.l10n_entries['content_type_entry']
        content_encoding_entry = self.l10n_entries['content_encoding_entry']

        headers = {
            "Project-Id-Version":  '%s %s\\n' % (project_name_entry.get_text(),
                                                 project_version_entry.get_text()),
            "Report-Msgid-Bugs-To": '%s\\n' % bugee_entry.get_text(),
            "Last-Translator": '%s\\n' % last_translator_entry.get_text(),
            "Language-Team": '%s\\n' % language_team_entry.get_text(),
            "MIME-Version": '%s\\n' % mime_version_entry.get_text(),
            "Content-Type": '%s\\n' % content_type_entry.get_text(),
            "Content-Transfer-Encoding": '%s\\n' % content_encoding_entry.get_text()}

        self._project.set_po_headers(headers)
        self.l10n_param_window.hide()
        return True

    def _add_extrastr_plural_box(self, button, vbox, str=None):
        num_str = vbox.get_data('num_strings') + 1
        label = gtk.Label('String# %d:' % num_str)
        field = gtk.Entry()
        field.set_name('string_field')
        if str:
            field.set_text(str)
        
        remove_button = gtk.Button('gtk-remove')
        remove_button.set_use_stock(True)
       
        hbox = gtk.HBox()
        hbox.set_spacing(5)
        hbox.add(label)
        hbox.add(field)
        hbox.add(remove_button)
        hbox.set_child_packing(remove_button, False, False, 0, gtk.PACK_START)
        hbox.set_data('string_field', field)
        vbox.add(hbox)
        
        remove_button.connect('clicked', self._remove_extrastr_plural_hbox, vbox, hbox)
        self.l10n_param_window.show_all()
        
        vbox.set_data('num_strings', num_str)
        return True
    
    def _remove_extrastr_vbox(self, button, vbox):
        # remove the corresponding entry from the dictionary first
        field = vbox.get_data('key_field')
        id = field.get_text()

        if vbox.name == 'singular':
            if id in self._project.extra_strings:
                del(self._project.extra_strings[id])
        elif vbox.name == 'plural':
            if id in self._project.extra_plural_strings:
                del(self._project.extra_plural_strings[id])

        grid_vbox = self.l10n_param_window_wt.get_widget('grid_vbox')
        grid_vbox.remove(vbox)
        vbox.destroy()

        self.l10n_param_window.show_all()
        return True

    def _remove_extrastr_plural_hbox(self, button, vbox, hbox):
        # remove the corresponding entry from the dictionary first
        field = vbox.get_data('key_field')
        id = field.get_text()

        field = hbox.get_data('string_field')
        string = field.get_text()

        if id in self._project.extra_plural_strings:
            if string in self._project.extra_plural_strings[id]:
                self._project.extra_plural_strings[id].remove(string)

        vbox.remove(hbox)
        hbox.destroy()

        self.l10n_param_window.show_all()
        return True

    def _widget_tree_view_create(self):
        view = WidgetTreeView(self)
        self._project_views.insert(0, view)
        view.set_project(self._project)
        view.set_size_request(150, 160)
        view.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        return view
 
    def _command_stack_view_create(self):
        view = CommandStackView()
        self._project_views.insert(0, view)
        view.set_project(self._project)
        view.set_size_request(300, 200)
        view.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        return view

    def _gactions_view_create(self):
        view = GActionsView()
        view.set_project(self._project)
        self._project_views.insert(0, view)        
        view.set_size_request(150, 160)
        view.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        view.connect('selection-changed', self._gactions_selection_changed_cb)
        return view
    
    def _application_window_create(self):
        application_window = gtk.Window(gtk.WINDOW_TOPLEVEL)
        application_window.move(0, 0)
        application_window.set_default_size(700, -1)
        iconfilename = environ.find_pixmap('gazpacho-icon.png')
        gtk.window_set_default_icon_from_file(iconfilename)
                                                
        application_window.connect('delete-event', self._delete_event)

        # Create the different widgets
        menubar, toolbar = self._construct_menu_and_toolbar(application_window)

        self._palette = Palette(self._catalogs)
        self._palette.connect('toggled', self._palette_button_clicked)

        self.treemodel_editor = TreeModelEditor(self)
        
        self._editor = Editor(self)

        widget_view = self._widget_tree_view_create()

        self.gactions_view = self._gactions_view_create()
        
        self._statusbar = self._construct_statusbar()

        # Layout them on the window
        main_vbox = gtk.VBox()
        application_window.add(main_vbox)

        main_vbox.pack_start(menubar, False)
        main_vbox.pack_start(toolbar, False)

        hbox = gtk.HBox(spacing=6)
        hbox.pack_start(self._palette, False, False)
 
        vpaned = gtk.VPaned()
        vpaned.set_position(150)
 
        action_vbox = gtk.VBox()

        action_toolbar = self._ui_manager.get_widget('/ActionToolbar')
        action_vbox.pack_start(action_toolbar, False, False)

        action_vbox.pack_start(self.gactions_view, True, True)

        self._project_views.insert(0, self.treemodel_editor)

        notebook = gtk.Notebook()
        notebook.append_page(vpaned, gtk.Label(_('Widgets')))
        notebook.append_page(action_vbox, gtk.Label(_('Actions')))
        notebook.append_page(self.treemodel_editor, gtk.Label(_('Tree Models')))
        
        hbox.pack_start(notebook, True, True)
 
        vpaned.add1(widget_view)
        vpaned.add2(self._editor)
 
        main_vbox.pack_start(hbox)
        
        main_vbox.pack_end(self._statusbar, False)

        # dnd doesn't seem to work with Konqueror, at least not when
        # gtk.DEST_DEFAULT_ALL or gtk.DEST_DEFAULT_MOTION is used. If
        # handling the drag-motion event it will work though, but it's
        # a bit tricky.
        application_window.drag_dest_set(gtk.DEST_DEFAULT_ALL,
                                         Application.targets,
                                         gtk.gdk.ACTION_COPY)
        
        application_window.connect('drag_data_received',
                                   self._dnd_data_received_cb)
        
        self.refresh_undo_and_redo()

        return application_window

    def _expander_activated(self, expander, other_expander):
        # Make the widget tree and property editor use all available
        # space when the other one is collapsed.
        box = expander.parent

        # When both are collapsed, neither should expand to get them
        # neatly packed at the top.
        if expander.get_expanded() and not other_expander.get_expanded():
            expand = False
            other_expand = False
        else:
            expand = not expander.get_expanded()
            other_expand = not expand
        
        box.set_child_packing(expander,
                              expand,
                              True,
                              0,
                              gtk.PACK_START)
        box.set_child_packing(other_expander,
                              other_expand,
                              True,
                              0,
                              gtk.PACK_START)
            
    def _create_expander(self, label, widget, expanded=True):
        expander = gtk.Expander('<span size="large" weight="bold">%s</span>' % label)
        expander.set_use_markup(True)
        expander.set_expanded(expanded)
        expander.add(widget)
        return expander
    
    def _create_frame(self, label, widget):
        frame = gtk.Frame()
        label = gtk.Label('<span size="large" weight="bold">%s</span>' % label)
        label.set_use_markup(True)
        frame.set_label_widget(label)
        frame.set_shadow_type(gtk.SHADOW_NONE)
        frame.add(widget)
        return frame
    
    def _construct_menu_and_toolbar(self, application_window):
        actions =(
            ('FileMenu', None, _('_File')),
            ('New', gtk.STOCK_NEW, None, None,
             _('New Project'), self._new_cb),
            ('Open', gtk.STOCK_OPEN, None, None,
             _('Open Project'), self._open_cb),
            ('Save', gtk.STOCK_SAVE, None, None,
             _('Save Project'), self._save_cb),
            ('SaveAs', gtk.STOCK_SAVE_AS, _('Save _As...'), '<shift><control>S',
             _('Save project with different name'), self._save_as_cb),
            ('Publish', gtk.STOCK_SAVE_AS, _('Publish...'), None,
             _('Publish project to a project repository'), self._publish_cb),
            ('Close', gtk.STOCK_CLOSE, None, None,
             _('Close Project'), self._close_cb),
            ('Quit', gtk.STOCK_QUIT, None, None,
             _('Quit'), self._quit_cb),
            ('EditMenu', None, _('_Edit')),
            ('Cut', gtk.STOCK_CUT, None, None,
             _('Cut'), self._cut_cb),
            ('Copy', gtk.STOCK_COPY, None, None,
             _('Copy'), self._copy_cb),
            ('Paste', gtk.STOCK_PASTE, None, None,
             _('Paste'), self._paste_cb),
            ('Delete', gtk.STOCK_DELETE, None, '<control>D',
             _('Delete'), self._delete_cb),
            ('ActionMenu', None, _('_Actions')),
            ('AddAction', gtk.STOCK_ADD, _('_Add Action...'), '<control>A',
             _('Add an action'), self._add_action_cb),
            ('AddActionGroup', gtk.STOCK_ADD, _('Add Action _Group...'),
             '<control>G', _('Add an action group'),
             self._add_action_group_cb),
            ('EditAction', gtk.STOCK_EDIT, _('_Edit...'), None,
             _('Edit selected action or action group'), self._edit_action_cb),
            ('RemoveAction', gtk.STOCK_DELETE, None, None,
             _('Remove selected action or action group'),
             self._remove_action_cb),
            ('ProjectMenu', None, _('_Project')),
            ('L10NMenu', None, _('_Localisation')),
            ('GeneratePo', gtk.STOCK_CONVERT, _('Gene_rate Translation File'), None,
             _('Generates Translation meta-file'), self._generate_po_cb),
            ('ImportPo', gtk.STOCK_GO_DOWN, _('Imp_ort Translation'), None,
             _('Import Translation meta-file'), self._import_po_cb),
            ('EditL10NParameters', None, _('_Edit L10n Parameters'), None,
             _('Edit l10n Parameters'), self._edit_l10n_parameters),
            ('DebugMenu', None, _('_Debug')),
            ('ServerMenu', None, _('_Server')),
            ('ServerSettings', gtk.STOCK_PREFERENCES, None, None, 'Server settings',
             self._server_settings_cb),
            ('HelpMenu', None, _('_Help')),
            ('About', gtk.STOCK_ABOUT, None, None, _('About Gazpacho'),
             self._about_cb),
            ('DumpData', None, _('Dump data'), '<control>M',
              _('Dump debug data'), self._dump_data_cb),
            ('Reload', None, _('Reload'), None,
             _('Reload python code'), self._reload_cb)
            )

        toggle_actions = (
            ('ShowStructure', None, _('Show _structure'), '<control><shift>t',
             _('Show container structure'), self._show_structure_cb, False),
            ('ShowCommandStack', None, _('Show _command stack'), 'F3',
             _('Show the command stack'), self._show_command_stack_cb, False),
            ('ShowClipboard', None, _('Show _clipboard'), 'F4',
             _('Show the clipboard'), self._show_clipboard_cb, False),
            ('ServerActive', None, '_Active', None,
             'Active', self._server_activate_cb, True),
            )
        
        undo_action = (
            ('Undo', gtk.STOCK_UNDO, None, '<control>Z',
             _('Undo last action'), self._undo_cb),
            )

        redo_action = (
            ('Redo', gtk.STOCK_REDO, None, '<shift><control>Z',
             _('Redo last action'), self._redo_cb),
            )
        
        ui_description = """
<ui>
  <menubar name="MainMenu">
    <menu action="FileMenu">
      <menuitem action="New"/>
      <menuitem action="Open"/>
      <separator name="FM1"/>
      <menuitem action="Save"/>
      <menuitem action="SaveAs"/>
      <menuitem action="Publish"/>
      <separator name="FM2"/>
      <placeholder name="RecentProjects"/>
      <separator name="FM3"/>
      <menuitem action="Close"/>
      <menuitem action="Quit"/>
    </menu>
    <menu action="EditMenu">
      <menuitem action="Undo"/>
      <menuitem action="Redo"/>
      <separator name="EM1"/>
      <menuitem action="Cut"/>
      <menuitem action="Copy"/>
      <menuitem action="Paste"/>
      <menuitem action="Delete"/>
      <separator name="EM2"/>
      <menuitem action="ShowStructure"/>
    </menu>
    <menu action="ActionMenu">
      <menuitem action="AddAction"/>
      <menuitem action="AddActionGroup"/>
      <menuitem action="EditAction"/>
      <menuitem action="RemoveAction"/>
    </menu>
    <menu action="ProjectMenu">
    </menu>
    <menu action="L10NMenu">
      <menuitem action="GeneratePo"/>
      <menuitem action="ImportPo"/>
      <menuitem action="EditL10NParameters"/>
    </menu>
    <menu action="DebugMenu">
      <menuitem action="ShowCommandStack"/>
      <menuitem action="ShowClipboard"/>
      <separator/>
      <menuitem action="DumpData"/>
      <menuitem action="Reload"/>
    </menu>
    <menu action="ServerMenu">
      <menuitem action="ServerActive"/>
      <menuitem action="ServerSettings"/>
    </menu>
    <menu action="HelpMenu">
      <menuitem action="About"/>
    </menu>
  </menubar>
  <toolbar name="MainToolbar">
    <toolitem action="Open"/>
    <toolitem action="Save"/>
    <toolitem action="Publish"/>
    <separator name="MT1"/>
    <toolitem action="Undo"/>
    <toolitem action="Redo"/>    
    <separator name="MT2"/>
    <toolitem action="Cut"/>
    <toolitem action="Copy"/>
    <toolitem action="Paste"/>
    <toolitem action="Delete"/>
  </toolbar>
  <toolbar name="ActionToolbar">
    <toolitem action="AddAction"/>
    <toolitem action="AddActionGroup"/>
    <toolitem action="EditAction"/>
    <toolitem action="RemoveAction"/>
  </toolbar>
</ui>
"""
        self._ui_manager = gtk.UIManager()

        menu_action_group = gtk.ActionGroup('MenuActions')
        menu_action_group.add_actions(actions)
        menu_action_group.add_toggle_actions(toggle_actions)

        action_group = gtk.ActionGroup('UndoAction')
        action_group.add_actions(undo_action)
        self._ui_manager.insert_action_group(action_group, 0)
        
        action_group = gtk.ActionGroup('RedoAction')
        action_group.add_actions(redo_action)
        self._ui_manager.insert_action_group(action_group, 0)

        self.project_action_group = gtk.ActionGroup('ProjectActions')
        self._ui_manager.insert_action_group(self.project_action_group, 0)
        
        recent_template = '''<ui>
  <menubar name="MainMenu">
    <menu action="FileMenu">
      <placeholder name="RecentProjects">
      %s
      </placeholder>
    </menu>
  </menubar>
</ui>'''
        ui = ""
        for i, path in enumerate(self._config.recent_projects):
            if os.path.exists(path):
                basename = os.path.basename(path)
                if basename.endswith('.glade'):
                    basename = basename[:-6]
                    
                ui += """<menuitem action="%s"/>""" % basename
                label = '%d. %s' % (i+ 1, basename)
                action = gtk.Action(basename, label, '', '')
                action.connect('activate', self._open_project_cb, path)
                menu_action_group.add_action(action)
            else:
                self._config.recent_projects.remove(path)
        self._ui_manager.insert_action_group(menu_action_group, 0)
        
        self._ui_manager.add_ui_from_string(ui_description)
        self._ui_manager.add_ui_from_string(recent_template % ui)
        
        application_window.add_accel_group(self._ui_manager.get_accel_group())
        
        menu = self._ui_manager.get_widget('/MainMenu')
        toolbar = self._ui_manager.get_widget('/MainToolbar')
        
        # Make sure that the project menu isn't removed if empty
        project_action = self._ui_manager.get_action('/MainMenu/ProjectMenu/')
        project_action.set_property('hide-if-empty', False)

        if not have_gazui:
          publish = self._ui_manager.get_action('/MainMenu/FileMenu/Publish')
          publish.set_property('sensitive', False)

        return (menu, toolbar)

    def _construct_statusbar(self):
        statusbar = gtk.Statusbar()
        self._statusbar_menu_context_id = statusbar.get_context_id("menu")
        self._statusbar_actions_context_id = statusbar.get_context_id("actions")
        return statusbar

    # some properties
    def get_window(self):
        return self._window
    window = property(get_window)

    def get_command_manager(self):
        return self._command_manager
    
    def run(self):
        """Display the window and run the application"""
        self.show_all()
        
        gtk.main()
        
    def _push_statusbar_hint(self, msg):
        self._statusbar.push(self._statusbar_menu_context_id, msg)

    def _pop_statusbar_hint(self):
        self._statusbar.pop(self._statusbar_menu_context_id)

    def show_message(self, msg, howlong=5000):
        self._statusbar.push(self._statusbar_menu_context_id, msg)
        def remove_msg():
            self._statusbar.pop(self._statusbar_menu_context_id)
            return False
        
        gobject.timeout_add(howlong, remove_msg)

    def _refresh_undo_and_redo_cb(self, undo_stack):
        self.refresh_undo_and_redo()
    
    def refresh_undo_and_redo(self):
        undo_info = redo_info = None        
        if self._project is not None:
            undo_info = self._project.undo_stack.get_undo_info()
            redo_info = self._project.undo_stack.get_redo_info()

        undo_action = self._ui_manager.get_action('/MainToolbar/Undo')
        undo_group = undo_action.get_property('action-group')
        undo_group.set_sensitive(undo_info is not None)
        undo_widget = self._ui_manager.get_widget('/MainMenu/EditMenu/Undo')
        label = undo_widget.get_child()
        if undo_info is not None:
            label.set_text_with_mnemonic(_('_Undo: %s') % \
                                         undo_info)
        else:
            label.set_text_with_mnemonic(_('_Undo: Nothing'))
            
        redo_action = self._ui_manager.get_action('/MainToolbar/Redo')
        redo_group = redo_action.get_property('action-group')
        redo_group.set_sensitive(redo_info is not None)
        redo_widget = self._ui_manager.get_widget('/MainMenu/EditMenu/Redo')
        label = redo_widget.get_child()
        if redo_info is not None:
            label.set_text_with_mnemonic(_('_Redo: %s') % \
                                         redo_info)
        else:
            label.set_text_with_mnemonic(_('_Redo: Nothing'))

        if self._command_stack_window is not None:
            command_stack_view = self._command_stack_window.get_child()
            command_stack_view.update()

    def create(self, type_name):
        adaptor = widget_registry.get_by_name(type_name)
        self._command_manager.create(adaptor, None, self._project)
        
    def show_all(self):
        self.window.show_all()
        
    # projects
    def get_projects(self):
        return self._projects
    
    def new_project(self):
        project = Project(True, self)
        self._add_project(project)

    def _confirm_open_project(self, old_project, path):
        """Ask the user whether to reopen a project that has already
        been loaded (and thus reverting all changes) or just switch to
        the loaded version.
        """
        # If the project hasn't been modified, just switch to it.
        if not old_project.changed:
            self._set_project(old_project)
            return
        
        submsg1 = _('The project "%s" is already loaded.') % \
                  old_project.name
        submsg2 = _('Do you wish to revert your changes and reload the original glade file?') + \
                  ' ' + _('Your changes will be permanently lost if you choose to revert.')
        msg = '<span weight="bold" size="larger">%s</span>\n\n%s\n' % \
              (submsg1, submsg2)
        dialog = gtk.MessageDialog(self.window, gtk.DIALOG_MODAL,
                                   gtk.MESSAGE_WARNING, gtk.BUTTONS_NONE,
                                   msg)
        dialog.set_title('')
        dialog.label.set_use_markup(True)
        dialog.add_buttons(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                           gtk.STOCK_REVERT_TO_SAVED, gtk.RESPONSE_YES)
        dialog.set_default_response(gtk.RESPONSE_CANCEL)

        ret = dialog.run()
        if ret == gtk.RESPONSE_YES:
            # Close the old project
            old_project.selection_clear(False)
            
            for widget in old_project.widgets:
                widget.destroy()

            self._ui_manager.remove_ui(old_project.uim_id)
            self._projects.remove(old_project)

            # Open the project again
            project = Project.open(path, self)
            if project is not None:
                self._add_project(project)
        else:
            # No revert - just switch to the old project.
            self._set_project(old_project)
            
        dialog.destroy()

    def open_project(self, path):
        # Check if the project is loaded and ask the user what to do.
        for project in self._projects:
            if project.path and project.path == path:
                self._confirm_open_project(project, path)
                return project
        
        try:
            project = Project.open(path, self)
            if project is not None:
                self._add_project(project)
            return project
        except ParseError, e:
            submsg1 = _('The project could not be loaded')
            submsg2 = _('An error occurred while parsing the file "%s".') % \
                      os.path.abspath(path)
            msg = '<span weight="bold" size="larger">%s</span>\n\n%s\n' % \
                      (submsg1, submsg2)
            dialogs.error(msg)
            self._update_sensitivity()
        except IOError:
            if path in self._config.recent_projects:
                self._config.recent_projects.remove(path)
                #self._update_recent_items()
                
            submsg1 = _('The project could not be loaded')
            submsg2 = _('The file "%s" could not be opened') % \
                      os.path.abspath(path)
            msg = '<span weight="bold" size="larger">%s</span>\n\n%s\n' % \
                      (submsg1, submsg2)
            dialogs.error(msg)
            self._update_sensitivity()
            
        

    def _add_project(self, project):
        # if the project was previously added, don't reload
        for prj in self._projects:
            if prj.path and prj.path == project.path:
                self._set_project(prj)
                return

        self._projects.insert(0, project)

        # add the project in the /Project menu
        project_action= gtk.Action(project.get_id(), project.name, project.name,
                                   '')

        project_action.connect('activate', self._set_project_cb, project)
        self.project_action_group.add_action(project_action)

        project_ui = """
        <ui>
          <menubar name="MainMenu">
            <menu action="ProjectMenu">
              <menuitem action="%s"/>
            </menu>
          </menubar>
        </ui>
        """ % project.get_id()
        
        project.uim_id = self._ui_manager.add_ui_from_string(project_ui)

        # connect to the project signals so that the editor can be updated
        project.connect('selection_changed',
                         self._project_selection_changed_cb)
        project.connect('project_changed', self._project_changed_cb)
        project.undo_stack.connect('changed', self._refresh_undo_and_redo_cb)

        self._set_project(project)

        # make sure the palette and the right actions are sensitive
        self._update_sensitivity()

        self._config.add_recent_project(project.path)

    def _project_changed_cb(self, project):
        self._refresh_title()
        self._update_save_action()
        
    def _set_project_cb(self, action, project):
        self._set_project(project)
        
    def _set_project(self, project):
        if project is self._project:
            return
        
        if not project in self._projects:
            print ('Could not set project because it could not be found in the list')
            return

        self._project = project
        self._refresh_title()

        for view in self._project_views:
            view.set_project(self._project)

        self.refresh_undo_and_redo()
        self._update_save_action()

        # trigger the selection changed signal to update the editor
        self._project.selection_changed()
        
        if self._server is not None:
            self._server.set_xml(project.serialize())
        
    def _refresh_title(self):
        if self._project:
            title = '%s - Gazpacho' % self._project.name
            if self._server is not None:
              title = title + ' at %s:%i' % self._server._address
            if self._project.changed:
                title = '*' + title
        else:
            title = 'Gazpacho'
        self.set_title(title)

    def set_title(self, title):
        self.window.set_title(title)
        
    def _refresh_project_entry(self, project):
        """Update the project label in the project menu."""
        action_path = "/MainMenu/ProjectMenu/%s" % project.get_id()
        action = self._ui_manager.get_action(action_path)
        action.set_property('label', project.name)
        
    # callbacks
    def _delete_event(self, window, event):
        self._quit_cb()

        # return TRUE to stop other handlers
        return True
    
    def _new_cb(self, action):
        self.new_project()

    def _open_cb(self, action):
        filename = dialogs.open(parent=self.window, patterns=['*.glade'],
                                folder=self._config.lastdirectory)
        if filename:
            self.open_project(filename)
            self._config.set_lastdirectory(filename)
            
    def _is_writable(self, filename):
        """
        Check if it is possible to write to the file. If the file
        doesn't exist it checks if it is possible to create it.
        """
        if os.access(filename, os.W_OK):
            return True

        path = os.path.dirname(filename)
        if not os.path.exists(filename) and os.access(path, os.W_OK):
            return True

        return False

    def _save(self, project, path):
        """ Internal save """
        if self._is_writable(path):
            project.save(path)
            if self._server is not None:
                self._server.set_xml(project.serialize())
            self._refresh_project_entry(project)
            self._refresh_title()
            success = True
        else:
             submsg1 = _('Could not save project "%s"') % project.name
             submsg2 = _('Project "%s" could not be saved to %s. Permission denied.') % \
                       (project.name, os.path.abspath(path))
             text = '<span weight="bold" size="larger">%s</span>\n\n%s\n' % \
                    (submsg1, submsg2)
             result = dialogs.messagedialog(gtk.MESSAGE_ERROR,
                                            text,
                                            parent=self.window,
                                            buttons=gtk.BUTTONS_NONE,
                                            additional_buttons=(gtk.STOCK_CANCEL,
                                                                gtk.RESPONSE_CANCEL,
                                                                gtk.STOCK_SAVE_AS,
                                                                gtk.RESPONSE_YES))

             if result == gtk.RESPONSE_YES:
                 success = self._project_save_as(_('Save as'), project)
             else:
                 success = False

        return success
    
    def _save_cb(self, action):
        project = self._project

        # check that all the widgets have valid names
        if not self.validate_widget_names(project):
            return
                
        if project.path is not None:
            self._save(project, project.path)
            return

        # If instead we don't have a path yet, fire up a file chooser
        self._save_as_cb(None)

    def _save_as_cb(self, action):
        if action is None:
            # we were called from the _save_cb callback
            title = _('Save')
        else:
            title = _('Save as')
        self._project_save_as(title, self._project)

    def _project_save_as(self, title, project):
        filename = dialogs.save(title, folder=self._config.lastdirectory)
        if not filename:
            return False

        self._config.set_lastdirectory(filename)
        
        return self._save(project, filename)

    def _gactions_selection_changed_cb(self, actionview, item):
        """Update the state of the Action related actions."""
        self._update_gaction_actions()

    def _confirm_close_project(self, project):
        """Show a dialog asking the user whether or not to save the
        project before closing them.

        Return False if the user has chosen not to close the project.
        """
        return self._confirm_close_projects([project])

    def _confirm_close_projects(self, projects):
        """Show a dialog listing the projects and ask whether the user
        wants to save them before closing them.

        Return False if the user has chosen not to close the projects.
        """
        if not projects:
            return True
        
        if len(projects) == 1:
            dialog = SingleCloseConfirmationDialog(projects[0], self.window)
        else:
            dialog = MultipleCloseConfirmationDialog(projects, self.window)

        ret = dialog.run()
        if ret == gtk.RESPONSE_YES:
            # Go through the chosen projects and save them. Ask for a
            # file name if necessary.
            close = True
            projects_to_save = dialog.get_projects()
            for project in projects_to_save:
                if project.path:
                    saved = self._save(project, project.path)
                else:
                    title = _('Save as')
                    saved = self._project_save_as(title, project)
                    # If the user has pressed cancel we abort everything.
                if not saved:
                    close = False
                    break
        elif ret == gtk.RESPONSE_NO:
            # The user has chosen not to save any projects.
            close = True
        else:
            # The user has cancel the close request.
            close = False

        dialog.destroy()
        return close

    def close_current_project(self):
        if not self._project:
            return
        
        self._project.selection_clear(False)
        self._project.selection_changed()
        
        for widget in self._project.widgets:
            widget.destroy()

        self._ui_manager.remove_ui(self._project.uim_id)
        self._projects.remove(self._project)

        # If no more projects
        if not self._projects:
            for view in self._project_views:
                view.set_project(None)
            self._project = None
            self._refresh_title()
            self.refresh_undo_and_redo()
            self._update_sensitivity()
            return

        self._set_project(self._projects[0])
        
    def _open_project_cb(self, action, path):
        self.open_project(path)
        
    def _close_cb(self, action):
        if not self._project:
            return

        if self._project.changed:
            close = self._confirm_close_project(self._project)
            if not close:
                return

        self.close_current_project()


    def _quit_cb(self, action=None):
        unsaved_projects = [p for p in self._projects if p.changed]
        close = self._confirm_close_projects(unsaved_projects)
        if not close:
            return

        self._config.save()
        gtk.main_quit()

    def _undo_cb(self, action):
        self._command_manager.undo(self._project)
        self._editor.refresh()

    def _redo_cb(self, action):
        self._command_manager.redo(self._project)
        self._editor.refresh()

    def _real_cut(self):
        selection = self._project.selection
        if selection:
            gwidget = Widget.from_widget(selection[0])
            self._command_manager.cut(gwidget)
            
    def _cut_cb(self, action):
        self._real_cut()

    def _real_copy(self):
        selection = self._project.selection
        if selection:
            gwidget = Widget.from_widget(selection[0])
            self._command_manager.copy(gwidget)
            # Make sure the sensitivity of the edit actions (cut,
            # copy,paste,delete) are updated.
            self._update_edit_actions()
    
    def _copy_cb(self, action):
        self._real_copy()

    def _real_paste(self):
        item = self._clipboard.get_selected_item()
        if item is None:
            print ('There is nothing in the clipboard to paste')
            return

        if self._project is None:
            print ("No project has been specified. Cannot paste the widget")
            return
        
        selection = None
        if self._project.selection:
            selection = self._project.selection[0]

        if isinstance(selection, Placeholder) or item.is_toplevel:
            self._command_manager.paste(selection, self._project)
        else:
            print ('Please, select a placeholder to paste the widget on')
        
    def _paste_cb(self, action):
        self._real_paste()
        
    def _delete_cb(self, action):
        if self._project is not None:
            self._project.delete_selection()

    def _show_structure_cb(self, action):
        self._show_structure = not self._show_structure
        for project in self._projects:
            for widget in project.widgets:
                if isinstance(widget, gtk.Window):
                    widget.queue_draw()

    def _server_activate_cb(self, action):
        if not use_server:
          return
        if self._server is not None:
            self._server.server_close()
            self._server = None
        
        if action.get_active():
            self._server = GazUIServer(self._server_address)
        self._refresh_title()

    def _server_settings_cb(self, action):
        if not use_server:
          return

        dialog = gtk.Dialog(title="Gazpacho server address",
                            flags=gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
                            buttons=(gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT,
                                     gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))
        (address, port) = self._server_address
        sg = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
        label = gtk.Label("Address:")
        label.set_alignment(0, 0.5)
        sg.add_widget(label)
        entry = gtk.Entry()
        entry.set_text(address)
        hbox = gtk.HBox(spacing=6)
        hbox.add(label)
        hbox.add(entry)
        hbox.set_border_width(12)
        dialog.vbox.add(hbox)
        label = gtk.Label("Port:")
        label.set_alignment(0, 0.5)
        sg.add_widget(label)
        entry2 = gtk.Entry()
        entry2.set_text(str(port))
        hbox = gtk.HBox(spacing=6)
        hbox.add(label)
        hbox.add(entry2)
        hbox.set_border_width(12)
        dialog.vbox.add(hbox)
        dialog.show_all()
        result = dialog.run()
        dialog.destroy()
        if result == gtk.RESPONSE_ACCEPT:
          self._server_address = (entry.get_text(), int(entry2.get_text()))
                
    def _about_cb(self, action):
        docs = environ.get_docs_dir()
        about = gtk.AboutDialog()
        about.set_name(__program_name__)
        about.set_version(__version__)
        authorsfile = file(os.path.join(docs, 'AUTHORS'))
        authors = [a.strip() for a in authorsfile.readlines()]
        authors.append('') # separate authors from contributors
        contributorsfile = file(os.path.join(docs, 'CONTRIBUTORS'))
        authors.extend([c.strip() for c in contributorsfile.readlines()[:-2]])
        about.set_authors(authors)
        license = file(os.path.join(docs, 'COPYING')).read()
        about.set_license(license)
        about.set_website('http://gazpacho.sicem.biz')
        about.run()
        about.destroy()

    def _palette_button_clicked(self, palette):
        adaptor = palette.current

        # adaptor may be None if the selector was pressed
        self._add_class = adaptor
        if adaptor and adaptor.is_toplevel():
            self._command_manager.create(adaptor, None, self._project)
            self._palette.unselect_widget()
            self._add_class = None

    def _project_selection_changed_cb(self, project):
        if self._project != project:
            self._set_project(project)
            return

        if self._editor:
            children = self._project.selection
            if len(children) == 1 and not isinstance(children[0], Placeholder):
                self._editor.display(Widget.from_widget(children[0]))
            else:
                self._editor.clear()
                
        self._update_edit_actions()
        
    # debugging windows
    def _delete_event_for_debugging_window(self, window, event, action):
        # this will hide the window
        action.activate()
        # we don't want the window to be destroyed
        return True
    
    def _create_debugging_window(self, view, title, action):
        win = gtk.Window()
        win.set_title(title)
        win.set_transient_for(self.window)
        win.add(view)
        view.show_all()
        win.connect('delete-event', self._delete_event_for_debugging_window, action)
        return win
    
    def _show_command_stack_cb(self, action):
        if self._command_stack_window is None:
            view = self._command_stack_view_create()
            title = _('Command Stack')
            action = self._ui_manager.get_action('/MainMenu/DebugMenu/ShowCommandStack')
            self._command_stack_window = self._create_debugging_window(view,
                                                                       title,
                                                                       action)
            self._command_stack_window.show()
        else:
            if self._command_stack_window.get_property('visible'):
                self._command_stack_window.hide()
            else:
                self._command_stack_window.show_all()
                
    def _show_clipboard_cb(self, action):
        """Show/hide the clipboard window."""
        if self._clipboard_window is None:
            action = self._ui_manager.get_action('/MainMenu/DebugMenu/ShowClipboard')
            self._clipboard_window = ClipboardWindow(self.window,
                                                     self._clipboard)
            self._clipboard_window.connect('delete-event',
                                           self._delete_event_for_debugging_window,
                                           action)
            self._clipboard_window.show_window()
        else:
            if self._clipboard_window.get_property('visible'):
                self._clipboard_window.hide_window()
            else:
                self._clipboard_window.show_window()

    # useful properties
    def get_add_class(self): return self._add_class
    add_class = property(get_add_class)

    def get_palette(self): return self._palette
    palette = property(get_palette)

    def get_command_manager(self): return self._command_manager
    command_manager = property(get_command_manager)
    
    # some accesors needed by the unit tests
    def get_action(self, action_name):
        action = self._ui_manager.get_action('ui/' + action_name)
        return action

    def get_current_project(self):
        return self._project

    def get_accel_groups(self):
        return (self._ui_manager.get_accel_group(),)
    
    def get_clipboard(self):
        return self._clipboard

    # operations on actions and action group
    def _add_action_cb(self, action):
        gaction = self.gactions_view.get_selected_action()
        self.gactions_view.add_action(gaction)

    def _add_action_group_cb(self, action):
        self.gactions_view.add_action_group()

    def _remove_action_cb(self, action):
        gaction = self.gactions_view.get_selected_action()
        if gaction is not None:
            self.gactions_view.remove_action(gaction)        
        
    def _edit_action_cb(self, action):
        gaction = self.gactions_view.get_selected_action()
        if gaction is not None:
            self.gactions_view.edit_action(gaction)

    def _generate_po_cb(self, action):
        dialog = gtk.FileChooserDialog(action=gtk.FILE_CHOOSER_ACTION_SAVE,
                                       buttons=(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                                                gtk.STOCK_OK, gtk.RESPONSE_OK))

        response = dialog.run()

        if response == gtk.RESPONSE_OK:
            filename = dialog.get_filename()
            self._project.generate_po_file(filename)
        dialog.destroy()

    def _import_po_cb(self, action):
        dialog = gtk.FileChooserDialog(action=gtk.FILE_CHOOSER_ACTION_OPEN,
                                       buttons=(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                                                gtk.STOCK_OK, gtk.RESPONSE_OK))

        response = dialog.run()

        if response == gtk.RESPONSE_OK:
            filename = dialog.get_filename()
            self._project.import_po_file(filename)
        dialog.destroy()

    def _edit_l10n_parameters(self, action):
        extra_strings = self._project.extra_strings
        extra_plural_strings = self._project.extra_plural_strings
        
        for id in extra_strings:
            self._add_extrastr_fields(id, extra_strings[id])
        for id in extra_plural_strings:
            strings = extra_plural_strings[id]
            self._add_extrastr_fields(id, strings[0], True, strings[1:])
            
        # set the headers
        headers = self._project.get_po_headers()

        project_name_entry = self.l10n_entries['project_name_entry']
        project_version_entry = self.l10n_entries['project_version_entry']
        bugee_entry = self.l10n_entries['bugee_entry']
        last_translator_entry = self.l10n_entries['last_translator_entry']
        language_team_entry = self.l10n_entries['language_team_entry']
        mime_version_entry = self.l10n_entries['mime_version_entry']
        content_type_entry = self.l10n_entries['content_type_entry']
        content_encoding_entry = self.l10n_entries['content_encoding_entry']

        project_name, project_version = headers["Project-Id-Version"][:-2].split()

        project_name_entry.set_text(project_name)
        project_version_entry.set_text(project_version)
        bugee_entry.set_text(headers["Report-Msgid-Bugs-To"][:-2])
        last_translator_entry.set_text(headers["Last-Translator"][:-2])
        language_team_entry.set_text(headers["Language-Team"][:-2])
        mime_version_entry.set_text(headers["MIME-Version"][:-2])
        content_type_entry.set_text(headers["Content-Type"][:-2])
        content_encoding_entry.set_text(headers["Content-Transfer-Encoding"][:-2])

        self.l10n_param_window.show_all()

    def _update_sensitivity(self):
        """Update sensitivity of actions and UI elements (Palette) depending on
        the project list. If the list is empty we should unsensitive
        everything. Otherwise we should set sensitive to True.
        """
        sensitive = len(self._projects) > 0
        
        self._palette.set_sensitive(sensitive)
        
        action_paths = ['/MainMenu/FileMenu/Save',
                       '/MainMenu/FileMenu/SaveAs',
                       '/MainMenu/FileMenu/Close',
                       '/MainMenu/EditMenu/Copy',
                       '/MainMenu/EditMenu/Cut',
                       '/MainMenu/EditMenu/Paste',
                       '/MainMenu/EditMenu/Delete',
                       '/MainMenu/ActionMenu/AddAction',
                       '/MainMenu/ActionMenu/AddActionGroup',
                       '/MainMenu/ActionMenu/RemoveAction',
                       '/MainMenu/ActionMenu/EditAction',
                       '/MainMenu/L10NMenu']
        u_list = []
        s_list = []
        if sensitive:
            s_list = action_paths
        else:
            u_list = action_paths

        self._change_action_state(sensitive=s_list, unsensitive=u_list)
        self._update_edit_actions()
        self._update_save_action()
        self._update_gaction_actions()

    def _update_gaction_actions(self):
        """Update the state of the Action actions."""
        if not self._project:
            return # _update_sensitivity() takes care of this situation
        
        u_list = []
        s_list = ['/MainMenu/ActionMenu/AddActionGroup']

        common_actions = ['/MainMenu/ActionMenu/AddAction',
                          '/MainMenu/ActionMenu/RemoveAction',
                          '/MainMenu/ActionMenu/EditAction']

        if (self.gactions_view.get_selected_action() 
            or self.gactions_view.get_selected_action_group()):
            s_list += common_actions
        else:
            u_list += common_actions
        self._change_action_state(sensitive=s_list, unsensitive=u_list)

    def _update_save_action(self):
        """Update the state of the Save action."""
        if self._project is None:
            return # _update_sensitivity() takes care of this situation
        
        u_list = []
        s_list = []        
        if self._project.changed:
            s_list += ['/MainMenu/FileMenu/Save',]
        else:
            u_list += ['/MainMenu/FileMenu/Save',]            
        self._change_action_state(sensitive=s_list, unsensitive=u_list)

    def _update_edit_actions(self):
        """Update the state of the Cut, Copy, Paste and Delete actions."""
        if self._project is None:
            return # _update_sensitivity() takes care of this situation

        u_list = []
        s_list = []
        selection = self._project.selection
        if selection:
            # No need to Cut, Copy or Delete a placeholder
            if isinstance(selection[0], Placeholder):
                u_list += ['/MainMenu/EditMenu/Copy',
                           '/MainMenu/EditMenu/Cut']
                # Unless it's in a Box
                parent = util.get_parent(selection[0])
                # XXX Not sure if we should hardcode this here
                if parent and isinstance(parent.gtk_widget, gtk.Box):
                    s_list.append('/MainMenu/EditMenu/Delete')
                else:
                    u_list.append('/MainMenu/EditMenu/Delete')
            else:
                s_list += ['/MainMenu/EditMenu/Copy',
                           '/MainMenu/EditMenu/Cut',
                           '/MainMenu/EditMenu/Delete']
        else:
            u_list += ['/MainMenu/EditMenu/Copy',
                       '/MainMenu/EditMenu/Cut',
                       '/MainMenu/EditMenu/Delete']

        # Unless the widget is toplevel it can only be pasted on a placeholder
        item = self._clipboard.get_selected_item()
        if item and (item.is_toplevel
                     or (selection and isinstance(selection[0],
                                                  Placeholder))):
            s_list.append('/MainMenu/EditMenu/Paste')
        else:
            u_list.append('/MainMenu/EditMenu/Paste')
            
        self._change_action_state(sensitive=s_list, unsensitive=u_list)

    def _change_action_state(self, sensitive=[], unsensitive=[]):
        """Set sensitive True for all the actions whose path is on the
        sensitive list. Set sensitive False for all the actions whose path
        is on the unsensitive list.
        """
        for action_path in sensitive:
            action = self._ui_manager.get_action(action_path)
            action.set_property('sensitive', True)
        for action_path in unsensitive:
            action = self._ui_manager.get_action(action_path)
            action.set_property('sensitive', False)            

    def get_current_context(self):
        """Return the context associated with the current project or None if
        there is not such a project.
        """
        if self._project is None:
            return None
        return self._project.context

    def refresh_editor(self):
        self._editor.refresh()

    def _dnd_data_received_cb(self, widget, context, x, y, data, info, time):
        """Callback that handles drag 'n' drop of glade files."""
        if info == Application.TARGET_TYPE_URI:    
            for uri in data.data.split('\r\n'):
                uri_parts = urlparse.urlparse(uri)
                if uri_parts[0] == 'file':
                    path = urllib.url2pathname(uri_parts[2])
                    self.open_project(path)


    def _dump_data_cb(self, action):
        """This method is only useful for debugging.

        Any developer can print whatever he/she wants here
        the only rule is: clean everything before you commit it.

        This will be called upon CONTROL+M or by using the menu
        item Debug/Dump Data
        """
        project = self.get_current_project()
        print project.serialize()[:-1]

        def dump_widget(widget, lvl=0):
            print '%s %s (%s)' % (' ' * lvl,
                                  gobject.type_name(widget),
                                  widget.get_name())
            if isinstance(widget, gtk.Container):
                for child in widget.get_children():
                    dump_widget(child, lvl+1)

        for widget in project.widgets:
            if widget.get_parent():
                continue
            dump_widget(widget)
        
    def _reload_cb(self, action):
        gobject.idle_add(self._reload)

    def _reload(self):
        try:
            from twisted.python.rebuild import rebuild
        except ImportError:
            print 'You need twisted installed to be able to reload'
            return

        sys.stdout.write('** reloading... ')
        sys.stdout.flush()
        
        _ignore = ()
        modules = sys.modules.keys()
        modules.sort()
        for name in modules:
            if name in _ignore:
                continue
            
            if not name.startswith('gazpacho'):
                continue

            if not sys.modules.has_key(name):
                continue
            
            module = sys.modules[name]
            if not module:
                continue
            
            rebuild(module, doLog=0)
        sys.stdout.write('done\n')

        
    def validate_widget_names(self, project, show_error_dialogs=True):
        """Return True if all the widgets in project have valid names.

        A widget has a valid name if the name is not empty an unique.
        As soon as this function finds a widget with a non valid name it
        select it in the widget tree and returns False
        """

        widget_names = [widget.name for widget in project.widgets]
        for widget in project.widgets:
            # skip internal children (they have None as the name)
            if widget.name is None:
                continue
            
            if widget.name == '':
                if show_error_dialogs:
                    dialogs.error(_("There is a widget with an empty name"))
                project.selection_set(widget, True)
                return False

            widget_names.remove(widget.name)

            if widget.name in widget_names:
                if show_error_dialogs:
                    msg = _('The name "%s" is used in more than one widget')
                    dialogs.error(msg % widget.name)
                project.selection_set(widget, True)
                return False

        return True

    def _publish_cb(self, action):
        if not have_gazui:
            return

        name = self._project.name.replace(".glade", "")
        xmlstring = self._project.serialize()
        
        gazui_publish(name, xmlstring, toplevel=self._window)


class CloseConfirmationDialog(gtk.Dialog):
    """A base class for the close confirmation dialogs. It lets the
    user choose to Save the projects, Close without saving them and to
    abort the close operation.
    """
    
    def __init__(self, projects, toplevel=None):
        gtk.Dialog.__init__(self,
                            title='',
                            parent=toplevel,
                            flags=gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT)
        
        self.add_buttons(_("Close _without Saving"), gtk.RESPONSE_NO,
                         gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                         gtk.STOCK_SAVE, gtk.RESPONSE_YES)
        
        self.set_default_response(gtk.RESPONSE_YES)
        
        self.set_border_width(6)
        self.set_resizable(False)
        self.set_has_separator(False)
        self.vbox.set_spacing(12)

        # Add the warning image to the dialog
        warning_image = gtk.image_new_from_stock(gtk.STOCK_DIALOG_WARNING,
                                                 gtk.ICON_SIZE_DIALOG)
        warning_image.set_alignment(0.5, 0)

        self._hbox = gtk.HBox(False, 12)        
        self._hbox.set_border_width(6)
        self._hbox.pack_start(warning_image, False, False, 0)
        
        self.vbox.pack_start(self._hbox)


class SingleCloseConfirmationDialog(CloseConfirmationDialog):
    """A close confirmation dialog for a single project."""
    
    def __init__(self, project, toplevel=None):
        CloseConfirmationDialog.__init__(self, toplevel)
        self._project = project

        submsg1 = _('Save changes to project "%s" before closing?') % \
                  project.name
        submsg2 = _("Your changes will be lost if you don't save them.")
        msg = '<span weight="bold" size="larger">%s</span>\n\n%s\n' % \
              (submsg1, submsg2)
        label = gtk.Label(msg)
        label.set_use_markup(True)
        label.set_line_wrap(True)
        label.set_alignment(0.0, 0.5)        
        self._hbox.pack_start(label)

        self.vbox.show_all()

    def get_projects(self):
        """Get a list of the projects that should be saved."""
        return [self._project]

class MultipleCloseConfirmationDialog(CloseConfirmationDialog):
    """A close confirmation dialog for a multiple project. It presents
    a list of the projects and let the user choose which to save."""
    
    def __init__(self, projects, toplevel=None):
        CloseConfirmationDialog.__init__(self, toplevel)        
        self._projects = projects
        self._projects.sort(lambda x, y: cmp(x.name, y.name))

        # ListStore (boolean, Project) storing information about which
        # projects should be saved.
        self._model = None

        submsg1 = ('There are %d projects with unsaved changes. Do you want to save those changes before closing?') % \
                 len(projects)        
        submsg2 = _("Your changes will be lost if you don't save them.")
        msg = '<span weight="bold" size="larger">%s</span>\n\n%s\n' % \
              (submsg1, submsg2)
        label = gtk.Label(msg)
        label.set_use_markup(True)
        label.set_line_wrap(True)
        label.set_alignment(0.0, 0.5)   

        list_label = gtk.Label()
        list_label.set_markup_with_mnemonic(_("S_elect the projects you want to save:"))
        list_label.set_line_wrap(True)
        list_label.set_alignment(0.0, 0.5)
        list_label.set_padding(0, 6)
        
        view = self._create_project_list()
        list_label.set_mnemonic_widget(view)
        
        scrolled_window = gtk.ScrolledWindow()
        scrolled_window.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        scrolled_window.add_with_viewport(view)        
        scrolled_window.set_size_request(200, 100)
        
        project_vbox = gtk.VBox(False)
        project_vbox.pack_start(label)
        project_vbox.pack_start(list_label)
        project_vbox.pack_start(scrolled_window)
        self._hbox.pack_start(project_vbox)
        
        self.vbox.show_all()

    def _create_project_list(self):
        """Create, populate and return a TreeView containing the
        unsaved projects."""
        # Create a ListStore model
        self._model = gtk.ListStore(bool, object)
        for project in self._projects:
            self._model.append([True, project])

        # Create the TreeView
        view = gtk.TreeView(self._model)
        view.set_headers_visible(False)

        # Create the check-box column
        toggle_renderer = gtk.CellRendererToggle()
        toggle_renderer.set_property('activatable', True)
        toggle_renderer.connect("toggled", self._toggled_cb, (self._model, 0))
        toggle_column = gtk.TreeViewColumn('Save', toggle_renderer)
        toggle_column.add_attribute(toggle_renderer, 'active', 0)
        view.append_column(toggle_column)

        # Create the project column
        def render_func(treeviewcolumn, renderer, model, iter):
            project = model.get_value(iter, 1)
            renderer.set_property('text', project.name)
            return
        text_renderer = gtk.CellRendererText()        
        text_column = gtk.TreeViewColumn('Project', text_renderer)
        text_column.set_cell_data_func(text_renderer, render_func)
        view.append_column(text_column)

        return view

    def _toggled_cb(self, renderer, path, user_data):
        """Callback to change the state of the check-box."""
        model, column = user_data
        model[path][column] = not model[path][column]
        
    def get_projects(self):
        """Get a list of the projects that should be saved."""        
        return [project for saved, project in self._model if saved]

