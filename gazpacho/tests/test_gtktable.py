from gazpacho.unittest import common

import gtk

class GtkTableTest(common.GazpachoTest):

    def setUp(self):
        common.GazpachoTest.setUp(self)

        # add a window
        self.window = self.create_gwidget('GtkWindow')
        self.window.get_prop('visible').set(False)
        self.project.add_widget(self.window.gtk_widget)
        
    def testCreation(self):
        table = self.add_child(self.window, 'GtkTable',
                               self.window.gtk_widget.get_child())

        self.assertEqual(gtk.Table, type(table.gtk_widget))

    def testProperty(self):
        table = self.add_child(self.window, 'GtkTable',
                               self.window.gtk_widget.get_child())

        prop = table.get_prop('n-rows').set(10)
        self.assertEqual(table.gtk_widget.get_property('n-rows'), 10)

        prop = table.get_prop('n-columns').set(20)
        self.assertEqual(table.gtk_widget.get_property('n-columns'), 20)

    def testAddChildren(self):
        table = self.add_child(self.window, 'GtkTable',
                               self.window.gtk_widget.get_child())

        table.get_prop('n-rows').set(4)
        table.get_prop('n-columns').set(4)
        
        for placeholder in table.gtk_widget.get_children():
            button = self.add_child(table, 'GtkButton', placeholder)

        for child in table.gtk_widget.get_children():
            self.assertEqual(gtk.Button, type(child))
            
    def testSavingLoading(self):
        table = self.add_child(self.window, 'GtkTable',
                               self.window.gtk_widget.get_child())
        self.project.add_widget(table.gtk_widget)
        self.project.save(__file__+'.glade')

        self.app.open_project(__file__+'.glade')
        self.remove_file(__file__+'.glade')
