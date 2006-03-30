#!/usr/bin/python2.4
'''Application

Demonstrates a typical application window, with menubar, toolbar, statusbar.'''
# pygtk version: Maik Hertha <maik.hertha@berlin.de>

import gobject
import gtk
import hildon

(
  COLOR_RED,
  COLOR_GREEN,
  COLOR_BLUE
) = range(3)

(
  SHAPE_SQUARE,
  SHAPE_RECTANGLE,
  SHAPE_OVAL,
) = range(3)

ui_info = \
'''<ui>
  <menubar name='MenuBar'>
    <menu action='FileMenu'>
      <menuitem action='New'/>
      <menuitem action='Open'/>
      <menuitem action='Save'/>
      <menuitem action='SaveAs'/>
      <separator/>
      <menuitem action='Quit'/>
    </menu>
    <menu action='PreferencesMenu'>
      <menu action='ColorMenu'>
        <menuitem action='Red'/>
        <menuitem action='Green'/>
        <menuitem action='Blue'/>
      </menu>
      <menu action='ShapeMenu'>
        <menuitem action='Square'/>
        <menuitem action='Rectangle'/>
        <menuitem action='Oval'/>
      </menu>
      <menuitem action='Bold'/>
    </menu>
    <menu action='HelpMenu'>
      <menuitem action='About'/>
    </menu>
  </menubar>
  <toolbar  name='ToolBar'>
    <toolitem action='Open'/>
    <toolitem action='Quit'/>
  </toolbar>
</ui>'''


