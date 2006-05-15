import os
import tempfile

import gobject
import gtk
from gtk import keysyms
from twisted.trial import unittest

from gazpacho.loader.loader import ParseError, ObjectBuilder
from gazpacho.loader.custom import str2bool

def ui(uid, content):
    return '<ui id="%s"><![CDATA[%s]]></ui>' % (uid, content)

def widget(adaptor, id, props=(), children=[], ui=None,
           constructor=None, packprops=(), signals=[]):
    if constructor:
        extra = ' constructor="%s"' % constructor
    else:
        extra = ''
        
    s = '<widget class="%s" id="%s"%s>' % (adaptor, id, extra)
    for key, value in props:
        s += '<property name="%s">%s</property>' % (key, str(value))
    if children:
        for child in children:
            s += '<child>'
            s += child
            s += '</child>'
    if ui:
        s += ui

    if signals:
        s += signals
        
    s += '</widget>'
    if packprops:
        s += '<packing>'
        for key, value in packprops:
            s += '<property name="%s">%s</property>' % (key, str(value))

        s += '</packing>'

        
    return s
        
class BaseTest(unittest.TestCase):
    def openraw(self, data):
        return ObjectBuilder(buffer=data)

    def open(self, data, placeholder=None, root=None, custom=None):
        data = "<glade-interface>%s</glade-interface>" % data
        obj = ObjectBuilder(buffer=data,
                            root=root,
                            placeholder=placeholder,
                            custom=custom)
        self.failUnless(isinstance(obj, ObjectBuilder))
        return obj

    def check(self, ob, name, gtype):
        self.failUnless(ob != None)
        object = ob.get_widget(name)
        self.failUnless(object != None)
        self.failUnless(isinstance(object, gtype), object)
        return object
                                   
class SimpleLoaderTest(BaseTest):
    def testEmpty(self):
        self.assertRaises(TypeError, ObjectBuilder, buffer="")
        
    def testSimple(self):
        ob = self.openraw("<glade-interface></glade-interface>")
        self.assertEqual(len(ob), 0)

    def testRoot(self):
        ob = self.open(widget('GtkLabel', 'label') +
                       widget('GtkButton', 'button'), root='label')
        self.assertEqual(len(ob), 1)
        child = ob.get_widgets()[0]
        self.assertEqual(isinstance(child, gtk.Label), True)
        
class WidgetTests(BaseTest):
    def testWithoutClass(self):
        data = '<widget></widget>'
        self.assertRaises(ParseError, self.open, data)
        
    def testWithoutID(self):
        data = '<widget class="GtkWidget"></widget>'
        self.assertRaises(ParseError, self.open, data)

    def testBadConstructor(self):
        data = '<widget class="GtkWidget" id="a" constructor="b"></widget>'
        self.assertRaises(ParseError, self.open, data)

    def testGObject(self):
        ob = self.open(widget('GObject', 'obj'))
        self.assertEqual(len(ob), 1)
        self.check(ob, 'obj', gobject.GObject)

    def testLabel(self):
        ob = self.open(widget('GtkLabel', 'label'))
        self.assertEqual(len(ob), 1)
        obj = self.check(ob, 'label', gtk.Label)
        self.assertEqual(obj.get_name(), 'label')

class UIManagerTest(BaseTest):
    def testNew(self):
        uidata = """<ui>
<menubar action="menubar1" name="menubar1">
  <menu action="File" name="File">
    <menuitem action="New" name="New"/>
  </menu>
</menubar>
<toolbar action="toolbar1" name="toolbar1">
  <toolitem action="New" name="New"/>
</toolbar></ui>"""
        # ''' 
        
        group = widget('GtkActionGroup', 'group', {},
                       [widget('GtkAction', 'File',
                               [('name', 'File'), ('label', "_File")]),
                        widget('GtkAction', 'New',
                               [('name','New'), ('label', "_New")]),
                        ])
        uim = widget('GtkUIManager', 'uimanager1', {}, [group],
                     ui=ui('ui1', uidata))
        menubar = widget('GtkMenuBar', 'menubar1', constructor='uimanager1')
        window = widget('GtkWindow', 'window1',
                        props=[('type','toplevel')],
                        children=[menubar])
        ob = self.open(uim + window)

        self.assertEqual(len(ob), 6)
        menu = self.check(ob, 'menubar1', gtk.MenuBar)
        win = self.check(ob, 'window1', gtk.Window)
        self.assertEqual(win.get_property('type'), gtk.WINDOW_TOPLEVEL)
        self.assertEqual(menu.get_parent(), win)

    def testActionNoName(self):
        ob = self.open(widget('GtkAction', 'UnnamedAction'))
        action = self.check(ob, 'UnnamedAction', gtk.Action)
        self.assertEqual(action.get_name(), 'UnnamedAction')
        
