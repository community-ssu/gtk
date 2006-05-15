import gettext

import gobject
import gtk

from gazpacho.choice import enum_to_string, flags_to_string

_ = gettext.gettext

UNICHAR_PROPERTIES = ('invisible-char', )

class PropertyCustomEditor(object):
    """
    Base class for editors for custom properties

    The create method is called the first time a widget with this property is
    shown in the editor. Then, everytime a instance of this type of widget is
    selected the 'update' method is called with the proper information to
    manipulate the widget.

    Because of that this should not store any information about the state of
    the widget that is being edited.
    """

    # if wide_editor is True the editor will expand both columns of the
    # editor page
    wide_editor = False

    def get_editor_widget(self):
        "Return the gtk widget that implements this editor"
        
    def update(self, context, gtk_widget, proxy):
        """
        Called when a widget with this property is selected

        @param    context: gazpacho context to get the current app and project
        @param gtk_widget: the widget we are editing
        @param      proxy: an object that should be used to update the widget.
                           This is what allows the undo/redo mechanism actually
                           works
        """

class PropertyError(Exception):
    pass

class PropRegistry:
    def __init__(self):
        self._prop_types = {}
        self._bases = {}

        # A list of custom properties
        self._custom = {}
        
        # Cache used by _list_properties, list of pspecs
        self._pspec_cache = {}
        
    def add_base_type(self, klass, gtype):
        if not issubclass(klass, PropType):
            raise TypeError("klass needs to be a subclass of PropType")
        
        if gtype in self._bases:
            raise PropertyError("base type %r is already defined" % gtype)
        elif klass.base_type != None:
            raise PropertyError("klass type %r is already registered to %r" %
                                (klass, klass.base_type))
        
        self._bases[gtype] = klass
        klass.base_type = gtype
        
    def override_property(self, full_name, klass, child=False):
        """
        Create a property for full_name, (type::prop_name)
        If it doesn't exist add a custom property
        @param klass property class
        @type  klass subclass of PropType
        @param full_name complete name of property 
        @type  full_name string (typename::prop_name)
        """

        if not issubclass(klass, PropType):
            raise TypeError("klass needs to be a subclass of PropType")
        
        type_name, prop_name = full_name.split('::')
        pspec = self._get_pspec(type_name, prop_name)

        # If we can't find the property in the GType, it's a custom type
        if not self._has_property(type_name, prop_name):
            self._add_custom_type(klass, type_name, prop_name, pspec)
        else:
            self._add_type(full_name, klass, pspec)

        # Note, that at this point we need to setup the overriden class
        # since we're not going to call the type constructor, so
        # this needs to be in sync with PropMeta.new

        klass.name = prop_name
        klass.owner_name = type_name
        klass.owner_type = gobject.type_from_name(type_name)
        klass.child = child
        
        if not issubclass(klass.editor, PropertyCustomEditor):
            raise TypeError("editor %r for property class %r needs to be a "
                            "subclass of PropertyCustomEditor " % (
                klass.editor, klass))

    def override_simple(self, full_name, bases=(), **kwargs):
        """
        Same as override_property, but creates and registers the class
        @param full_name complete name of property
        @param bases tuple of bases
        @param kwargs namespace to use in class
        """

        if not bases:
            bases = CustomProperty,
        elif type(bases) != tuple:
            bases = bases,
            
        type_name, prop_name = full_name.split('::')
        pspec = self._get_pspec(type_name, prop_name)
        if not pspec:
            raise TypeError("You can only override existing properties, %s "
                            "is not a property" % full_name)

        bases += self._get_base_type(pspec.value_type),
        klass = type(full_name, bases, kwargs)
        self.override_property(full_name, klass)
        
    def list(self, type_name, parent=None):
        """
        List all the properties for type_name
        If parent is specified, child properties will also be included.
        This method will create the prop_types and save if they're
        not already created.
        @param type_name name of type
        @type  type_name string
        @param parent    parent type of object
        @type  parent    GType
        """

        property_types = []
        for pspec in self._list_properties(type_name):
            property_types.append(self._get_type_from_pspec(pspec))

        if parent:
            parent_types = self._list_child_properties(parent)
            property_types.extend(parent_types)
            
        property_types.extend(self._list_custom(type_name))
        
        for prop_type in property_types[:]:
            if (not prop_type.readable or not prop_type.writable):
                property_types.remove(prop_type)

        return property_types

    def get_prop_type(self, type_name, prop_name, parent=None):
        """
        Fetch a property from type_name called prop_name, note that it
        does not create the property type if it's not already created.
        @param type_name gtype name of owner
        @type  type_name string
        @param prop_name name of property 
        @type  prop_name string
        """
        pspec = self._get_pspec(type_name, prop_name, parent)
        if not pspec:
            prop_type = self._get_custom(type_name, prop_name)
            if not prop_type:
                raise PropertyError("not created: %s::%s" % (type_name,
                                                             prop_name))
            return prop_type

        child = False
        if pspec in self._list_child_properties(parent):
            child = True
                                                
        return self._get_type_from_pspec(pspec, child=child)
            
    def _get_type_from_pspec(self, pspec, child=False):
        full_name = gobject.type_name(pspec.owner_type) + '::' + pspec.name
        if full_name in self._prop_types:
            return self._prop_types[full_name]
        
        return self._create_type_from_pspec(pspec, child=child)

    def _get_base_type(self, value_type):
        if gobject.type_is_a(value_type, gobject.TYPE_OBJECT):
            base_type = gobject.TYPE_OBJECT
        elif gobject.type_is_a(value_type, gobject.TYPE_ENUM):
            base_type = gobject.TYPE_ENUM 
        elif gobject.type_is_a(value_type, gobject.TYPE_FLAGS):
            base_type = gobject.TYPE_FLAGS
        elif gobject.type_is_a(value_type, gobject.TYPE_BOXED):
            base_type = gobject.TYPE_BOXED
        else:
            base_type = value_type

        base = self._bases.get(base_type)
        if not base:
            raise PropertyError("Can't find base type for %r" % base_type)
        return base
        
    def _create_type_from_pspec(self, pspec, child=False):
        base = self._get_base_type(pspec.value_type)
        
        readable = pspec.flags & gobject.PARAM_READABLE != 0
        writable = pspec.flags & gobject.PARAM_WRITABLE != 0
        prop_type = PropMeta.new(gobject.type_name(pspec.owner_type),
                                 pspec.name, base,
                                 pspec.value_type,
                                 pspec.owner_type,
                                 readable=readable,
                                 writable=writable,
                                 child=child,
                                 label=pspec.nick)
        self._add_type(prop_type.__name__, prop_type, pspec)
        return prop_type

    def _add_type(self, prop_name, prop_type, pspec=None):
        if pspec:
            if gobject.type_is_a(pspec.value_type, gobject.TYPE_ENUM):
                prop_type.enum_class = pspec.enum_class
            elif gobject.type_is_a(pspec.value_type, gobject.TYPE_FLAGS):
                prop_type.flags_class = pspec.flags_class

            prop_type.label = pspec.nick
            prop_type.type = pspec.value_type
        else:
            if not prop_type.label:
                prop_type.label = prop_name.capitalize()

        self._prop_types[prop_name] = prop_type
    
    def _add_custom_type(self, klass, type_name, prop_name, pspec):
        if not type_name in self._custom:
            custom = self._custom[type_name] = []
        else:
            custom = self._custom[type_name]
        if prop_name in custom:
            return
        klass.custom = True
        custom.append(prop_name)
        self._add_type(type_name + '::' + prop_name, klass, pspec)

    def _get_custom(self, type_name, prop_name):
        """
        Get custom properties from type_name, called prop_name
        """
        for prop_type in self._list_custom(type_name):
            if prop_type.name == prop_name:
                return prop_type

    def _list_custom(self, type_name):
        """
        List custom properties for type_name, check the type and
        all the parent types
        """
        retval = []
        while True:
            for prop_name in self._custom.get(type_name, []):
                full_name = type_name + '::' + prop_name
                retval.append(self._prop_types[full_name])

            if type_name == 'GObject':
                break

            type_name = gobject.type_name(gobject.type_parent(type_name))
            
        return retval

    def _get_pspec(self, type_name, prop_name, parent=None):
        for pspec in self._list_properties(type_name):
            if pspec.name == prop_name:
                return pspec

        if parent:
            for pspec in self._list_child_properties(parent):
                if pspec.name == prop_name:
                    return pspec
        
    def _has_property(self, type_name, prop_name):
        "Checks if  type_name has a property with name prop_name"
        return self._get_pspec(type_name, prop_name) != None
    
    def _list_properties(self, type_name):
        "List all properties for type_name, and use a simple cache"
        pspecs = self._pspec_cache.get(type_name, [])
        if pspecs:
            return pspecs
        else:
            try:
                pspecs = gobject.list_properties(type_name)
            except TypeError:
                raise TypeError("Unknown GType name: %s" % type_name)
            pspecs = list(pspecs)
            
        self._pspec_cache[type_name] = pspecs

        try:
            parent_type = gobject.type_parent(type_name)
            parent_pspecs = gobject.list_properties(parent_type)
        except RuntimeError:
            return pspecs
        
        return pspecs
    
    def _list_child_properties(self, parent):
        "List all the child properties for parent"
        if not parent:
            return []

        retval = []
        parent_type = parent.__gtype__
        for pspec in gtk.container_class_list_child_properties(parent_type):
            prop_type = self._create_type_from_pspec(pspec, child=True)
            retval.append(prop_type)
        return retval

