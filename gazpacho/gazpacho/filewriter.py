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

import string

import cStringIO
import xml.dom
from xml.sax.saxutils import escape

import gobject
import gtk

from gazpacho.choice import enum_to_string, flags_to_string
from gazpacho.loader import tags
from gazpacho.placeholder import Placeholder
from gazpacho.properties import prop_registry
from gazpacho.widget import Widget
from gazpacho.modelplop import model_to_xml
from gazpacho.util import get_all_children

interesting_properties = ['title', 'expand']

def write_xml(file, xml_node, indent=0, indent_increase=4):
    if xml_node.nodeType == xml_node.TEXT_NODE:
        file.write(xml_node.data)
        return
    elif xml_node.nodeType == xml_node.CDATA_SECTION_NODE:
        file.write('<![CDATA[%s]]>' % xml_node.data)
        return

    file.write(' '*indent)
    
    file.write('<%s' % xml_node.tagName)
    if len(xml_node.attributes) > 0:
        attr_string = ' '.join(['%s="%s"' % (n, v)
                                    for n, v in xml_node.attributes.items()])
        file.write(' ' + attr_string)

    children = [a for a in xml_node.childNodes 
                    if a.nodeType != a.ATTRIBUTE_NODE]
    if children:
        has_text_child = False
        for child in children:
            if child.nodeType in (child.TEXT_NODE,
                                  child.CDATA_SECTION_NODE):
                has_text_child = True
                break

        if has_text_child:
            file.write('>')
        else:
            file.write('>\n')
        for child in children:
            write_xml(file, child, indent+indent_increase, indent_increase)

        if not has_text_child:
            file.write(' '*indent)
        file.write('</%s>\n' % xml_node.tagName)
    else:
        file.write('/>\n')

def write_extra_strings(file, extra_strings):
    for key in extra_strings:
        file.write('     \"%s\" \"%s\"\n' % (key, extra_strings[key]))

def write_extra_strings_element(file, extra_strings):
    if extra_strings:
        file.write('\n<!-- EXTRA_STRINGS\n')
        write_extra_strings(file, extra_strings)
        file.write('-->\n')

def write_extra_plural_strings(file, extra_plural_strings):
    for key in extra_plural_strings:
        file.write('    \"%s\"' % key)
        for string in extra_plural_strings[key]:
            file.write(' \"%s\"' % string)
        file.write('\n')

def write_extra_plural_strings_element(file, extra_plural_strings):
    if extra_plural_strings:
        file.write('\n<!-- EXTRA_PLURAL_STRINGS\n')
        write_extra_plural_strings(file, extra_plural_strings)
        file.write('-->\n')