class ApplicationDemo(hildon.App):
    def __init__(self, parent=None):
        hildon.App.__init__(self)
        self.set_title(self.__class__.__name__)
        self.view = hildon.AppView("")
        self.set_appview(self.view)

        table = gtk.Table(1, 4, False)
        self.view.add(table)

        # menu
        merge = gtk.UIManager()
        self.set_data("ui-manager", merge)
        merge.insert_action_group(self.__create_action_group(), 0)
        self.add_accel_group(merge.get_accel_group())
        try:
            mergeid = merge.add_ui_from_string(ui_info)
        except gobject.GError, msg:
            print "building menus failed: %s" % msg

        bar = self.view.get_menu()
        for menu in merge.get_widget("/MenuBar").get_children():
            menu.reparent(bar)
        bar.show_all()

        # toolbar
        bar = merge.get_widget("/ToolBar")
        bar.set_tooltips(True)
        bar.show()
        self.view.get_vbox().add(bar)

        # Create document
        sw = gtk.ScrolledWindow()
        sw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        sw.set_shadow_type(gtk.SHADOW_IN)

        table.attach(sw,
            # X direction           Y direction
            0, 1,                   2, 3,
            gtk.EXPAND | gtk.FILL,  gtk.EXPAND | gtk.FILL,
            0,                      0)

        contents = gtk.TextView()
        contents.grab_focus()

        sw.add (contents)

        # Create statusbar
        self.statusbar = gtk.Statusbar()
        table.attach(self.statusbar,
            # X direction           Y direction
            0, 1,                   3, 4,
            gtk.EXPAND | gtk.FILL,  0,
            0,                      0)

        # Show text widget info in the statusbar
        buffer = contents.get_buffer()
        buffer.connect("changed", self.update_statusbar)
        mark_set_callback = (lambda buffer, new_location, mark:
            self.update_statusbar(buffer))

        # cursor moved
        buffer.connect("mark_set", mark_set_callback)

        self.connect("window_state_event", self.update_resize_grip)
        self.update_statusbar(buffer)

        self.show_all()

    def __create_action_group(self):
        # GtkActionEntry
        entries = (
          ( "FileMenu", None, "_File" ),               # name, stock id, label
          ( "PreferencesMenu", None, "_Preferences" ), # name, stock id, label
          ( "ColorMenu", None, "_Color"  ),            # name, stock id, label
          ( "ShapeMenu", None, "_Shape" ),             # name, stock id, label
          ( "HelpMenu", None, "_Help" ),               # name, stock id, label
          ( "New", gtk.STOCK_NEW,                      # name, stock id
            "_New", "<control>N",                      # label, accelerator
            "Create a new file",                       # tooltip
            self.activate_action ),
          ( "Open", gtk.STOCK_OPEN,                    # name, stock id
            "_Open","<control>O",                      # label, accelerator
            "Open a file",                             # tooltip
            self.activate_action ),
          ( "Save", gtk.STOCK_SAVE,                    # name, stock id
            "_Save","<control>S",                      # label, accelerator
            "Save current file",                       # tooltip
            self.activate_action ),
          ( "SaveAs", gtk.STOCK_SAVE,                  # name, stock id
            "Save _As...", None,                       # label, accelerator
            "Save to a file",                          # tooltip
            self.activate_action ),
          ( "Quit", gtk.STOCK_QUIT,                    # name, stock id
            "_Quit", "<control>Q",                     # label, accelerator
            "Quit",                                    # tooltip
            self.activate_action ),
          ( "About", None,                             # name, stock id
            "_About", "<control>A",                    # label, accelerator
            "About",                                   # tooltip
            self.activate_about ),
          ( "Logo", "demo-gtk-logo",                   # name, stock id
             None, None,                              # label, accelerator
            "GTK+",                                    # tooltip
            self.activate_action ),
        );

        # GtkToggleActionEntry
        toggle_entries = (
          ( "Bold", gtk.STOCK_BOLD,                    # name, stock id
             "_Bold", "<control>B",                    # label, accelerator
            "Bold",                                    # tooltip
            self.activate_action,
            True ),                                    # is_active
        )

        # GtkRadioActionEntry
        color_entries = (
          ( "Red", None,                               # name, stock id
            "_Red", "<control><shift>R",               # label, accelerator
            "Blood", COLOR_RED ),                      # tooltip, value
          ( "Green", None,                             # name, stock id
            "_Green", "<control><shift>G",             # label, accelerator
            "Grass", COLOR_GREEN ),                    # tooltip, value
          ( "Blue", None,                              # name, stock id
            "_Blue", "<control><shift>B",              # label, accelerator
            "Sky", COLOR_BLUE ),                       # tooltip, value
        )

        # GtkRadioActionEntry
        shape_entries = (
          ( "Square", None,                            # name, stock id
            "_Square", "<control><shift>S",            # label, accelerator
            "Square",  SHAPE_SQUARE ),                 # tooltip, value
          ( "Rectangle", None,                         # name, stock id
            "_Rectangle", "<control><shift>R",         # label, accelerator
            "Rectangle", SHAPE_RECTANGLE ),            # tooltip, value
          ( "Oval", None,                              # name, stock id
            "_Oval", "<control><shift>O",              # label, accelerator
            "Egg", SHAPE_OVAL ),                       # tooltip, value
        )

        # Create the menubar and toolbar
        action_group = gtk.ActionGroup("AppWindowActions")
        action_group.add_actions(entries)
        action_group.add_toggle_actions(toggle_entries)
        action_group.add_radio_actions(color_entries, COLOR_RED, self.activate_radio_action)
        action_group.add_radio_actions(shape_entries, SHAPE_OVAL, self.activate_radio_action)

        return action_group

    def activate_about(self, action):
        dialog = gtk.AboutDialog()
        dialog.set_name("PyGTK Demo")
        dialog.set_copyright("\302\251 Copyright 200x the PyGTK Team")
        dialog.set_website("http://www.pygtk.org./")
        ## Close dialog on user response
        dialog.connect ("response", lambda d, r: d.destroy())
        dialog.show()

    def activate_action(self, action):
        dialog = gtk.MessageDialog(self, gtk.DIALOG_DESTROY_WITH_PARENT,
            gtk.MESSAGE_INFO, gtk.BUTTONS_CLOSE,
            'You activated action: "%s" of type "%s"' % (action.get_name(), type(action)))
        # Close dialog on user response
        dialog.connect ("response", lambda d, r: d.destroy())
        dialog.show()

    def activate_radio_action(self, action, current):
        active = current.get_active()
        value = current.get_current_value()

        if active:
            dialog = gtk.MessageDialog(self, gtk.DIALOG_DESTROY_WITH_PARENT,
                gtk.MESSAGE_INFO, gtk.BUTTONS_CLOSE,
                "You activated radio action: \"%s\" of type \"%s\".\nCurrent value: %d" %
                (current.get_name(), type(current), value))

            # Close dialog on user response
            dialog.connect("response", lambda d, r: d.destroy())
            dialog.show()

    def update_statusbar(self, buffer):
        # clear any previous message, underflow is allowed
        self.statusbar.pop(0)
        count = buffer.get_char_count()
        iter = buffer.get_iter_at_mark(buffer.get_insert())
        row = iter.get_line()
        col = iter.get_line_offset()
        self.statusbar.push(0,
        'Cursor at row %d column %d - %d chars in document' % (row, col, count))

    def update_resize_grip(self, widget, event):
        mask = gtk.gdk.WINDOW_STATE_MAXIMIZED | gtk.gdk.WINDOW_STATE_FULLSCREEN
        if (event.changed_mask & mask):
            self.statusbar.set_has_resize_grip(not (event.new_window_state & mask))

def main():
    app = hildon.App()
    app.set_title("Application Demo")
    app.connect('destroy', lambda w: gtk.main_quit())

    view = ApplicationDemo()
    app.set_appview(view)
    app.show_all()

    gtk.main()

if __name__ == '__main__':
    main()
