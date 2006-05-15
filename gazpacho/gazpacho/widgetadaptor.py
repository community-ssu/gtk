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

import gobject
import gtk

from gazpacho.cursor import Cursor
from gazpacho.environ import environ
from gazpacho.interfaces import BaseWidgetAdaptor
from gazpacho.widget import Widget, load_child

def create_pixbuf(generic_name, palette_name, library_name, resource_path):
    pixbuf = None

    if generic_name and palette_name:
        filename = '%s.png' % generic_name
        if resource_path is not None:
            path = environ.find_resource(library_name, filename)
        else:
            path = environ.find_pixmap(filename)

        try:
            pixbuf = gtk.gdk.pixbuf_new_from_file(path)
        except gobject.GError:
            pass

    # If we cannot find the icon and it's going to be shown in the palette
    # Print a warning and create an icon of a missing image instead
    if not pixbuf and palette_name:
        print 'Warning: Icon for %s is missing' % palette_name
        da = gtk.DrawingArea()
        pixbuf = da.render_icon(gtk.STOCK_MISSING_IMAGE,
                                gtk.ICON_SIZE_MENU)
    return pixbuf

def create_icon(pixbuf):
    image = gtk.Image()
    image.set_from_pixbuf(pixbuf)
    return image

class WidgetAdaptor(object, BaseWidgetAdaptor):
    """Base class for all widget adaptors

    A widget adaptor is the main class of a library plugin. There is a widget
    adaptor for every widget in the library that needs to do anything special
    upon certain events in Gazpacho. See this class virtual methods for
    examples of such special events.

    The rest of widgets (e.g. regular widgets) use just an instance of this
    base class, which provides the basic functionality.

    Check gazpacho/widgets/gtk+.py for examples of widget adaptors.
    """

    # GTK type for this widget. Used to create widgets with gobject.new()
    type = None
    
    # type_name is used when saving the xml
    type_name = None

    # Name displayed in the editor
    editor_name = None

    # Optional identifier of adaptor, GType name of type will be used if
    # not specified
    name = None
    
    # generic_name is used to create default widget names
    generic_name = None

    # palette_name is used in the palette
    palette_name = None

    # the tooltip is shown in the palette
    tooltip = None
    
    library = None
    icon = None
    cursor = None
    pixbuf = None

    # default widget as created in gobject.new(). It is used to decide if
    # any property has changed
    default = None

    # for packing properties we need to save the default child too
    default_child = {}
    
    def pre_create(self, context, gtk_widget, interactive=True):
        """Called just after the gtk_widget is instanciated but before any
        setup is done to it (e.g. event conection or name assignment)

        Executed before setting the properties of the widget to its initial
        value as specified in the xml file
        """

    def post_create(self, context, gtk_widget, interactive=True):
        """Called after all initialization is done in the creation process.
        
        It takes care of creating the GazpachoWidgets associated with internal
        children. It's also the place to set sane defaults, e.g. set the size
        of a window.
        """

    def fill_empty(self, context, gtk_widget):
        """After the widget is created this function is called to put one or
        more placeholders in it. Only useful for container widgets"""

    def replace_child(self, context,
                      old_gtk_widget, new_gtk_widget, parent_gtk_widget):
        """Called when the user clicks on a placeholder having a palette icon
        selected. It replaced a placeholder for the new widget.

        It's also called in the reverse direction (replacing a widget for a
        placeholder) when removing a widget or undoing a create operation.
        """

    def save(self, context, widget):
        """Prepares the widget to be saved. Basically this mean setting all
        the gazpacho widgets for internal children so the filewriter can
        iterate through them and write them correctly."""

    def load(self, context, gtk_widget, blacklist):
        """Build a GWidget from a GtkWidget.
        
        The loading is a two step process: first we get the widget tree from
        gazpacho.loader or libglade and then we generate the wrappers
        (GWidgets) from that widget tree. This function is responsable of
        the second step of this loading process
        """
        project = context.get_project()
        gwidget = Widget.load(gtk_widget, project, blacklist)
        gwidget.name = gtk_widget.name

        # create the children
        if isinstance(gtk_widget, gtk.Container):
            for child in gtk_widget.get_children():
                load_child(gwidget, child, project, blacklist)

        gwidget.load_signals()

        # if the widget is a toplevel we need to attach the accel groups
        # of the application
        if gwidget.is_toplevel():
            gwidget.setup_toplevel()

        return gwidget

    def button_release(self, context, gtk_widget, event):
        """Called when a button release event occurs in the widget.

        Note that if the widget is a windowless widget the event is actually
        produced in its first parent with a gdk window so you will probably
        want to translate the event coordinates.
        """
        return False # keep events propagating the usual way

    def motion_notify(self, context, gtk_widget, event):
        """Called when the mouse is moved on the widget.
        
        Note that if the widget is a windowless widget the event is actually
        produced in its first parent with a gdk window so you will probably
        want to translate the event coordinates.
        """
        return False # keep events propagating the usual way

    # non virtual methods:
    def setup(self, type_name, generic_name, palette_name, library,
              resource_path, tooltip):
        """Called at Gazpacho startup time to set icons, names, cursors of each
        adaptor"""
        self.editor_name = type_name
        self.generic_name = generic_name
        self.palette_name = palette_name
        self.library = library
        self.tooltip = tooltip
        self.pixbuf = create_pixbuf(generic_name, palette_name, library.name,
                                    resource_path)
        self.icon = create_icon(self.pixbuf)
        self.cursor = Cursor.create(self.pixbuf)

        if self.type:
            if not issubclass(self.type, gtk.Widget):
                raise TypeError("%s.type needs to be a GtkWidget subclass")
            type_name = gobject.type_name(self.type)
        else:
            try:
                self.type = gobject.type_from_name(type_name)
            except RuntimeError:
                raise AttributeError(
                    "There is no registered widget called %s" % type_name)
        self.type_name = type_name

    def is_toplevel(self):
        return gobject.type_is_a(self.type, gtk.Window)

    def list_signals(self):
        result = []

        gobject_gtype = gobject.GObject.__gtype__
        gtype = self.type
        while True:
            kn = gobject.type_name(gtype)
            signal_names = list(gobject.signal_list_names(gtype))
            signal_names.sort()
            result += [(name, kn) for name in signal_names]
            if gtype == gobject_gtype:
                break
            
            gtype = gobject.type_parent(gtype)

        return result

    def get_default_prop_value(self, prop, parent_type):
        """
        This is responsible for accessing the default value of a property.
        And what we consider default is the value assigned to an
        object immediatelly after it's called gobject.new.
        parent_name is used for packing properties
        @param prop a property
        @param parent_type parent gtype
        """
        if not prop.child:
            if not self.default:
                self.default = self.library.create_widget(self.type)

            default_value = prop.get_default(self.default)
        else:
            # Child properties are trickier, since we do not only
            # Need to create the object, but the parent too
            if not parent_type in self.default_child:
                parent = gobject.new(parent_type)
                child = gobject.new(self.type_name)
                self.default_child[parent_type] = parent, child
                parent.add(child)
            else:
                parent, child = self.default_child[parent_type]

            default_value = parent.child_get(child, prop.name)[0]
        
        return default_value

    def get_default(self):
        return self.default
