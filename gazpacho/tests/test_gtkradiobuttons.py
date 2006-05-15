from gazpacho.unittest import common
from gazpacho.project import GazpachoObjectBuilder

import gtk

class GtkRadioButtonsTest(common.GazpachoTest):
    def setUp(self):
        common.GazpachoTest.setUp(self)

        # add a window
        self.window = self.create_gwidget('GtkWindow')
        self.window.get_prop('visible').set(False)
        self.project.add_widget(self.window.gtk_widget)

    def testGroupChange(self):
        "Test for bug #172373"
        vbox = self.add_child(self.window, 'GtkVBox',
                               self.window.gtk_widget.get_child())
        self.project.add_widget(vbox.gtk_widget)
        rb1 = self.add_child(vbox, 'GtkRadioButton',
                             vbox.gtk_widget.get_children()[0])
        self.project.add_widget(rb1.gtk_widget)
        rb2 = self.add_child(vbox, 'GtkRadioButton',
                             vbox.gtk_widget.get_children()[1])
        self.project.add_widget(rb2.gtk_widget)
        rb2.get_prop('group').set(rb1.gtk_widget)

        self.remove_child(vbox, rb1)
        self.project.remove_widget(rb1.gtk_widget)

        self.project.selection_set(rb2.gtk_widget, True)
