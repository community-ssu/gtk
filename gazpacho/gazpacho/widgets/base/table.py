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

import gettext

import gtk

from gazpacho.placeholder import Placeholder
from gazpacho.properties import prop_registry, TransparentProperty, IntType
from gazpacho.widget import Widget
from gazpacho.widgets.base.base import ContainerAdaptor

_ = gettext.gettext

def glade_gtk_table_set_n_common(app, table, value, for_rows):
    new_size = value
    p = for_rows and 'n-rows' or 'n-columns'
    old_size = table.get_property(p)
    if new_size == old_size:
        return

    if new_size < 1:
        return

    gwidget = Widget.from_widget(table)
    if new_size > old_size:
        if for_rows:
            table.resize(new_size, table.get_property('n-columns'))
            for i in range(table.get_property('n-columns')):
                for j in range(old_size, table.get_property('n-rows')):
                    table.attach(Placeholder(app), i, i+1, j, j+1)
        else:
            table.resize(table.get_property('n-rows'), new_size)
            for i in range(old_size, table.get_property('n-columns')):
                for j in range(table.get_property('n-rows')):
                    table.attach(Placeholder(app), i, i+1, j, j+1)
    else:
        # Remove from the bottom up
        l = table.get_children()
        l.reverse()
        for child in l:
            p = for_rows and 'top-attach' or 'left-attach'
            start = table.child_get_property(child, p)
            p = for_rows and 'bottom-attach' or 'right-attach'
            end = table.child_get_property(child, p)

            # We need to completely remove it
            if start >= new_size:
                table.remove(child)
                continue
            # If the widget spans beyond the new border, we should resize it to
            # fit on the new table
            if end > new_size:
                p = for_rows and 'bottom-attach' or 'right-attach'
                table.child_set_property(child, p, new_size)
        table.resize(for_rows and new_size or table.get_property('n-rows'),
                     for_rows and table.get_property('n-columns') or new_size)

        p = for_rows and 'n-columns' or 'n-rows'
                       

class TableNRowsProp(TransparentProperty, IntType):
    default = 3
    def set(self, value):
        app = self._project.get_app()
        glade_gtk_table_set_n_common(app, self._object, value, True)
        self._value = value
        
    def get(self):
        return self._value
prop_registry.override_property('GtkTable::n-rows', TableNRowsProp)

class TableNColumnsProp(TransparentProperty, IntType):
    default = 3
    def set(self, value):
        app = self._project.get_app()
        glade_gtk_table_set_n_common(app, self._object, value, False)
        self._value = value
        
    def get(self):
        return self._value
prop_registry.override_property('GtkTable::n-columns', TableNColumnsProp)

