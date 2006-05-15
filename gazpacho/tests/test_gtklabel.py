from gazpacho.unittest import common

import gtk

class GtkLabelTest(common.GazpachoTest):
    #what will we do with these?
    widgetProperties = ['app_paintable', 'can_default', 'can_focus', \
                        'composite_child', 'events', 'extension_events', \
                        'has_default', 'has_focus', 'height_request', \
                        'is_focus', 'name', 'no_show_all', 'parent', \
                        'receives_default', 'sensitive', 'style', 'visible']

    #2.6 -> single-line-mode
    boolProperties = ['selectable', 'use-markup', 'use-underline', 'wrap']
    # missing: attributes, ellipsize, justify, width-chars
    customProperties = {'label':['label', 'Read Me!'], \
                        'xalign':[0.0, 0.5, 1.0], \
                        'yalign':[0.0, 0.3, 1.0], \
                        'xpad':[1, 3], \
                        'ypad':[2, 6]}
    #not used yet
    readProperties = ['cursor_position', 'mnemonic_keyval', 'selection_bound']
 
    
    def setUp(self):
        common.GazpachoTest.setUp(self)

        # add a window
        self.window = self.create_gwidget('GtkWindow')
        self.window.get_prop('visible').set(False)
        self.project.add_widget(self.window.gtk_widget)
   
    def testCreation(self):
        label = self.add_child(self.window, 'GtkLabel',
                               self.window.gtk_widget.get_child())
        self.assertEqual(gtk.Label, type(label.gtk_widget))

    def testboolProperties(self):
        label = self.add_child(self.window, 'GtkLabel',
                               self.window.gtk_widget.get_child())

        for prop_name in self.boolProperties:
            prop = label.get_prop(prop_name)
            if prop is not None:
                prop.set(True)
                self.assertEqual(label.gtk_widget.get_property(prop_name), True)

                prop.set(False)
                self.assertEqual(label.gtk_widget.get_property(prop_name),False)
            else:
                self.fail("prop for %s is None" % property)

    def testcustomProperties(self):
        label = self.add_child(self.window, 'GtkLabel',
                               self.window.gtk_widget.get_child())

        for prop_name in self.customProperties.keys():
            prop = label.get_prop(prop_name)
            #should we check custom setters and getters?
            for value in self.customProperties[prop_name]:
                prop.set(value)
                saved_value = label.gtk_widget.get_property(prop_name)
                if isinstance(value, (basestring, bool, int)):
                    self.assertEqual(saved_value , value)
                else:
                    self.assertApproximates(saved_value, value, 2)
                    
    def testreadProperties(self):
        #gwidget = project.get_widget_by_name('label1')
        #prop = gwidget.get_prop('xalign')
        #cm = app.get_command_manager()
        #cm.set_property(prop, 'False')
        #refresh_gui()
        #self.assertEqual(gwidget.widget.get_label(), 'False')
        pass
 
    def testSavingLoading(self):
        label = self.add_child(self.window, 'GtkLabel',
                               self.window.gtk_widget.get_child())
        self.project.add_widget(label.gtk_widget)
        self.project.save(__file__+'.glade')

        self.app.open_project(__file__+'.glade')
        self.remove_file(__file__+'.glade')