class XMLWriter:
    def __init__(self, document=None, project=None):
        if not document:
            dom = xml.dom.getDOMImplementation()
            document = dom.createDocument(None, None, None)
        self._doc = document
        self._project = project
        
    def write(self, path, widgets, uim):
        self._write_root(widgets, uim)
        self._write_gazpacho_document(path, self._doc.documentElement)

    def _write_header(self, f, dtd_url):
        f.write("""<?xml version="1.0" standalone="no"?> <!--*- mode: xml -*-->
<!DOCTYPE glade-interface SYSTEM "%s">
""" % dtd_url)
        
    def _write_gazpacho_document(self, path, root):
        f = file(path, 'w')
        self._write_header(f, "http://gazpacho.sicem.biz/gazpacho-0.1.dtd")
        write_xml(f, root)
        write_extra_strings_element(f, self._project.extra_strings)
        write_extra_plural_strings_element(f, self._project.extra_plural_strings)
        f.close()

    # FIXME: Should this really be exported
    write_node = _write_gazpacho_document
    
    def _write_libglade_document(self, path, root):
        f = file(path, 'w')
        self._write_header(f, "http://glade.gnome.org/glade-2.0.dtd")

        f.write('\n<glade-interface>\n\n')
        for node in root.childNodes:
            write_xml(f, node, indent_increase=2)
        f.write('\n</glade-interface>\n')
        write_extra_strings_element(f, self._project.extra_strings)
        write_extra_plural_strings_element(f, self._project.extra_plural_strings)
        
        f.close()
        
    def serialize_node(self, widget):
        element = self._doc.createElement(tags.XML_TAG_PROJECT)
        root = self._doc.appendChild(element)

        # FIXME: Requirements? see _write_root
        
        # save the UI Manager if needed
        widget.project.uim.save_widget(widget, root, self._doc)
        root.appendChild(self._write_widget(widget))

        return self._doc.documentElement
        
    def serialize(self, widget):
        fp = cStringIO.StringIO()
        node = self.serialize_node(widget)
        write_xml(fp, node)
        write_extra_strings_element(fp, self._project.extra_strings)
        write_extra_plural_strings_element(fp, self._project.extra_plural_strings)
        
        fp.seek(0)
        
        return fp.read()

    def serialize_widgets(self, widgets, uim):
        fp = cStringIO.StringIO()
        write_xml(fp, self._write_root(widgets, uim))
        write_extra_strings_element(fp, self._project.extra_strings)
        write_extra_plural_strings_element(fp, self._project.extra_plural_strings)
        fp.seek(0)
        
        return fp.read()

    def _get_requirements(self, widgets):
        # check what modules are the widgets using
        # Not implemented
        return []
    
    def _write_root(self, widgets, uim):
        project_node = self._doc.createElement(tags.XML_TAG_PROJECT)
        node = self._doc.appendChild(project_node)

        for module in self._get_requirements(widgets):
            n = self._doc.createElement(tags.XML_TAG_REQUIRES)
            n.setAttribute(tags.XML_TAG_LIB, module)
            node.appendChild(n)

        self._write_widgets(node, widgets, uim)

        return node
    
    def _write_widgets(self, node, widgets, uim):

        # Append uimanager
        ui_widgets = [Widget.from_widget(w) 
                          for w in widgets
                               for t in uim.managed_types
                                    if isinstance(w, t)]

        if ui_widgets:
            ui_node = uim.save(self._doc, ui_widgets)
            if ui_node:
                node.appendChild(ui_node)

        treemodel_registry = uim.project.treemodel_registry

        # Append TreeModels
        for model in treemodel_registry.list_models():
            model_node = self._write_model_object(model, treemodel_registry)
            node.appendChild(model_node)
        
        # Append toplevel widgets. Each widget then takes care of
        # appending its children
        for widget in widgets:
            gwidget = Widget.from_widget(widget)
            if not gwidget.is_toplevel():
                continue
            
            wnode = self._write_widget(gwidget)
            node.appendChild(wnode)
    
    def _get_all_children(self, child, children):
        children.append(child)

    def _write_widget(self, widget):
        """Serializes this widget into a XML node and returns this node"""

        widget.maintain_gtk_properties = True
        
        widget.adaptor.save(widget.project.context, widget)

        # otherwise use the default saver
        node = self._write_basic_information(widget)
        
        self._write_properties(widget, node, child=False)
        self._write_signals(widget, node)
        
        gtk_widget = widget.gtk_widget
        
        if isinstance(gtk_widget, gtk.TreeView):
            for column in gtk_widget.get_columns():
                column_node = self._write_child_object(column)
                for i in column_node.childNodes:
                    if i.nodeName == "widget":
                        self._write_layout_object(i, column, widget.project.layout_registry)
                node.appendChild(column_node)

        if isinstance(gtk_widget, gtk.ComboBox):
            self._write_layout_object(node, gtk_widget, widget.project.layout_registry)
            
        # Children
        if not isinstance(gtk_widget, gtk.Container):
            return node

        # We're not writing children when we have a constructor set
        if widget.constructor:
            return node

        # gtk_widget.get_children() is not enought since some internal children
        # are not returned with that method
        children = get_all_children(gtk_widget)

        if isinstance(gtk_widget, gtk.Table):
            table = gtk_widget
            def table_sort(a, b):
                res =  cmp(table.child_get_property(a, 'left-attach'),
                           table.child_get_property(b, 'left-attach'))
                if res == 0:
                    res = cmp(table.child_get_property(a, 'top-attach'),
                              table.child_get_property(b, 'top-attach'))
                return res
            children.sort(table_sort)
        # Boxes doesn't need to be sorted, they're already in the right order

        for child_widget in children:
            child = self._write_child(child_widget)
            if child:
                node.appendChild(child)


        # save internal children that do not show in .get_children()
        for child_widget in widget.internal_children:
            child = self._write_child(child_widget)
            if child:
                node.appendChild(child)

        widget.maintain_gtk_properties = False

        return node

    def _write_basic_information(self, widget):
        assert widget.adaptor.type_name
        assert widget.name, 'widget %r is nameless' % widget.gtk_widget
        node = self._doc.createElement(tags.XML_TAG_WIDGET)

        # If name is set write it to the file
        if widget.adaptor.name:
            type_name = widget.adaptor.name
        else:
            type_name = widget.adaptor.type_name

        node.setAttribute(tags.XML_TAG_CLASS, type_name)
        node.setAttribute(tags.XML_TAG_ID, widget.name)
        if widget.constructor:
            node.setAttribute('constructor', widget.constructor)
        return node

    def _write_properties(self, widget, widget_node, child):
        # Sort them properties, to have a stable order
        gtk_widget = widget.gtk_widget
        properties = prop_registry.list(gobject.type_name(gtk_widget),
                                        gtk_widget.get_parent())
        properties.sort(lambda a, b: cmp(a.name, b.name))

        # write the properties
        for prop_type in properties:
            # don't save the name property since it is saved in the id
            # attribute of the tag
            if prop_type.name == 'name':
                continue

            # Do not save non-editable gobject properties, eg
            # GtkWindow::screen
            # hack?
            if (prop_type.base_type == gobject.GObject.__gtype__ and
                not prop_type.editable):
                continue

            if not prop_type.persistent:
                continue
            
            # child properties are saved later
            if prop_type.child != child:
                continue

            prop = widget.get_prop(prop_type.name)
           
            gtk_widget = widget.gtk_widget
            hacked = bool(gtk_widget.get_data('hacked_%s' % prop.name))
            if hacked:
                prop.translatable = bool(gtk_widget.get_data('i18n_is_translatable_%s' % prop.name))
                prop.has_i18n_context = bool(gtk_widget.get_data('i18n_has_context_%s' % prop.name))
                prop.i18n_comment = gtk_widget.get_data('i18n_comment_%s' % prop.name)
                prop.engineering_english = gtk_widget.get_data('engineering_english_%s' % prop.name) 
                gtk_widget.set_data('hacked_%s' % prop.name, False)
            
            child_node = self._write_property(prop)
            if child_node:
                widget_node.appendChild(child_node)

    def _write_signals(self, widget, widget_node):
        for signal_name, handlers in widget.signals.items():
            for handler in handlers:
                child = self._doc.createElement(tags.XML_TAG_SIGNAL)
                child.setAttribute(tags.XML_TAG_NAME, handler['name'])
                child.setAttribute(tags.XML_TAG_HANDLER, handler['handler'])
                child.setAttribute(tags.XML_TAG_AFTER,
                                   handler['after'] \
                                   and tags.TRUE or tags.FALSE)
                widget_node.appendChild(child)
                
    def _write_child(self, gtk_widget):
        child_tag = self._doc.createElement(tags.XML_TAG_CHILD)
        if isinstance(gtk_widget, Placeholder):
            child = self._doc.createElement(tags.XML_TAG_PLACEHOLDER)
            child_tag.appendChild(child)
            # we need to write the packing properties of the placeholder.
            # otherwise the container gets confused when loading its
            # children
            packing = self._write_placeholder_properties(gtk_widget)
            if packing is not None:
                child_tag.appendChild(packing)
            return child_tag

        child_widget = Widget.from_widget(gtk_widget)
        if child_widget is None:
            # if there is no GazpachoWidget for this child
            # we don't save it. If your children are not being
            # saved you should create a GazpachoWidget for them
            # in your Adaptor
            return
            
        if child_widget.internal_name is not None:
            child_tag.setAttribute(tags.XML_TAG_INTERNAL_CHILD,
                                   child_widget.internal_name)

        child = self._write_widget(child_widget)
        child_tag.appendChild(child)

        # Append the packing properties
        packing = self._doc.createElement(tags.XML_TAG_PACKING)
        self._write_properties(child_widget, packing, child=True)

        if packing.childNodes:
            child_tag.appendChild(packing)

        return child_tag

    def _write_placeholder_properties(self, placeholder):
        parent = placeholder.get_parent()
        # get the non default packing properties
        packing_list = []
        props = gtk.container_class_list_child_properties(parent)
        for prop in props:
            v = parent.child_get_property(placeholder, prop.name)
            if v != prop.default_value:
                packing_list.append((prop, v))
        
        if not packing_list:
            return
        
        packing_node = self._doc.createElement(tags.XML_TAG_PACKING)
        for prop, value in packing_list:
            prop_node = self._doc.createElement(tags.XML_TAG_PROPERTY)
            prop_name = prop.name.replace('-', '_')
            prop_node.setAttribute(tags.XML_TAG_NAME, prop_name)

            if prop.value_type == gobject.TYPE_ENUM:
                v = enum_to_string(value, prop)
            elif prop.value_type == gobject.TYPE_FLAGS:
                v = flags_to_string(value, prop)
            else:
                v = escape(str(value))

            text = self._doc.createTextNode(v)
            prop_node.appendChild(text)
            packing_node.appendChild(prop_node)

        return packing_node
        
    def _write_property(self, prop):
        value = prop.save()
        
        # None means it doesn't need to be saved, it already contains
        # the default value or shouldn't be saved
        if value == None:
            return
        
        node = self._doc.createElement(tags.XML_TAG_PROPERTY)

        # We should change each '-' by '_' on the name of the property
        # put the name="..." part on the <property ...> tag
        node.setAttribute(tags.XML_TAG_NAME, prop.name.replace('-', '_'))

        if prop.cdata:
            escape_func = lambda value: '<![CDATA[%s]]>' % value
        else:
            escape_func = escape

        # Only write context and comment if translatable is
        # enabled, to mimic glade-2
        if prop.is_translatable():
            node.setAttribute(tags.XML_TAG_TRANSLATABLE, tags.YES)
            if prop.has_i18n_context:
                node.setAttribute(tags.XML_TAG_CONTEXT, tags.YES)
            if prop.i18n_comment:
                node.setAttribute(tags.XML_TAG_COMMENT, prop.i18n_comment)
            if prop.engineering_english:
                node.setAttribute(tags.XML_TAG_ENGINEERING_ENGLISH, escape_func(value))
                text = self._doc.createTextNode(escape_func(prop.engineering_english))
            else:
                text = self._doc.createTextNode(escape_func(value))
                
        else:
            text = self._doc.createTextNode(escape_func(value))

        node.appendChild(text)
        return node

