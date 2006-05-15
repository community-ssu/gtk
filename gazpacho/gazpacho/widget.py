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

import gobject
import gtk

from gazpacho import util
from gazpacho.dndhandlers import WidgetDnDHandler
from gazpacho.annotator import Annotator
from gazpacho.loader import tags
from gazpacho.loader.custom import adapter_registry
from gazpacho.popup import Popup
from gazpacho.placeholder import Placeholder
from gazpacho.properties import prop_registry
from gazpacho.widgetregistry import widget_registry
from gazpacho.celllayouteditor import CellLayoutProxy

_ = gettext.gettext

def copy_widget(source_widget, project):
    """Create a copy of the widget. You must specify a project since
    a name conflict resolution will be performed for the copy."""
    from gazpacho.project import GazpachoObjectBuilder
    from gazpacho.filewriter import XMLWriter
    # Convert to XML
    xw = XMLWriter(project=project)
    xml = xw.serialize(source_widget)

    # Create new widget
    wt = GazpachoObjectBuilder(buffer=xml, placeholder=Placeholder)
    project.set_loader(wt)
    gtk_widget = wt.get_widget(source_widget.name)
    gwidget_copy = load_widget_from_gtk_widget(gtk_widget, project)

    return gwidget_copy

def load_widget_from_xml(xml_data, project):
    """Load a widget from an XML representation of that widget. If
    there are multiple root widgets in the XML none of them will be
    created."""
    from gazpacho.project import GazpachoObjectBuilder    

    wt = GazpachoObjectBuilder(buffer=xml_data, placeholder=Placeholder)
    project.set_loader(wt)
    gtk_widgets = wt.get_widgets()
    
    if not gtk_widgets or len(gtk_widgets) > 1:
        # We fail if none or too many widgets were found
        return None
            
    gwidget = load_widget_from_gtk_widget(gtk_widgets[0], project)
    return gwidget

def load_widget_from_gtk_widget(gtk_widget, project,
                                blacklist=None):
    """  This function creates a GWidget from a GtkWidget.
    It recursively creates any children of the original gtk_widget.
    This method is mainly used when loading a .glade file with
    gazpacho.loader

    Also makes sure the name of the widget is unique in the project
    parameter and if not, change it for a valid one. Blacklist is a list
    used to avoid repeating the names of the widgets that are being loaded
    """
    if blacklist is None:
        blacklist = []

    loader = project.get_loader()
    
    # gtk_widget may not have a name if it is an intermediate internal children
    # For example, the label of a button does not have a name
    if gtk_widget.name is None:
        # this is not an important widget but maybe its children are
        if isinstance(gtk_widget, gtk.Container):
            for child in gtk_widget.get_children():
                load_widget_from_gtk_widget(child, project, blacklist)
        return

    type_name = gobject.type_name(gtk_widget)
    adaptor = widget_registry.get_by_name(type_name)
    if adaptor is None:
        print ('Warning: could not get the class from widget: %s' % 
               gtk_widget)
        return

    # Create layoutproxies for CellLayout implementations
    # in order to save the data even without editing the layout
    if gobject.type_is_a(type(gtk_widget), gtk.CellLayout):
        if not project.layout_registry.proxy_for(gtk_widget):
            layoutproxy = CellLayoutProxy(gtk_widget)
            project.layout_registry.add(layoutproxy)

    # Treeview needs to iterate over it's children here, as they are not widgets
    if gobject.type_is_a(type(gtk_widget), gtk.TreeView):
        for column in gtk_widget.get_columns():
            layoutproxy = CellLayoutProxy(column)
            project.layout_registry.add(layoutproxy)
            
    context = project.context
    return adaptor.load(context, gtk_widget, blacklist)

def load_child(gparent, child, project, blacklist):
    """Helper function for load_widget_from_gtk_widget. Load one child
    of gparent only if it is not a placeholder.
    """
    if isinstance(child, Placeholder):
        return

    load_widget_from_gtk_widget(child, project, blacklist)
   
def resolve_name_collisions(name, adaptor, project, blacklist=[]):
    """Resolve potential name conflicts for the widget. If the name of
    the widget is already used in the project return a new valid name"""
    new_name = name
    if project.get_widget_by_name(name) is not None or name in blacklist:
        # we need to generate a new name
        new_name = project.new_widget_name(adaptor.generic_name, blacklist)

    return new_name