class BaseAttach:
    """Base class for LeftAttach, RightAttach, TopAttach and BottomAttach
    adaptors"""
    def _get_attach(self, child):
        """Returns the four attach packing properties in a tuple"""
        right = self.table.child_get_property(child, 'right-attach')
        left = self.table.child_get_property(child, 'left-attach')
        top = self.table.child_get_property(child, 'top-attach')
        bottom = self.table.child_get_property(child, 'bottom-attach')
        return (left, right, top, bottom)

    def _cell_empty(self, x, y):
        """Returns true if the cell at x, y is empty. Exclude child from the
        list of widgets to check"""
        empty = True
        for w in self.table.get_children():
            left, right, top, bottom = self._get_attach(w)
            if (left <= x and (x + 1) <= right 
                and top <= y and (y + 1) <= bottom):
                empty = False
                break

        return empty

    def _create_placeholder(self, context, x, y):
        """Puts a placeholder at cell (x, y)"""
        placeholder = context.create_placeholder()
        self.table.attach(placeholder, x, x+1, y, y+1)

    def _fill_with_placeholders(self, context, y_range, x_range):
        """Walk through the table creating placeholders in empty cells.
        Only iterate between x_range and y_range.
        Child is excluded in the computation to see if a cell is empty
        """
        for y in range(self.n_rows):
            for x in range(self.n_columns):
                if self._cell_empty(x, y):
                    self._create_placeholder(context, x, y)

    def _initialize(self, child, prop_name):
        """Common things all these adaptors need to do at the beginning"""
        self.table = child.get_parent()
        (self.left_attach,
         self.right_attach,
         self.top_attach,
         self.bottom_attach) = self._get_attach(child)

        self.n_columns = self.table.get_property('n-columns')
        self.n_rows = self.table.get_property('n-rows')

        self.prop_name = prop_name
        self.child = child
        self.gchild = Widget.from_widget(self.child)
        self.gprop = self.gchild.get_prop(self.prop_name)

    def _value_is_between(self, value, minimum, maximum):
        if value < minimum:
            self.gprop._value = minimum
            return False

        if value > maximum:
            self.gprop._value = maximum
            return False

        return True

    def _internal_set(self, context, value, minimum, maximum,
                      y_range, x_range):
        """Check if value is between minium and maximum and then remove or
        add placeholders depending if we are growing or shrinking.
        If we are shrinking check the cells in y_range, x_range to add
        placeholders
        """
        if not self._value_is_between(value, minimum, maximum):
            return

        placeholder = context.create_placeholder()

        # are we growing?
        if self._is_growing(value):
            # check if we need to remove some placeholder
            for ph in filter(lambda w: isinstance(w, type(placeholder)),
                             self.table.get_children()):
                lph, rph, tph, bph = self._get_attach(ph)
                if self._cover_placeholder(value, lph, rph, tph, bph):
                    self.table.remove(ph)

            self.table.child_set_property(self.child, self.prop_name, value)

        # are we shrinking? maybe we need to create placeholders
        elif self._is_shrinking(value):
            self.table.child_set_property(self.child, self.prop_name, value)
            self._fill_with_placeholders(context, y_range, x_range)


    # virtual methods that should be implemented by subclasses:
    def _is_growing(self, value):
        """Returns true if the child widget is growing"""

    def _is_shrinking(self, value):
        """Returns true if the child widget is shrinking"""

    def _cover_placeholder(self, value, left, right, top, bottom):
        """Return True if there is a placeholder in these coordinates"""
        
class LeftAttachAdaptor(BaseAttach):
    def _is_growing(self, value):
        return value < self.left_attach

    def _is_shrinking(self, value):
        return value > self.left_attach

    def _cover_placeholder(self, value, left, right, top, bottom):
        if value < right and self.left_attach > left:
            if top >= self.top_attach and bottom <= self.bottom_attach:
                return True
        return False

    def set(self, context, child, value):
        self._initialize(child, 'left-attach')
        self._internal_set(context, value, 0, self.right_attach - 1,
                           range(self.n_rows),
                           range(self.left_attach, value))

class RightAttachAdaptor(BaseAttach):
    def _is_growing(self, value):
        return value > self.right_attach

    def _is_shrinking(self, value):
        return value < self.right_attach

    def _cover_placeholder(self, value, left, right, top, bottom):
        if value > left and self.right_attach < right:
            if top >= self.top_attach and bottom <= self.bottom_attach:
                return True
        return False

    def set(self, context, child, value):
        self._initialize(child, 'right-attach')
        self._internal_set(context, value,
                           self.left_attach + 1, self.n_columns,
                           range(value, self.right_attach),
                           range(self.n_rows))

class BottomAttachAdaptor(BaseAttach):
    def _is_growing(self, value):
        return value > self.bottom_attach

    def _is_shrinking(self, value):
        return value < self.bottom_attach

    def _cover_placeholder(self, value, left, right, top, bottom):
        if value > top and self.bottom_attach < bottom:
            if left >= self.left_attach and right <= self.right_attach:
                return True                
        return False

    def set(self, context, child, value):
        self._initialize(child, 'bottom-attach')
        self._internal_set(context, value,
                           self.top_attach + 1, self.n_rows,
                           range(value, self.bottom_attach),
                           range(self.n_columns))