class PropMeta(type):
    def new(meta, owner_name, name, base, value_type, owner_type,
            readable=True, writable=True, child=False, label=None):

        if not readable:
            getter = PropType._value_not_readable
        elif child:
            getter = base._child_get
        else:
            getter = base._get
            
        if not writable:
            setter = PropType._value_not_writable
        elif child:
            setter = base._child_set
        else:
            setter = base._set

        cls = meta(owner_name + '::' + name, (base,), {})
        cls.name =  name
        cls.owner_name = owner_name
        cls.type = value_type
        cls.owner_type = owner_type
        cls.value = property(getter, setter)
        cls.readable = readable
        cls.writable = writable
        cls.set = setter
        cls.get = getter
        cls.child = child

        # XXX: tooltip
        if not label:
            label = name.capitalize()
        cls.label = label
        
        return cls
    new = classmethod(new)
        
    def __repr__(cls):
        child = ''
        if cls.child:
            child = ' (child)'
        return '<PropType %s%s>' % (cls.__name__, child)
        
class PropType(object):
    __metaclass__ = PropMeta
    # name of the property, eg expand
    name = None
    # owner gobject name: GtkWindow
    owner_name = None
    # type, GType of property
    type = None
    # type of owner
    owner_type = None
    # value property
    value = None
    readable = True
    writable = True
    set = None
    get = None
    child = False
    enabled = True
    translatable = False
    cdata = False
    label = None
    default = None
    custom = False
    editable = True
    base_type = None
    editor = PropertyCustomEditor
    # Should we save the property?
    persistent = True
    def __init__(self, widget):
        self.widget = widget
        self._project = widget.project

        object = widget.gtk_widget
        # If we're a child property we're actually modifying the
        # parent
        if self.child:
            self._child = object
            self._object = object.get_parent()
        else:
            if not gobject.type_is_a(object, self.owner_type):
                raise TypeError("Expected an object of type %s, got %r" % (
                    gobject.type_name(self.owner_type), object))
            
            self._child = None
            self._object = object
            
        self.load()

    def load(self):
        parent_name = None
        if self.child:
            parent_name = gobject.type_name(self._object)
            
        # use the gtk value if this is an internal child
        if self.widget.internal_name or self.widget.maintain_gtk_properties:
            self._initial = self.widget.adaptor.get_default_prop_value(self, parent_name)
            # we have to initialize _value from the gtk widget
            if self.custom:
                self._value = self._initial
            else:
                if self.child:
                    self._value = self._object.child_get_property(self._child,
                                                                  self.name)
                else:
                    # FIXME: This is broke, but needed for GtkRadioButton::group,
                    # see tests.test_glade.GladeTest.test_radiobutton
                    try:
                        self._value = self._object.get_property(self.name)
                    except TypeError:
                        self._value = None
        else:
            if self.custom:
                self._initial = self.default
                self.set(self.default)
            else:
                self._initial = self.widget.adaptor.get_default_prop_value(self, parent_name)

    def get_project(self):
        return self._project
    
    def get_object_name(self):
        if isinstance(self._object, gtk.Widget):
            return self._object.name
        
        return repr(self._object)

    def _value_not_readable(self):
        raise PropertyError("%s not readable" % self.name)
    
    def _value_not_writable(self, value):
        raise PropertyError("%s not writable" % self.name)

    def _child_get(self):
        try:
            return self._object.child_get_property(self._child,
                                                   self.name)
        except TypeError:
            raise TypeError('Failed to get child property from %r for %s '
                            '(%s:%s)' % (self._object,
                                         self._child,
                                         gobject.type_name(self.owner_type),
                                         self.name))
        
    def _child_set(self, value):
        try:
            self._object.child_set_property(self._child,
                                            self.name,
                                            value)
        except TypeError:
            raise TypeError('Failed to set property %s:%s to %s' % (
                gobject.type_name(self.owner_type), self.name, value))
                

    def _get(self):
        return self._object.get_property(self.name)

    def _set(self, value):
        self._object.set_property(self.name, value)

    def get_default(cls, gobj):
        if cls.custom:
            return cls.default
        
        return gobj.get_property(cls.name)
        
    def is_translatable(self):
        """
        Is the property translatable?
        By default we're using a class variable, but in
        some cases we want to be able to check the instance
        state and check if we want translatation, eg GtkButton::label,
        in those cases, just override this method
        """
        return self.translatable
    
    def save(self):
        """
        Marshal the property into a string
        Return None if it already contains the default value
        """
        value = self.get()
        if value == None or value == self._initial:
            return
        
        return str(value)
        
    def __repr__(self):
        return '<Property %s:%s value=%r>' % (self.owner_name, self.name,
                                              self.get())

