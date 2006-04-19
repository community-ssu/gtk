#!/usr/bin/env python2.4

import gtk
import hildon

def interface_file_chooser (window, action):
  dialog = hildon.FileChooserDialog (window, action)
  dialog.show_all ()
  print dialog.run ()
  dialog.destroy ()

def create_menu (window):
  main_menu = gtk.Menu ()
  window.set_menu (main_menu)

  menu_others = gtk.Menu ()
  item_others    = gtk.MenuItem ("Others")
  item_radio1    = gtk.RadioMenuItem (None,        "Radio1")
  item_radio2    = gtk.RadioMenuItem (item_radio1, "Radio2")
  item_check     = gtk.CheckMenuItem ("Check")
  item_close     = gtk.MenuItem ("Close")
  item_separator = gtk.SeparatorMenuItem ()
  main_menu.append (item_others)
  menu_others.append (item_radio1)
  menu_others.append (item_radio2)
  menu_others.append (item_separator)
  menu_others.append (item_check)
  menu_others.append (item_close)
  item_others.set_submenu (menu_others)
  item_close.connect ('activate', gtk.main_quit)
  main_menu.show_all ()

def insert_with_callback (toolbar, stock_id, func):
  button = gtk.ToolButton (stock_id)
  toolbar.insert (button, -1)
  button.connect ('clicked', func)

def callback_new (w):
  print 'new'

def callback_open (w, data):
  interface_file_chooser (data, 'open')

def create_toolbar (window):
  toolbar = gtk.Toolbar ()
  toolbar.set_border_width (3)
  toolbar.set_orientation ('horizontal')
  toolbar.set_style ('both-horiz')
  insert_with_callback (toolbar, 'gtk-new', callback_new)
  insert_with_callback (toolbar, 'gtk-open',
                        lambda (w): interface_file_chooser (window, 'open'))
  window.add_toolbar (toolbar)

def callback_buffer_modified (textarea):
  print 'modified'  
  
def create_textarea (window):
  scrolledwindow = gtk.ScrolledWindow (None, None)
  scrolledwindow.show ()
  scrolledwindow.set_policy ('automatic', 'automatic')
  textview = gtk.TextView ()
  textview.set_editable (1)
  textview.set_left_margin (10)
  textview.set_right_margin (10)
  scrolledwindow.add (textview)
  window.add (scrolledwindow)
  buffer = textview.get_buffer ()
  buffer.connect ('modified-changed', callback_buffer_modified)
  buffer.connect ('changed', callback_buffer_modified)


program = hildon.Program ()

window = hildon.Window ()
window.set_title ("MaemoPad")
window.connect ('destroy', gtk.main_quit)

program.add_window (window)

create_textarea (window)
create_menu (window)
create_toolbar (window)

window.show_all ()

gtk.main ()
