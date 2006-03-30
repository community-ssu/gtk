#!/usr/bin/env python2.4
# vim:ts=4:et:sw=4:tw=80

import hildon
import gtk
import pango
import gobject

import sys
import os
import urlparse
import urllib
import ConfigParser

def getHomeDir():
    ''' Try to find user's home directory, otherwise return current directory.'''
    try:
        path1 = os.path.expanduser("~")
    except:
        path1 = ""
    try:
        path2 = os.environ["HOME"]
    except:
        path2 = ""
    try:
        path3 = os.environ["USERPROFILE"]
    except:
        path3 = ""

    if not os.path.exists(path1):
        if not os.path.exists(path2):
            if not os.path.exists(path3):
                return os.getcwd()
            else: return path3
        else: return path2
    else: return path1

class Downloader(object):
    def __init__(self):
        self.configChanged = False

        self.loadConfig()

    def loadConfig(self):
        self.config = ConfigParser.ConfigParser()
        self.config.read(os.path.expanduser('~/.download.cfg'))
        
        if not self.config.has_section("main"):
            self.config.add_section("main")
            self.configChanged = True

        # download dir
        if not self.config.has_option("main", "download_directory"):
            self.config.set("main", "download_directory", \
                    os.path.expanduser('~'))
            self.configChanged = True
        else:
            self.downloadDirectory = self.config.get("main", \
                    "download_directory")

        if not self.config.has_section("history"):
            self.config.add_section("history")
            self.configChanged = True

        # history 
        self.history = []
        if not self.config.has_option("history", "size"):
            self.config.set("history", "size", 0)
            self.configChanged = True
        else:
            for k, v in self.config.items("history"):
                if k.startswith("url"):
                    self.history.append(str(v))

        self.saveConfig()

    def saveConfig(self):
        if not self.configChanged:
            return

        configfile = open(os.path.expanduser('~/.download.cfg'), "w")
        self.config.write(configfile)
        configfile.close()

    def getFilename(self, url):
        try:
            fname = os.path.basename(urlparse.urlparse(url)[2])
        except Except, e:
            fname = "noname"

        i = 1
        name = fname
        while os.path.exists(name):
            name = "%s.%d" % ( fname, i )
            i += 1
        return "%s/%s" % (self.downloadDirectory, name)

class Download(hildon.App):
    def __init__(self, model):
        hildon.App.__init__(self)
        self.model = model

        self.set_title("Download")
        self.connect('destroy', self.onExit)
        self.view = hildon.AppView("")
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
        dialog.set_name("PyHildon Downloader")
        dialog.set_copyright("\302\251 Copyright 2005 the INdT Team")
        dialog.set_website("http://www.indt.org/")
        dialog.connect ("response", lambda d, r: d.destroy())
        dialog.show()

    def run(self):
        gtk.main()

    def _createPanel(self):
        self.hbox = gtk.VBox()
        self.view.add(self.hbox)

        self.label = gtk.Label("URL")
        self.hbox.pack_start(self.label, expand=False)

        #self.url = gtk.()
        #self.hbox.pack_start(self.notebook, expand=True)

        # Documentation Tab
        scrolled_window, self.documentation = self._createText(False)
        self._createDocTab(scrolled_window, "_Documentation")

        # Code Tab
        scrolled_window, self.code = self._createText(True)
        self._createDocTab(scrolled_window, "_Source Code")
        tag = self.code.create_tag('source')
        tag.set_property('font', 'monospace')
        tag.set_property('pixels_above_lines', 0)
        tag.set_property('pixels_below_lines', 0)
        tag = self.code.create_tag('keyword', foreground='#00007F', weight=pango.WEIGHT_BOLD)
        tag = self.code.create_tag('string', foreground='#7F007F')
        tag = self.code.create_tag('comment', foreground='#007F00', style=pango.STYLE_ITALIC)

    def _createDocTab(self, widget, label):
        l = gtk.Label('')
        l.set_text_with_mnemonic(label)
        #self.notebook.append_page(widget, l)

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
    model = Downloader()
    Download(model).run()

"""
#!/usr/bin/python2.4

for url in sys.argv[ 1 : ]:
    try:
        fname = get_file_name( url )
        print >> sys.stderr, "Downloading:", url, "->", fname

        remote = urllib.urlopen( url )

        i = remote.info()
        clength = int( i[ "Content-Length" ] )
        try:
            ctype = i[ "Content-Type" ]
        except:
            ctype = "Unknow-Type"
        print >> sys.stderr, "\t", ctype, "is", clength, "bytes long."

        local = open( fname, "wb" )
        last  = 0
        size  = 0
        for b in remote:
            size += len( b )
            pos = int( size * 79 / clength )
            if pos > last:
                last = pos
                sys.stderr.write( "#" )

            local.write( b )
        # for b in remote

        sys.stderr.write( "\n" )

        remote.close()
        local.close()

    except Exception, e:
        print >> sys.stderr, "Unknow error: ", e
"""