class CustomProperty(PropType):

    def get(self):
        return self._get()

    def set(self, value):
        self._set(value)
        
    value = property(lambda self: self.get(),
                     lambda self, value: self.set(value))
                
class TransparentProperty(CustomProperty):
    """
    A transparent property is a property which does not
    update the property of the GObject, instead it keeps track
    of the value using other means.
    For instance GtkWindow::modal is a transparent property, since
    we do not want to make the window we editing modal.
    """
    def __init__(self, *args):
        super(TransparentProperty, self).__init__(*args)
        self._value = self._initial
        
    def get(self):
        return self._value
    
    def set(self, value):
        self._value = value
        
prop_registry = PropRegistry()

class BooleanType(PropType):
    default = False
prop_registry.add_base_type(BooleanType, gobject.TYPE_BOOLEAN)

class PointerType(PropType):
    pass
prop_registry.add_base_type(PointerType, gobject.TYPE_POINTER)

class StringType(PropType):
    default = ''
    multiline = False
    translatable = True
    has_i18n_context = True
    i18n_comment = ''
    engineering_english = ''
prop_registry.add_base_type(StringType, gobject.TYPE_STRING)

class ObjectType(PropType):
    editable = False

    def save(self):
        value = self.get()
        if value:
            return value.get_name()
    
