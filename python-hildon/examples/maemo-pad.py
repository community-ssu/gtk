#!/usr/bin/env python2.4

import gtk
import hildon

def interface_file_chooser (mainview, action):
  dialog = hildon.FileChooserDialog (mainview.app, action)
  dialog.show_all ()
  print dialog.run ()
  dialog.destroy ()

def create_menu (mainview):
  main_menu   = mainview.get_menu ()
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

def create_toolbar (mainview):
  toolbar = gtk.Toolbar ()
  toolbar.set_border_width (3)
  toolbar.set_orientation ('horizontal')
  toolbar.set_style ('both-horiz')
  insert_with_callback (toolbar, 'gtk-new', callback_new)
  insert_with_callback (toolbar, 'gtk-open',
                        lambda (w): interface_file_chooser (mainview, 'open'))
  mainview.set_toolbar (toolbar)

def callback_buffer_modified (textarea):
  print 'modified'  
  
def create_textarea (mainview):
  scrolledwindow = gtk.ScrolledWindow (None, None)
  scrolledwindow.show ()
  scrolledwindow.set_policy ('automatic', 'automatic')
  textview = gtk.TextView ()
  textview.set_editable (1)
  textview.set_left_margin (10)
  textview.set_right_margin (10)
  scrolledwindow.add (textview)
  mainview.add (scrolledwindow)
  buffer = textview.get_buffer ()
  buffer.connect ('modified-changed', callback_buffer_modified)
  buffer.connect ('changed', callback_buffer_modified)

app = hildon.App ()
app.set_title ("MaemoPad")
app.set_two_part_title (1)

appview = hildon.AppView ("AppView Title")
app.set_appview (appview)
appview.app = app

create_textarea (appview)
create_menu (appview)
create_toolbar (appview)

app.show_all ()


gtk.main ()
