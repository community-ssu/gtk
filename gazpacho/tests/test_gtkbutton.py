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

from gazpacho.filewriter import XMLWriter
from gazpacho.project import GazpachoObjectBuilder
from gazpacho.unittest import common

import gtk

class GtkButtonTest(common.GazpachoTest):

    def setUp(self):
        common.GazpachoTest.setUp(self)

        # add a window
        self.window = self.create_gwidget('GtkWindow')
        self.window.get_prop('visible').set(False)
        self.project.add_widget(self.window.gtk_widget)
        
    def testCreation(self):
        button = self.add_child(self.window, 'GtkButton',
                                self.window.gtk_widget.get_child())
        
        self.assertEqual(gtk.Button, type(button.gtk_widget))

    def testProperty(self):
        button = self.add_child(self.window, 'GtkButton',
                                self.window.gtk_widget.get_child())

        prop = button.get_prop('relief')
        prop.set(gtk.RELIEF_HALF)
        self.assertEqual(button.gtk_widget.get_relief(), gtk.RELIEF_HALF)
    
    def testSavingLoading(self):
        button = self.add_child(self.window, 'GtkButton',
                                self.window.gtk_widget.get_child())
        self.project.add_widget(button.gtk_widget)
        self.project.save(__file__+'.glade')

        self.app.open_project(__file__+'.glade')
        self.remove_file(__file__+'.glade')

class ButtonSave(common.GazpachoTest):

    def setUp(self):
        common.GazpachoTest.setUp(self)

        # add a window
        self.window = self.create_gwidget('GtkWindow')
        self.window.get_prop('visible').set(False)
        self.project.add_widget(self.window.gtk_widget)
        self.button = self.add_child(self.window, 'GtkButton',
                                     self.window.gtk_widget.get_child())

    def serialize(self):
        xw = XMLWriter(project=self.project)
        node = xw.serialize_node(self.button)
        self.properties = node.getElementsByTagName('property')
        self.project.save(__file__+'.glade')
        ob = GazpachoObjectBuilder(filename=__file__+'.glade')
        self.remove_file(__file__+'.glade')

        self.gbutton = ob.get_widget('button1')

    def testSaveLabel(self):
        prop = self.button.get_prop('label')
        prop.set('Simple')
        self.serialize()
        self.assertEqual(self.gbutton.get_label(), 'Simple')

    def testSaveLabelStock(self):
        self.button.get_prop('use-stock').set(True)
        self.button.get_prop('label').set('gtk-new')
        self.serialize()
        self.assertEqual(self.gbutton.get_label(), 'gtk-new')
        self.assertEqual(self.gbutton.get_use_stock(), True)

    def testSaveStock(self):
        self.button.get_prop('use-stock').set(True)
        self.serialize()
        self.assertEqual(self.gbutton.get_use_stock(), True)

class ButtonSave2(common.GazpachoTest):

    def testAdvanced(self):
        glade_data = '''<widget class="GtkButton" id="button1">
    <property name="receives_default">True</property>
    <property name="events"></property>
    <property name="can_focus">True</property>
    <child>
        <widget class="GtkAlignment" id="button1-alignment">
            <property name="visible">True</property>
            <property name="xalign">0.5</property>
            <property name="yalign">0.5</property>
            <property name="xscale">1.0</property>
            <property name="yscale">1.0</property>
            <child>
                <widget class="GtkHBox" id="button1-box">
                    <property name="visible">True</property>
                    <child>
                        <widget class="GtkImage" id="button1-image">
                            <property name="visible">True</property>
                            <property name="file">image.png</property>
                        </widget>
                    </child>
                    <child>
                        <widget class="GtkLabel" id="button1-label">
                            <property name="visible">True</property>
                            <property name="label">Details</property>
                        </widget>
                        <packing>
                            <property name="position">1</property>
                        </packing>
                    </child>
                </widget>
            </child>
        </widget>
    </child>
</widget>'''
        
        # XXX: crack api, fix, open_buffer iss probably better.
        project = self.project.open('', self.app, buffer=glade_data)
        self.failUnless(project.widgets != [], 'no loaded widgets')
        button = project.get_widget_by_name('button1')
        self.failUnless(button != None, 'missing button')
        gbutton = button.gtk_widget
        self.assertEqual(isinstance(gbutton, gtk.Button), True)
        self.assertEqual(len(gbutton.get_children()), 1,
                         'missing child for button')
        child = gbutton.get_children()[0]
        self.assertEqual(isinstance(child, gtk.Alignment), True)
        
        xw = XMLWriter()
        node = xw.serialize_node(button)
        xw.write_node(__file__+'.glade', node)
        ob = GazpachoObjectBuilder(filename=__file__+'.glade')
        button = ob.get_widget('button1')
        self.failUnless(button != None, 'button not saved')
        self.assertEqual(isinstance(button, gtk.Button), True)
        self.assertEqual(len(button.get_children()), 1,
                         'alignment not saved')
        child = button.get_children()[0]
        self.failUnless(button != None, 'alignment not saved')
        self.assertEqual(isinstance(child, gtk.Alignment), True)