class TopAttachAdaptor(BaseAttach):
    def _is_growing(self, value):
        return value < self.top_attach

    def _is_shrinking(self, value):
        return value > self.top_attach

    def _cover_placeholder(self, value, left, right, top, bottom):
        if value < bottom and self.top_attach > top:
            if left >= self.left_attach and right <= self.right_attach:
                return True
        return False

    def set(self, context, child, value):
        self._initialize(child, 'top-attach')
        self._internal_set(context, value, 0, self.bottom_attach - 1,
                           range(self.n_columns),
                           range(self.top_attach, value))

class TableAdaptor(ContainerAdaptor):

    def pre_create(self, context, table, interactive=True):
        """ a GtkTable starts with a default size of 1x1, and setter/getter of
        rows/columns expect the GtkTable to hold this number of placeholders,
        so we should add it. """
        table.attach(context.create_placeholder(), 0, 1, 0, 1)

    def post_create(self, context, table, interactive=True):
        if not interactive:
            return
        gwidget = Widget.from_widget(table)
        property_rows = gwidget.get_prop('n-rows')
        property_cols = gwidget.get_prop('n-columns')
        dialog = gtk.Dialog(_('Create a table'), None,
                            gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT | \
                            gtk.DIALOG_NO_SEPARATOR,
                            (gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))
        dialog.set_position(gtk.WIN_POS_MOUSE)
        dialog.set_default_response(gtk.RESPONSE_ACCEPT)
        
        label_rows = gtk.Label(_('Number of rows')+':')
        label_rows.set_alignment(0.0, 0.5)
        label_cols = gtk.Label(_('Number of columns')+':')
        label_cols.set_alignment(0.0, 0.5)
        
        spin_button_rows = gtk.SpinButton()
        spin_button_rows.set_increments(1, 5)
        spin_button_rows.set_range(1.0, 10000.0)
        spin_button_rows.set_numeric(False)
        spin_button_rows.set_value(3)
        spin_button_rows.set_property('activates-default', True)
        
        spin_button_cols = gtk.SpinButton()
        spin_button_cols.set_increments(1, 5)
        spin_button_cols.set_range(1.0, 10000.0)
        spin_button_cols.set_numeric(False)
        spin_button_cols.set_value(3)
        spin_button_cols.set_property('activates-default', True)
        
        table = gtk.Table(2, 2, True)
        table.set_col_spacings(4)
        table.set_border_width(12)
        
        table.attach(label_rows, 0, 1, 0, 1)
        table.attach(spin_button_rows, 1, 2, 0, 1)
        
        table.attach(label_cols, 0, 1, 1, 2)
        table.attach(spin_button_cols, 1, 2, 1, 2)
        
        dialog.vbox.pack_start(table)
        table.show_all()
        
        # even if the user destroys the dialog box, we retrieve the number and
        # we accept it.  I.e., this function never fails
        dialog.run()
        
        property_rows.set(spin_button_rows.get_value_as_int())
        property_cols.set(spin_button_cols.get_value_as_int())
        
        dialog.destroy()

prop_registry.override_simple('GtkTable::row-spacing',
                              minimum=0, maximum=10000)
prop_registry.override_simple('GtkTable::column-spacing',
                              minimum=0, maximum=10000)
#<child-properties>
#  <property id="left-attach" adaptor-class="LeftAttachAdaptor">
#      <parameter key="Min" value="0"/>
#      <parameter key="StepIncrement" value="1"/>
#      <parameter key="PageIncrement" value="1"/>
#    <function name="set" type="override"/>
#  <property id="right-attach" adaptor-class="RightAttachAdaptor">
#      <parameter key="Min" value="0"/>
#      <parameter key="StepIncrement" value="1"/>
#      <parameter key="PageIncrement" value="1"/>
#    <function name="set" type="override"/>
#  <property id="bottom-attach" adaptor-class="BottomAttachAdaptor">
#      <parameter key="Min" value="0"/>
#      <parameter key="StepIncrement" value="1"/>
#      <parameter key="PageIncrement" value="1"/>
#    <function name="set" type="override"/>
#  <property id="top-attach" adaptor-class="TopAttachAdaptor">
#      <parameter key="Min" value="0"/>
#      <parameter key="StepIncrement" value="1"/>
#      <parameter key="PageIncrement" value="1"/>
#    <function name="set" type="override"/>

