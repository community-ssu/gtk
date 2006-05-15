import difflib
from glob import glob
import os

import gtk
from twisted.trial import unittest

from gazpacho.unittest.common import app

glade_dir = os.path.split(__file__)[0]
glade_files = glob(os.path.join(glade_dir, 'glade', '*.glade'))

class BaseTest(unittest.TestCase):
    def _test_gladefile(self, filename):
        tmp = filename + '.tmp'
        if os.path.exists(tmp):
            os.unlink(tmp)
        project = app.open_project(filename)
        project.save(tmp)

        correct = open(filename).readlines()
        new = open(tmp).readlines()
        lines = difflib.unified_diff(correct, new, n=3)
        if lines:
            short = os.path.basename(filename)
            try:
                first = lines.next()
            except StopIteration:
                pass
            else:
                print
                print '%s: %s' % (short, first[:-1])
                for line in lines:
                    print '%s: %s' % (short, line[:-1])
                raise SystemExit("%s can not be read properly" % short)

        if os.path.exists(tmp):
            os.unlink(tmp)

SKIPPED = {
    'frame': 'label widget packing properties lost',
    'table': 'n-columns/n-rows are not read properly',
    }

namespace = {}
for glade_file in glade_files:
    func = lambda self, f=glade_file: self._test_gladefile(f)
    testname = os.path.basename(glade_file)[:-6]
    name = 'test_%s' % testname
    func.__name__ = name
    if testname in SKIPPED:
        func.skip = SKIPPED[testname]
    namespace[name] = func
GladeTest = type('GladeTest', (BaseTest, object),
                 namespace)

