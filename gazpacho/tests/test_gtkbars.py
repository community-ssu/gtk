from gazpacho.unittest import common
from gazpacho.widget import Widget

class GtkBarsTest(common.GazpachoTest):

    def setUp(self):
        common.GazpachoTest.setUp(self)

        # add a window
        self.window = self.create_gwidget('GtkWindow')
        self.window.get_prop('visible').set(False)
        self.project.add_widget(self.window.gtk_widget)


    def testPastingTwoBars(self):
        """There is a bug that appears when:

        1 - Create menubar1
        2 - Create menubar2
        3 - Copy menubar1
        4 - Delete menubar1
        5 - Paste copy of menubar1
        6 - Paste it again
        """
        vbox = self.add_child(self.window, 'GtkVBox',
                              self.window.gtk_widget.get_child())
        self.project.add_widget(vbox.gtk_widget)
        vbox.get_prop('size').set(4)
        
        children = vbox.gtk_widget.get_children()
        self.assertEqual(len(children), 4)
        ph1, ph2, ph3, ph4 = children
        
        # create menubar1
        menubar1 = self.add_child(vbox, 'GtkMenuBar', ph1)
        self.project.add_widget(menubar1.gtk_widget)

        # create menubar2
        menubar2 = self.add_child(vbox, 'GtkMenuBar', ph2)
        self.project.add_widget(menubar2.gtk_widget)

        # copy menubar1
        clipboard = self.app.get_clipboard()
        clipboard.add_widget(menubar1)

        # delete menubar1
        self.remove_child(vbox, menubar1)
        self.project.remove_widget(menubar1.gtk_widget)

        # paste copy of menubar1
        menubar1_copy1 = clipboard.get_selected_widget(self.project)
        Widget.replace(ph3, menubar1_copy1.gtk_widget, vbox)
        self.project.add_widget(menubar1_copy1.gtk_widget)

        self.assertNotEqual(menubar1_copy1, menubar1)

        # paste it again

        # this line was the one that used to break
        menubar1_copy2 = clipboard.get_selected_widget(self.project)
        Widget.replace(ph4, menubar1_copy2.gtk_widget, vbox)
        self.project.add_widget(menubar1_copy2.gtk_widget)

        self.assertNotEqual(menubar1_copy2, menubar1)
        self.assertNotEqual(menubar1_copy1, menubar1_copy2)