class NotebookTest(BaseTest):
    def testChild(self):
        button = widget('GtkButton', 'button1')
        notebook = widget('GtkNotebook', 'notebook1',
                          children=[button])
        ob = self.open(notebook)
        notebook1 = self.check(ob, 'notebook1', gtk.Notebook)
        button1 = self.check(ob, 'button1', gtk.Button)
        self.failUnless(button1 in notebook1.get_children())
        
    def testTab(self):
        button = widget('GtkButton', 'button1')
        label = widget('GtkLabel', 'label1',
                       packprops=[('type', 'tab')])
        notebook = widget('GtkNotebook', 'notebook1',
                        children=[button, label])
        ob = self.open(notebook)
        notebook1 = self.check(ob, 'notebook1', gtk.Notebook)
        button1 = self.check(ob, 'button1', gtk.Button)
        label1 = self.check(ob, 'label1', gtk.Label)

        self.assertEqual(notebook1.get_tab_label(button1), label1)

    def testTabProperty(self):
        button = widget('GtkButton', 'button1',
                       packprops=[('tab-label', 'label')])
        notebook = widget('GtkNotebook', 'notebook1',
                          children=[button])
        ob = self.open(notebook)
        notebook1 = self.check(ob, 'notebook1', gtk.Notebook)
        button1 = self.check(ob, 'button1', gtk.Button)
        self.assertEqual(notebook1.get_tab_label_text(button1), 'label')
        
class FrameExpanderTest(BaseTest):
    def _testChild(self, gtype, name):
        button = widget('GtkButton', 'button1')
        obj = widget(gobject.type_name(gtype), name,
                       children=[button])
        ob = self.open(obj)
        obj1 = ob.get_widget(name)
        self.failUnless(isinstance(obj1, gtype))
        button1 = self.check(ob, 'button1', gtk.Button)
        self.failUnless(button1 in obj1.get_children())
        
    def _testTab(self, gtype, name):
        label = widget('GtkLabel', 'label1',
                       packprops=[('type','label_item')])
        obj = widget(gobject.type_name(gtype), name,
                     children=[label])
        ob = self.open(obj)
        obj1 = self.check(ob, name, gtype)
        label1 = self.check(ob, 'label1', gtk.Label)

        self.assertEqual(obj1.get_label_widget(), label1)

    def testFrame(self):
        self._testTab(gtk.Frame, 'frame1')
        self._testChild(gtk.Frame, 'frame1')
        
    def testExpander(self):
        self._testTab(gtk.Expander, 'expander1')
        self._testChild(gtk.Expander, 'expander1')

class RulerTest(BaseTest):
    def testMetric(self):
        ob = self.open(widget('GtkRuler', 'ruler1',
                              props=[('metric','pixels')]))
        ruler1 = self.check(ob, 'ruler1', gtk.Ruler)
        self.assertEqual(ruler1.get_metric(), gtk.PIXELS)
        
class CalendarTest(BaseTest):
    def testMetric(self):
        ob = self.open(widget('GtkCalendar', 'calendar1',
                              props=[('display_options','show-heading')]))
        calendar1 = self.check(ob, 'calendar1', gtk.Calendar)
        self.assertEqual(calendar1.get_display_options(),
                         gtk.CALENDAR_SHOW_HEADING)


