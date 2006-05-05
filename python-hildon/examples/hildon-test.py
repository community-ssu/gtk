#!/usr/bin/env python2.4

# vim:ts=4:et:sw=4:tw=80
import hildon
import gtk
import pango
import gobject
import os

class PyGtkDemo(hildon.Program):
    def __init__(self):
        hildon.Program.__init__(self)
        
        self.window = hildon.Window()
        self.add_window(self.window)
        
        self.window.set_title("PyHildonPad")
        self.window.connect('destroy', self.onExit)
        
        self.noteTestWindow = HildonNoteTestWindow()
        self.add_window(self.noteTestWindow)
        
        self.noteTestWindow.back_button.connect('clicked', self.onBackNoteWindow)
        self.noteTestWindow.connect('destroy', self.onExit)

        self.passwordTestWindow = HildonPasswordTestWindow()
        self.add_window(self.passwordTestWindow)
        
        self.passwordTestWindow.back_button.connect('clicked', self.onBackPasswordWindow)
        self.passwordTestWindow.connect('destroy', self.onExit)
        
        self.weekdayPickerWindow = WeekdayPickerWindow()
        self.add_window(self.weekdayPickerWindow)
        
        self.weekdayPickerWindow.back_button.connect('clicked', self.onBackWeekdayPickerWindow)
        self.weekdayPickerWindow.connect('destroy', self.onExit)

        self._createMenu()
        self._createToolbar()
        self._createPanel()

        self.window.show_all()
        
    def onBackNoteWindow(self, widget):
        self.noteTestWindow.hide()
        self.window.show_all()
        
    def onBackPasswordWindow(self, widget):
        self.passwordTestWindow.hide()
        self.window.show_all()
    
    def onBackWeekdayPickerWindow(self, widget):
        self.weekdayPickerWindow.hide()
        self.window.show_all()    
        

    def _createToolbar(self):
        self.toolbar = gtk.Toolbar()
        self.toolbar.set_border_width(3)
        self.toolbar.set_orientation('horizontal')
        self.toolbar.set_style('both-horiz')
        
        obj = gtk.ToolButton(gtk.STOCK_QUIT)
        obj.connect('clicked', self.onExit)
        self.toolbar.insert(obj, -1)
        
        obj = gtk.ToolButton(gtk.STOCK_FIND)
        obj.connect('clicked', self.onShowFind)
        self.toolbar.insert(obj, -1)

        self.set_common_toolbar(self.toolbar)
        
        self.toolbar.show_all()
        
        self.findToolBar = hildon.FindToolbar("Find something...")
        self.window.add_toolbar(self.findToolBar)
        
        self.findToolBar.connect("close", self.onHideFind)
        self.findToolBar.set_no_show_all(True)
        
    def _createMenu(self):
        self.menu = gtk.Menu()

        # Main Menu
        self.mnuItmFile = gtk.MenuItem("_File")
        self.mnuItmHelp = gtk.MenuItem("_Help")

        self.mnuFile = gtk.Menu()
        self.mnuHelp = gtk.Menu()

        self.mnuItmFile.set_submenu(self.mnuFile)
        self.mnuItmHelp.set_submenu(self.mnuHelp)

        # Menu File
        self.mnuItmFileExit   = gtk.MenuItem("_Exit")
        self.mnuFile.add(self.mnuItmFileExit)
        self.mnuItmFileExit.connect_object("activate", self.onExit, "file.exit")

        # Menu Help
        self.mnuItmHelpAbout   =  gtk.MenuItem("_About...")
        self.mnuHelp.add(self.mnuItmHelpAbout)
        self.mnuItmHelpAbout.connect_object("activate", self.onAbout, "help.about")

        self.menu.add(self.mnuItmFile)
        self.menu.add(self.mnuItmHelp)
        
        self.set_common_menu (self.menu);

    def onExit(self, widget):
        gtk.main_quit()
        
    def onShowFind(self, widget):
        self.findToolBar.show()
        
        #Terrible hack to force a redraw on self.window
        self.window.hide()
        self.window.show()
        
    def onHideFind(self, widget):
        self.findToolBar.hide()
        
        #Terrible hack to force a redraw on self.window
        self.window.hide()
        self.window.show()

    def onAbout(self, widget):
        print "Help->About"
        dialog = gtk.AboutDialog()
        dialog.set_name("PyHildon Demo")
        dialog.set_copyright("\302\251 Copyright 2005 the INdT Team")
        dialog.set_website("http://www.indt.org/")
        dialog.connect ("response", lambda d, r: d.destroy())
        dialog.show()

    def run(self):
        gtk.main()

    def onTest(self, widget):
        test = widget.get_label()
        print test,

        if test == "Color Selector":
            dialog = hildon.ColorSelector(self.window)
            print "run: %s" % dialog.run(),
            print "color: %s, %s, %s" % (
                    dialog.get_color().red,
                    dialog.get_color().green,
                    dialog.get_color().blue
            ),
            dialog.destroy()
        elif test == "Calendar Popup":
            dialog = hildon.CalendarPopup(self.window, 2004, 9, 1)
            print "run: %s" % dialog.run(),
            print "date: %s-%s-%s" % dialog.get_date(),
            dialog.destroy()
        elif test == "Add Home":
            dialog = hildon.AddHomeDialog(self.window, "foo", "bar")
            print "run: %s" % dialog.run(),
            print "name: %s" % dialog.get_name(),
            dialog.destroy()
        elif test == "File Selector":
            dialog = hildon.FileChooserDialog(self.window, "open")
            print "run: %s" % dialog.run(),
            dialog.destroy()
        elif test == "Sort":
            dialog = hildon.SortDialog(self.window)
            dialog.add_sort_key("foo")
            dialog.add_sort_key("bar")
            dialog.add_sort_key("baz")
            print "run: %s" % dialog.run(),
            print "sort: %s %s" % (dialog.get_sort_order(), dialog.get_sort_key()),
            dialog.destroy()
        elif test == "File Details":
            try:
                os.mkdir("%s/MyDocs" % os.environ['HOME'])
            except Exception, e:
                pass
            try:
                f = open("%s/MyDocs/foobar.txt" % os.environ['HOME'], "w")
                f.write("Test file\n")
                f.close()
            except Exception, e:
                print "Xerror: %s" % e,

            dialog = hildon.FileDetailsDialog(self.window, 
                    "%s/MyDocs/foobar.txt" % os.environ['HOME'])
            print "run: %s" % dialog.run(),
            dialog.destroy()
        elif test == "Font Selection":
            dialog = hildon.FontSelectionDialog(self.window, "Choose font...")
            print "run: %s" % dialog.run(),
            print "size: %d" % dialog.get_property("size")
            # TODO: verify how can i obtain the font selected...
            dialog.destroy()
        elif test == "Insert Object":
            dialog = hildon.InsertObjectDialog(self.window)
            print "run: %s" % dialog.run(),
            dialog.destroy()
        elif test == "Note":
            self.window.hide()
            self.noteTestWindow.show_all()
        elif test == "Password":
            self.window.hide()
            self.passwordTestWindow.show_all()
        elif test == "Weekday Picker":
            self.window.hide()
            self.weekdayPickerWindow.show_all()
               
        print "tested."
        return

    def _createPanel(self):
        self.hbox = gtk.HBox(True)
        self.window.add(self.hbox)

        # Test Buttons
        self.vboxTests = []
        self.vboxTests.append(gtk.VBox())
        self.vboxTests.append(gtk.VBox())
        self.vboxTests.append(gtk.VBox())
        for vbox in self.vboxTests:
            self.hbox.pack_start(vbox, expand=True)

        obj = gtk.Button ("Calendar Popup")
        obj.connect("clicked", self.onTest)
        self.vboxTests[0].pack_start(obj)
        
        obj = gtk.Button ("Weekday Picker")
        obj.connect("clicked", self.onTest)
        self.vboxTests[0].pack_start(obj)
        
        obj = gtk.Button ("Color Selector")
        obj.connect("clicked", self.onTest)
        self.vboxTests[1].pack_start(obj)
        
        self.vboxTests[2].pack_start(hildon.ColorButton())
        
        obj = gtk.Button ("Add Home")
        obj.connect("clicked", self.onTest)
        self.vboxTests[0].pack_start(obj)
        
        self.vboxTests[1].pack_start(hildon.Controlbar(0, 50, 25))
        
        self.vboxTests[2].pack_start(hildon.DateEditor(2004, 9, 1))
        
        obj = gtk.Button ("File Selector")
        obj.connect("clicked", self.onTest)
        self.vboxTests[0].pack_start(obj)
        
        obj = gtk.Button ("Password")
        obj.connect("clicked", self.onTest)
        self.vboxTests[2].pack_start(obj)
        
        obj = gtk.Button ("Sort")
        obj.connect("clicked", self.onTest)
        self.vboxTests[2].pack_start(obj)
        
        obj = gtk.Button ("File Details")
        obj.connect("clicked", self.onTest)
        self.vboxTests[0].pack_start(obj)
        
        obj = gtk.Button ("Font Selection")
        obj.connect("clicked", self.onTest)
        self.vboxTests[1].pack_start(obj)
        
        self.vboxTests[0].pack_start(hildon.HVolumebar())
        
        self.vboxTests[1].pack_start(hildon.VVolumebar())
        
        obj = gtk.Button ("Insert Object")
        obj.connect("clicked", self.onTest)
        self.vboxTests[2].pack_start(obj)
        
        obj = gtk.Button ("Note")
        obj.connect("clicked", self.onTest)
        self.vboxTests[2].pack_start(obj)
        
        self.vboxTests[2].pack_start(hildon.NumberEditor(0, 10))

        obj = hildon.RangeEditor()
        self.vboxTests[2].pack_start(obj)
        obj.set_range(-5, 12)
        obj.set_limits(-100, 100)
        
        obj = hildon.Seekbar(total_time = 180, fraction = 100)
        obj.set_position(30)
        self.vboxTests[0].pack_start(obj)
        
    def _createDocTab(self, widget, label):
        l = gtk.Label('')
        l.set_text_with_mnemonic(label)
        self.notebook.append_page(widget, l)

    def _createText(self, is_source=False):
        scrolled_window = gtk.ScrolledWindow()
        scrolled_window.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        scrolled_window.set_shadow_type(gtk.SHADOW_IN)
        text_view = gtk.TextView()
        scrolled_window.add(text_view)
        buffer = gtk.TextBuffer(None)
        text_view.set_buffer(buffer)
        text_view.set_editable(False)
        text_view.set_cursor_visible(False)
        text_view.set_wrap_mode(not is_source)
        return scrolled_window, buffer

    def _createDemoList(self):
        demos = [
            'foo',
            'bar',
            'baz'
        ]

        # model
        model = gtk.TreeStore(
                gobject.TYPE_STRING
        )
        for demo in demos:
            iter = model.append(None)
            model.set(iter, 0, demo)

        # view
        view = gtk.TreeView(model)
        view.set_rules_hint(True)
        view.get_selection().set_mode(gtk.SELECTION_SINGLE)