prop_registry.add_base_type(ObjectType, gobject.TYPE_OBJECT)

#
# Numbers
#

class BaseNumericType(PropType):
    """
    Base type class used for all numeric types,
    The additional class variables here are used to create a
    GtkAlignment for the editors
    """
    default = 0
    # GtkAdjustment
    minimum = 0
    maximum = 0
    step_increment = 1
    page_increment = 10

    # SpinButton
    # acceleration factor
    climb_rate = 1
    
    # number of decimals
    digits = 0
    
    def get_adjustment(cls):
        if not cls.minimum <= cls.default <= cls.maximum:
            raise TypeError("default must be larger than minimum and lower "
                            "than maximum (%r <= %r <= %r)" % (
                cls.minimum, cls.default, cls.maximum))
            
        return gtk.Adjustment(value=cls.default,
                              lower=cls.minimum,
                              upper=cls.maximum,
                              step_incr=cls.step_increment,
                              page_incr=cls.page_increment)
    get_adjustment = classmethod(get_adjustment)
    
class BaseFloatType(BaseNumericType):
    """
    Base float type, used by float and double
    """
    default = 0.0
    minimum = -10.0**308
    maximum = 10.0**308
    digits = 2
    
class IntType(BaseNumericType):
    minimum = -int(2**31-1)
    maximum = int(2**31-1)