class MenuItemTest(BaseTest):
    def testLabel(self):
        ob = self.open(widget('GtkMenuItem', 'menu1',
                              props=[('label','Label')]))
        menu1 = self.check(ob, 'menu1', gtk.MenuItem)
        self.assertEqual(len(menu1.get_children()), 1)
        child = menu1.get_children()[0]
        self.failUnless(isinstance(child, gtk.AccelLabel))
        self.failUnless(child.get_text(), 'Label')
        
    def testUseStock(self):
        ob = self.open(widget('GtkMenuItem', 'menu1',
                              props=(('label', gtk.STOCK_OPEN),
                                     ('use_stock', True))))
        menu1 = self.check(ob, 'menu1', gtk.MenuItem)
        self.assertEqual(len(menu1.get_children()), 1)
        child = menu1.get_children()[0]
        self.failUnless(isinstance(child, gtk.AccelLabel))
        self.failUnless(child.get_text(), gtk.STOCK_OPEN)
        
        self.open(widget('GtkImageMenuItem', 'menu1',
                         props=(('use_stock', False),)))
        
        ob = self.open(widget('GtkImageMenuItem', 'imagemenu1',
                              props=(('label', gtk.STOCK_OPEN),
                                     ('use_stock', True),)))
        imagemenu1 = self.check(ob, 'imagemenu1', gtk.ImageMenuItem)
        image = imagemenu1.get_image()
        self.failUnless(isinstance(image, gtk.Image))
        self.assertEqual(image.get_storage_type(), gtk.IMAGE_STOCK)
        self.assertEqual(image.get_stock(), (gtk.STOCK_OPEN,
                                             gtk.ICON_SIZE_MENU))
        
    def testUseUnderline(self):
        ob = self.open(widget('GtkMenuItem', 'menu1',
                              props=[('use_underline','True')]))
        menu1 = self.check(ob, 'menu1', gtk.MenuItem)
        self.assertEqual(len(menu1.get_children()), 1)
        child = menu1.get_children()[0]
        self.failUnless(isinstance(child, gtk.AccelLabel))
        self.failUnless(child.get_use_underline(), True)
        
    def testAddMenu(self):
        ob = self.open(widget('GtkMenuItem', 'menu1',
                              children=[widget('GtkMenu', 'menu2')]))
        menu1 = self.check(ob, 'menu1', gtk.MenuItem)
        self.assertEqual(len(menu1.get_children()), 0)
        menu2 = menu1.get_submenu()
        self.failUnless(isinstance(menu2, gtk.Menu))
        
class PropertyTest(BaseTest):
    def testWithoutName(self):
        data = '''<widget class="GtkWidget" id="widget">
           <property/>
        </widget>'''
        self.assertRaises(ParseError, self.open, data)

    def testBadParent(self):
        data = '''<property name="prop-name"/>'''
        self.assertRaises(ParseError, self.open, data)

    def testAlignment(self):
        ob = self.open(widget('GtkScrolledWindow', 'sw1',
                              props=[('vadjustment','1 2 3 4 5 6')]))
        sw1 = self.check(ob, 'sw1', gtk.ScrolledWindow)
        vadj = sw1.get_property('vadjustment')
        self.assertEqual(vadj.value, 1)
        self.assertEqual(vadj.lower, 2)
        self.assertEqual(vadj.upper, 3)
        self.assertEqual(vadj.step_increment, 4)
        self.assertEqual(vadj.page_increment, 5)
        self.assertEqual(vadj.page_size, 6)
        
    def testBoolean(self):
        ob = self.open(widget('GtkWindow', 'window1',
                              props=[('modal','True')]))
        window1 = self.check(ob, 'window1', gtk.Window)
        self.assertEqual(window1.get_modal(), True)
        
    def testDouble(self):
        adj = widget('GtkAdjustment', 'adj1',
                     props=[('lower','3.14')])
        ob = self.open(adj)
        adj1 = self.check(ob, 'adj1', gtk.Adjustment)
        self.assertEqual(adj1.get_property('lower'), 3.14)

    def testStr2bool(self):
        for value in ['YES', 'Yes', 'yes', 'y',
                      'TRUE', 'True', 'true', 't', '1']:
            self.assertEqual(str2bool(value), True, value)
            
        for value in ['NO', 'No', 'no', 'n',
                      'FALSE', 'False', 'false', 'f', '0']:
            self.assertEqual(str2bool(value), False, value)

    def testObject(self):
        ob = self.open(widget('GtkButton', 'button')  +
                       widget('GtkLabel', 'label',
                              props=[('mnemonic-widget', 'button')]))

        self.assertEqual(len(ob), 2)
        button = self.check(ob, 'button', gtk.Button)
        label = self.check(ob, 'label', gtk.Label)
        self.assertEqual(label.get_property('mnemonic-widget'), button)
        
    def testObjectAfter(self):
        ob = self.open(widget('GtkLabel', 'label',
                              props=[('mnemonic-widget', 'button')]) +
                       widget('GtkButton', 'button'))

        self.assertEqual(len(ob), 2)
        button = self.check(ob, 'button', gtk.Button)
        label = self.check(ob, 'label', gtk.Label)
        self.assertEqual(label.get_property('mnemonic-widget'), button)
        
    def testObjectNonexistent(self):
        data = widget('GtkLabel', 'label',
                      props=[('mnemonic-widget', 'invalid')])
        
        self.assertRaises(ParseError, self.open, data)
        
