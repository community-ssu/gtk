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

        self.window = self.create_gwidget('GtkWindow')
        self.window.get_prop('visible').set(False)
        self.project.add_widget(self.window.gtk_widget)

    def testSetLower(self):
        spinbutton = self.add_child(self.window, 'GtkSpinButton',
                                     self.window.gtk_widget.get_child())
        self.project.add_widget(spinbutton.gtk_widget)
        prop = spinbutton.get_prop('lower')
        prop.set(10)
        self.assertEqual(prop.get(), 10)
