import os

from twisted.trial import unittest

from gazpacho import application, placeholder
from gazpacho.widget import Widget
from gazpacho.widgetregistry import widget_registry
from gazpacho.catalog import load_catalogs

load_catalogs(external=False)

app = application.Application()

class GazpachoTest(unittest.TestCase):

    def setUp(self):
        global app
        self.app = app
        app.new_project()
        self.project = app.get_current_project()
        
    def tearDown(self):
        self.app.close_current_project()
        
    def create_gwidget(self, class_name):
        """Convenient method for creating widgets"""
        adaptor = widget_registry.get_by_name(class_name)
        gwidget = Widget(adaptor, self.project)
        gwidget.create_gtk_widget(interactive=False)
        return gwidget

    def add_child(self, gparent, child_class_name, placeholder):
        """Create a widget of class 'child_class_name' and add it to the
        parent in the placeholder specified.
        """
        gchild = self.create_gwidget(child_class_name)
        Widget.replace(placeholder, gchild.gtk_widget, gparent)
        return gchild

    def remove_child(self, gparent, gchild):
        """Remove gchild from gparent and put a placeholder in that place"""
        ph = placeholder.Placeholder(self.app)
        Widget.replace(gchild.gtk_widget, ph, gparent)
        #gparent.set_default_gtk_packing_properties(ph, gchild.adaptor)

    def remove_file(self, filename):
        if os.path.exists(filename):
            os.remove(filename)