class Widget(gobject.GObject):
    """ 
    This is essentially a wrapper around regular GtkWidgets with more data and
    functionality needed for Gazpacho.
    
    It has access to the GtkWidget it is associated with, the Project it
    belongs to and the instance of its WidgetAdaptor.
    
    Given a GtkWidget you can ask which (Gazpacho) Widget is associated with it
    using the get_from_gtk_widget function or, if no one has been created yet you
    can create one with the create_from_gtk_widget method.
    
    A Widget have a signal dictionary where the keys are signal names and the
    values are lists of signal handler names. There are methods to
    add/remove/edit these.
    
    It store all the Properies (wrappers for GtkWidgets properties) and you can
    get any of them with get_prop. It knows how to handle normal
    properties and packing properties.
    """
    __gproperties__ = {
        'name' :    (str,
                     'Name',
                     'The name of the widget',
                     '',
                     gobject.PARAM_READWRITE),
        }
    
    __gsignals__ = {
        'add_signal_handler' :    (gobject.SIGNAL_RUN_LAST, None, (object,)),
        'remove_signal_handler' : (gobject.SIGNAL_RUN_LAST, None, (object,)),
        'change_signal_handler' : (gobject.SIGNAL_RUN_LAST, None,
                                   (object, object))
        }

    def __init__(self, adaptor, project):
        """This init method is not complete because there are several scenarios
        where widgets are created:
            
            - Normal creation in Gazpacho
            - When loading from a .glade file
            - When copying from another widget
            - Internal widgets
            
        So you should call the appropiate method after this basic initialization.
        """
        gobject.GObject.__init__(self)

        # If the widget is an internal child of another widget this is the
        # name of the internal child, otherwise is None
        # Internal children can not be deleted
        self.internal_name = None
        
        self.properties = {}
        
        # A dict of Properties. Note that these properties are related to
        # the container of the widget, thus they change after pasting the
        # widget to a different container. Toplevels widget do not have
        # packing properties.
        self.packing_properties = {}
        
        # A dictionary of signals (signal, handlers) indexed by their names
        self.signals = {}
        
        # The WidgetAdaptor this widget is an instance of
        self.adaptor = adaptor
        
        # The Project this widget belongs to
        self.project = project
        
        # The GtkWidget associated with this Widget
        self.gtk_widget = None

        self._name = None

        # Constructor, used by UIManger
        self.constructor = None

        self.disable_view = False
        
        # we need to store the events in a separate variable because we can
        # not trust the events property of a widget since Gazpacho manipulates
        # it for its own purposes (see EventsAdaptor)
        self._events = 0
        
        # DND the widget that should be draged (not necessarily this one)
        self.dnd_widget = None
        self._dnd_drop_region = None
        #self._dnd_drop_region_handler_id = None

        # flag that tells the property code if this widget should keep the
        # values of its properties when creating the GazpachoProperties or not
        self.maintain_gtk_properties = False
        
        # If we are dealing with internal children, keep a list of them
        # Not needed for internal children that show up with .get_children
        self.internal_children = []
        
    def create_gtk_widget(self, interactive=True):
        """Second part of the init process when creating a widget in
        the usual way.
        If the 'interactive' argument is false no popups will show up to
        ask for options
        """

        gtk_widget = self.adaptor.library.create_widget(self.adaptor.type)

        self.adaptor.pre_create(self.project.context, gtk_widget, interactive)
            
        self._name = self.project.new_widget_name(self.adaptor.generic_name)
        gtk_widget.set_property('name', self._name)

        self.setup_widget(gtk_widget)

        self.adaptor.post_create(self.project.context, gtk_widget, interactive)

        self.adaptor.fill_empty(self.project.context, gtk_widget)

    def guess_name(gtk_widget, adaptor, project, blacklist):
        # Guess the internal name and the parent for gtk_child.
        ancestor = gtk_widget.get_parent()
        while ancestor is not None:
            adapter = adapter_registry.get_adapter(type(ancestor))
            internal_name = adapter.get_internal_child_name(ancestor,
                                                            gtk_widget)
            if internal_name:
                gtk_widget.set_name(ancestor.name + '-' + internal_name)
                return internal_name
            
            ancestor = ancestor.get_parent()
            
        # If we don't have an internal name, generate a new unique one
        new_name = resolve_name_collisions(gtk_widget.name, adaptor, project,
                                           blacklist)
        if new_name != gtk_widget.name:
            blacklist.append(new_name)
            
        gtk_widget.set_name(new_name)

    guess_name = staticmethod(guess_name)
    
    def load(gtk_widget, project, blacklist):
        """Helper function of load_widget_from_gtk_widget. Create the Gazpacho
        widget and create a right name for it
        """
        assert project != None
        
        adaptor = widget_registry.get_by_type(gtk_widget)
        assert adaptor != None, gtk_widget
        
        widget = Widget(adaptor, project)

        # Setup the name, and the internal name if there is one
        internal_name = Widget.guess_name(gtk_widget,
                                          adaptor,
                                          project, blacklist)
        if internal_name:
            widget.internal_name = internal_name
            
        widget.setup_widget(gtk_widget)

        return widget
    load = staticmethod(load)
    
    def replace(old_widget, new_widget, parent):
        gnew_widget = Widget.from_widget(new_widget)
        gold_widget = old_widget and Widget.from_widget(old_widget)

        gtk_new_widget = gnew_widget and gnew_widget.gtk_widget or new_widget
        gtk_old_widget = gold_widget and gold_widget.gtk_widget or old_widget

        if parent is None:
            parent = util.get_parent(old_widget)

        parent.adaptor.replace_child(parent.project.context,
                                     gtk_old_widget,
                                     gtk_new_widget,
                                     parent.gtk_widget)
        if gnew_widget:
            #gnew_widget._set_packing_properties(parent)
            gnew_widget._connect_signal_handlers(gnew_widget.gtk_widget)

    replace = staticmethod(replace)
    
    def from_widget(gtk_widget):
        return gtk_widget.get_data('GladeWidgetDataTag')
    from_widget = staticmethod(from_widget)

    def load_signals(self):
        """Load the signals for the widget"""
        # set the signals

        loader = self.project.get_loader()
        if not loader:
            return
        
        signals = loader.get_signals()
        for (gobj, signal_name, signal_handler,
             signal_after, signal_object) in signals:
            if gobj != self.gtk_widget:
                continue
            
            handler = dict(name=signal_name, handler=signal_handler,
                           after=signal_after)
            self.add_signal_handler(handler)
        
    def setup_toplevel(self):
        """Add the action groups of Gazpacho to this toplevel and
        also set this toplevel transient for the main window.
        """
        gtk_widget = self.gtk_widget
        for ag in self.project._app.get_accel_groups():
            gtk_widget.add_accel_group(ag)

        # make window management easier by making created windows
        # transient for the editor window
