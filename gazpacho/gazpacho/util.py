# Copyright (C) 2004,2005 by SICEm S.L.
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

import random
from xml.sax.saxutils import escape, unescape

import gobject
import gtk

from gazpacho.annotator import Annotator
from gazpacho.loader import tags
from gazpacho.properties import UNICHAR_PROPERTIES

HAS_NODES = "glade_util_has_nodes"

def hide_window(window):
    x, y = window.get_position()
    window.hide()
    window.move(x, y)

def get_parent(gtk_widget):
    from gazpacho.widget import Widget
    parent_widget = gtk_widget
    gwidget = None
    while True:
        parent_widget = parent_widget.get_parent()
        if parent_widget is None:
            return None
        gwidget = Widget.from_widget(parent_widget)
        if gwidget is not None:
            return gwidget

    return None

def circle_select(leaf_gtk_widget):
    """Select the parent of the selected widget. If the selected
    widget is a toplevel widget or there isn't a selected widget in
    the current branch of the tree, select the leaf widget.
    """
    from gazpacho.widget import Widget
    leaf_gwidget = Widget.from_widget(leaf_gtk_widget)
    parent_gwidget = get_parent(leaf_gtk_widget)

    # If the leaf widget doesn't have a parent select it.
    if not parent_gwidget:
        leaf_gwidget.project.selection_set(leaf_gtk_widget, True)
        return

    # If there is no or multiple selections, select the leaf widget.
    project = parent_gwidget.project
    if not project.selection or len(project.selection) != 1:
        project.selection_set(leaf_gtk_widget, True)
        return

    selected = project.selection[0]

    # If the leaf widget is selected, select its parent
    if leaf_gtk_widget is selected:
        project.selection_set(parent_gwidget.gtk_widget, True)
        return

    # Circle through the tree
    new_selection = leaf_gtk_widget
    while parent_gwidget:
        if parent_gwidget.gtk_widget is selected:
            # Choose the leaf if selected is toplevel
            if parent_gwidget.is_toplevel():
                new_selection = leaf_gtk_widget
            # otherwise choose the parent
            else:
                new_selection = parent_gwidget.get_parent().gtk_widget
            break
        parent_gwidget = parent_gwidget.get_parent()
    project.selection_set(new_selection, True)

def _get_window_positioned_in(widget):
    """This returns the window that the given widget's allocation is relative to.
    Usually this is widget.get_parent_window(). But if the widget is a
    toplevel, we use its own window, as it doesn't have a parent."""
    if widget.get_parent():
        return widget.get_parent_window()
    else:
        return widget.window

def _calculate_window_offset(gdkwindow):
    """ This calculates the offset of the given window within its toplevel.
    It also returns the toplevel """
    x = y = 0
    window = gdkwindow
    while True:
        if window.get_window_type() != gtk.gdk.WINDOW_CHILD:
            break
        tmp_x, tmp_y = window.get_position()
        x += tmp_x
        y += tmp_y
        window = window.get_parent()

    return window, x, y

def _get_border_color(widget):
    if not isinstance(widget, (gtk.Box, gtk.Table)):
        return None

    if not (widget.parent):
        return None
    
    if isinstance(widget.parent, (gtk.Notebook, gtk.Window)):
        return None
    
    colors = [(1, 0, 0),
              (0, 1, 0),
              (0, 0, 1),
              (1, 1, 0),
              (0, 0.6, 0.6),
              (1, 0.5, 0)]
    
    index = widget.get_data("gazpacho-border-color")
    if index == None:
        index = random.randint(0, len(colors) - 1)
        widget.set_data("gazpacho-border-color", index)

    return colors[index]
        
def _draw_box_borders(project, widget, expose_win, annotator):
    if not project._app._show_structure:
        return

    if not isinstance(widget, gtk.Container):
        return

    if not widget.flags() & gtk.MAPPED:
        return

    border_win = _get_window_positioned_in(widget)
    toplevel, border_x, border_y = _calculate_window_offset(border_win)
    toplevel, expose_x, expose_y = _calculate_window_offset(expose_win)
    
    if isinstance(widget, gtk.Button):
        return

    color = _get_border_color(widget)
    if color:
        allocation = widget.allocation
        annotator.draw_border(border_x + allocation.x - expose_x,
                              border_y + allocation.y - expose_y,
                              allocation.width,
                              allocation.height,
                              color)
    
    children = widget.get_children()
    for child in children:
        _draw_box_borders(project, child, expose_win, annotator)

