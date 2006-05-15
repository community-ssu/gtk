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

import gtk

import hildon

from gazpacho.widget import Widget
from gazpacho.widgetadaptor import WidgetAdaptor
from gazpacho.properties import prop_registry, TransparentProperty, StringType, CustomProperty
from gazpacho.loader.custom import adapter_registry

_ = gettext.gettext


# Dialogs

class ModalDialogAdaptor(WidgetAdaptor):
    def post_create(self, context, dialog, interactive=True):
        self._make_usable(dialog)

    def _make_usable(self, dialog):
        dialog.set_modal(False)
        dialog.set_type_hint(gtk.gdk.WINDOW_TYPE_HINT_NORMAL)
        dialog.set_property("decorated", True)
        dialog.realize()
        dialog.window.set_functions(gtk.gdk.FUNC_CLOSE)
        dialog.window.set_decorations(gtk.gdk.DECOR_ALL)

class NoteAdaptor(ModalDialogAdaptor):
    def post_create (self, context, note, interactive=True):
        note.set_property("description", "Something noteworthy")
        self._make_usable(note)

    def fill_empty(self, context, note):
        note.vbox.add(context.create_placeholder())

    def replace_child(self, context, current, new, note):
        note.vbox.remove(current)
        note.vbox.add(new)

class WizardAdaptor(WidgetAdaptor):
    def fill_empty(self, context, wizard):
        notebook = gtk.Notebook()
        notebook.append_page(context.create_placeholder())
        wizard.set_property("wizard-notebook", notebook)

    def replace_child(self, context, current, new, wizard):
        notebook = wizard.get_property("wizard-notebook")
        if notebook:
          notebook.remove(current)
          notebook.add(new)

# Widgets

class WindowAdaptor(WidgetAdaptor):
    def post_create (self, context, window, interactive=True):
        window.set_size_request (672, 396)

    def fill_empty (self, context, widget):
        widget.add (context.create_placeholder ())
    
    def replace_child (self, context, current, new, container):
        container.add (new)


class GridAdaptor(WidgetAdaptor):
    def post_create (self, context, grid, interactive=True):
        print "post_create"
        #grid.set_size_request(100,100)

    def fill_empty(self, context, grid):
        print "fill_empty"
        grid.add(hildon.hildon_grid_item_new_with_label('debian-logo', 'New item'))
        grid.show_all()

    def replace_child(self, context, current, new, grid):
        print "replace_child"
        grid.remove(current)
        grid.add(new)

class ColorButtonAdaptor(WidgetAdaptor):
    def post_create (self, context, button, interactive=True):
        print button

#    def load(self, context, gtk_widget, blacklist):
#        project = context.get_project()
#        gwidget = Widget.load(gtk_widget, project, blacklist)
#        return gwidget

    def fill_empty(self, context, grid):
        pass

    def replace_child(self, context, grid):
        pass

class ButtonContentsAdaptor(TransparentProperty, StringType):
    pass

prop_registry.override_property('HildonColorButton::contents', ButtonContentsAdaptor)

class Items(CustomProperty, StringType):
    label = 'Items'
    default = ''
    def __init__(self, widget):
        # XXX: Adapter should be accessible in all properties
        self._adapter = adapter_registry.get_adapter(widget.gtk_widget, None)
        super(Items, self).__init__(widget)

    def load(self):
        self._value = ''
        self._initial = self.default

    def set_items(self, gtk_widget):
        for val in self._value.split('\n'):
        		gtk_widget.add_sort_key(val)

#    def save(self):
#        pass

    def get(self):
        return self._value

    def set(self, value):
        self._value = value
        self.set_items(self._object)

prop_registry.override_property('HildonSortDialog::items', Items)

class SortKeys(CustomProperty, StringType):
    label = 'Sort Keys'
    default = ''
    custom = True

    def __init__(self, widget):
        # XXX: Adapter should be accessible in all properties
        self._adapter = adapter_registry.get_adapter(widget.gtk_widget, None)
        super(SortKeys, self).__init__(widget)

    def get(self):
        return self._adapter.prop_get_sort_keys(self._object)

    def set(self, value):
      if value != '':
        self._adapter.prop_set_sort_keys(self._object, value)
prop_registry.override_property('HildonSortDialog::sort-keys', SortKeys)
