#!/usr/bin/env python2.4

import gtk
import hildon

app = hildon.App ()
app.set_title ("Hello maemo!")

main_view = hildon.AppView (None)

button = gtk.Button ("Hello")
main_view.add (button)

app.set_appview (main_view)

app.show_all ()

print app.get_default_font ()

gtk.main ()