def _can_draw_nodes(sel_widget, expose_win):
    """ This returns TRUE if it is OK to draw the selection nodes for the given
    selected widget inside the given window that has received an expose event.
    This is true if the expose window is a descendent of the window to which
    widget->allocation is relative. (The check for a descendent preserves
    clipping in a situation like a viewport) """
    
    sel_win = _get_window_positioned_in(sel_widget)
    window = expose_win
    while window:
        if window == sel_win:
            return True
        window = window.get_parent()
            
    return False

def _draw_nodes(project, expose_widget, expose_win, annotator):
    """ This is called to redraw any selection nodes that intersect the given
    exposed window.  It steps through all the selected widgets, converts the
    coordinates so they are relative to the exposed window, then calls
    _draw_nodes if appropriate """
    
    # Calculate the offset of the expose window within its toplevel
    expose_toplevel, expose_win_x, expose_win_y = \
                     _calculate_window_offset(expose_win)

    expose_win_w, expose_win_h = expose_win.get_size()

    # Step through all the selected widgets in the project
    for sel_widget in project.selection:
        sel_win = _get_window_positioned_in(sel_widget)

        if sel_win is None:
            continue
        
        # Calculate the offset of the selected widget's window within
        # its toplevel
        sel_toplevel, sel_x, sel_y = _calculate_window_offset(sel_win)

        # We only draw the nodes if the window that got the expose event is
        # in the same toplevel as the selected widget
        if (expose_toplevel == sel_toplevel and
            _can_draw_nodes(sel_widget, expose_win)):
            x = sel_x + sel_widget.allocation.x - expose_win_x
            y = sel_y + sel_widget.allocation.y - expose_win_y
            w = sel_widget.allocation.width
            h = sel_widget.allocation.height

            # Draw the selection nodes if they intersect the
            # expose window bounds
            if (x < expose_win_w and x + w >= 0 and
                y < expose_win_h and y + h >= 0):
                annotator.draw_nodes(x, y, w, h)

def draw_annotations(expose_widget, expose_win):
    """ This is called to redraw any gazpacho annotations that intersect the given
    exposed window. We only draw nodes on windows that are actually owned
    by the widget. This keeps us from repeatedly drawing nodes for the
    same window in the same expose event. """
    
    from gazpacho.widget import Widget

    expose_gwidget = Widget.from_widget(expose_widget)
    if not expose_gwidget:
        expose_gwidget = get_parent(expose_widget)
    if not expose_gwidget:
        return False

    project = expose_gwidget.project

    if not expose_win.is_viewable():
        return False

    # Find the corresponding GtkWidget and GladeWidget
    if expose_widget != expose_win.get_user_data():
        return False

    annotator = Annotator(expose_widget, expose_win)
    
    _draw_box_borders(project, expose_widget.get_toplevel(), expose_win, annotator)
    _draw_nodes(project, expose_widget, expose_win, annotator)
        
def invalidate_node_windows(widget):
    """ Invalidates the portion of the window covered by the selection
    nodes for the given widget """

    if not widget.window:
        return
    
    sel_win = _get_window_positioned_in(widget)
    sel_win.invalidate_rect(widget.allocation, True)
    
def has_nodes(widget):
    nodes = widget.get_data(HAS_NODES)
    if nodes and nodes == 1: return True
    return False

def add_nodes(widget):
    widget.set_data(HAS_NODES, 1)
    invalidate_node_windows(widget)
    
def remove_nodes(widget):
    widget.set_data(HAS_NODES, 0)
    invalidate_node_windows(widget)

# xml utility functions
def xml_create_string_prop_node(xml_doc, prop_name, prop_value):
    node = xml_doc.createElement(tags.XML_TAG_PROPERTY)
    node.setAttribute(tags.XML_TAG_NAME, prop_name)
    text_node = xml_doc.createTextNode(escape(prop_value))
    node.appendChild(text_node)
    return node

def xml_get_text_from_node(xmlnode):
    text = ''
    for node in xmlnode.childNodes:
        if node.nodeType == node.TEXT_NODE:
            text += node.data
    return unescape(text)

def get_bool_from_string_with_default(the_string, default):
    if the_string in ['True', 'TRUE', 'true', 'yes', '1']:
        return True
    elif the_string in ['False', 'FALSE', 'false', 'no', '0']:
        return False
    else:
        return default
    
def xml_get_property_boolean(xmlnode, tag, default):
    value = xmlnode.getAttribute(tag)
    return get_bool_from_string_with_default(value, default)

def xml_filter_nodes(nodes, node_type):
    return [node for node in nodes if node.nodeType == node_type]
        
