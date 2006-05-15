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

import gtk

from gazpacho.properties import prop_registry, TransparentProperty, EnumType, \
     CustomProperty
from gazpacho.widgets.base.base import ContainerAdaptor

class WindowAdaptor(ContainerAdaptor):
    def post_create(self, context, window, interactive=True):
        window.set_default_size(440, 250)

    def fill_empty(self, context, gtk_widget):
        gtk_widget.add(context.create_placeholder())

class TypeHintProperty(CustomProperty, EnumType):
    def get(self):
        value = self._object.get_data('gazpacho::type-hint')
        return value or gtk.gdk.WINDOW_TYPE_HINT_NORMAL
    
    def set(self, value):
        self._object.set_data('gazpacho::type-hint', value)
prop_registry.override_simple('GtkWindow::type-hint', TypeHintProperty)

prop_registry.override_simple('GtkWindow::default-width',
                              minimum=-1, maximum=10000)
prop_registry.override_simple('GtkWindow::default-height',
                              minimum=-1, maximum=10000)
prop_registry.override_simple('GtkWindow::modal', TransparentProperty)
prop_registry.override_simple('GtkWindow::type', TransparentProperty,
                              editable=False)
prop_registry.override_simple('GtkWindow::allow-shrink', editable=False)
prop_registry.override_simple('GtkWindow::allow-grow', editable=False)
prop_registry.override_simple('GtkWindow::role', translatable=False)
