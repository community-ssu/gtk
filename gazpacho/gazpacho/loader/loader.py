# Copyright (C) 2005 Johan Dahlin
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

# TODO:
# Parser tags: atk, relation
# Document public API
# Parser subclass
# Improved unittest coverage
# Old style toolbars
# Require importer/resolver (gazpacho itself)
# GBoxed properties

import os
from gettext import textdomain, dgettext
from xml.parsers import expat

import gobject
import gtk
from gtk import gdk

from gazpacho.loader.custom import adapter_registry, flagsfromstring, \
     str2bool

__all__ = ['ObjectBuilder', 'ParseError']

def odd(x):
    return bool(x & 1)

class ParseError(Exception):
    pass

class Stack(list):
    push = list.append
    def peek(self):
        if self:
            return self[-1]

class BaseInfo:
    def __init__(self):
        self.data = ''

    def __repr__(self):
        return '<%s data=%r>' % (self.__class__.__name__,
                                 self.data)
        
class WidgetInfo(BaseInfo):
    def __init__(self, attrs, parent):
        BaseInfo.__init__(self)
        self.klass = str(attrs.get('class'))
        self.id = str(attrs.get('id'))
        self.constructor  = attrs.get('constructor')
        self.children = []
        self.properties = []
        self.signals = []
        self.uis = []
        self.accelerators = []
        self.parent = parent
        self.gobj = None
        # If it's a placeholder, used by code for unsupported widgets
        self.placeholder = False
        
    def is_internal_child(self):
        return self.parent and self.parent.internal_child

    def __repr__(self):
        return '<WidgetInfo of type %s>' % (self.klass)
    
class ChildInfo(BaseInfo):
    def __init__(self, attrs, parent):        
        BaseInfo.__init__(self)
        self.internal_child = attrs.get('internal-child')
        self.properties = []
        self.packing_properties = []
        self.placeholder = False
        self.parent = parent
        self.widget = None
        
    def __repr__(self):
        return '<ChildInfo containing a %s>' % (self.widget)
    
class PropertyInfo(BaseInfo):
    def __init__(self, attrs):
        BaseInfo.__init__(self)
        self.name = str(attrs.get('name'))
        self.translatable = str2bool(attrs.get('translatable', 'no'))
        self.context = str2bool(attrs.get('context', 'no'))
        self.agent = attrs.get('agent') # libglade
        self.i18n_comment = attrs.get('comment')
        self.engineering_english = attrs.get('engineering_english')
        
    def __repr__(self):
        return '<PropertyInfo of type %s=%r>' % (self.name, self.data)

class SignalInfo(BaseInfo):
    def __init__(self, attrs):
        BaseInfo.__init__(self)
        self.name = attrs.get('name')
        self.handler = attrs.get('handler')
        self.after = str2bool(attrs.get('after', 'no'))
        self.object = attrs.get('object')
        self.last_modification_time = attrs.get('last_modification_time')
        self.gobj = None

class AcceleratorInfo(BaseInfo):
    def __init__(self, attrs):
        BaseInfo.__init__(self)
        self.key = gdk.keyval_from_name(attrs.get('key'))
        self.modifiers = flagsfromstring(attrs.get('modifiers'),
                                         flags=gdk.ModifierType)
        self.signal = str(attrs.get('signal'))
  
class UIInfo(BaseInfo):
    def __init__(self, attrs):
        BaseInfo.__init__(self)
        self.id = attrs.get('id')
        self.filename = attrs.get('filename')
        self.merge = str2bool(attrs.get('merge', 'yes'))

