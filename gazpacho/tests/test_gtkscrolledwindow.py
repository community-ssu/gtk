from gazpacho.unittest import common

import gtk

class GtkScrolledWindowTest(common.GazpachoTest):

    def setUp(self):
        common.GazpachoTest.setUp(self)

        # add a window
        self.window = self.create_gwidget('GtkWindow')
        self.window.get_prop('visible').set(False)
        self.project.add_widget(self.window.gtk_widget)


    def testUndoRemoveViewport(self):
        """To reproduce this bug:

        1 - Create scrolled window
        2 - Remove the viewport
        3 - Undo
        """
        sw = self.add_child(self.window, 'GtkScrolledWindow',
                            self.window.gtk_widget.get_child())
        self.project.add_widget(sw.gtk_widget)
        
        # when creating a scrolled window a viewport is automatically
        # created for us
        viewport = self.project.get_widget_by_name('viewport1')
        self.assertEqual(isinstance(viewport.gtk_widget, gtk.Viewport), True)
        self.assertEqual(sw.gtk_widget.child, viewport.gtk_widget)

        cm = self.app.get_command_manager()
        
        # delete the viewport
        cm.delete(viewport)

        cm.undo(self.project)

        self.assertEqual(sw.gtk_widget.child, viewport.gtk_widget)