#
# The following is padding for the gap between now and real GObject support in G
#    

    def _write_child_object(self, gobject):
        child_tag = self._doc.createElement(tags.XML_TAG_CHILD)

        child = self._write_object(gobject)
        child_tag.appendChild(child)
        
        return child_tag

    def _write_object(self, g_object):
        node = self._doc.createElement(tags.XML_TAG_WIDGET)
        
        if isinstance(g_object, gtk.TreeViewColumn):
            node.setAttribute(tags.XML_TAG_ID, g_object.get_title())
        else:
            node.setAttribute(tags.XML_TAG_ID, gobject.type_name(g_object))
        node.setAttribute(tags.XML_TAG_CLASS, gobject.type_name(g_object))

        for prop in gobject.list_properties(g_object):
            value = None
            if prop.name in interesting_properties:
                try:
                    value = g_object.get_property(prop.name)
                except TypeError:
                    pass
                if value is not None:
                        prop_node = self._doc.createElement(tags.XML_TAG_PROPERTY)
                        prop_node.setAttribute(tags.XML_TAG_NAME, prop.name)
                        text = self._doc.createTextNode(escape(str(value)))
                        prop_node.appendChild(text)
                        node.appendChild(prop_node)
        
        return node

    def _write_layout_object(self, node, gobject, layout_registry):
        layoutproxy = layout_registry.proxy_for(gobject)

        if layoutproxy is not None:
            for renderer in layoutproxy.get_renderers():
                renderer_node = self._write_renderer_object(renderer, layoutproxy)
                node.appendChild(renderer_node)

        
    def _write_renderer_object(self, renderer, layoutproxy):
        child_tag = self._doc.createElement(tags.XML_TAG_CHILD)

        child = self._write_object(renderer)

        attributes = layoutproxy.get_attributes(renderer)
        
        if attributes is not None:
            for (name, column) in attributes:
                prop_node = self._doc.createElement(tags.XML_TAG_PROPERTY)
                prop_node.setAttribute(tags.XML_TAG_NAME, name)
                text = self._doc.createTextNode(str(column))
                prop_node.appendChild(text)
                child.appendChild(prop_node)

        child_tag.appendChild(child)
        
        return child_tag

    def _write_model_columns(self, model):
        columns = model.get_n_columns()
        if columns == 0:
            return None
            
        columns_tag = self._doc.createElement(tags.XML_TAG_PROPERTY)

        columns_tag.setAttribute(tags.XML_TAG_NAME, "columns")
        
        types = []
        for c in range(columns):
            types.append(gobject.type_name(model.get_column_type(c)))

        text = self._doc.createTextNode(string.join(types, ":"))

        columns_tag.appendChild(text)
        
        return columns_tag

    def _write_model_object(self, model, treemodel_registry):
        model_node = self._write_object(model)
        column_node = self._write_model_columns(model)
        if column_node is not None:
            model_node.appendChild(column_node)
        
        model_name = treemodel_registry.lookup_name(model)
        model_node.setAttribute(tags.XML_TAG_ID, treemodel_registry.lookup_name(model))
        
        # Save the data as a separate file
        
        xml = model_to_xml(model)
        
        f = open(model_name + ".xml", 'w')
        f.write(xml)
        f.close();

        return model_node