prop_registry.add_base_type(IntType, gobject.TYPE_INT)

class UIntType(BaseNumericType):
    maximum = 2**32-1
prop_registry.add_base_type(UIntType,  gobject.TYPE_UINT)

class FloatType(BaseFloatType):
    pass
prop_registry.add_base_type(FloatType, gobject.TYPE_FLOAT)

class DoubleType(BaseFloatType):
    pass
prop_registry.add_base_type(DoubleType, gobject.TYPE_DOUBLE)


#
# Enum & Flags
#

class EnumType(PropType):
    enum_class = None
    def get_choices(cls):
        choices = []
        for value, enum in cls.enum_class.__enum_values__.items():
            choices.append((enum.value_nick, enum))
        choices.sort()
        return choices
    get_choices = classmethod(get_choices)
    
    def save(self):
        value = self.get()
        if value == self._initial:
            return
        
        return enum_to_string(value, enum=self.enum_class)

prop_registry.add_base_type(EnumType, gobject.TYPE_ENUM)

class FlagsType(PropType):
    flags_class = None
    def get_choices(cls):
        flags = [(flag.first_value_name, mask)
                     for mask, flag in cls.flags_class.__flags_values__.items()
                         # we avoid composite flags
                         if len(flag.value_names) == 1]
        flags.sort()
        return flags
    get_choices = classmethod(get_choices)
    
    def save(self):
        value = self.get()
        if value == self._initial or value == 0:
            return
        return flags_to_string(value, flags=self.flags_class)
    
prop_registry.add_base_type(FlagsType, gobject.TYPE_FLAGS)

class BoxedType(PropType):
    pass
prop_registry.add_base_type(BoxedType, gobject.TYPE_BOXED)

class CharType(PropType):
    default = '\0'
prop_registry.add_base_type(CharType, gobject.TYPE_CHAR)

class AdjustmentType(TransparentProperty):
    def save(self):
        adj = self.get()
        s = [] 
        for value in (adj.value, adj.lower, adj.upper,
                      adj.step_increment, adj.page_increment,
                      adj.page_size):
            # If it's a float which ends with .0, convert to an int,
            # so we can avoid saving unnecessary decimals
            if str(value).endswith('.0'):
                value = int(value)
            s.append(str(value))
        return ' '.join(s)
prop_registry.add_base_type(AdjustmentType, gtk.Adjustment)

class ProxiedProperty(TransparentProperty):
    target_name = None
    
    def get_target(self):
        raise NotImplementedError
    
    def get(self):
        target = self.get_target()
        return target.get_property(self.target_name)
        
    def set(self, value):
        target = self.get_target()
        return target.set_property(self.target_name, value)

# TODO
#   TYPE_INT64
#   TYPE_LONG
#   TYPE_UCHAR
#   TYPE_UINT64
#   TYPE_ULONG
#   TYPE_UNICHAR


if __name__ == '__main__':
    win = gtk.Window()
    box = gtk.HBox()
    win.add(box)
    button = gtk.Button()
    box.pack_start(button)

    from gazpacho.catalog import load_catalogs
    load_catalogs()

    for ptype in prop_registry.list('GtkButton', gtk.VBox):
        print ptype, ptype.value

    #for ptype in prop_registry.list('GtkHBox', gtk.Window):
    #    #if not ptype.child:
    #    #    continue
    #    prop = ptype(box, None)
    #    if prop.readable:
    #        print ptype, prop.value
        
    raise SystemExit