class HildonPasswordTestWindow(hildon.Window):
    def __init__(self):
        hildon.Window.__init__(self)

        self.set_title("PyHildon - Password")
        
        self.vbox = gtk.VBox(True)
        self.add(self.vbox)

        obj = gtk.Button ("Name Password")
        obj.connect ('clicked', self.onNamePassword)
        self.vbox.pack_start(obj)
        
        obj = gtk.Button ("Set Password")
        obj.connect ('clicked', self.onSetPassword)
        self.vbox.pack_start(obj)
        
        obj = gtk.Button ("Get Password")
        obj.connect ('clicked', self.onGetPassword)
        self.vbox.pack_start(obj)
        
        obj = gtk.HSeparator ()
        self.vbox.pack_start(obj)
        
        self.back_button = gtk.Button ("Back")
        self.vbox.pack_start(self.back_button)
        
    def onNamePassword(self, widget):
        dialog = hildon.NamePasswordDialog(self)
        print "run: %s" % dialog.run(),
        print "name: %s" % dialog.get_name(),
        print "password: %s" % dialog.get_password(),
        dialog.destroy()
        
    def onSetPassword(self, widget):
        dialog = hildon.SetPasswordDialog(self, True)
        print "run: %s" % dialog.run(),
        print "password: %s" % dialog.get_password(),
        dialog.destroy()
        
    def onGetPassword(self, widget):
        dialog = hildon.GetPasswordDialog(self, False)
        dialog.set_domain("Password domain")
        dialog.set_caption("This is the caption")
        print "run: %s" % dialog.run(),
        print "password: %s" % dialog.get_password(),
        dialog.destroy()

    