class ExpatParser(object):
    def __init__(self, domain):
        self._domain = domain
        self.requires = []
        self._stack = Stack()
        self._state_stack = Stack()
        self._parser = expat.ParserCreate()
        self._parser.buffer_text = True
        self._parser.StartElementHandler = self._handle_startelement
        self._parser.EndElementHandler = self._handle_endelement
        self._parser.CharacterDataHandler = self._handle_characterdata
        self._parser.CommentHandler = self._handle_comment
       
        self.extra_strings = {}
        self.extra_plural_strings = {}

    # Public API
    def parse_file(self, filename):
        fp = open(filename)
        self._parser.ParseFile(fp)
        
    def parse_stream(self, buffer):
        self._parser.Parse(buffer)

    # Expat callbacks
    def _handle_startelement(self, name, attrs):
        self._state_stack.push(name)
        name = name.replace('-', '_')
        func = getattr(self, '_start_%s' % name, None)
        if func:
            item = func(attrs)
            self._stack.push(item)
            
    def _handle_endelement(self, name):
        self._state_stack.pop()
        name = name.replace('-', '_')
        func = getattr(self, '_end_%s' % name, None)
        if func:
            item = self._stack.pop()
            func(item)
        
    def _handle_characterdata(self, data):
        info = self._stack.peek()
        if info:
            info.data += str(data)

    def _handle_comment(self, data):
        lines = data.split('\n')

        if lines and lines[0] == ' EXTRA_STRINGS':
            lines.remove(lines[0])
            for line in lines:
                words = line.split('\"')
                if len(words) == 5:
                    self.extra_strings[words[1]] = words[3]
        
        elif lines and lines[0] == ' EXTRA_PLURAL_STRINGS':
            lines.remove(lines[0])
            for line in lines:
                words = line.split('\"')
                if len(words) >= 5 and odd(len(words)):
                    value = []
                    for i in range(3, len(words)):
                        if odd(i):
                           value.append(words[i])
                        
                    self.extra_plural_strings[words[1]] = value

    # Tags
    def _start_glade_interface(self, attrs):
        # libglade extension, add a domain argument to the interface
        if 'domain' in attrs:
            self._domain = str(attrs['domain'])
        
    def _end_glade_interface(self, obj):
        pass
    
    def _start_requires(self, attrs):
        self.requires.append(attrs)
        
    def _end_requires(self, obj):
        pass
    
    def _start_signal(self, attrs):
        if not 'name' in attrs:
            raise ParseError("<signal> needs a name attribute")
        if not 'handler' in attrs:
            raise ParseError("<signal> needs a handler attribute")
        return SignalInfo(attrs)

    def _end_signal(self, signal):
        obj = self._stack.peek()
        obj.signals.append(signal)

    def _start_widget(self, attrs):
        if not 'class' in attrs:
            raise ParseError("<widget> needs a class attribute")
        if not 'id' in attrs:
            raise ParseError("<widget> needs an id attribute")
        
        return WidgetInfo(attrs, self._stack.peek())
    _start_object = _start_widget
    
    def _end_widget(self, obj):
        obj.parent = self._stack.peek()

        if not obj.gobj:
            obj.gobj = gobj = self._build_phase1(obj)

        self._build_phase2(obj)

        if obj.parent:
            obj.parent.widget = obj.gobj
            
    _end_object = _end_widget
        
    def _start_child(self, attrs):
        obj = self._stack.peek()
        obj.gobj = self._build_phase1(obj)

        return ChildInfo(attrs, parent=obj)

    def _end_child(self, child):
        obj = self._stack.peek()
        obj.children.append(child)
        
    def _start_property(self, attrs):
        if not 'name' in attrs:
            raise ParseError("<property> needs a name attribute")
        return PropertyInfo(attrs)

    def _end_property(self, prop):
        if prop.agent and prop.agent not in ('libglade', 'gazpacho'):
            return

        if prop.translatable:
            #prop.data = dgettext(self._domain, prop.data)
            temp = prop.data
            prop.data = prop.engineering_english
            prop.engineering_english = temp 
        
        obj = self._stack.peek()
        
        property_type = self._state_stack.peek()
        if property_type == 'widget' or property_type == 'object':
            obj.properties.append(prop)
        elif property_type == 'packing':
            obj.packing_properties.append(prop)
        else:
            raise ParseError("property must be a node of widget or packing")

    def _start_ui(self, attrs):
        if not 'id' in attrs:
            raise ParseError("<ui> needs an id attribute")
        return UIInfo(attrs)
    
    def _end_ui(self, ui):
        if not ui.data or ui.filename:
            raise ParseError("<ui> needs CDATA or filename")
        
        obj = self._stack.peek()
        obj.uis.append(ui)

    def _start_placeholder(self, attrs):
        pass

    def _end_placeholder(self, placeholder):
        obj = self._stack.peek()
        obj.placeholder = True

    def _start_accelerator(self, attrs):
        if not 'key' in attrs:
            raise ParseError("<accelerator> needs a key attribute")
        if not 'modifiers' in attrs:
            raise ParseError("<accelerator> needs a modifiers attribute")
        if not 'signal' in attrs:
            raise ParseError("<accelerator> needs a signal attribute")
        obj = self._stack.peek()
        return AcceleratorInfo(attrs)
    
    def _end_accelerator(self, accelerator):
        obj = self._stack.peek()
        obj.accelerators.append(accelerator)

