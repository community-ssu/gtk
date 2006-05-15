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

from gazpacho.unittest import common

class GtkBoxTest(common.GazpachoTest):
    def setUp(self):
        common.GazpachoTest.setUp(self)

        # add a window
        self.window = self.create_gwidget('GtkWindow')
        self.window.get_prop('visible').set(False)
        self.project.add_widget(self.window.gtk_widget)

    def testExpand(self):
        """Test for bug #313439

        Steps to reproduce:

          1. Create a Window
          2. Create a VBox
          3. Create another VBox inside the first vbox
          4. Select the window, then the inner vbox, then the outter box
        """

        parent_vbox = self.add_child(self.window, 'GtkVBox',
                                     self.window.gtk_widget.get_child())
        self.project.add_widget(parent_vbox.gtk_widget)

        placeholder = parent_vbox.gtk_widget.get_children()[0]
        child_vbox = self.add_child(parent_vbox, 'GtkVBox',
                                    placeholder)
        self.project.add_widget(child_vbox.gtk_widget)

        self.project.selection_set(self.window.gtk_widget, True)
        self.project.selection_set(child_vbox.gtk_widget, True)
        self.project.selection_set(parent_vbox.gtk_widget, True)