class HildonNoteTestWindow(hildon.Window):
    def __init__(self):
        hildon.Window.__init__(self)
        
        self.set_title("PyHildon - Note")
        
        self.vbox = gtk.VBox(True)
        self.add(self.vbox)

        obj = gtk.Button ("Confirmation")
        obj.connect ('clicked', self.onConfirmation)
        self.vbox.pack_start(obj)
        
        obj = gtk.Button ("Confirmation with Icon Name")
        obj.connect ('clicked', self.onConfirmationWithIconName)
        self.vbox.pack_start(obj)
        
        obj = gtk.Button ("Information")
        obj.connect ('clicked', self.onInformation)
        self.vbox.pack_start(obj)
        
        obj = gtk.Button ("Information with Icon Name")
        obj.connect ('clicked', self.onInformationWithIconName)
        self.vbox.pack_start(obj)
        
        obj = gtk.Button ("Cancel with Progress Bar")
        obj.connect ('clicked', self.onCancelWithProgressBar)
        self.vbox.pack_start(obj)
        
        obj = gtk.HSeparator ()
        self.vbox.pack_start(obj)
        
        self.back_button = gtk.Button ("Back")
        self.vbox.pack_start(self.back_button)
        
    def onConfirmation(self, widget):
            dialog = hildon.Note("confirmation", (self, "Confirmation"))
            dialog.run()
            dialog.destroy()
            
    def onConfirmationWithIconName(self, widget):        
            dialog = hildon.Note("confirmation", (self, "Conf. with Icon Name", gtk.STOCK_DIALOG_INFO))
            dialog.run()
            dialog.destroy()

    def onInformation(self, widget):
            dialog = hildon.Note("information", (self, "Information"))
            dialog.run()
            dialog.destroy()

    def onInformationWithIconName(self, widget):        
            dialog = hildon.Note("information", (self, "Inf. with Icon Name", gtk.STOCK_DIALOG_INFO))
            dialog.run()
            dialog.destroy()
            
    def onCancelWithProgressBar(self, widget):
            prog_bar = gtk.ProgressBar()
            prog_bar.set_fraction (0.35)
            dialog = hildon.Note("cancel_with_progress_bar", (self, "Cancel with Progress Bar", prog_bar))
            dialog.run()
            dialog.destroy()
            prog_bar.destroy()
    
