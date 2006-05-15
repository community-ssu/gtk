from gazpacho.unittest import common

class PropTypeTest(common.GazpachoTest):
    def setUp(self):
        common.GazpachoTest.setUp(self)

        # add a window
        self.window = self.create_gwidget('GtkWindow')
        self.window.get_prop('visible').set(False)
        self.project.add_widget(self.window.gtk_widget)

    def testReparentWidget(self):
        # Test to reproduce Bug 314111. When a widget is moved from
        # one container to another the parent widget in the property
        # isn't updated to reflect this.

        # Add a VBox
        vbox = self.add_child(self.window, 'GtkVBox',
                                     self.window.gtk_widget.get_child())
        self.project.add_widget(vbox.gtk_widget)

        # Add a HBox and a Label to the VBox
        children = vbox.gtk_widget.get_children()
        assert len(children) == 3
        p0, p1, p2 = children
        
        hbox = self.add_child(vbox, 'GtkHBox', p0)
        self.project.add_widget(hbox.gtk_widget)
        
        label = self.add_child(vbox, 'GtkLabel', p1)
        self.project.add_widget(label.gtk_widget)

        # Move the Label from the VBox to the HBox
        placeholder = hbox.gtk_widget.get_children()[0]
        self.app.command_manager.execute_drag_drop(label, placeholder)
    testReparentWidget.skip = "Fails on Bug 314111"

        
