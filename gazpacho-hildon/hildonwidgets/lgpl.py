# Gazpacho Hildon widget bindings
#
# Copyright (C) 2005, 2006 Nokia Corporation.
# Contact: Erik Karlsson <erik.b.karlsson@nokia.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation
# version 2.1 of the License.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA


import gettext

import xml.dom.minidom
from xml.sax.saxutils import escape, unescape

import gtk
import gobject

import hildon

from gazpacho.widgetadaptor import WidgetAdaptor
from gazpacho.widget import Widget, load_child, load_widget_from_gtk_widget
from gazpacho.context import Context
from gazpacho.placeholder import Placeholder
from gazpacho.uieditor import UIEditor, MenuBarUIEditor, AddMenuDialog, AddMenuitemDialog
from gazpacho.properties import prop_registry, TransparentProperty, StringType, IntType, CustomProperty, PropertyCustomEditor, ObjectType, PropertyError
from gazpacho.editor import PropertyCustomEditorWithDialog, EditorPropertyText, PropertyEditorDialog
from gazpacho.util import unselect_when_press_escape, xml_filter_nodes
from gazpacho.widgetregistry import widget_registry
from gazpacho.loader.custom import adapter_registry

_ = gettext.gettext


class AppAdaptor(WidgetAdaptor):
    def post_create (self, context, app, interactive=True):
        # Resize the window to target size
        app.set_size_request(672,396)

    def fill_empty(self, context, widget):
        widget.add(context.create_placeholder())

    def replace_child(self, context, current, new, container):
        if not isinstance(current, hildon.AppView):
    		    container.remove(current)
        container.add(new)

class AppViewAdaptor(WidgetAdaptor):
    def post_create (self, context, appview, interactive=True):
        gwidget = Widget.from_widget(appview)
        ui_string = "<popup name='HildonApp'></popup>"
        self._setup_internal_children(gwidget)

        project = gwidget.project
        project.uim.create_default_actions()

        project.uim.add_ui(gwidget, ui_string)

    def fill_empty(self, context, appview):
        appview.add(context.create_placeholder())
        appview.vbox.add(context.create_placeholder())

    def replace_child(self, context, current, new, container):
        if current in container.get_children():
            container.remove(current)
            container.add(new)
        if current in container.vbox.get_children():
            position = container.vbox.get_children().index(current)
            container.vbox.remove(current)
            container.vbox.add(new)
            container.vbox.reorder_child(new, position)

    def load(self, context, gtk_widget, blacklist):
        project = context.get_project()

        old_name = gtk_widget.name
        gwidget = Widget.load(gtk_widget, project, blacklist)
        gwidget._name = gwidget.gtk_widget.name

        # change the gtk_widget for the one we get from the uimanager
        project.uim.load_widget(gwidget, old_name)

        # create the children
        if isinstance(gtk_widget, gtk.Container):
            for child in gtk_widget.get_children():
                load_child(gwidget, child, project, blacklist)

        # load the vbox
        self._setup_internal_children(gwidget)

        # create children for the vbox
        for child in gtk_widget.vbox.get_children():
            load_child(gwidget, child, project, blacklist)

        gwidget.load_signals()

        return gwidget

    def _setup_internal_children(self, gwidget):
        child_class = widget_registry.get_by_name('GtkVBox')
        vbox_widget = Widget(child_class, gwidget.project)
        vbox_widget.setup_internal_widget(gwidget.gtk_widget.vbox, 'vbox',
                                          gwidget.name or '')

class ToolBarsProp(CustomProperty, IntType):
    minimum = 0
    maximum = 10
    step_increment = 1
    page_increment = 1
    climb_rate = 1
    label = "Number of Toolbars"
    default = 1
    def get(self):
        return len(self._object.vbox.get_children())

    def set(self, value):
        old_size = len(self._object.vbox.get_children())
        new_size = value
        if new_size == old_size:
            return

        if new_size > old_size:
            while new_size > old_size:
                placeholder = Placeholder(self._project.get_app())
                self._object.vbox.add(placeholder)
                self._object.vbox.reorder_child(placeholder, 0)
                old_size += 1
        else:
            children = self._object.vbox.get_children()
            while old_size > new_size:
                self._object.vbox.remove(children[0])
                self._project.remove_widget(children[0])
                if old_size > 1:
                    children = children[1:]
                old_size -= 1

    def save (self):
        return None

    def load (self):
        self._value = len(self._object.vbox.get_children())

prop_registry.override_property('HildonAppView::ToolBars', ToolBarsProp)