class WeekdayPickerWindow(hildon.Window):
    def __init__(self):
        hildon.Window.__init__(self)
        
        self.set_title("PyHildon - Weekday Picker")
        
        self.vbox = gtk.VBox(True)
        self.add(self.vbox)

        self.weekdayPicker = hildon.WeekdayPicker() 
        self.vbox.pack_start(self.weekdayPicker)
        
        change_day_box = gtk.HBox (False)
        self.vbox.pack_start (change_day_box)

        self.change_buttons = gtk.HBox(True)
        self.vbox.pack_start(self.change_buttons)

        obj = gtk.HSeparator ()
        self.vbox.pack_start(obj)
        
        self.back_button = gtk.Button ("Back")
        self.vbox.pack_start(self.back_button)
        
        obj = gtk.Button ("Set All")
        obj.connect ('clicked', self.onSetAll)
        self.change_buttons.pack_start (obj)
        
        obj = gtk.Button ("Unset All")
        obj.connect ('clicked', self.onUnsetAll)
        self.change_buttons.pack_start (obj)
        
        self.day_entry = gtk.Entry()

        obj = gtk.Button ("Set")
        obj.connect ('clicked', self.onSetDay)
        change_day_box.pack_start (obj)
        
        obj = gtk.Button ("Unset")
        obj.connect ('clicked', self.onUnsetDay)
        change_day_box.pack_start (obj)
        
        obj = gtk.Button ("Toggle")
        obj.connect ('clicked', self.onToggleDay)
        change_day_box.pack_start (obj)
        
        change_day_box.pack_start (self.day_entry)
        
    def onSetAll(self, widget):
        self.weekdayPicker.set_all()
        
    def onUnsetAll(self, widget):
        self.weekdayPicker.unset_all()
        
    def onSetDay(self, widget):
        self.weekdayPicker.set_day(self.day_entry.get_text())
        
    def onUnsetDay(self, widget):
        self.weekdayPicker.unset_day(self.day_entry.get_text())
        
    def onToggleDay(self, widget):
        self.weekdayPicker.toggle_day(self.day_entry.get_text())
        
if __name__ == "__main__":
    PyGtkDemo().run()

