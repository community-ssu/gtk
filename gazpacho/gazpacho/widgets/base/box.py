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

import gtk

from gazpacho.properties import prop_registry, CustomProperty, IntType
from gazpacho.placeholder import Placeholder
from gazpacho.widgets.base.base import ContainerAdaptor
from gazpacho.widget import Widget

# TODO: we shouldn't need to specify these adaptors if adaptor inheritance
#       would work
class HBoxAdaptor(ContainerAdaptor):
    pass

class VBoxAdaptor(ContainerAdaptor):
    pass

class HButtonBoxAdaptor(HBoxAdaptor):
    pass

class VButtonBoxAdaptor(VBoxAdaptor):
    pass

# GtkBox
class BoxSizeProp(CustomProperty, IntType):
    minimum = 1
    default = 3
    label = 'Size'
    persistent = False
    def load(self):
        self._initial = self.default
    	
        # Don't set default if object has childs already
        if not self.get():
            self.set(self.default)

    def get(self):
        return len(self._object.get_children())

    def set(self, new_size):
        old_size = len(self._object.get_children())
        if new_size == old_size:
            return
        elif new_size > old_size:
            # The box has grown. Add placeholders
            while new_size > old_size:
                self._object.add(Placeholder(self._project.get_app()))
                old_size += 1
        elif new_size > 0:
            # The box has shrunk. Remove the widgets that are not on those slots
            child = self._object.get_children()[-1]
            while old_size > new_size and child:
                gwidget = Widget.from_widget(child)
                if gwidget: # It may be None, e.g a placeholder
                    gwidget.project.remove_widget(child)

                gtk.Container.remove(self._object, child)
                child = self._object.get_children()[-1]
                old_size -= 1
    
prop_registry.override_property('GtkBox::size', BoxSizeProp)

