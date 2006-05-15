# Copyright (C) 2005 by SICEm S.L. and Async Open Source
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

from gazpacho.editor import PropertyCustomEditorWithDialog
from gazpacho.properties import prop_registry, TransparentProperty, StringType
from gazpacho.uieditor import MenuBarUIEditor, ToolbarUIEditor
from gazpacho.widget import Widget
from gazpacho.widgets.base.base import ContainerAdaptor

_ = gettext.gettext

class UIPropEditor(PropertyCustomEditorWithDialog):
    button_text = _('Edit UI Definition...')

class MenuBarUIPropEditor(UIPropEditor):
    dialog_class = MenuBarUIEditor

class ToolbarUIPropEditor(UIPropEditor):
    dialog_class = ToolbarUIEditor


class UIAdaptor(TransparentProperty, StringType):
    """This represents a fake property to edit the ui definitions of
    menubars and toolbars.

    It does not store anything because this information is stored in
    the uimanager itself. It's just a way to put a button in the Gazpacho
    interface to call the UIEditor.
    """
    editor = UIPropEditor
    
class MenuBarUIAdapter(UIAdaptor):
    editor = MenuBarUIPropEditor

prop_registry.override_property('GtkMenuBar::ui', MenuBarUIAdapter)

class ToolbarUIAdapter(UIAdaptor):
    editor = ToolbarUIPropEditor

prop_registry.override_property('GtkToolbar::ui', ToolbarUIAdapter)

class CommonBarsAdaptor(ContainerAdaptor):

    def post_create(self, context, gtk_widget, ui_string):
        # create some default actions
        gwidget = Widget.from_widget(gtk_widget)
        project = gwidget.project
        project.uim.create_default_actions()

        project.uim.add_ui(gwidget, ui_string)
        new_gtk_widget = project.uim.get_gtk_widget(gwidget)

        # we need to replace gtk_widget with new_widget
        gwidget.setup_widget(new_gtk_widget)
        #gwidget.apply_properties()
    
    def save(self, context, gwidget):
        """This saver is needed to avoid saving the children of toolbars
        and menubars
        """
        gwidget.constructor = 'initial-state'

    def load(self, context, gtk_widget, blacklist):
        """This loader is special because of these features:
        - It does not load the children of the menubar/toolbar
        - Load the uimanager and put its content (action groups) into the
        project
        """
        
#         # we need to save the properties of this gtk_widget because otherwise
#         # when we got it from the uimanager it's gonna be another widget with
#         # different properties
#         props = {}
#         for prop in gobject.list_properties(gtk_widget):
#             if 1 or prop.flags != gobject.PARAM_READWRITE:
#                 continue
#             if propertyclass.get_type_from_spec(prop) is gobject.TYPE_OBJECT:
#                 continue
#             # FIXME: This need to use the values from the catalog.
#             # But it doesn't work right now, the property in
#             # klass.properties is always set to False.
#             if prop.name == 'parent' or prop.name == 'child':
#                 continue
#             props[prop.name] = gtk_widget.get_property(prop.name)

        project = context.get_project()

        old_name = gtk_widget.name
        gwidget = Widget.load(gtk_widget, project, blacklist)
        gwidget._name = gwidget.gtk_widget.name

        # change the gtk_widget for the one we get from the uimanager
        project.uim.load_widget(gwidget, old_name)

        #gwidget.load_properties()
        gwidget.load_signals()

        return gwidget
    
class MenuBarAdaptor(CommonBarsAdaptor):

    def post_create(self, context, menubar, interactive=True):
        gwidget = Widget.from_widget(menubar)
        # A None in this list means a separator
        names = [gwidget.name, _('FileMenu'), _('New'), _('Open'), _('Save'),
                 _('SaveAs'), _('Quit'), _('EditMenu'), _('Copy'), _('Cut'),
                 _('Paste')]
        tmp = []
        for name in names:
            tmp.extend([name, name])
        
        ui_string = """<menubar action="%s" name="%s">
  <menu action="%s" name="%s">
    <menuitem action="%s" name="%s"/>
    <menuitem action="%s" name="%s"/>
    <menuitem action="%s" name="%s"/>
    <menuitem action="%s" name="%s"/>
    <separator/>
    <menuitem action="%s" name="%s"/>
  </menu>
  <menu action="%s" name="%s">
    <menuitem action="%s" name="%s"/>
    <menuitem action="%s" name="%s"/>
    <menuitem action="%s" name="%s"/>
  </menu>
</menubar>""" % tuple(tmp)
    
        super(MenuBarAdaptor, self).post_create(context, menubar, ui_string)

class ToolbarAdaptor(CommonBarsAdaptor):
    def post_create(self, context, toolbar, interactive=True):
        gwidget = Widget.from_widget(toolbar)

        names = [gwidget.name, _('New'), _('Open'), _('Save'),
                 _('Copy'), _('Cut'), _('Paste')]
        tmp = []
        for name in names:
            tmp.extend([name, name])
    
        ui_string = """<toolbar action="%s" name="%s">
  <toolitem action="%s" name="%s"/>
  <toolitem action="%s" name="%s"/>
  <toolitem action="%s" name="%s"/>
  <separator/>
  <toolitem action="%s" name="%s"/>
  <toolitem action="%s" name="%s"/>
  <toolitem action="%s" name="%s"/>
</toolbar>""" % tuple(tmp)

        super(ToolbarAdaptor, self).post_create(context, toolbar, ui_string)