def xml_get_elements_one_level(xml_node, tag_name):
    """Just like getElementsByTagName except that it just traverses
    one level
    """
    l = []
    for node in xml_node.childNodes:
        if node.nodeType != xml_node.ELEMENT_NODE:
            continue
        if node.tagName != tag_name:
            continue

        l.append (node)

    return l
    
METADATA_I18N_IS_TRANSLATABLE = 'i18n_is_translatable'
METADATA_I18N_HAS_CONTEXT = 'i18n_has_context'
METADATA_I18N_COMMENT = 'i18n_comment'

def gtk_widget_set_metadata(widget, prefix, key, value):
    full_key = '%s_%s' % (prefix, key)
    widget.set_data(full_key, value)

def gtk_widget_get_metadata(widget, prefix, key):
    full_key = '%s_%s' % (prefix, key)
    return widget.get_data(full_key)

def get_property_value_from_string(property_class, str_value):
    if property_class.type == gobject.TYPE_BOOLEAN:
        value = get_bool_from_string_with_default(str_value, True)
    elif property_class.type in (gobject.TYPE_FLOAT, gobject.TYPE_DOUBLE):
        value = float(str_value or 0.0)
    elif property_class.type == gobject.TYPE_INT:
        value = int(str_value or 0)
    elif property_class.type == gobject.TYPE_STRING:
        value = str_value or ""
    elif property_class.type == gobject.TYPE_UINT:
        if property_class.name in UNICHAR_PROPERTIES:
            value = unicode(str_value and str_value[0] or "")
        else:
            value = int(str_value or 0)
    elif property_class.type == gobject.TYPE_ENUM:
        value = int(str_value or 0)
    elif property_class.type == gobject.TYPE_FLAGS:
        value = int(str_value or 0)
    elif property_class.type == gobject.TYPE_OBJECT:
        value = str_value
    elif property_class.type is None:
        print ("Error: the property %s should not be of type None" %
               property_class.name)
        return None

    return value

# useful treeview functions
def unselect_when_clicked_on_empty_space(treeview, event):
    result = treeview.get_path_at_pos(int(event.x), int(event.y))
    if not result:
        selection = treeview.get_selection()
        selection.unselect_all()

def unselect_when_press_escape(treeview, event):
    if event.keyval == gtk.keysyms.Escape:
        selection = treeview.get_selection()
        selection.unselect_all()

def select_iter(treeview, item_iter):
    model = treeview.get_model()
    path = model.get_path(item_iter)
    if treeview.flags() & gtk.REALIZED:
        treeview.expand_to_path(path)
        treeview.scroll_to_cell(path)
    treeview.get_selection().select_path(path)

def get_button_state(button):
    """Get the state of the button in the form of a tuple with the following
    fields:
    - stock_id: string with the stock_id or None if the button is not using a
                stock_id
    - notext: boolean that says if the button has only a stock icon or if it
              also has the stock label
    - label: string with the contents of the button text or None
    - image_path: string with the path of a custom file for the image or None
    - position: one of gtk.POS_* that specifies the position of the image with
                respect to the label
    """
    stock_id = label = image_path = position = None
    notext = False

    use_stock = button.get_use_stock()
    child = button.get_child()

    # it is a stock button
    if use_stock:
        stock_id = button.get_label()

    # it only has a text label
    elif isinstance(child, gtk.Label):
        label = child.get_text()

    # it has an image without text. it can be stock icon or custom image
    elif isinstance(child, gtk.Image):
        stock_id = child.get_property('stock')
        if stock_id:
            notext = True
        else:
            image_path = child.get_data('image-file-name')

    # it has custom image and text
    elif isinstance(child, gtk.Alignment):
        box = child.get_child()
        if isinstance(box, gtk.HBox):
            position = gtk.POS_LEFT
        elif isinstance (box, gtk.Box):
            position = gtk.POS_TOP
        else:
          return (stock_id, notext, label, image_path, position)

        children = box.get_children()
        image_child = None
        text_child = None
        for c in children:
            if isinstance(c, gtk.Image):
                image_child = c
            elif isinstance(c, gtk.Label):
                text_child = c

        if image_child:
            stock_id = image_child.get_property('stock')
            image_path = image_child.get_data('image-file-name')

        if text_child:
            label = text_child.get_text()
        else:
            notext = True

    return (stock_id, notext, label, image_path, position)

def get_all_children(container):
    children = []
    def fetch_child(child, bag):
        bag.append(child)
    container.forall(fetch_child, children)
    return children
