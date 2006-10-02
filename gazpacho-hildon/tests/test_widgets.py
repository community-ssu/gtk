# Basic tests for the hildon widgets
# It's based on gazpacho/tests/test_widgets.py but it's rewritten since
# gazpacho tests use Twisted trial framework and Trial can not be installed
# on maemo due to the stripped Python version that is included in the platform

import os
import unittest

import gobject
import gtk

from gazpacho import gapi
from gazpacho.clipboard import clipboard
from gazpacho.commandmanager import command_manager
from gazpacho.properties import prop_registry
from gazpacho.widgetregistry import widget_registry

from common import HildonTest, diff_files

class BaseWidgetTest(HildonTest):

    def setUp(self):
        super(BaseWidgetTest, self).setUp()
        self._tempfiles = {}

    def tearDown(self):
        super(BaseWidgetTest, self).tearDown()
        for filename, remove in self._tempfiles.iteritems():
            if remove and os.path.exists(filename):
                os.unlink(filename)

    def keep_file(self, filename):
        """
        Do not delete the file after the test is finished.

        @param filename: the name of the file to keep
        @type filename: str
        """
        if self._tempfiles.has_key(filename):
            self._tempfiles[filename] = False

    def save_project(self, project, name):
        """
        Save the project to a file based on the specified project
        name. The file will be removed after the test is finished
        unless the method keep_file(filename) is called.

        @param project: the project that should be saved
        @type project: gazpacho.project.Project
        @param name: the name of the project, will be used as a base for the filename
        @type name: str
        @return: the actual file name of the save project
        @rtype: str
        """
        filename = "%s.tmp" % name
        self._tempfiles[filename] = True

        if os.path.exists(filename):
            os.unlink(filename)
        project.save(filename)

        return filename

    def _get_properties(self, widget):
        """
        Get all properties for this widget.

        @param widget: the widget
        @type widget: gazpacho.gadget.Gadget
        @return: dict mapping property names their values
        @rtype: dict
        """
        # XXX is this the best way?
        widget_type = widget.adaptor.type_name
        prop_names = [prop.name for prop in prop_registry.list(widget_type)]
        prop_values = {}
        for name in prop_names:
            prop_values[name] = widget.get_prop(name).value
        return prop_values

    def _assert_properties(self, test_widget, ref_properties):
        """
        Assert that the properties in the test widget are the same as
        the reference properties.

        @param test_widget: the widget who's properties we should test
        @type test_widget: gazpacho.gadget.Gadget
        @param ref_properties: the reference properties. This is a
        mapping of property name to value
        @type ref_properties: dict
        """
        test_properties = self._get_properties(test_widget)

        # They should contain the same number of items
        ref_size, test_size = len(ref_properties), len(test_properties)
        error = ("Number of properties differ: %s != %s"
                 % (ref_size, test_size))
        assert ref_size == test_size, error

        # The values should be the same
        for key, value in ref_properties.iteritems():
            error = "Can't find property '%s'" % key
            assert test_properties.has_key(key), error

            if isinstance(value, gobject.GObject):
                # XXX don't know how to handle this
                continue

            new_value = test_properties[key]
            error = "property %s should be %r but is %r'" % (
                key, value, new_value)
            assert new_value == value, error

    def _assert_gladefiles(self, test_name, ref_file):
        """
        Test that the glade file produces when saving the current
        project is the same as the reference file.

        @param test_name: the name of the test file that will be produced
        @type test_name: str
        @param ref_file: the name of the reference file
        @type ref_file: str
        """
        test_file = self.save_project(self.project, "test_%s" % test_name)
        diff = diff_files(ref_file, test_file)
        if diff:
            self.keep_file(test_file)
            self.keep_file(ref_file)
            error = "Glade files '%s' and '%s' differ" % (ref_file, test_file)
            self.fail(error)

    def _test_toplevel(self, adaptor):
        mgr = command_manager
        type_name = adaptor.type_name

        # Create widget
        toplevel = gapi.create_gadget(self.project, adaptor, interactive=False)
        toplevel.get_prop('visible').set(False)

        widget_name = toplevel.name
        ref_properties = self._get_properties(toplevel)
        ref_file = self.save_project(self.project, "orig_%s" % type_name)

        # Cut 'n' paste
        clipboard.cut(toplevel)
        clipboard.paste(None, self.project)
        test_widget = self.project.get_gadget_by_name(widget_name)
        self._assert_properties(test_widget, ref_properties)
        self._assert_gladefiles(type_name, ref_file)

        # Undo cut 'n' paste
        mgr.undo(self.project)
        mgr.undo(self.project)
        test_widget = self.project.get_gadget_by_name(widget_name)
        self._assert_properties(test_widget, ref_properties)
        self._assert_gladefiles(type_name, ref_file)

        # Redo cut 'n' paste
        mgr.redo(self.project)
        mgr.redo(self.project)
        test_widget = self.project.get_gadget_by_name(widget_name)
        self._assert_properties(test_widget, ref_properties)
        self._assert_gladefiles(type_name, ref_file)

    def _test_widget(self, adaptor):
        mgr = command_manager
        test_name = adaptor.type_name

        # Create a window
        window = self.create_gadget('GtkWindow')
        window.get_prop('visible').set(False)

        self.project.add_widget(window.widget)

        # Add the widget
        placeholder = window.widget.get_child()
        assert placeholder, "Should have found a placeholder"

        widget = gapi.create_gadget(self.project, adaptor, placeholder, window,
                                    interactive=False)
        widget.get_prop('visible').set(False)
        # Hack of the year
        widget.widget.set_data('gazpacho::visible', True)
        assert widget.get_prop('visible').value == True

        widget_name = widget.name
        ref_properties = self._get_properties(widget)

        ref_file = self.save_project(self.project, "orig_%s" % test_name)

        # Cut 'n' paste
        clipboard.cut(widget)
        placeholder = window.widget.get_child()
        assert placeholder, "Should have found a placeholder"

        clipboard.paste(placeholder, self.project)
        test_widget = self.project.get_gadget_by_name(widget_name)
        self._assert_properties(test_widget, ref_properties)
        self._assert_gladefiles(test_name, ref_file)

        # Undo cut 'n' paste
        mgr.undo(self.project)
        mgr.undo(self.project)
        test_widget = self.project.get_gadget_by_name(widget_name)
        self._assert_properties(test_widget, ref_properties)
        self._assert_gladefiles(test_name, ref_file)

        # Redo cut 'n' paste
        mgr.redo(self.project)
        mgr.redo(self.project)
        test_widget = self.project.get_gadget_by_name(widget_name)
        self._assert_properties(test_widget, ref_properties)
        self._assert_gladefiles(test_name, ref_file)


def suite():
    ABSTRACT = ['HildonVolumebar']
    namespace = {}

    # XXX need a better way of getting the widgets we should test
    for adaptor in widget_registry._widget_adaptors.itervalues():
        testname = adaptor.type_name

        if testname in ABSTRACT:
            continue

        # we only test the Hildon widgets
        if not testname.startswith('Hildon'):
            continue

        if not gobject.type_is_a(adaptor.type, gtk.Widget):
            continue

        if adaptor.is_toplevel():
            func = lambda self, a=adaptor: self._test_toplevel(a)
        else:
            func = lambda self, a=adaptor: self._test_widget(a)

        name = 'test_%s' % testname
        func.__name__ = name
        namespace[name] = func

    WidgetTest = type('WidgetTest', (BaseWidgetTest, object),
                      namespace)

    return unittest.makeSuite(WidgetTest)

if __name__ == '__main__':
    unittest.TextTestRunner(verbosity=2).run(suite())
