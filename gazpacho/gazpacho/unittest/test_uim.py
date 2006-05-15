import os
import xml.dom

import gtk

from gazpacho.unittest import common
from gazpacho import placeholder, widget
from gazpacho.filewriter import XMLWriter
from gazpacho.project import GazpachoObjectBuilder
from gazpacho.widget import Widget

class FakeWidget(object):
    def __init__(self, name):
        self.name = name

class UIMTest(common.GazpachoTest):

    def setUp(self):
        common.GazpachoTest.setUp(self)

        # add a window
        self.window = self.create_gwidget('GtkWindow')
        self.project.add_widget(self.window.gtk_widget)

    def create_toolbar(self):
        placeholder = self.window.gtk_widget.get_child()
        toolbar = self.create_gwidget('GtkToolbar')
        Widget.replace(placeholder, toolbar.gtk_widget, self.window)
        self.project.add_widget(toolbar.gtk_widget)
        return toolbar
        
    def testCreateDefaultActions(self):
        self.project.uim.create_default_actions()
        
        # now we should have a bunch of actions inside an action group in the uim        
        action_groups = self.project.uim.uim.get_action_groups()
        self.assertEqual(len(action_groups), 1)

        ag = action_groups[0]
        
        self.assertEqual(ag.get_name(), 'DefaultActions')

        actions = ag.list_actions()

        # we should have 10 actions
        self.assertEqual(len(actions), 10)

    def testCreateDefaultActionsTwice(self):
        self.project.uim.create_default_actions()

        # add them again
        self.project.uim.create_default_actions()

        # we should only have one action group and 10 actions
        action_groups = self.project.uim.uim.get_action_groups()
        self.assertEqual(len(action_groups), 1)
        actions = action_groups[0].list_actions()
        self.assertEqual(len(actions), 10)
        
    def testRemoveActionGroup(self):
        self.project.uim.create_default_actions()

        gaction_group = self.project.uim.action_groups[0]

        self.project.uim.remove_action_group(gaction_group)

        action_groups = self.project.uim.uim.get_action_groups()

        self.assertEqual(len(action_groups), 0)

    def testCreate(self):
        self.project.uim.reset()
        self.assertEqual(isinstance(self.project.uim.uim, gtk.UIManager), True)

    def testGetUI(self):
        xml = """<toolbar action='toolbar1' name='toolbar1'/>"""
        gwidget = FakeWidget('toolbar1')

        self.project.uim.reset()

        self.project.uim.add_ui(gwidget, xml)

        xml_string, merge_id = self.project.uim.get_ui(gwidget, 'initial-state')

        self.assertEqual(xml_string, xml)
        self.assertEqual(merge_id > 0, True)

        # now, let's get all of them
        uis = self.project.uim.get_ui(gwidget)
        self.assertEqual(isinstance(uis, dict), True)
        self.assertEqual(len(uis.keys()), 1)
        self.assertEqual(uis.keys(), ['initial-state'])
        self.assertEqual(uis['initial-state'], (xml_string, merge_id))

    def testGetGtkWidget(self):
        xml = """<toolbar action='toolbar1' name='toolbar1'/>"""
        gwidget = FakeWidget('toolbar1')
        
        self.project.uim.reset()

        # we haven't added the ui yet
        self.assertEqual(self.project.uim.get_gtk_widget(gwidget), None)

        
        self.project.uim.add_ui(gwidget, xml)        

        gtk_widget = self.project.uim.get_gtk_widget(gwidget)
        self.assertEqual(isinstance(gtk_widget, gtk.Toolbar), True)

    def testUpdateUI(self):
        xml1 = """<toolbar action='toolbar1' name='toolbar1'/>"""
        gwidget = self.create_toolbar()
        old_gtk_widget = gwidget.gtk_widget
        old_parent = old_gtk_widget.get_parent()

        self.project.uim.update_ui(gwidget, xml1)

        new_gtk_widget = gwidget.gtk_widget
        new_parent = new_gtk_widget.get_parent()
        
        self.assertEqual(isinstance(new_gtk_widget, gtk.Toolbar), True)
        self.assertEqual(old_parent, new_parent)

    def testUpdateWidgetName(self):
        gwidget = self.create_toolbar()
        xml_string1, mi1 = self.project.uim.get_ui(gwidget, 'initial-state')

        gwidget.name = 'toolbar2'

        self.project.uim.update_widget_name(gwidget)

        xml_string2, mi2 = self.project.uim.get_ui(gwidget, 'initial-state')

        self.assertEqual(xml_string1.replace('toolbar1', 'toolbar2'),
                         xml_string2)

    def testSave(self):
        self.project.uim.create_default_actions()
        gwidget1 = FakeWidget('toolbar1')
        xml1 = """<toolbar action="toolbar1" name="toolbar1"/>"""
        gwidget2 = FakeWidget('menubar1')
        xml2 = """<menubar action="menubar1" name="menubar1"/>"""
        
        self.project.uim.add_ui(gwidget1, xml1)
        self.project.uim.add_ui(gwidget2, xml2)

        doc = xml.dom.getDOMImplementation().createDocument(None, None, None)
        
        node = self.project.uim.save(doc, [gwidget1, gwidget2])

        pathname = "%s.glade" % os.path.splitext(__file__)[0]
        
        # let's compare the good file with the one we just create now

        new_pathname = pathname + '.tmp'
        xw = XMLWriter()
        xw.write_node(new_pathname, node)

        data = file(pathname).read()

        new_data = file(new_pathname).read()
        
        self.assertEqual(data, new_data)

        os.unlink(new_pathname)

    def testLoad(self):
        pathname = os.path.join(os.path.dirname(__file__),
                                "uimanager-example.glade")
        wt = GazpachoObjectBuilder(filename=pathname,
                                   placeholder=placeholder.Placeholder)
        self.project.set_loader(wt)
        # load the uim
        self.project.uim.load(wt)

        # we should have again 10 actions and 1 action group
        action_groups = self.project.uim.uim.get_action_groups()
        self.assertEqual(len(action_groups), 1)

        ag = action_groups[0]
        
        self.assertEqual(ag.get_name(), 'DefaultActions')

        actions = ag.list_actions()

        self.assertEqual(len(actions), 10)        

        menubar1 = self.project.uim.loaded_uis['menubar1']
        toolbar1 = self.project.uim.loaded_uis['toolbar1']
        self.assertEqual(isinstance(menubar1, list), True)
        self.assertEqual(isinstance(toolbar1, list), True)
        self.assertEqual(len(menubar1), 1)
        self.assertEqual(len(toolbar1), 1)
        self.assertEqual(menubar1[0][0], 'initial-state')
        self.assertEqual(toolbar1[0][0], 'initial-state')
        self.assertEqual(isinstance(menubar1[0][1], basestring), True)
        self.assertEqual(isinstance(toolbar1[0][1], basestring), True)

        # there is no easy way to test load_widget because the gwidget
        # parameter to this function has to be a valid GazpachoWidget with
        # a real Toolbar/Menubar gtk_widget properly placed in the widget
        # hierarchy. So we just simulate the project.open behavior

        widgets = []
        for key, gtk_widget in wt._widgets.items():
            if isinstance(gtk_widget, gtk.Widget):
                if gtk_widget.flags() & gtk.TOPLEVEL:
                    self.assertEqual(gtk_widget.get_property('visible'), False)
                    widget.load_widget_from_gtk_widget(gtk_widget,
                                                       self.project)
                    widgets.append(gtk_widget)

        window = widgets[0]
        vbox = window.get_child()
        menubar = vbox.get_children()[0]
        self.assertEqual(isinstance(menubar, gtk.MenuBar), True)
        gwidget = widget.Widget.from_widget(menubar)
        self.assertEqual(menubar, self.project.uim.get_gtk_widget(gwidget))

    def testGroupUIDefinitions(self):
        self.project.uim.reset()

        gwidgets = []
        xml_strings = []
        for i in range(10):
            gwidgets.append(FakeWidget('toolbar%d' % i))
            xml = "<toolbar action='toolbar%d' name='toolbar%d'/>" % (i, i)
            xml_strings.append(xml)
            self.project.uim.add_ui(gwidgets[i], xml)

        uis = self.project.uim._group_ui_definitions_by_name(gwidgets[2:7])

        for name, strings in uis.items():
            strings.sort()
            uis[name] = strings
            
        expected_result = {'initial-state' : [x for x in xml_strings[2:7]] }

        self.assertEqual(uis, expected_result)
        