class AppViewMenuEditor(UIEditor):
    def _create_widgets(self):
        super(AppViewMenuEditor, self)._create_widgets()

        self.left = gtk.Button(stock=gtk.STOCK_GO_BACK)
        self.left.connect('clicked', self._on_left_clicked)
        self.right = gtk.Button(stock=gtk.STOCK_GO_FORWARD)
        self.right.connect('clicked', self._on_right_clicked)
        self.v_button_box.pack_start(self.left)
        self.v_button_box.pack_start(self.right)

    def set_widget(self, widget, proxy):
        super(AppViewMenuEditor, self).set_widget(widget, proxy)
        self.add_managed_type (hildon.AppView)

    def set_ui(self):
        ui_string = self.root_node.toxml()
        self.widget.set_property('menu-ui', ui_string)
        self.gwidget.project.changed = True

    def load_ui(self):
        ui = self.widget.get_property("menu-ui")

        self.model.clear()
        if ui:
            xml_string = ui
            self.xml_doc = xml.dom.minidom.parseString(xml_string)
            self.xml_doc.normalize()
            self.root_node = self.xml_doc.documentElement
            typename = self.root_node.tagName
        else:
            # create empty XML document
            self.create_empty_xml()
            typename = "popup"

        for xml_node in xml_filter_nodes(self.root_node.childNodes,
                                         self.root_node.ELEMENT_NODE):
            self._load_node(xml_node, None)

        self.set_buttons_sensitiviness(typename)

    def set_buttons_sensitiviness(self, typename):
        buttons = (self.add, self.remove, self.up, self.down, self.left,
                   self.right)
        sensitivity = {
            'popup': (True, False, False, False, False, False),
            'menu': (True, True, True, True, False, True),
            'menuitem': (True, True, True, True, True, False),
            'separator': (False, True, True, True, False, False),
            }
        for i, s in enumerate(sensitivity[typename]):
            buttons[i].set_sensitive(s)

    def create_add_dialog(self):
        selected_iter = self._get_selected_iter()
        dialog = AddMenuitemDialog(self.uim.action_groups, self)
        dialog.typename = 'menuitem'
        if selected_iter is None:
            # we are adding a toplevel
            dialog.parent_node = self.root_node
        else:
            # we are adding a child menuitem for a menu
            dialog.parent_node = self.model.get_value(selected_iter, 0)
        return dialog

    def create_empty_xml(self):
        self.xml_doc = xml.dom.getDOMImplementation().createDocument(None,
                                                                     None,
                                                                     None)

        element = self.xml_doc.createElement('popup')
        element.setAttribute('name', 'HildonApp')

        self.root_node = element

        self.widget.set_property("menu-ui", self.root_node.toxml())

    def create_xml_node(self, parent, typename, name=None):
        element = self.xml_doc.createElement(typename)
        print "<" + parent.tagName + ">"
        if parent.tagName == 'menuitem':
            print "Changing parent"
            node = self.xml_doc.createElement('menu')
            node.setAttribute('name', parent.getAttribute('name'))
            node.setAttribute('action', parent.getAttribute('action'))
            grandparent = parent.parentNode
            if grandparent is not None:
                grandparent.removeChild(parent)
                grandparent.appendChild(node)
                parent = node
        else:
            print "not changing"

        if typename != 'separator':
            element.setAttribute('name', name)
            element.setAttribute('action', name)

        print "Appending", element.tagName, "to", parent.tagName
        return parent.appendChild(element)


gobject.type_register(AppViewMenuEditor)

class AppViewMenuPropEditor(PropertyCustomEditorWithDialog):
    button_text = _('Edit UI Definition...')
    dialog_class = AppViewMenuEditor

class AppViewMenuAdaptor(CustomProperty, StringType):
    editor = AppViewMenuPropEditor
    translatable = False
    context = False
    cdata = True

prop_registry.override_property('HildonAppView::menu-ui', AppViewMenuAdaptor)

prop_registry.override_simple('HildonAppView::fullscreen', disabled=True)

