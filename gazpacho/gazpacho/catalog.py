# Copyright (C) 2004,2005 by SICEm S.L., Imendio AB and Async Open Source
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

import os
import sys
import warnings
import xml.dom.minidom

from gazpacho import dialogs, util
from gazpacho.environ import environ
from gazpacho.library import load_library, LibraryLoadError
from gazpacho.loader import tags
from gazpacho.widgetadaptor import WidgetAdaptor
from gazpacho.widgetregistry import widget_registry

class BadCatalogSyntax(Exception):
    def __init__(self, file, msg):
        self.file, self.msg = file, msg
        
    def __str__(self):
        return "Bad syntax in the catalog %s: %s" % (self.file, self.msg)

class CatalogError(Exception):
    pass

class WidgetGroup(list):
    """List class used to store a widget group and its child nodes.

    The name property sets/gets the group internal name.
    The title property sets/gets the group title string.
    The class list contains the widget type strings for the group.
    """
    def __init__(self, name, title):
        self.name = name
        self.title = title

class Catalog:
    """Class to hold widget classes and groups.

    The title property gets/sets the catalog title string.
    The library property gets/sets the catalog library string.
    The widget_groups attribute holds a list of widget groups in the catalog.
    """
    def __init__(self, filename, resources_path):
        self.title = None
        self.library = None
        self.resource_path = resources_path
        self.widget_groups = []
        self.filename = filename
        self._parse_filename(filename)
        
    def _parse_filename(self, filename):
        fp = file(environ.find_catalog(filename))
        document = xml.dom.minidom.parse(fp)
        root = document.documentElement
        self._parse_root(root)

    def _parse_root(self, node):
        """<!ELEMENT glade-catalog (glade-widget-classes?, 
                                    glade-widget-group*)>"""

        if node.nodeName == tags.GLADE_CATALOG:
            self._parse_catalog(node)
        else:
            msg = "Root node is not glade catalog: %s" % node.nodeName
            raise BadCatalogSyntax(self.filename, msg)
        
        for child in node.childNodes:
            if child.nodeType != child.ELEMENT_NODE:
                continue

            if child.nodeName == tags.GLADE_WIDGET_CLASSES:
                self._parse_widget_classes(child)
            elif child.nodeName == tags.GLADE_WIDGET_GROUP:
                self._parse_widget_group(child)
            else:
                print 'Unknown node: %s' % child.nodeName
                
    def _parse_catalog(self, node):
        """<!ELEMENT glade-catalog (glade-widget-classes?, 
                                    glade-widget-group*)>
           <!ATTLIST glade-catalog name            CDATA #REQUIRED
                                   priority        CDATA #IMPLIED
                                   library         CDATA #IMPLIED
                                   library-type  (c|python) #IMPLIED
                                   requires        CDATA #IMPLIED>"""

        # name
        self.title = str(node.getAttribute(tags.NAME))

        # priority
        priority = sys.maxint
        if node.hasAttribute(tags.PRIORITY):
            priority = int(node.getAttribute(tags.PRIORITY))
        self.priority = priority

        # library
        library_name = str(node.getAttribute(tags.LIBRARY))

        # library-type
        library_type = "python"
        if node.hasAttribute(tags.LIBRARY_TYPE):
            library_type = str(node.getAttribute(tags.LIBRARY_TYPE))
    
        name = os.path.basename(self.filename[:-4])
        try:
            self.library = load_library(library_type, name, library_name)
        except LibraryLoadError, e:
            raise CatalogError('Could not load library %s: %s' % (
                name, e))
    
    def _parse_widget_classes(self, node):
        "<!ELEMENT glade-widget-classes (glade-widget-class+)>"

        for class_node in node.childNodes:
            if class_node.nodeName != tags.GLADE_WIDGET_CLASS:
                continue

            self._parse_widget_class(class_node)

    def _parse_widget_class(self, node):
        """<!ELEMENT glade-widget-class (tooltip?)>
           <!ATTLIST glade-widget-class name          CDATA #REQUIRED
                                        generic-name  CDATA #IMPLIED
		                        title         CDATA #IMPLIED
                                        adaptor-class CDATA #IMPLIED>
                                        register-type-function CDATA #IMPLIED>"""

        name = str(node.getAttribute(tags.NAME))
        if widget_registry.has_name(name):
            print 'The widget class %s has at least two different definitions' % name
            return
        
        generic_name = str(node.getAttribute(tags.GENERIC_NAME))
        palette_name = str(node.getAttribute(tags.TITLE))
        adaptor_class_name = str(node.getAttribute(tags.ADAPTOR_CLASS))

        tooltip = None
        packing_defaults = {}
        for child in node.childNodes:
            if child.nodeName == tags.TOOLTIP:
                tooltip = util.xml_get_text_from_node(child)

        adaptor = self.library.get_widget_adaptor_class(name,
                                                        adaptor_class_name)
        if not adaptor:
            # TODO: create a more apropiate adaptor seeing the gtype
            adaptor = WidgetAdaptor()
            
        adaptor.setup(name, generic_name, palette_name, self.library,
                      self.resource_path, tooltip)

        # store the Widget Adaptor on the cache
        widget_registry.add(adaptor)

    def _parse_widget_group(self, node):
        """<!ELEMENT glade-widget-group (glade-widget-class-ref+)>
           <!ATTLIST glade-widget-group name  CDATA #REQUIRED
                                        title CDATA #REQUIRED>"""

        name = str(node.getAttribute(tags.NAME))
        title = str(node.getAttribute(tags.TITLE))

        widget_group = WidgetGroup(name, title)
        
        for child in node.childNodes:
            if child.nodeName != tags.GLADE_WIDGET_CLASS_REF:
                continue
            
            name = str(child.getAttribute(tags.NAME))
            widget_adaptor = widget_registry.get_by_name(name)
            if widget_adaptor is not None:
                widget_group.append(widget_adaptor)
            else:
                print 'could not load widget class %s' % name

        self.widget_groups.append(widget_group)
        
    def __repr__(self):
        return '<Catalog %s>' % self.title

_catalogs = []

def get_all_catalogs():
    """Return a list of existing Catalogs.

    The application widget registry is cleared and previously loaded classes
    and groups removed before rereading existing catalogs.
    The GTK+ catalog takes precedence and catalogs are ordered after
    their declared priority.
    """
    global _catalogs

    if not _catalogs:
        load_catalogs()
        
    assert _catalogs
    return _catalogs

def load_catalogs(external=True):
    global _catalogs
    widget_registry.clean()

    try:
        catalogs = environ.get_catalogs()
    except EnvironmentError, e:
        dialogs.error("<b>There were errors while loading Gazpacho</b>", str(e))
        raise SystemExit

    if 'base' not in catalogs:
        raise SystemExit, "Unable to find GTK+ catalog which is mandatory"

    problems = []
    for name, filename in catalogs.items():
        if not external and filename != 'base.xml':
            continue

        if filename.endswith('gtk+.xml'):
            warnings.warn("gtk+.xml found, it has been replaced by " +
                          "base.xml. Please remove it.", stacklevel=2)
            continue
        
        resources_path = environ.get_resources_dir(name)
        try:
            catalog = Catalog(filename, resources_path)
            _catalogs.append(catalog)
        except IOError, e:
            problems.append(str(e))
        except BadCatalogSyntax, e:
            problems.append(str(e))
        except CatalogError, e:
            print str(e)

    if problems:
        dialogs.error('\n'.join(problems))

    _catalogs.sort(lambda c1, c2: cmp(c1.priority,
                                      c2.priority))
