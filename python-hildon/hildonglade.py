# python-hildon - Python bindings for the Hildon toolkit.
#
#   hildonglade.py: Hildon wrapper for gtk.glade
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA

import sys

import gtk
import gtk.glade
import hildon

from xml.sax import make_parser, handler

class HildonGladeError(Exception):
    pass

class GladeParser(handler.ContentHandler):
    def __init__(self, signals, ui):
        handler.ContentHandler.__init__(self)
        self.signals = signals
        self.ui = ui

        # internal use
        self.last_widget = None
        self.start_ui = ''
        self.raw_ui = []

    def startElement(self, name, attrs):
        if name.lower() == 'ui':
            self.start_ui = attrs['id']

        if name.lower() == 'widget':
            self.last_widget = attrs['id']

        if name.lower() == 'signal' and self.last_widget is not None:
            try:
                self.signals.append( (attrs['handler'], self.last_widget, attrs['name']) )
            except KeyError, e:
                raise HildonGladeError("Inconsistent Glade file: '%s'" % (e))

    def endElement(self, name):
        if name.lower() == 'ui':
            if self.raw_ui:
                self.ui.append((self.start_ui, ''.join(self.raw_ui)))
            self.start_ui = ''
            self.raw_ui = []

    def characters(self, ch):
        if not self.start_ui:
            return
        self.raw_ui.append(ch.strip())

class XML(gtk.glade.XML):
    def _replace_window(self, window):
        appview = hildon.AppView(window.get_title())
        appview.set_name(window.get_name())
        self._reparent_children(window, appview)
        self._internal_widgets[window.get_name()] = appview
        self._to_destroy.append(window)

    def _replace_menubar(self, menubar):
        appview = menubar
        while not isinstance(appview, hildon.AppView):
            appview = appview.get_parent()
        if appview is None:
            return

        menu = appview.get_menu()
        self._reparent_children(menubar, menu)
        self._internal_widgets[menubar.get_name()] = menu
        self._to_destroy.append(menubar)

    def _replace_toolbar(self, toolbar):
        appview = toolbar.get_parent()
        while not isinstance(appview, hildon.AppView):
            appview = appview.get_parent()
        appview.get_vbox().show()
        toolbar.reparent(appview.get_vbox())

    def _reparent_children(self, from_, to):
        for child in from_.get_children():
            child.reparent(to)

    def get_widget(self, name):
        try:
            return self._internal_widgets[name]
        except KeyError, e:
            return gtk.glade.XML.get_widget(self, name)

    def get_widget_prefix(self, prefix):
        ret = []
        for k, v in self._internal_widgets.items():
            if k.startswith(prefix):
                ret.append(v)

        for widget in gtk.glade.XML.get_widget_prefix(self, prefix):
            if not self._internal_widgets.has_key(widget.get_name()):
                ret.append(widget)

        return ret

    def _widget_order(self, widget, other):
        if isinstance(widget, gtk.Window):
            return -1
        return 0

    def __init__(self, *args, **kwargs):
        gtk.glade.XML.__init__(self, *args, **kwargs)
        try:
            fname = args[0]
        except IndexError, e:
            fname = kwargs['fname']

        try:
            root = args[1]
        except IndexError, e:
            root = kwargs.get('root', '')

        try:
            self.app = args[4]
        except IndexError, e:
            if kwargs.has_key('app'):
                self.app = kwargs['app']
            else:
                self.app = hildon.App()

        # parse .xml to get widget and signals information
        self._xml_ui_managers = []
        self._xml_signals = []
        parser = make_parser()
        parser.setFeature(handler.feature_external_ges, False)
        parser.setFeature(handler.feature_external_pes, False)
        parser.setFeature(handler.feature_namespaces, False)
        parser.setFeature(handler.feature_validation, False)
        parser.setContentHandler(GladeParser(self._xml_signals, self._xml_ui_managers))
        parser.parse(fname)
        # /parse

        self._internal_widgets = {}
        self._to_destroy = []

        # Dialog widgets is a special case
        if isinstance(self.get_widget(root), gtk.Dialog):
            return

        # rearrange widgets in windows
        replace = {
            gtk.Window: self._replace_window,
            gtk.MenuBar: self._replace_menubar,
            gtk.Toolbar: self._replace_toolbar,
        }
        widgets = self.get_widget_prefix("")
        widgets.sort(self._widget_order)
        for widget in widgets:
            name = widget.get_name()
            cls = widget.__class__
            try:
                replace[cls](widget)
            except KeyError, e:
                pass 

        # set app title with the first appview
        for widget in self._internal_widgets.values():
            if isinstance(widget, hildon.AppView):
                self.app.set_appview(widget)
                self.app.set_title(widget.get_title())
                break

        # destroy old widgets from children to parent
        self._to_destroy.reverse()
        for widget in self._to_destroy:
            if isinstance(widget.get_parent(), gtk.Container):
                widget.get_parent().remove(widget)
            widget.destroy()
        del self._to_destroy

        self.app.show()

    def __repr__(self):
        return gtk.glade.XML.__repr__(self).replace("PyGladeXML", "HildonGladeXML")

    def signal_autoconnect(self, connections):
        if not isinstance(connections, dict):
            dic = {}
            for attr in dir(connections):
                if callable(getattr(connections, attr)):
                    dic[attr] = getattr(connections, attr)
            connections = dic

        for handler, func in connections.items():
            self.signal_connect(handler, func)

    def signal_connect(self, handler_name, func):
        for handler, widget_name, signal_name in self._xml_signals:
            if handler == handler_name:
                if not self._internal_signal_connect(widget_name, signal_name, func):
                    self.get_widget(widget_name).connect(signal_name, func)

    def _internal_signal_connect(self, widget, signal, func):
        try:
            self._internal_widgets[widget].connect(signal, func)
        except TypeError, e:
            if isinstance(self._internal_widgets[widget], hildon.AppView):
                try:
                    self._internal_widgets[widget].get_parent().connect(signal, func)
                except AttributeError, e:
                    raise HildonGladeError("AppView need a parent widget")
        except KeyError, e:
            return False
        return True

# vim:ts=4:sw=4:tw=120:et:smarttab:showmatch:si:ai
