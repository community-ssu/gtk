from gazpacho.unittest import common
from gazpacho.project import GazpachoObjectBuilder

import gtk

class GtkWindowTest(common.GazpachoTest):
    def testCreation(self):
        gwidget = self.create_gwidget('GtkWindow')
        self.assertEqual(gtk.Window, type(gwidget.gtk_widget))

    def testProperty(self):
        gwidget = self.create_gwidget('GtkWindow')
        prop = gwidget.get_prop('title')
        prop.set('Window Title Test')
        self.assertEqual(gwidget.gtk_widget.get_title(), 'Window Title Test')

    def testSavingLoading(self):
        window = self.create_gwidget('GtkWindow')
        prop = window.get_prop('visible')
        prop.set(False)
        self.project.add_widget(window.gtk_widget)

        self.app.get_current_project().set_changed(True)
        self.project.save(__file__+'.glade')

        self.app.close_current_project()
        
        self.app.open_project(__file__+'.glade')
        project = self.app.get_current_project()

        gwidget = project.get_widget_by_name('window1')
        prop = window.get_prop('resizable')
        prop.set(not prop.value)
        filename = __file__+'-2.glade'
        project.save(filename)
        tree = GazpachoObjectBuilder(filename=filename)
        widgets = tree.get_widgets()
        self.assertEqual(len(widgets), 1)
        window = widgets[0]
        self.assertEqual(window.get_property('type'),
                         gtk.WINDOW_TOPLEVEL)
        self.assertEqual(window.get_property('resizable'),
                         not prop.value)
        self.remove_file(filename)
        self.remove_file(__file__+'.glade')
        
    def testOptionalDefault(self):
        window = self.create_gwidget('GtkWindow')
        prop = window.get_prop('default-width')
        self.assertEqual(prop.klass.optional, True)
        self.assertEqual(prop.klass.optional_default, 0)

        prop = window.get_prop('default-height')
        self.assertEqual(prop.klass.optional, True)
        self.assertEqual(prop.klass.optional_default, 0)
    testOptionalDefault.skip = "Optional default not implemented"