class ObjectBuilder:
    def __init__(self, filename='', buffer=None, root=None, placeholder=None,
                 custom=None, domain=None):
        if ((not filename and not buffer) or
            (filename and buffer)):
            raise TypeError("need a filename or a buffer")

        self._filename = filename
        self._buffer = buffer
        self._root = root
        self._placeholder = placeholder
        self._custom = custom
        
        self.toplevels = []
        # name -> GObject
        self._widgets = {}
        self._signals = []
        # GObject -> Constructor
        self._constructed_objects = {}
        # ui definition name -> UIMerge, see _mergeui
        self._uidefs = {}
        # ui definition name -> constructor name (backwards compatibility)
        self._uistates = {}
        self._tooltips = gtk.Tooltips()
        self._tooltips.enable()
        self._focus_widget = None
        self._default_widget = None
        self._toplevel = None
        self._accel_group = None
        self._delayed_properties = {}
        self._internal_children = {}
       
        self.extra_strings = {}
        self.extra_plural_strings = {}

        # If domain is not specified, fetch the default one by
        # calling textdomain() without arguments
        if not domain:
            domain = textdomain()
            
        self._parser = ExpatParser(domain)
        self._parser._build_phase1 = self._build_phase1
        self._parser._build_phase2 = self._build_phase2

        self.extra_strings = self._parser.extra_strings
        self.extra_plural_strings = self._parser.extra_plural_strings

        if filename:
            self._parser.parse_file(filename)
        elif buffer:
            self._parser.parse_stream(buffer)
        self._parse_done()
        
    def __len__(self):
        return len(self._widgets)

    def __nonzero__(self):
        return True

    # Public API

    def get_widget(self, widget):
        return self._widgets.get(widget)

    def get_widgets(self):
        return self._widgets.values()
    
    def signal_autoconnect(self, obj):
        for gobj, name, handler_name, after, object_name in self.get_signals():
            # Firstly, try to map it as a dictionary
            try:
                handler = obj[handler_name]
            except (AttributeError, TypeError):
                # If it fails, try to map it to an attribute
                handler = getattr(obj, handler_name, None)
                if not handler:
                    continue

            if object_name:
                other = self._widgets.get(object_name)
                if after:
                    gobj.connect_object_after(name, handler, other)
                else:
                    gobj.connect_object(name, handler, other)
            else:
                if after:
                    gobj.connect_after(name, handler)
                else:
                    gobj.connect(name, handler)

    def show_windows(self):
        # First set focus, warn if more than one is focused
        toplevel_focus_widgets = []
        for widget in self.get_widgets():
            if not isinstance(widget, gtk.Widget):
                continue
            
            if widget.get_data('gazpacho::is-focus'):
                toplevel = widget.get_toplevel()
                name = toplevel.get_name()
                if name in toplevel_focus_widgets:
                    print ("Warning: Window %s has more than one "
                           "focused widget" % name)
                toplevel_focus_widgets.append(name)

        # At last, display all of the visible windows
        for toplevel in self.toplevels:
            if not isinstance(toplevel, gtk.Window):
                continue
            value = toplevel.get_data('gazpacho::visible')
            toplevel.set_property('visible', value)
            
    def get_internal_children(self, gobj):
        if not gobj in self._internal_children:
            return []
        return self._internal_children[gobj]
    
    # Adapter API
    
    def add_signal(self, gobj, name, handler, after=False, sig_object=None):
        self._signals.append((gobj, name, handler, after, sig_object))

    def get_signals(self):
        return self._signals
    
    def find_resource(self, filename):
        dirname = os.path.dirname(self._filename)
        path = os.path.join(dirname, filename)
        if os.access(path, os.R_OK):
            return path
        
    def get_ui_definitions(self):
        return [(name, info.data) for name, info in self._uidefs.items()]
    
    def get_constructor(self, gobj):
        return self._constructed_objects[gobj]
    
    def ensure_accel(self):
        if not self._accel_group:
            self._accel_group = gtk.AccelGroup()
            if self._toplevel:
                self._toplevel.add_accel_group(self._accel_group)
        return self._accel_group

    def add_delayed_property(self, obj_id, pspec, value):
        delayed = self._delayed_properties
        if not obj_id in delayed:
            delayed_properties = delayed[obj_id] =[]
        else:
            delayed_properties = delayed[obj_id]

        delayed_properties.append((pspec, value))

    # Private
    
    def _setup_signals(self, gobj, signals):
        for signal in signals:
            self.add_signal(gobj, signal.name, signal.handler,
                           signal.after, signal.object)
            
    def _setup_accelerators(self, widget, accelerators):
        if not accelerators:
            return
        
        accel_group = self.ensure_accel()
        widget.set_data('gazpacho::accel-group', accel_group)
        for accelerator in accelerators:
            widget.add_accelerator(accelerator.signal,
                                   accel_group,
                                   accelerator.key,
                                   accelerator.modifiers,
                                   gtk.ACCEL_VISIBLE)

    def _apply_delayed_properties(self):
        for obj_id, props in self._delayed_properties.items():
            widget = self._widgets.get(obj_id)
            if widget is None:
                raise AssertionError
            
            adapter = adapter_registry.get_adapter(widget, self)

            prop_list = []
            for pspec, value in props:
                if gobject.type_is_a(pspec.value_type, gobject.GObject):
                    other = self._widgets.get(value)
                    if other is None:
                        raise ParseError(
                            "property %s of %s refers to widget %s which "
                            "does not exist" % (pspec.name, obj_id,value))
                    prop_list.append((pspec.name, other))
                else:
                    raise NotImplementedError(
                        "Only delayed object properties are "
                        "currently supported")

            adapter.set_properties(widget, prop_list)

    def _merge_ui(self, uimanager_name, name,
                  filename='', data=None, merge=True):
        uimanager = self._widgets[uimanager_name]
        if merge:
            if filename:
                filename = self.find_resource(filename)
                # XXX Catch GError
                merge_id = uimanager.add_ui_from_file(filename)
            elif data:
                # XXX Catch GError
                merge_id = uimanager.add_ui_from_string(data)
            else:
                raise AssertionError
        else:
            merge_id = -1

        class UIMerge:
            def __init__(self, uimanager, filename, data, merge_id):
                self.uimanager = uimanager,
                self.filename = filename
                self.data = data
                self.merge_id = merge_id
                         
        current = self._uidefs.get(name)
        if current:
            current.merge_id = merge_id
        else:
            self._uidefs[name] = UIMerge(uimanager, filename, data,
                                         merge_id)

        # Backwards compatibility
        self._uistates[name] = uimanager_name
        
    def _uimanager_construct(self, uimanager_name, obj_id):
        uimanager = self._widgets[uimanager_name]
        
        widget = uimanager.get_widget('ui/' + obj_id)
        if widget is None:
            # XXX: untested code
            uimanager_name = self._uistates.get(obj_id)
            if not uimanager_name:
                raise AssertionError
            uimanager = self._widgets[uimanager_name]
        
        return widget
    
    def _find_internal_child(self, obj):
        child = None
        childname = str(obj.parent.internal_child)
        parent = obj.parent
        while parent:
            if isinstance(parent, ChildInfo):
                parent = parent.parent
                continue
            
            gparent = parent.gobj
            if not gparent:
                break

            adapter = adapter_registry.get_adapter(gparent, self)
            child = adapter.find_internal_child(gparent, childname)
            if child is not None:
                break
            
            parent = parent.parent

        if child is not None:
            if not gparent in self._internal_children:
                self._internal_children[gparent] = []
            self._internal_children[gparent].append((childname, child))

        return child

    def _create_custom(self, obj):
        kwargs = dict(name=obj.id)
        for prop in obj.properties:
            prop_name = prop.name
            if prop_name in ('string1', 'string2',
                             'creation_function',
                             'last_modification_time'):
                kwargs[prop_name] = prop.data
            elif prop_name in ('int1', 'int2'):
                kwargs[prop_name] = int(prop.data)

        if not self._custom:
            return gtk.Label('<Custom: %s>' % obj.id)
        elif callable(self._custom):
            func = self._custom
            return func(**kwargs)
        else:
            func_name = kwargs['creation_function']
            try:
                func = self._custom[func_name]
            except (TypeError, KeyError, AttributeError):
                func = getattr(self._custom, func_name, None)

            return func(name=obj.id,
                        string1=kwargs.get('string1', None),
                        string2=kwargs.get('string2', None),
                        int1=kwargs.get('int1', None),
                        int2=kwargs.get('int2', None))
            
    def _create_placeholder(self, obj=None):
        if not obj:
            klass = name = 'unknown'
        else:
            name = obj.id
            klass = obj.klass
            
        if not self._placeholder:
            return

        return self._placeholder(name)
        
    def _build_phase1(self, obj):
        root = self._root
        if root and root != obj.id:
            return

        if obj.klass == 'Custom':
            return self._create_custom(obj)
        
        try:
            gtype = gobject.type_from_name(obj.klass)
        except RuntimeError:
            print 'Could not construct object: %s' % obj.klass
            obj.placeholder = True
            return self._create_placeholder(obj)

        adapter = adapter_registry.get_adapter(gtype, self)
        construct, normal = adapter.get_properties(gtype,
                                                   obj.id,
                                                   obj.properties)
        if obj.is_internal_child():
            gobj = self._find_internal_child(obj)
        elif obj.constructor:
            if self._widgets.has_key(obj.constructor):
                gobj = self._uimanager_construct(obj.constructor, obj.id)
                constructor = obj.constructor
            # Backwards compatibility
            elif self._uistates.has_key(obj.constructor):
                constructor = self._uistates[obj.constructor]
                gobj = self._uimanager_construct(constructor, obj.id)
            else:
                raise ParseError("constructor %s for object %s could not "
                                 "be found" % (obj.id, obj.constructor))
            self._constructed_objects[gobj] = self._widgets[constructor]
        else:
            gobj = adapter.construct(obj.id, gtype, construct)

        if gobj:
            self._widgets[obj.id] = gobj

            adapter.set_properties(gobj, normal)

            # This is a little tricky
            # We assume the default values for all these are nonzero, eg
            # either False or None
            # We also need to handle the case when we have two labels, if we
            # do we respect the first one. This is due to a bug in the save code
            for propinfo in obj.properties:
                key = 'i18n_is_translatable_%s' % propinfo.name
                if not gobj.get_data(key) and propinfo.translatable:
                    gobj.set_data(key, propinfo.translatable)

                key = 'i18n_has_context_%s' % propinfo.name
                if not gobj.get_data(key) and propinfo.context:
                    gobj.set_data(key, propinfo.context)

                # XXX: Rename to i18n_comments
                key = 'i18n_comment_%s' % propinfo.name
                if not gobj.get_data(key) and propinfo.i18n_comment:
                    gobj.set_data(key, propinfo.i18n_comment)
                
                key = 'engineering_english_%s' % propinfo.name
                if not gobj.get_data(key) and propinfo.engineering_english:
                    gobj.set_data(key, propinfo.engineering_english)
                    key = 'hacked_%s' % propinfo.name
                    gobj.set_data(key, True)
                    
        return gobj
    
    def _build_phase2(self, obj):
        # If we have a root set, we don't want to construct all
        # widgets, filter out unwanted here
        root = self._root
        if root and root != obj.id:
            return
            
        # Skip this step for placeholders, so we don't
        # accidentally try to pack something into unsupported widgets
        if obj.placeholder:
            return
       
        gobj = obj.gobj
        if not gobj:
            return
        
        adapter = adapter_registry.get_adapter(gobj, self)
        
        for child in obj.children:
            self._pack_child(adapter, gobj, child)

        self._setup_signals(gobj, obj.signals)
        self._setup_accelerators(gobj, obj.accelerators)
        
        # Toplevels
        if not obj.parent:
            if isinstance(gobj, gtk.UIManager):
                for ui in obj.uis:
                    self._merge_ui(obj.id,
                                   ui.id, ui.filename, ui.data, ui.merge)
                    self.accelgroup = gobj.get_accel_group()
            elif isinstance(gobj, gtk.Window):
                self._set_toplevel(gobj)

            self.toplevels.append(gobj)
        
    def _pack_child(self, adapter, gobj, child):

        if child.placeholder:
            widget = self._create_placeholder()
            if not widget:
                return
        elif child.widget:
            widget = child.widget
        else:
            return

        if child.internal_child:
            gobj = child.parent.gobj
            name = child.parent.id
            if isinstance(gobj, gtk.Widget):
                gobj.set_name(name)
            self._widgets[name] = gobj
            return
        
        # 5) add child
        try:
            adapter.add(gobj,
                        widget,
                        child.packing_properties)
        except NotImplementedError, e:
            print TypeError('%s does not support children' % (
                gobject.type_name(gobj)))

    def _parse_done(self):
        self._apply_delayed_properties()
        self.show_windows()
        
    def _set_toplevel(self, window):
        if self._focus_widget:
            self._focus_widget.grab_focus()
            self._focus_widget = None
        if self._default_widget:
            if self._default_widget.flags() & gtk.CAN_DEFAULT:
                self._default_widget.grab_default()
            self._default_widget = None
        if self._accel_group:
            self._accel_group = None

        # the window should hold a reference to the tooltips object 
        window.set_data('gazpacho::tooltips', self._tooltips)
        self._toplevel = window

if __name__ == '__main__':
    import sys
    ob = ObjectBuilder(filename=sys.argv[1])
    for toplevel in ob.toplevels:
        if not isinstance(toplevel, gtk.Window):
            continue
        toplevel.connect('delete-event', gtk.main_quit)
        toplevel.show_all()
        
    gtk.main()