class Placeholder(gtk.Label):
    pass
gobject.type_register(Placeholder)

class PlaceHolderTest(BaseTest):
    def testEmpty(self):
        data = widget('GtkWindow', 'window1',
                      children=['<placeholder/>'])
        ob = self.open(data)
        window = self.check(ob, 'window1', gtk.Window)
        children = window.get_children()
        self.assertEqual(len(children), 0)

    def testPH(self):
        ob = self.open(widget('GtkWindow', 'window1',
                              children=['<placeholder/>']),
                       placeholder=Placeholder)
        window = self.check(ob, 'window1', gtk.Window)
        children = window.get_children()
        self.assertEqual(len(children), 1)
        self.failUnless(isinstance(children[0], Placeholder))

class Object:
    def __init__(self, obj, object_send=None):
        self.object = object_send or obj
        self.called_on_activate_focus = False
        self.called_after_activate_focus = False
        self.called_on_activate_default = False
        self.called_after_activate_default = False

        connect_args = ['activate-default',
                        self.on_activate_default]
        connect_after_args = ['activate-focus',
                              self.after_activate_focus]

        if object_send:
            connect = obj.connect_object
            connect_after = obj.connect_object_after
            connect_args.append(self.object)
            connect_after_args.append(self.object)
        else:
            connect = obj.connect
            connect_after = obj.connect_after
            
        connect(*connect_args)
        connect_after(*connect_after_args)

    # Manually connected
    def after_activate_focus(self, obj):
        assert obj == self.object
        self.called_after_activate_focus = True
        assert self.called_on_activate_focus
        
    def on_activate_default(self, obj):
        assert obj == self.object
        self.called_on_activate_default = True
        assert not self.called_after_activate_default

    # Connected by signal_autoconnect
    def on_activate_focus(self, obj):
        assert obj == self.object
        self.called_on_activate_focus = True
        assert not self.called_after_activate_focus
        
    def after_activate_default(self, obj):
        assert obj == self.object
        self.called_after_activate_default = True
        assert self.called_on_activate_default

class SignalTest(BaseTest):
    def testWithoutName(self):
        data = '''<widget class="GtkWidget" id="widget">
           <signal/>
        </widget>'''
        self.assertRaises(ParseError, self.open, data)
        
    def testWithoutHandler(self):
        data = '''<widget class="GtkWidget" id="widget">
           <signal name="signal-name"/>
        </widget>'''
        self.assertRaises(ParseError, self.open, data)

    def testAutoConnect(self):
        signal_data = ('<signal name="activate-focus" '
                               'handler="on_activate_focus"/>' +
                       '<signal name="activate-default" after="yes" ' +
                               'handler="after_activate_default"/>')
        
        ob = self.open(widget('GtkWindow', 'window1',
                              signals=signal_data))
        window = ob.get_widget('window1')
        obj = Object(window)
        ob.signal_autoconnect(obj)

        window.emit('activate-focus')
        window.emit('activate-default')
        self.assertEqual(obj.called_on_activate_focus, True)
        self.assertEqual(obj.called_after_activate_default, True)

    def testAutoConnectObject(self):
        signal_data = ('<signal name="activate-focus" ' 
                               'object="button1" '
                               'handler="on_activate_focus"/>'
                       '<signal name="activate-default" after="yes" '
                               'object="button1" '
                               'handler="after_activate_default"/>')
        
        ob = self.open(widget('GtkWindow', 'window1',
                              signals=signal_data,
                              children=[widget('GtkButton', 'button1')]))
        window = ob.get_widget('window1')
        obj = Object(window, ob.get_widget('button1'))
        ob.signal_autoconnect(obj)

        window.emit('activate-focus')
        window.emit('activate-default')
        self.assertEqual(obj.called_on_activate_focus, True)
        self.assertEqual(obj.called_after_activate_default, True)
        
    def testAutoConnectNonExisting(self):
        signal_data = ('<signal name="activate-focus" handler="foobar"/>')
        
        ob = self.open(widget('GtkWindow', 'window1',
                              signals=signal_data,
                              children=[widget('GtkButton', 'button1')]))
        class Foo:
            pass
        
        obj = Foo()
        ob.signal_autoconnect(obj)

        window = ob.get_widget('window1')
        window.emit('activate-focus')
        
