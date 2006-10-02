import difflib
import os
import unittest

def setUpEnviron():
    if not 'GAZPACHO_PATH' in os.environ.keys():
        test_dir = os.path.dirname(os.path.abspath(__file__))
        os.environ['GAZPACHO_PATH'] = os.path.dirname(test_dir)

setUpEnviron()

from kiwi.component import provide_utility

from gazpacho.app.app import Application
from gazpacho.gadget import Gadget
from gazpacho.interfaces import IGazpachoApp, IPluginManager
from gazpacho.plugins import PluginManager
from gazpacho.widgetregistry import widget_registry

plugin_manager = PluginManager()
provide_utility(IPluginManager, plugin_manager)

app = Application()
provide_utility(IGazpachoApp, app)

class HildonTest(unittest.TestCase):

    def setUp(self):
        self.app = app
        self.app.new_project()
        self.project = app.get_current_project()

    def tearDown(self):
        self.app.close_current_project()

    def create_gadget(self, class_name):
        """Convenient method for creating widgets"""
        adaptor = widget_registry.get_by_name(class_name)
        gadget = Gadget(adaptor, self.project)
        gadget.create_widget(interactive=False)
        return gadget

def _diff(orig, new, short, verbose):
    lines = difflib.unified_diff(orig, new)
    if not lines:
        return

    diff = False
    try:
        first = lines.next()
        diff = True
    except StopIteration:
        pass
    else:
        print
        print '%s: %s' % (short, first[:-1])
        for line in lines:
            print '%s: %s' % (short, line[:-1])

    return diff

def diff_files(orig, new, verbose=False):
    """
    Diff two files.

    @return: True i the files differ otherwise False
    @rtype: bool
    """
    return _diff(open(orig).readlines(),
                 open(new).readlines(),
                 short=os.path.basename(orig),
                 verbose=verbose)

def diff_strings(orig, new, verbose=False):
    """
    Diff two strings.

    @return: True i the files differ otherwise False
    @rtype: bool
    """
    def _tolines(s):
        return [s + '\n' for line in s.split('\n')]

    return _diff(_tolines(orig),
                 _tolines(new),
                 short='<input>',
                 verbose=verbose)
