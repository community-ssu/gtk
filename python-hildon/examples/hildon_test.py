#!/usr/bin/env python2.4

# vim:ts=4:et:sw=4:tw=80
import hildon
import gtk
import pango
import gobject
import os

class PyGtkDemo(hildon.App):
    def __init__(self):
        hildon.App.__init__(self)
        self.set_title("PyHildonPad")
        self.connect('destroy', self.onExit)

        self.view = hildon.AppView("noname")
        self.vbox = self.view.get_vbox()
        self.set_appview(self.view)

        self._createMenu()
        self._createToolbar()
        self._createPanel()
        self.show_all()

    def _createToolbar(self):
        self.toolbar = gtk.Toolbar()
        self.toolbar.set_border_width(3)
        self.toolbar.set_orientation('horizontal')
        self.toolbar.set_style('both-horiz')
        
        self.toolQuit = gtk.ToolButton('gtk-quit')
        self.toolQuit.connect('clicked', self.onExit)
        self.toolbar.insert(self.toolQuit, -1)

        self.vbox.add(self.toolbar)

    def _createMenu(self):
        self.menubar = self.view.get_menu()

        # Main Menubar
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

        self.menubar.add(self.mnuItmFile)
        self.menubar.add(self.mnuItmHelp)
        self.menubar.show_all()

    def onExit(self, widget):
        gtk.main_quit()

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
            dialog = hildon.ColorSelector(self)
            print "run: %s" % dialog.run(),
            print "color: %s, %s, %s" % (
                    dialog.get_color().red,
                    dialog.get_color().green,
                    dialog.get_color().blue
            ),
            dialog.destroy()
        elif test == "Calendar Popup":
            dialog = hildon.CalendarPopup(self, 2004, 9, 1)
            print "run: %s" % dialog.run(),
            print "date: %s-%s-%s" % dialog.get_date(),
            dialog.destroy()
        elif test == "Add Home":
            dialog = hildon.AddHomeDialog(self, "foo", "bar")
            print "run: %s" % dialog.run(),
            print "name: %s" % dialog.get_name(),
            dialog.destroy()
        elif test == "File Selector":
            dialog = hildon.FileChooserDialog(self, "open")
            print "run: %s" % dialog.run(),
            dialog.destroy()
        elif test == "Set Password":
            dialog = hildon.SetPasswordDialog(self, True)
            print "run: %s" % dialog.run(),
            print "password: %s" % dialog.get_password(),
            dialog.destroy()
        elif test == "Sort":
            dialog = hildon.SortDialog(self)
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

            dialog = hildon.FileDetailsDialog(self, 
                    "%s/MyDocs/foobar.txt" % os.environ['HOME'])
            print "run: %s" % dialog.run(),
            dialog.destroy()
        elif test == "Font Selection":
            dialog = hildon.FontSelectionDialog(self, "Choose font...")
            print "run: %s" % dialog.run(),
            # TODO: verify how can i obtain the font selected...
            dialog.destroy()
        elif test == "Get Password":
            dialog = hildon.GetPasswordDialog(self, False)
            print "run: %s" % dialog.run(),
            print "password: %s" % dialog.get_password(),
            dialog.destroy()
        elif test == "Insert Object":
            dialog = hildon.InsertObjectDialog(self)
            print "run: %s" % dialog.run(),
            dialog.destroy()
        elif test == "Name Password":
            dialog = hildon.NamePasswordDialog(self)
            print "run: %s" % dialog.run(),
            print "name: %s" % dialog.get_name(),
            print "password: %s" % dialog.get_password(),
            dialog.destroy()
        elif test == "Note":
            dialog = hildon.Note(self, "Note Dialog...")
            print "run: %s" % dialog.run(),
            dialog.destroy()
               
        print "tested."
        return

    def _createPanel(self):
        self.hbox = gtk.HBox()
        self.view.add(self.hbox)

        # Test Buttons
        self.vboxTests = []
        self.vboxTests.append(gtk.VBox())
        self.vboxTests.append(gtk.VBox())
        self.vboxTests.append(gtk.VBox())
        for vbox in self.vboxTests:
            self.hbox.pack_start(vbox, expand=True)
        self.tests = {
            "Calendar Popup":   [0, gtk.Button, ("Calendar Popup",), 'clicked', self.onTest],
            "Color Selector":   [1, gtk.Button, ("Color Selector",), 'clicked', self.onTest],
            "Color Button":     [0, hildon.ColorButton, None, None, None],
            "Add Home":         [1, gtk.Button, ("Add Home",), 'clicked', self.onTest],
            "Control Bar":      [0, hildon.Controlbar, (0, 50), None, None],
            "Date Editor":      [1, hildon.DateEditor, (2004, 9, 1), None, None],
            "File Selector":    [0, gtk.Button, ("File Selector",), 'clicked', self.onTest],
            "Set Password":     [1, gtk.Button, ("Set Password",), 'clicked', self.onTest],
            "Sort":             [0, gtk.Button, ("Sort",), 'clicked', self.onTest],
            "File Details":     [1, gtk.Button, ("File Details",), 'clicked', self.onTest],
            "Font Selection":   [0, gtk.Button, ("Font Selection",), 'clicked', self.onTest],
            "Get Password":     [1, gtk.Button, ("Get Password",), 'clicked', self.onTest],
            "HVolume Bar":      [0, hildon.HVolumebar, None, None, None],
            "VVolume Bar":      [2, hildon.VVolumebar, None, None, None],
            "Insert Object":    [1, gtk.Button, ("Insert Object",), 'clicked', self.onTest],
            "Name Password":    [0, gtk.Button, ("Name Password",), 'clicked', self.onTest],
            "Note":             [1, gtk.Button, ("Note",), 'clicked', self.onTest],
        }

        for i, test in enumerate(self.tests.keys()):
            pos, obj, parms, event, handle = self.tests[test]
            if parms:
                self.tests[test].append(obj(*parms))
            else:
                self.tests[test].append(obj())
            if event:
                self.tests[test][-1].connect(event, handle)
            self.vboxTests[pos].pack_start(self.tests[test][-1], expand=True, fill=True)

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

        



if __name__ == "__main__":
    PyGtkDemo().run()

