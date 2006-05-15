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

import gobject
import gtk

from gazpacho.properties import prop_registry, TransparentProperty, CustomProperty, StringType
from gazpacho.celllayouteditor import LayoutEditor
from gazpacho.widgetadaptor import WidgetAdaptor
from gazpacho.widgets.base.treeview import ModelPropertyEditor

# This is for Gtk+ 2.6 compatibility
class ComboBox(gtk.ComboBox):
    pass
gobject.type_register(ComboBox)

class ComboBoxAdaptor(WidgetAdaptor):
    type = ComboBox
    name = 'GtkComboBox'
    def pre_create(self, context, combo, interactive=True):
        model = gtk.ListStore(gobject.TYPE_STRING)
        combo.set_model(model)

    def fill_empty(self, context, widget):
        pass

class ComboBoxEntryAdaptor(WidgetAdaptor):
    pass

# These properties are buggy in GTK+ 2.4
prop_registry.override_simple('GtkComboBox::column-span-column', editable=False)
prop_registry.override_simple('GtkComboBox::row-span-column', editable=False)

# GtkComboBoxEntry
prop_registry.override_simple('GtkComboBoxEntry::text-column', default=0)

# This allows the editing of the CellRenderers, but not the data
class CellLayoutAdaptor(TransparentProperty):
    editor = LayoutEditor

prop_registry.override_property('GtkComboBox::columns', CellLayoutAdaptor)

class ComboBoxModelAdaptor(CustomProperty, StringType):
    editor = ModelPropertyEditor
    label = "Data model"
    
    def set(self, value):
        print "Setting model to", value
        
    
    def get(self):
        pass

prop_registry.override_property('GtkComboBox::model-edit', ComboBoxModelAdaptor)
