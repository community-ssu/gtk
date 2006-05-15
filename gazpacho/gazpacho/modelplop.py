# Copyright (C) 2005 by Nokia Corporation
#
# Modelplop, an utility to convert a treemodel to a xml representation
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

from string import join
from xml.dom import minidom, Node

import gobject
import gtk


docstring ="""\
<?xml version="1.0" standalone="no"?> <!--*- mode: xml -*-->
<store>
</store>
"""

def xml_append_row(doc, parentnode, rows):
    if not rows:
        return
    
    for row in rows:
        rowtag = doc.createElement("row")
        for column, value in enumerate(row):
            try:
                type_name = gobject.type_name(type(value))
                columntag = doc.createElement(type_name)
                columntag.setAttribute("column", str(column))
                if type(value) is gtk.gdk.Pixbuf:
                    savevalue = value.get_data("gazpacho::pixbuf-save-data")
                    texttag = doc.createTextNode(str(savevalue))
                else:
                    texttag = doc.createTextNode(str(value))
                columntag.appendChild(texttag)
                rowtag.appendChild(columntag)
            except TypeError:
                pass
        xml_append_row(doc, rowtag, row.iterchildren())
    
        parentnode.appendChild(rowtag)

def model_to_xml(model):
  doc = minidom.parseString(docstring)

  storetag = doc.getElementsByTagName("store")[0]
  
  storetag.setAttribute("class", gobject.type_name(model))

  types = []
  for c in range (model.get_n_columns())  :
    types.append(gobject.type_name(model.get_column_type(c)))

  columnstag = doc.createElement("columns")
  texttag = doc.createTextNode(join(types, ":"))
  columnstag.appendChild(texttag)
  
  storetag.appendChild(columnstag)
  
  if len(model) > 0:
    xml_append_row(doc, storetag, model)
  
  return doc.toxml()

def tree_store_append_row(node, store, parent):

    treeiter = store.append(parent)
    # Parse data into an array
    data = []
    for child_node in node.childNodes:
        if child_node.tagName == "row":
            tree_store_append_row(child_node, store, treeiter)
            continue

        data_type = gobject.type_from_name(child_node.tagName)
        for n in child_node.childNodes:
            if n.nodeType == node.TEXT_NODE:
                if gobject.type_is_a(data_type, int):
                    data.append(int(n.data))
                elif gobject.type_is_a(data_type, bool):
                    data.append(n.data == 'True')
                elif gobject.type_is_a(data_type, gtk.gdk.Pixbuf):
                    pixbuf = None
                    try:
                        pixbuf = gtk.gdk.pixbuf_new_from_file_at_size(n.data,
                                                                      24, 24)
                        data.append(pixbuf)
                    except:
                        try:
                            theme = gtk.icon_theme_get_default()
                            pixbuf = theme.load_icon(n.data, 24, 0)
                            data.append(pixbuf)
                        except:
                            pass
                        
                else:
                    data.append(n.data)
                break
    
    for i, d in enumerate(data):
        try:
            store.set(treeiter, i, d)
        except TypeError:
            print "Type", type(d), "is invalid for column", i, \
                  "of type", store.get_column_type(i), ", skipping."

def list_store_append_row(node, store):

    treeiter = store.append()
    # Parse data into an array
    data = []
    for child_node in node.childNodes:
        if child_node.tagName == "row":
            continue

        data_type = gobject.type_from_name(child_node.tagName)
        for n in child_node.childNodes:
            if n.nodeType == node.TEXT_NODE:
                if gobject.type_is_a(data_type, int):
                    data.append(int(n.data))
                elif gobject.type_is_a(data_type, bool):
                    data.append(n.data == 'True')
                elif gobject.type_is_a(data_type, gtk.gdk.Pixbuf):
                    pixbuf = None
                    try:
                        pixbuf = gtk.gdk.pixbuf_new_from_file_at_size(n.data,
                                                                      24, 24)
                        data.append(pixbuf)
                    except:
                        try:
                            theme = gtk.icon_theme_get_default()
                            pixbuf = theme.load_icon(n.data, 24, 0)
                            data.append(pixbuf)
                        except:
                            pass
                        
                else:
                    data.append(n.data)
                break
    
    for i, d in enumerate(data):
        try:
            store.set(treeiter, i, d)
        except TypeError:
            print "Type", type(d), "is invalid for column", i, \
                  "of type", store.get_column_type(i), ", skipping."


def xml_to_model(xmlstring):
    doc = minidom.parseString(xmlstring)

    storetag = doc.getElementsByTagName("store")[0]
    
    class_type = storetag.getAttribute("class")

    columnstag = doc.getElementsByTagName("columns")[0]
    for node in columnstag.childNodes:
        if node.nodeType == node.TEXT_NODE:
            type_names = node.data.split(":")
            types = []
            for name in type_names:
                types.append(gobject.type_from_name(name))
            break

    if not types:
        return None

    if class_type == "GtkTreeStore":
        store = gtk.TreeStore(*types)
        for node in storetag.childNodes:
            if node.nodeType == node.TEXT_NODE:
                continue
            if node.tagName == "columns":
                continue
            
            if node.tagName == "row":
                tree_store_append_row(node, store, None)
                
    elif class_type == "GtkListStore":
        store = gtk.ListStore(*types)
        for node in storetag.childNodes:
            if node.nodeType == node.TEXT_NODE:
                continue
            if node.tagName == "columns":
                continue
            
            if node.tagName == "row":
                list_store_append_row(node, store)
    else:
        return None


    return store

