# Copyright (C) 2005 by Imendio AB
#               2005 by Async Open Source
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

from gazpacho.interfaces import BaseLibrary
from gazpacho.loader.custom import adapter_registry

__all__ = ['LibraryLoadError', 'load_library', 'registry_library']

class LibraryLoadError(Exception):
    pass

class LibraryRegistry:
    def __init__(self):
        self._libraries = {}

    def register(self, name, library_class):
        if name in self._libraries:
            raise ValueError("A library called %s is already registered: %s" %
                             name)

        self._libraries[name] = library_class

    def get(self, name):
        if not name in self._libraries:
            raise ValueError("Unknown library: %s" % name)

        return self._libraries[name]

_registry = LibraryRegistry()

def registry_library(name, library_class):
    """
    Register a library, can be used by external parts to be
    able to write bindings for other langauges.
    library_class must be a subclass of BaseLibrary

    @param name          name of the library loader
    @param library_class class to be called
    """
    
    global _registry
    
    if not isinstance(library_class, BaseLibrary):
        raise TypeError("library_class must be a subclass of BaseLibrary")

    _registry.register(name, library_class)
    
def load_library(library_type, name, library_name):
    """
    Load a library, named library_type

    @param library_type
    @param name
    @param library_name
    """
    global _registry
    library = _registry.get(library_type)
    return library(name, library_name)

# Register a python library/loader
class PythonLibrary(BaseLibrary):
    def __init__(self, name, library_name):
        self.name = name
        
        full = 'gazpacho.widgets.' + name
        namespace = locals()
        try:
            module = __import__(full, namespace, namespace, full)
        except ImportError, e:
            raise LibraryLoadError("ImportError: %s" % e)

        if not hasattr(module, 'root_library'):
            raise LibraryLoadError("module `%s' does not define a root_library" % name)
        
        self._fullname = full
        self._module = module
        self._root_library = module.root_library
	
    def _get_adaptor_class(self, type_name, class_name):
        if not class_name:
            return

        longname = self._fullname + '.' + class_name
        parts = longname.split('.')
        modulename = '.'.join(parts[:-1])
        adaptor_class = parts[-1]

        try:
            module = __import__(modulename, {}, {}, modulename)
        except ImportError, e:
            print "Could not load module: %s" % modulename
            return

        adaptor_class = getattr(module, adaptor_class, None)
        if adaptor_class is None:
            print "Adaptor class %s not found" % class_name
            return
            
        return adaptor_class()

    def get_property_adaptor_class(self, type_name, property_name,
                                   property_type, class_name):
        return self._get_adaptor_class(type_name, class_name)

    def get_widget_adaptor_class(self, type_name, class_name):
        return self._get_adaptor_class(type_name, class_name)
        
    def get_class(self, class_name):
        return self._namespace.get(class_name)

    def create_widget(self, gtype):
        adapter = adapter_registry.get_adapter(gtype, None)
        return adapter.construct('', gtype, {})
    
_registry.register('python', PythonLibrary)