#        app_window = self.project._app.get_window()
#        gtk_widget.set_transient_for(app_window)
        
    def setup_widget(self, gtk_widget):
        if self.gtk_widget is not None:
            self.gtk_widget.set_data('GladeWidgetDataTag', None)
            self.gtk_widget = None

        self.gtk_widget = gtk_widget
        self.gtk_widget.set_data('GladeWidgetDataTag', self)
        self.gtk_widget.add_events(gtk.gdk.BUTTON_PRESS_MASK |
                                   gtk.gdk.BUTTON_RELEASE_MASK |
                                   gtk.gdk.KEY_PRESS_MASK |
                                   gtk.gdk.POINTER_MOTION_MASK)
        if self.gtk_widget.flags() & gtk.TOPLEVEL:
            self.gtk_widget.connect('delete_event', self._hide_on_delete)
        
        self.gtk_widget.connect('popup_menu', self._popup_menu)
        self.gtk_widget.connect('key_press_event', self._key_press)

        self._connect_signal_handlers(gtk_widget)

        # Init Drag and Drop
        self.dndhandler = WidgetDnDHandler()
        self.dndhandler.connect_drag_handlers(self.gtk_widget)
        if not isinstance(self.gtk_widget, gtk.Container):
            self.dndhandler.connect_drop_handlers(self.gtk_widget)            
        
    def set_drop_region(self, region):
        """Set the drop region and make sure it is painted.

        @param region: a tuple of x, y, width and height
        @type region: tuple
        """
        self._dnd_drop_region = region
        self.gtk_widget.queue_draw()

    def clear_drop_region(self):
        """Clear the drop region and make sure it is not painted anymore."""
        if self._dnd_drop_region:
            self._dnd_drop_region = None
            self.gtk_widget.queue_draw()

    def  _draw_drop_region(self, widget, expose_window, annotator=None):
        if not annotator:
            annotator = Annotator(widget, expose_window)
        
        color = (0, 0, 1)
 
        (x, y, width, height) = self._dnd_drop_region
       
        annotator.draw_border(x, y, width, height, color)

    def setup_internal_widget(self, gtk_widget, internal_name, parent_name):
        self.internal_name = internal_name
        self._name = parent_name + '-' + internal_name

        self.setup_widget(gtk_widget)
 
    def do_get_property(self, prop):
        try:
            return getattr(self, 'get_' + prop.name)(self)
        except:
            raise AttributeError(_('Unknown property %s') % prop.name)

    def do_set_property(self, prop, value):
        try:
            getattr(self, 'set_' + prop.name)(self, value)
        except:
            raise AttributeError(_('Unknown property %s') % prop.name)

    # the properties
    def get_name(self): return self._name
    def set_name(self, value):
        self._name = value
        self.notify('name')
    name = property(get_name, set_name)

    # utility getter methods
    def get_parent(self):
        if self.gtk_widget.flags() & gtk.TOPLEVEL:
            return None

        return util.get_parent(self.gtk_widget)

    def add_signal_handler(self, signal_handler):
        self.emit('add_signal_handler', signal_handler)
        
    def do_add_signal_handler(self, signal_handler):
        signals = self.list_signal_handlers(signal_handler['name'])
        if not signals:
            self.signals[signal_handler['name']] = []

        self.signals[signal_handler['name']].append(signal_handler)

    def remove_signal_handler(self, signal_handler):
        self.emit('remove_signal_handler', signal_handler)
        
    def do_remove_signal_handler(self, signal_handler):
        signals = self.list_signal_handlers(signal_handler['name'])
        # trying to remove an inexistent signal?
        assert signals != []

        self.signals[signal_handler['name']].remove(signal_handler)
        
    def change_signal_handler(self, old_signal_handler, new_signal_handler):
        self.emit('change_signal_handler',
                   old_signal_handler, new_signal_handler)
        
    def do_change_signal_handler(self, old_signal_handler,
                                  new_signal_handler):
        if old_signal_handler['name'] != new_signal_handler['name']:
            return

        signals = self.list_signal_handlers(old_signal_handler['name'])
        # trying to remove an inexistent signal?
        assert signals != []

        index = signals.index(old_signal_handler)
        signals[index]['handler'] = new_signal_handler['handler']
        signals[index]['after'] = new_signal_handler['after']

    def _expose_event_after(self, widget, event):
        util.draw_annotations(widget, event.window)
        
        if self._dnd_drop_region:
            self._draw_drop_region(widget, event.window)

    def _find_deepest_child_at_position(self, toplevel, container, x, y):
        found = None
        for widget in container.get_children():
            coords = toplevel.translate_coordinates(widget, x, y)
            if len(coords) != 2:
                continue
            child_x, child_y = coords

            # sometimes the found widget is not mapped or visible
            # think about a widget in a notebook page which is not selected
            if (0 <= child_x < widget.allocation.width and
                0 <= child_y < widget.allocation.height and
                widget.flags() & (gtk.MAPPED | gtk.VISIBLE) and
                Widget.from_widget(widget)):
                found = widget
                break
            
        if found and isinstance(found, gtk.Container):
            return self._find_deepest_child_at_position(toplevel, found, x, y)
        elif found:
            return Widget.from_widget(found)
        else:
            return Widget.from_widget(container)
    
    def _retrieve_from_event(self, base, event):
        """ Returns the real GtkWidget in x, y of base.
        This is needed because for GtkWidgets than does not have a
        GdkWindow the click event goes right to their parent.
        """

        x = int(event.x)
        y = int(event.y)
        window = base.get_toplevel()
        if window.flags() & gtk.TOPLEVEL:
            top_x, top_y = base.translate_coordinates(window, x, y)
            return self._find_deepest_child_at_position(window, window,
                                                        top_x, top_y)

    def _event(self, widget, event):
        """We only delegate this call to the appropiate event handler"""
        if event.type == gtk.gdk.BUTTON_PRESS:
            return self._button_press_event(widget, event)
        elif event.type == gtk.gdk.BUTTON_RELEASE:
            return self._button_release_event(widget, event)
        elif event.type == gtk.gdk.MOTION_NOTIFY:
            return self._motion_notify_event(widget, event)

        return False

    def _event_after(self, widget, event):
        """We only delegate this call to the appropiate event handler"""
        if event.type == gtk.gdk.EXPOSE:
            self._expose_event_after(widget, event)
            
    def _motion_notify_event(self, widget, event):
        gwidget = self._retrieve_from_event(widget, event)
        if not gwidget:
            return
        return gwidget.adaptor.motion_notify(gwidget.project.context,
                                             gwidget.gtk_widget, event)
        
    def _button_release_event(self, widget, event):
        gwidget = self._retrieve_from_event(widget, event)
        if not gwidget:
            return
        return gwidget.adaptor.button_release(gwidget.project.context,
                                              gwidget.gtk_widget, event)
        
    def _button_press_event(self, widget, event):
        gwidget = self._retrieve_from_event(widget, event)
        if not gwidget:
            return

        # DND store the widget, we might need it later
        self.dnd_widget = gwidget
        
        widget = gwidget.gtk_widget
        # Make sure to grab focus, since we may stop default handlers
        if widget.flags() & gtk.CAN_FOCUS:
            widget.grab_focus()

        if event.button == 1:
            if event.type is not gtk.gdk.BUTTON_PRESS:
                #only single clicks allowed, thanks
                return False

            # Shift clicking circles through the widget tree by
            # choosing the parent of the currently selected widget.
            if event.state & gtk.gdk.SHIFT_MASK:
                util.circle_select(widget)
                return True

            # if it's already selected don't stop default handlers,
            # e.g toggle button
            selected = util.has_nodes(widget)
            self.project.selection_set(widget, True)
            return not selected
        elif event.button == 3:
            # first select the widget
            self.project.selection_set(widget, True)

            # then popup the menu
            mgr = self.project._app.get_command_manager()
            popup = Popup(mgr, gwidget)
            popup.pop(event)
            return True

        return False
    
    def _popup_menu(self, widget):
        return True # XXX TODO

    def _key_press(self, widget, event):
        gwidget = Widget.from_widget(widget)

        if event.keyval in (gtk.keysyms.Delete, gtk.keysyms.KP_Delete):
            # We will delete all the selected items
            gwidget.project.delete_selection()
            return True
        
        return False

    def _hide_on_delete(self, widget, event):
        return widget.hide_on_delete()
    
    def _connect_signal_handlers(self, gtk_widget):
        # don't connect handlers for placeholders
        if isinstance(gtk_widget, Placeholder):
            return

        gtk_widget.set_redraw_on_allocate(True)

        # check if we've already connected an event handler
        if not gtk_widget.get_data(tags.EVENT_HANDLER_CONNECTED):
            # we are connecting to the event signal instead of the more
            # apropiated expose-event and button-press-event because some
            # widgets (ComboBox) does not allows us to connect to those
            # signals. See http://bugzilla.gnome.org/show_bug.cgi?id=171125
            # Hopefully we will be able to fix this hack if GTK+ is changed
            # in this case
            gtk_widget.connect('event', self._event)
            gtk_widget.connect('event-after', self._event_after)
            
#            gtk_widget.connect("expose-event", self._expose_event)
#            gtk_widget.connect("button-press-event", self._button_press_event)
            gtk_widget.set_data(tags.EVENT_HANDLER_CONNECTED, 1)

        ## 
        ## # we also need to get expose events for any children
        ## if isinstance(gtk_widget, gtk.Container):
        ##     gtk_widget.forall(self._connect_signal_handlers)
    
    def is_toplevel(self):
        return self.adaptor.is_toplevel()

    def list_signal_handlers(self, signal_name):
        return self.signals.get(signal_name, [])

    def _create_prop(self, prop_name):
        # Get the property type
        type_name = gobject.type_name(self.gtk_widget)
        parent = self.gtk_widget.get_parent()
        prop_type = prop_registry.get_prop_type(type_name, prop_name,
                                                parent=parent)
        # Instantiate the property from the property type
        return prop_type(self)

    def get_prop(self, prop_name):
        properties = self.properties
        if prop_name in properties:
            return properties[prop_name]

        prop = self._create_prop(prop_name)
        properties[prop_name] = prop
        return prop

gobject.type_register(Widget)