class ComboBoxTest(BaseTest):
    def testItems(self):
        ob = self.open(widget('GtkComboBox', 'combobox1',
                              props=[('items','foo\nbar\nbaz')]))
        combobox1 = self.check(ob, 'combobox1', gtk.ComboBox)
        model = combobox1.get_model()
        self.assertEqual(len(model), 3, len(model))
        self.assertEqual(map(list, model), [['foo'], ['bar'], ['baz']])
        # This doesn't work, why?
        #self.assertEqual(combobox1.get_text_column(), 0)
    
class PixmapTest(BaseTest):
    def testBuildInsensitive(self):
        # Just run the code, hard to test
        self.open(widget('GtkPixmap', 'pixmap1',
                         props=[('build_insensitive','True')]))
        
    def testFilename(self):
        xpmdata  = '''/* XPM */
static char *%s[] = {
/* width height num_colors chars_per_pixel */
"    1    1        1            1",
/* colors */
". c #ffcc33",
/* pixels */
"."
};'''

        filename = tempfile.mktemp() + '.xpm'
        open(filename, 'w').write(xpmdata % filename)
        
        ob = self.open(widget('GtkPixmap', 'pixmap1',
                              props=[('filename', filename)]))
        os.unlink(filename)
        self.check(ob, 'pixmap1', gtk.Pixmap)

class TreeViewTest(BaseTest):
    def testBuild(self):
        ob = self.open(widget('GtkTreeView', 'treeview'))
        self.check(ob, 'treeview', gtk.TreeView)

    def testColumn(self):
        ob = self.open(widget('GtkTreeView', 'treeview',
                       children=[widget('GtkTreeViewColumn', 'column1'),
                                 widget('GtkTreeViewColumn', 'column2')]))
        treeview = self.check(ob, 'treeview', gtk.TreeView)
        column1 = self.check(ob, 'column1', gtk.TreeViewColumn)
        column2 = self.check(ob, 'column2', gtk.TreeViewColumn)
        self.assertEqual(treeview.get_columns(), [column1, column2])

    def testRenderers(self):
        ob = self.open(widget('GtkTreeView', 'treeview',
                       children=[widget('GtkTreeViewColumn', 'column',
                                        children=[
            widget('GtkCellRendererText', 'renderer')
            ])]))
        treeview = self.check(ob, 'treeview', gtk.TreeView)
        column = self.check(ob, 'column', gtk.TreeViewColumn)
        renderer = self.check(ob, 'renderer', gtk.CellRendererText)
        self.assertEqual(column.get_cell_renderers(), [renderer])

class InternalChildTest(BaseTest):
    def testDialog(self):
        ob = self.open("""<widget class="GtkDialog" id="dialog1">
  <child internal-child="vbox">
    <widget class="GtkButton" id="button1"/>
  </child>
</widget>""")
        dialog = self.check(ob, 'dialog1', gtk.Dialog)
        children = ob.get_internal_children(dialog)
        self.assertEqual(len(children), 1)
        name, vbox = children[0]
        self.assertEqual(name, "vbox")
        self.failUnless(isinstance(vbox, gtk.VBox))

    def testNonexisting(self):
        ob = self.open(widget('GtkDialog', 'dialog1'))
        dialog = self.check(ob, 'dialog1', gtk.Dialog)
        children = ob.get_internal_children(dialog)
        self.assertEqual(children, [])
    
class AcceleratorTest(BaseTest):
    def testBasic(self):
        ob = self.open("""<widget class="GtkWindow" id="window1">
  <accelerator key="F" modifiers="0" signal="grab_focus"/>
</widget>""")
        window = self.check(ob, 'window1', gtk.Window)
        accel_group = window.get_data('gazpacho::accel-group')
        window.remove_accelerator(accel_group, keysyms.F, 0)

class CustomTest(BaseTest):
    data = widget('Custom', 'custom1',
                  props=[('creation_function', 'func'),
                         ('string1', 'foo'),
                         ('string2', 'bar'),
                         ('int1', '10'),
                         ('int2', '20')])
    def setUp(self):
        self.ran = False
        
    def testCustom(self):
        ob = self.open(self.data, custom=self.func)
        self.assertEqual(self.ran, True)
        
    def testDict(self):
        d = {'func' : self.func}
        ob = self.open(self.data, custom=d)
        self.assertEqual(self.ran, True)
        
    def testInstance(self):
        ob = self.open(self.data, custom=self)
        self.assertEqual(self.ran, True)

    def func(self, name, string1, string2, int1, int2, **kwargs):
        self.assertEqual(name, 'custom1')
        self.assertEqual(string1, 'foo')
        self.assertEqual(string2, 'bar')
        self.assertEqual(int1, 10)
        self.assertEqual(int2, 20)
        self.ran = True
        
