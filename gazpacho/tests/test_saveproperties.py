# Authors: Mikael Hallendal <micke@imendio.com>
#          Lorenzo Gil Sanchez <lgs@sicem.biz>

from gazpacho.unittest import common
from gazpacho.filewriter import XMLWriter

import gtk

class SavePropertiesTest(common.GazpachoTest):

    def setUp(self):
        common.GazpachoTest.setUp(self)

        # add a window
        self.window = self.create_gwidget('GtkWindow')
        self.window.get_prop('visible').set(False)
        self.project.add_widget(self.window.gtk_widget)

    def testSaveProperties(self):
        """When the draw-value property is set to False this is not
        saved in the glade file. See bug #304539"""
        
        scale = self.add_child(self.window, 'GtkVScale',
                              self.window.gtk_widget.get_child())

        scale.name = 'my-scale'
        
        scale.get_prop('draw-value').set(False)
        self.assertEqual(scale.gtk_widget.get_property('draw-value'), False)

        self.project.save(__file__+'.glade')
      
        new_project = self.project.open(__file__+'.glade', self.app)
        gwidget = new_project.get_widget_by_name('my-scale')
        self.assertEqual(gwidget.gtk_widget.get_property('draw-value'), False)
        self.remove_file(__file__+'.glade')

    def testSavePositionProperty(self):
        "The position packing property is not being saved. See bug #313087"
        vbox = self.add_child(self.window, 'GtkVBox',
                              self.window.gtk_widget.get_child())
        self.project.add_widget(vbox.gtk_widget)
        for i in range(3):
            label = self.add_child(vbox, 'GtkLabel',
                                   vbox.gtk_widget.get_children()[i])
            self.project.add_widget(label.gtk_widget)

        for i in range(3):
            child = vbox.gtk_widget.get_children()[i]
            self.assertEqual(type(child), gtk.Label)
            pos = vbox.gtk_widget.child_get_property(child, 'position')
            self.assertEqual(pos, i)

        xml_string = """<glade-interface>
    <widget class="GtkVBox" id="vbox1">
        <child>
            <widget class="GtkLabel" id="label1"/>
        </child>
        <child>
            <widget class="GtkLabel" id="label2"/>
            <packing>
                <property name="position">1</property>
            </packing>
        </child>
        <child>
            <widget class="GtkLabel" id="label3"/>
            <packing>
                <property name="position">2</property>
            </packing>
        </child>
    </widget>
</glade-interface>
"""
        xw = XMLWriter()
        self.assertEqual(xw.serialize(vbox), xml_string)