class CreateSave(common.GazpachoTest):
    def setUp(self):
        common.GazpachoTest.setUp(self)
        
        window = self.create_gwidget('GtkWindow')
        window.get_prop('visible').set(False)
        self.project.add_widget(window.gtk_widget)

        self.button = self.add_child(window, 'GtkButton',
                                     window.gtk_widget.get_child())
        self.failUnless(self.button != None)
        
        self.mgr = self.app.get_command_manager()

    def save_and_load(self):
        xw = XMLWriter(project=self.project)
        node = xw.serialize_node(self.button)
        xw.write_node(__file__+'.glade', node)
        ob = GazpachoObjectBuilder(filename=__file__+'.glade')
        button = ob.get_widget('button1')
        self.remove_file(__file__+'.glade')

        self.failUnless(button != None, 'button not saved')
        self.assertEqual(isinstance(button, gtk.Button), True)
        return button
        
    def testStockAndText(self):
        self.mgr.set_button_contents(self.button, stock_id=gtk.STOCK_NEW,
                                     notext=False)
        gbutton = self.button.gtk_widget
        self.assertEqual(self.button.get_prop('use-stock').value,
                         True, 'use-stock not property set')
        self.assertEqual(isinstance(gbutton, gtk.Button), True)
        self.assertEqual(gbutton.get_use_stock(), True)
        self.assertEqual(len(gbutton.get_children()), 1,
                         'alignment not set')

        button = self.save_and_load()
        
        self.assertEqual(button.get_label(), gtk.STOCK_NEW,
                         'label is not saved')
        self.assertEqual(button.get_use_stock(), True,
                         'stock is not saved')

    def testStockNoText(self):
        self.mgr.set_button_contents(self.button, stock_id=gtk.STOCK_NEW,
                                     notext=True)
        gbutton = self.button.gtk_widget
        self.assertEqual(isinstance(gbutton, gtk.Button), True)
        self.assertEqual(len(gbutton.get_children()), 1,
                         'image not set')
        child = gbutton.get_child()
        self.failUnless(child != None)
        self.assertEqual(isinstance(child, gtk.Image), True)

        button = self.save_and_load()
        self.assertEqual(isinstance(button.get_child(), gtk.Image), True)
        
    def testJustLabel(self):
        self.mgr.set_button_contents(self.button, label='Test')
        gbutton = self.button.gtk_widget
        self.assertEqual(isinstance(gbutton, gtk.Button), True)
        self.assertEqual(len(gbutton.get_children()), 1, 'label not set')
        child = gbutton.get_child()
        self.failUnless(child != None)
        self.assertEqual(isinstance(child, gtk.Label), True)
        self.assertEqual(child.get_text(), 'Test')
        self.assertEqual(gbutton.get_label(), 'Test')

        button = self.save_and_load()
        self.assertEqual(isinstance(button.get_child(), gtk.Label), True)
        self.assertEqual(button.get_label(), 'Test')

    def testJustImage(self):
        self.mgr.set_button_contents(self.button, image_path='/path/to/image')
        gbutton = self.button.gtk_widget
        self.assertEqual(isinstance(gbutton, gtk.Button), True)
        self.assertEqual(len(gbutton.get_children()), 1, 'image not set')
        child = gbutton.get_child()
        self.failUnless(child != None)
        self.assertEqual(isinstance(child, gtk.Image), True)

        button = self.save_and_load()
        self.assertEqual(isinstance(button.get_child(), gtk.Image), True)

    def testLabelAndImage(self):
        self.mgr.set_button_contents(self.button, label='Test',
                                     image_path='/path/to/image')
        gbutton = self.button.gtk_widget
        self.assertEqual(isinstance(gbutton, gtk.Button), True)
        self.assertEqual(len(gbutton.get_children()), 1)
        align = gbutton.get_child()
        self.failUnless(align != None)
        self.assertEqual(isinstance(align, gtk.Alignment), True)
        box = align.get_child()
        self.failUnless(box != None)
        self.assertEqual(isinstance(box, gtk.VBox), True)
        label, image = box.get_children()
        self.assertEqual(isinstance(image, gtk.Image), True)
        self.assertEqual(isinstance(label, gtk.Label), True)
        self.assertEqual(label.get_text(), 'Test')

        button = self.save_and_load()
        self.assertEqual(isinstance(button.get_child(), gtk.Alignment), True)
        align = button.get_child()
        self.failUnless(align != None)
        self.assertEqual(isinstance(align, gtk.Alignment), True)
        box = align.get_child()
        self.failUnless(box != None)
        self.assertEqual(isinstance(box, gtk.VBox), True)
        label, image = box.get_children()
        self.assertEqual(isinstance(image, gtk.Image), True)
        self.assertEqual(isinstance(label, gtk.Label), True)
        self.assertEqual(label.get_text(), 'Test')
