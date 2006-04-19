#!/usr/bin/env python2.4

import gtk
import hildon

window = hildon.Window ()
window.set_title ("Hello maemo!")

button = gtk.Button ("Hello")
window.add (button)

window.show_all ()

#print app.get_default_font ()

gtk.main ()
