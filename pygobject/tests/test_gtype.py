import unittest

from gobject import GType
from common import gobject

class GTypeTest(unittest.TestCase):
    def checkType(self, expected, *objects):
        # First, double check so we get back what we sent
        str = GType(expected).name # pyg_type_from_object
        val = GType.from_name(str) # pyg_type_wrapper_new
        self.assertEqual(val, expected,
                         'got %r while %r was expected' % (val, expected))

        # Then test the objects
        for object in objects:
            val = GType.from_name(GType(expected).name)
            self.assertEqual(val, expected,
                             'got %r while %r was expected' %
                             (val, expected))

    def testBool(self):
        self.checkType(gobject.TYPE_BOOLEAN, 'gboolean', bool)

    def testInt(self):
        self.checkType(gobject.TYPE_INT, 'gint', int)
    #    model = gtk.ListStore(str, int)
    #    iter = model.append()
    #    model.set(iter, 1, 100000000)

    def testInt64(self):
        self.checkType(gobject.TYPE_INT64, 'gint64')

    def testUint(self):
        self.checkType(gobject.TYPE_UINT, 'guint')

    def testUint64(self):
        self.checkType(gobject.TYPE_UINT64, 'guint64')

    def testLong(self):
        self.checkType(gobject.TYPE_LONG, 'glong', long)

    def testUlong(self):
        self.checkType(gobject.TYPE_ULONG, 'gulong')

    def testDouble(self):
        self.checkType(gobject.TYPE_DOUBLE, 'gdouble', float)

    def testFloat(self):
        self.checkType(gobject.TYPE_FLOAT, 'gfloat')

    def testPyObject(self):
        self.checkType(gobject.TYPE_PYOBJECT, 'GObject', object)

    def testObject(self):
        self.checkType(gobject.TYPE_OBJECT, 'PyObject')

    # XXX: Flags, Enums

class MyObject(gobject.GObject):
    __gtype_name__ = 'MyObject'

class TypeNameTest(unittest.TestCase):
    def testTypeName(self):
        self.assertEqual(GType(MyObject).name, 'MyObject')

if __name__ == '__main__':
    unittest.main()
