# -*- Mode: Python -*-

import sys

import unittest

from common import gtk, gobject

class Tests(unittest.TestCase):

    def testUnicharArg(self):
        """ Test unichar values when used as arguments. """

        entry = gtk.Entry()
        for valid_value in ['a', u'b', u'\ufff0', u'\ufff0'.encode()]:
            entry.set_invisible_char(valid_value)
            self.assertEqual(entry.get_invisible_char(),
                             unicode(valid_value),
                             valid_value)

        for invalid_value in ('12', None, 1, ''):
            try:
                entry.set_invisible_char(invalid_value)
            except:
                pass
            else:
                raise AssertionError(
                    'exception not raised on invalid value w/ '
                    'set_invisible_char: %s'  % invalid_value)


    def testUnicharProperty(self):
        """ Test unichar values when used as properties. """

        entry = gtk.Entry()
        for valid_value in ['a', u'b', u'\ufff0', u'\ufff0'.encode()]:
            entry.set_property('invisible_char', valid_value)
            self.assertEqual(entry.get_property('invisible_char'),
                             valid_value, valid_value)

        for invalid_value in ('12', None, 1, ''):
            try:
                entry.set_property('invisible_char', invalid_value)
            except TypeError:
                pass
            else:
                raise AssertionError(
                    'exception not raised on invalid value w/ '
                    'set_invisible_char: %s'  % invalid_value)

    def testUnicharConstructor(self):
        for valid_value in ['a', u'b', u'\ufff0', u'\ufff0'.encode()]:
            entry = gobject.new(gtk.Entry, invisible_char=valid_value)
            self.assertEqual(entry.get_property('invisible_char'),
                             valid_value, valid_value)

    def testColorCreation(self):
        """ Test GdkColor creation """

        c = gtk.gdk.Color(1, 2, 3)
        self.assertEqual(c.red, 1)
        self.assertEqual(c.green, 2)
        self.assertEqual(c.blue, 3)

        c = gtk.gdk.Color(pixel=0xffff)
        self.assertEqual(c.pixel, 0xffff)

        c = gtk.gdk.Color(pixel=0xffffL)
        self.assertEqual(c.pixel, 0xffff)

        c = gtk.gdk.Color(pixel=0xffffffffL)
        self.assertEqual(c.pixel, 0xffffffffL)

    def testUIntArg(self):
        child = gtk.DrawingArea()
        table = gtk.Table(2, 2, False)
        table.attach(child, 1, 2, 0, 1, ypadding=2)
        self.assertEqual(table.child_get_property(child, 'y-padding'), 2)

        child = gtk.DrawingArea()
        table.attach(child, 1, 2, 0, 1, ypadding=2L)
        self.assertEqual(table.child_get_property(child, 'y-padding'), 2)

if __name__ == '__main__':
    unittest.main()
