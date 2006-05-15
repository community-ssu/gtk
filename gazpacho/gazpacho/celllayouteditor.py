
import gobject
import gtk

from gazpacho.environ import environ
from gazpacho.loader.loader import ObjectBuilder
from gazpacho.properties import PropertyCustomEditor

# This list is used for filtering out some unwanted properties, though
# more than what is listed here could probably make sense too
interesting_properties = [ "text",
                           "value",
                           "pixbuf",
                           "editable",
# Markup is write only, can't use it without hacking :(
#                           "markup",
                           "stock-id",
                           "active",
                           "activatable",
                           "inconsistent",
                           "radio"
                         ]


#
# Base class for editing a CellLayout
#
class LayoutEditor(PropertyCustomEditor):
    wide_editor = True
    selected_view = None
    editor = None
    edited_widget = None
    layoutproxy = None
    
    project = None
    
    renderers = []
    datatypes = []

    NAME_COLUMN = 0
    TARGETS = [
        ('MY_TREE_MODEL_ROW', gtk.TARGET_SAME_WIDGET, 0),
        ('text/plain', 0, 1),
        ('TEXT', 0, 2),
        ('STRING', 0, 3),
        ]

    # These are used for mapping type names to human readable ones.
    # TODO: Make the renderers pluggable by creating a registry for them
    supported_renderers = { "Text":"GtkCellRendererText",
                            "Image":"GtkCellRendererPixbuf",
                            "Progress bar":"GtkCellRendererProgress",
                            "Check or radio button":"GtkCellRendererToggle"
                          }
    renderer_names = { "GtkCellRendererText":"Text",
                       "GtkCellRendererPixbuf":"Image",
                       "GtkCellRendererProgress":"Progress bar",
                       "GtkCellRendererToggle":"Check or radio button"
                          }

    # Called when the edited widget is changed
    def update(self, context, gtk_widget, proxy):
        self.application_window = context.get_application_window()
        self.edited_widget = gtk_widget
        self.proxy = proxy
        self.context = context
        self.project = context.get_project()
        self.layout_registry = self.project.layout_registry
        self.treemodel_registry = self.project.treemodel_registry
        self.selected_model.clear()

        self.layoutproxy = self.layout_registry.proxy_for(gtk_widget)
        if self.layoutproxy is None:
            self.layoutproxy = CellLayoutProxy(gtk_widget)
            self.layout_registry.add(self.layoutproxy)
        
        for renderer in self.layoutproxy.get_renderers():
            try:
                self.selected_model.append([
                          self.renderer_names[gobject.type_name(renderer)]])
            except KeyError:
                pass    

        name = self.get_model_name()
        self.set_model(self.treemodel_registry.lookup_model(name))
        
        self.treemodel_registry.connect("model-updated", self.update_model)
        
        self.set_ui_visibility()