class  FileSelectEditor(PropertyCustomEditor, PropertyEditorDialog, EditorPropertyText):
    button_text = _('Select...')
    _widget = None
    image = None
    button = None
    hbox = None
    image_name = '/usr/share/gazpacho/resources/hildonwidgets/caption.png'
    dialog = None
    gwidget = None
    proxy = None
    application_window = None

    def set_widget(self, widget, proxy):
        self._widget = widget
        self.proxy = proxy

    def get_editor_widget(self):
        if self.button is None:
           self.button = gtk.Button(self.button_text)
           self.button.connect('clicked', self._clicked)
        if self.image is None:
           self.image = gtk.Image()
           self.image.set_from_file(self.image_name)
           self.image.set_pixel_size (24)
        if self.hbox is None:
           self.hbox = gtk.HBox(False, 6)
           self.hbox.add(self.image)
           self.hbox.add(self.button)

        return self.hbox

    def _clicked (self, button):
         dialog = gtk.FileChooserDialog(_('Open image ...'),
                                        self.application_window,
                                        gtk.FILE_CHOOSER_ACTION_OPEN,
                                        (gtk.STOCK_CANCEL,
                                         gtk.RESPONSE_CANCEL,
                                         gtk.STOCK_OPEN,
                                         gtk.RESPONSE_OK))
         file_filter = gtk.FileFilter()
         file_filter.set_name("Images")
         file_filter.add_mime_type("image/png")
         file_filter.add_mime_type("image/jpeg")
         file_filter.add_mime_type("image/gif")
         file_filter.add_mime_type("image/x-xpixmap")
         file_filter.add_pattern("*.png")
         file_filter.add_pattern("*.jpg")
         file_filter.add_pattern("*.gif")
         file_filter.add_pattern("*.xpm")

         dialog.add_filter(file_filter)

         dialog.connect('response', self.handle_response)
         response = dialog.run()

    def handle_response (self, dialog, response):
        dialog.hide()
        filename = dialog.get_filename()
        if response == gtk.RESPONSE_OK and filename:
            self.image_name = filename
            self.image.set_from_file(filename)
            image = self._widget.get_icon_image()
            if image is None:
                image = gtk.Image()
                self._widget.set_icon_image(image)

            image.set_from_file(self.image_name)
            self._widget.set_data("imagefilename", self.image_name)

            gwidget = Widget.from_widget(self._widget)
            gwidget.project.changed = True

        dialog.destroy()

    def update(self, context, gtk_widget, proxy):
        self._widget = gtk_widget
        self.image.set_from_file(gtk_widget.get_data("imagefilename"))


gobject.type_register(FileSelectEditor)


class CaptionImageEditor(FileSelectEditor):
    button_text = _('Select image...')
    dialog_class = FileSelectEditor

class CaptionImageAdaptor(TransparentProperty, StringType):
    editor = CaptionImageEditor

    def set(self, value):
        pass

    def get(self):
        pass

    def save(self):
        image = self._object.get_icon_image()
        if image is None:
          return None

        name = self._object.get_data("imagefilename")
        return name

prop_registry.override_property('HildonCaption::icon-setter', CaptionImageAdaptor)

class CaptionAdaptor(WidgetAdaptor):
    internal_child = None

    def post_create (self, context, caption, interactive=True):
        caption.set_label("Caption")
        caption.set_separator(":")


    def save(self, context, gwidget):
        project = context.get_project()

        self._setup_internal_children(gwidget)

        return gwidget

    def load(self, context, gtk_widget, blacklist):
        project = context.get_project()

        old_name = gtk_widget.name
        gwidget = Widget.load(gtk_widget, project, blacklist)
        gwidget._name = gwidget.gtk_widget.name

        # create the children
        if isinstance(gtk_widget, gtk.Container):
            for child in gtk_widget.get_children():
                load_child(gwidget, child, project, blacklist)

        self._setup_internal_children(gwidget)
        if self.internal_child is not None:
            try:
                prop = gwidget.get_prop('image-setter')
                self.internal_child.set_property('file', prop.value)
            except PropertyError:
                print "TODO: Find out why custom properties are not loaded"
                pass

        gwidget.load_signals()

        return gwidget

    def fill_empty(self, context, caption):
        pass

    def replace_child(self, context, current, new, container):
        container.remove(current)
        container.add(new)

    def _find_internal_child(self, child):
        if self.internal_child is None and isinstance (child, gtk.HBox):
            for child in child.get_children():
                if isinstance(child, gtk.Image):
                    self.internal_child = child
                    break

    def _setup_internal_children(self, gwidget):
        if self.internal_child is not None:
            return
        child_class = widget_registry.get_by_name('GtkImage')
        gwidget.gtk_widget.forall(self._find_internal_child)
        if self.internal_child is not None:
            image_widget = Widget(child_class, gwidget.project)
            image_widget.setup_internal_widget(self.internal_child, 'image',
                                              gwidget.name or '')

# Allow history items to be previewed
class Items(CustomProperty, StringType):
    label = 'Items'
    default = ''
    def __init__(self, widget):
        # XXX: Adapter should be accessible in all properties
        self._adapter = adapter_registry.get_adapter(widget.gtk_widget, None)
        super(Items, self).__init__(widget)

    def load(self):
        model = self._object.get_property('list')
        if not model:
            model = gtk.ListStore(str)
            self._object.set_property('list', model)
        self._value = '\n'.join([row[0] for row in model])
        self._initial = self.default

    def set_items(self, gtk_widget, value):
        model = gtk_widget.get_property('list')
        if not model:
            model = gtk.ListStore(str)
            gtk_widget.set_property('list', model)
        model.clear()
        for val in value.split('\n'):
        		iter = model.append()
        		model.set(iter, 0, val)

        self._value = '\n'.join([row[0] for row in model])

#    def save(self):
#        pass

    def get(self):
        return self._value

    def set(self, value):
        self._value = value
        self.set_items(self._object, value)

prop_registry.override_property('HildonFindToolbar::items', Items)

