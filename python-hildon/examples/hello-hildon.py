#!/usr/bin/env python2.5

import gtk
import hildon

window = hildon.Window ()
window.connect('destroy', gtk.main_quit)
window.set_title ("Hello maemo!")

button = gtk.Button ("Hello")
window.add (button)

window.show_all ()

#print app.get_default_font ()

gtk.main ()