# Editor

    def get_editor_widget(self):
        if self.editor is None:
            self._create_widgets()
        self.set_ui_visibility()
        return self.editor

    # Loads the editor UI
    def _create_widgets(self):
        self.ob = ObjectBuilder(
                           filename=environ.find_glade('layout-editor.glade'))
        self.ob.signal_autoconnect(self)
        win = self.ob.get_widget("renderer_editor_window")
        self.editor = self.ob.get_widget("renderer_editor")
        win.remove(self.editor)
        win.destroy()

        self.model = gtk.ListStore(str)
        for name in self.supported_renderers.keys():
            self.model.append([name])
            
        self.selected_model = gtk.ListStore(str)

        self.title_box = self.ob.get_widget("title_box")
        self.title_entry = self.ob.get_widget("title_entry")
        self.title_entry.connect("changed", self.title_changed)

        # The Renderer list
        self.view = self.ob.get_widget("renderer_treeview")
        
        self.view.set_size_request(-1, 48)
        self.view.set_model(self.model)
        self.view.connect("row-activated", self.renderer_row_activated)
                            
        column = gtk.TreeViewColumn()
        renderer = gtk.CellRendererText()
        column.pack_start(renderer)
        column.set_attributes(renderer, text=self.NAME_COLUMN)

        self.view.append_column(column)
        self.view.set_headers_visible(False)

        self.view.enable_model_drag_source(gtk.gdk.BUTTON1_MASK,
                                        self.TARGETS,
                                        gtk.gdk.ACTION_DEFAULT |
                                        gtk.gdk.ACTION_COPY)

        self.view.enable_model_drag_dest(self.TARGETS, gtk.gdk.ACTION_DEFAULT)

        self.view.connect("drag_data_get", self.drag_data_get_data)
        self.view.connect("drag_data_received", self.drag_data_received_data)

        # The selected list
        self.properties_button = self.ob.get_widget("properties_button")
        self.properties_button.set_sensitive(False)
        self.properties_button.connect("clicked", self.properties_button_clicked)
        
        self.selected_view = self.ob.get_widget("selected_treeview")
        self.selected_view.set_size_request(-1, 100)
        self.selected_view.set_model(self.selected_model)
        self.selected_view.connect("row-activated", self.selected_row_activated)
        self.selected_view.connect("key-release-event",
                                    self.selected_key_release)
        selection = self.selected_view.get_selection()
        selection.connect("changed", self.selected_selection_changed)

        column = gtk.TreeViewColumn()
        renderer = gtk.CellRendererText()
        column.pack_start(renderer)
        column.set_attributes(renderer, text=self.NAME_COLUMN)
        self.selected_view.append_column(column)

        self.selected_view.set_headers_visible(False)

        self.selected_view.enable_model_drag_source(gtk.gdk.BUTTON1_MASK,
                                                    self.TARGETS,
                                                    gtk.gdk.ACTION_DEFAULT |
                                                    gtk.gdk.ACTION_MOVE)

        self.selected_view.enable_model_drag_dest(self.TARGETS,
                                                    gtk.gdk.ACTION_DEFAULT)

        self.selected_view.connect("drag_data_get",
                                    self.drag_data_get_data)
        self.selected_view.connect("drag_data_received",
                                    self.drag_data_received_data)
                                    
        self.set_ui_visibility()
        
# Util
# Override for specific needs, eg. the treeview has the model, but we want to
# edit the treeviewcolumns instead

    def get_model_name(self):
        return self.edited_widget.get_data("gazpacho::named_treemodel")

    def set_model_name(self, name):
        self.edited_widget.set_data("gazpacho::named_treemodel", name)

    def get_model(self):
        return self.edited_widget.get_model()

    def set_model(self, model):
        self.edited_widget.set_model(model)
        self.project.changed = True

    def set_title(self, title):
        if self.layoutproxy is not None:
            self.layoutproxy.set_title(title)

    # Hide / show any widgets you don't need / need for a specific widget
    def set_ui_visibility(self):
        self.title_box.hide()

# Callbacks

    def update_model(self, treemodel_registry, name, model):
        self.set_model_name(name)
        self.set_model(model)
    
    def title_changed(self, entry):
        self.set_title(entry.get_text())

    def renderer_row_activated(self, view, path, column):
        model = view.get_model()
        self.add_renderer(model[path][self.NAME_COLUMN])

    def add_renderer(self, name):
        
        renderer = gobject.new(self.supported_renderers[name])
        self.layoutproxy.pack_start(renderer)
        
        self.selected_model.append([name])
        self.project.changed = True

    def selected_row_activated (self, view, path, column):
        self.run_attribute_dialog()
        

    def selected_key_release(self, view, event):
        if event.keyval == gtk.keysyms.Delete:
            selection = view.get_selection()
            (model, iter) = selection.get_selected()
            if iter is None:
                return
            renderers = self.layoutproxy.get_renderers()
            path = model.get_path(iter)
            self.layoutproxy.remove(renderers[path[0]])
            model.remove(iter)
            self.project.changed = True

    def selected_selection_changed(self, selection):
        (model, iter) = selection.get_selected()
        if iter is None:
            self.properties_button.set_sensitive(False)
        else:
            self.properties_button.set_sensitive(True)

        
    def drag_data_get_data(self, treeview, context, selection, target_id,
                            etime):
        treeselection = treeview.get_selection()
        (model, iter) = treeselection.get_selected()
        data = model[iter][self.NAME_COLUMN]
        selection.set(selection.target, 8, data)

    def drag_data_received_data(self, treeview, context, x, y, selection,
                                info, etime):
        if treeview is not self.selected_view:
            return
        
        data = selection.data
        drop_info = treeview.get_dest_row_at_pos(x, y)
        if drop_info:
            (model, paths) = treeview.get_selection().get_selected_rows()
            path, position = drop_info
            iter = model.get_iter(path)
            renderers = self.layoutproxy.get_renderers()

            try:
                self.layoutproxy.reorder(renderers[int(paths[0][0])], int(path[0]))

            except IndexError:
                renderer = gobject.new(self.supported_renderers[data])
                self.layoutproxy.pack_start(renderer)
                self.layoutproxy.reorder(renderer, int(path[0]))

            if (position == gtk.TREE_VIEW_DROP_BEFORE
                or position == gtk.TREE_VIEW_DROP_INTO_OR_BEFORE):
                model.insert_before(iter, [data])
            else:
                model.insert_after(iter, [data])

        else:
            model = treeview.get_model()
            renderer = gobject.new(self.supported_renderers[data])
            self.layoutproxy.pack_start(renderer)
            model.append([data])
        if context.action == gtk.gdk.ACTION_MOVE:
            context.finish(True, True, etime)

        self.project.changed = True

    def properties_button_clicked(self, button):
        self.run_attribute_dialog()

    # The dialog for binding attributes to CellRenderers, for now just for
    # binding them to model columns
    def run_attribute_dialog(self):
        selection = self.selected_view.get_selection()
        (model, iter) = selection.get_selected()
        path = model.get_path(iter)
        renderer = self.layoutproxy.get_renderers()[int(path[0])]

        dialog = gtk.Dialog("Attributes for %s element" % model[iter][0],
                            None,
                            gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
                            (gtk.STOCK_CLOSE, gtk.RESPONSE_ACCEPT))

        dialog.set_size_request(300, 400)

        sw = gtk.ScrolledWindow()
        sw.set_policy(gtk.POLICY_NEVER, gtk.POLICY_ALWAYS)

        vbox = gtk.VBox(spacing=6)

        sg = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)

        hbox = gtk.HBox()
        hbox.set_spacing(12)
        label = gtk.Label("Attribute")
        sg.add_widget(label)
        hbox.add(label)
        sep = gtk.VSeparator()
        hbox.add(sep)
        label = gtk.Label("Column number")
        hbox.add(label)
        vbox.pack_start(hbox, False, False)
        
        attributes = self.layoutproxy.get_attributes(renderer)

        for prop in gobject.list_properties(renderer):
            column_no = -1
            if prop.name in interesting_properties:
                if attributes is not None:
                    for (name, column) in attributes:
                        if prop.name == name:
                            column_no = column
                            break
                    
                hbox = gtk.HBox()
                hbox.set_spacing(12)
                button = gtk.ToggleButton(prop.name)
                input_widget = gtk.SpinButton(climb_rate=1, digits=0)
                input_widget.set_increments(1,4)
                input_widget.set_range(0,100)

                if column_no >= 0:
                    input_widget.set_value(int(column_no))
                    button.set_active(True)
                    input_widget.set_sensitive(True)
                else:
                    input_widget.set_value(0)
                    input_widget.set_sensitive(False)

                button.connect("toggled", self.attribute_toggled,
                               renderer, input_widget, prop.name)
                               
                input_widget.connect("value-changed",
                                     self.attribute_column_changed,
                                     renderer, button, prop.name)
                    
                sg.add_widget(button)
                
                foobox = gtk.VBox()
                hbox.pack_start(button)
                hbox.pack_start(input_widget)
                foobox.pack_start(hbox, False, False)
                vbox.pack_start(foobox, False, True)

        sw.add_with_viewport(vbox)
        sw.show_all()
        dialog.vbox.add(sw)
        response = dialog.run()

        dialog.destroy()

    def update_attribute(self, button, renderer, input_widget, attribute):
        if button.get_active():
            self.layoutproxy.add_attribute(renderer, attribute, int(input_widget.get_value()))
            input_widget.set_sensitive(True)
        else:
            self.layoutproxy.remove_attribute(renderer, attribute)
            input_widget.set_sensitive(False)

        self.project.changed = True
        

    def attribute_toggled(self, button, renderer, input_widget, attribute):
        self.update_attribute(button, renderer, input_widget, attribute)

    def attribute_column_changed(self, input_widget, renderer, button, attribute):
        self.update_attribute(button, renderer, input_widget, attribute)
        
# Registry for layout proxies

class LayoutRegistry:
    def __init__(self):
        self._proxies = {}
        
    def add(self, proxy):
        self._proxies[proxy.widget] = proxy
    
    def proxy_for(self, gtk_widget):
        try:
            return self._proxies[gtk_widget]
        except KeyError:
            return None
            

# Proxy object for CellLayouts
# Unifies the behaviour of layout implementations for features needed/missing
# in saving and editing
class CellLayoutProxy:

    def __init__(self, gtk_widget):

        if not isinstance(gtk_widget, gtk.CellLayout):
            raise TypeError("The widget for CellLayoutProxy should implement " \
                            "the CellLayout interface, %s does not implement " \
                            "it." % type(gtk_widget))

        self.widget = gtk_widget

        # Possible title
        if isinstance(self.widget, gtk.TreeViewColumn):
            self.title = gtk_widget.get_title()
        else:
            self.title = None

        # An ordered list of renderers
        # Loaded from data set in the loading phase if present
        loaded_renderers = gtk_widget.get_data("gazpacho::loaded_renderers")
        if loaded_renderers is None:
            self.renderers = []
        else:
            self.renderers = loaded_renderers
            # Does this have ill effects? It *should* be used only for loading.
            gtk_widget.set_data("gazpacho::loaded_renderers", None)
        
        # Dict binding renderer attributes.
        # Keys are the renderers
        # Values should be a list of (attribute name, column number)
        # Absolute values are not supported yet.
        self.attributes = {}

        for renderer in self.renderers:
            attributes = renderer.get_data("gazpacho::renderer_attributes")
            if attributes is None:
                self.attributes[renderer] = []
            else:
                self.set_attributes(renderer, attributes)

            # Does this have ill effects? It *should* be used only for loading.
            renderer.set_data("gazpacho::renderer_attributes", None)


        # Dict for the expand packing property
        # Keys are the renderers
        # Values are boolean
        self.expand = {}

# The CellLayout interface

    def clear(self):
        # Clear internal stuff first
        self.renderers = []
        self.clear_attributes()
        self.expand = {}
        
        self.widget.clear()

    # Support only pack_start for now
    def pack_start(self, renderer, expand=False):
        self.renderers.append(renderer)
        self.expand[renderer] = expand
        self.widget.pack_start(renderer, expand)

    # Reorder
    def reorder(self, renderer, position):
        self.renderers.remove(renderer)
        self.renderers[position:position] = [renderer]
        self.widget.reorder(renderer, position)
        
    # Add attribute
    def add_attribute(self, renderer, attribute, column):
        try:
            attributes = self.attributes[renderer]
        except KeyError:
            attributes = []
            self.attributes[renderer] = attributes

        for attr in attributes:
            (name, col) = attr
            if name == attribute:
                attributes.remove(attr)
                break

        attributes.append((attribute, column))
        
        self.widget.clear_attributes(renderer)
        self.set_attributes(renderer, attributes)
        
    # Clear _all_ associated attributes
    def clear_attributes(self, renderer):
        del self.attributes[renderer]
        
        self.widget.clear_attributes(renderer)

# Extra interface, whether these make sense for a particular layout
# implementation should be detected at runtime

    def remove(self, renderer):
        # Clear internal stuff
        try:
            self.renderers.remove(renderer)
            del self.expand[renderer]
            del self.attributes[renderer]
        except KeyError:
            pass        
        self.widget.clear()
        
        for r in self.renderers:
            try:
                self.widget.pack_start(r, self.expand[r])
                attributes = self.attributes[r]
                for (name, value) in attributes:
                    self.widget.add_attribute(r, name, value)
            except KeyError:
                pass

    def remove_attribute(self, renderer, attribute):
        attributes = self.attributes[renderer]
        
        for attr in attributes:
            (name, value) = attr
            if name == attribute:
                attributes.remove(attr)
                break

        self.widget.clear_attributes(renderer)
        self.set_attributes(renderer, attributes)

    def set_title(self, title):
        if isinstance(self.widget, gtk.TreeViewColumn):
            self.title = title
            self.widget.set_title(title)

    def get_title(self):
        return self.title
        

# Utility functions

    # Returns all renderers
    def get_renderers(self):
        return self.renderers

    # Sets the renderers from a list, destroying all attributes
    def set_renderers(self, renderers):
        self.clear()
        
        for renderer in renderers:
            self.pack_start(renderer)

    # Sets attributes for a renderer from a dict
    def set_attributes(self, renderer, attributes):
        for (name, value) in attributes:
            self.widget.add_attribute(renderer, name, int(value))
        
    # Returns attributes for a renderer
    # If renderer is None, returns all attributes set in the layout
    def get_attributes(self, renderer=None):
        if renderer is None:
            return self.attributes
        else:
            try:
                return self.attributes[renderer]
            except KeyError:
                return None

