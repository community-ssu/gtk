from gazpacho.unittest import common

from gazpacho.widgetregistry import widget_registry
from gazpacho.clipboard import ClipboardItem, Clipboard

class ClipboardTest(common.GazpachoTest):

    def setUp(self):
        common.GazpachoTest.setUp(self)
        self.clipboard = Clipboard()

    def testCreateToplevelClipboardItem(self):
        adaptor = widget_registry.get_by_name('GtkWindow')
        widget_instance = self.create_gwidget('GtkWindow')
        
        item = ClipboardItem(widget_instance)
        
        self.assertEqual(item.is_toplevel, True)
        self.assertEqual(item.icon, adaptor.pixbuf)
        
    def testCreateClipboardItem(self):
        adaptor = widget_registry.get_by_name('GtkButton')
        widget_instance = self.create_gwidget('GtkButton')

        item = ClipboardItem(widget_instance)
        
        self.assertEqual(item.is_toplevel, False)
        self.assertEqual(item.icon, adaptor.pixbuf)

    def testRemoveOldestItemEmpty(self):
        try:
            self.clipboard._remove_oldest_item()
        except:
            self.fail('No exception should be raised')

        self.assertEqual(len(self.clipboard.content), 0)

    def testRemoveOldestItem(self):
        button_widget = self.create_gwidget('GtkButton')
        self.clipboard.add_widget(button_widget)

        label_widget = self.create_gwidget('GtkLabel')
        self.clipboard.add_widget(label_widget)
        
        self.clipboard._remove_oldest_item()
        self.assertEqual(len(self.clipboard.content), 1)

        item = self.clipboard.content[0]
        self.assertEqual(item.name, label_widget.name)

    
    def testClipboardAdd(self):
        widget_instance = self.create_gwidget('GtkButton')
                
        self.clipboard.add_widget(widget_instance)

        self.assertEqual(len(self.clipboard.content), 1)

        item = self.clipboard.content[0]
        self.assertEqual(item.name, widget_instance.name)
        
    def testClipboardAutoRemove(self):
        self.clipboard._max_clipboard_items = 1
        
        # Add a button
        button_widget = self.create_gwidget('GtkButton')
        self.clipboard.add_widget(button_widget)

        # Add a Window, this should remove the button
        window_widget = self.create_gwidget('GtkWindow')

        self.clipboard.add_widget(window_widget)
        
        self.assertEqual(len(self.clipboard.content), 1)
        item = self.clipboard.content[0]
        self.assertEqual(item.name, window_widget.name)

    def testGetItemEmpty(self):
        item = self.clipboard.get_selected_item()
        self.assertEqual(item, None)

    def testGetItem(self):
        # Add a button
        button_widget = self.create_gwidget('GtkButton')
        self.clipboard.add_widget(button_widget)

        item = self.clipboard.get_selected_item()
        self.assertEqual(item.name, button_widget.name)

        # Add a Window
        window_widget = self.create_gwidget('GtkWindow')
        self.clipboard.add_widget(window_widget)
        
        item = self.clipboard.get_selected_item()
        self.assertEqual(item.name, window_widget.name)

    def testGetSelectedItem(self):
        # Add a button
        button_widget = self.create_gwidget('GtkButton')
        self.clipboard.add_widget(button_widget)

        # Add a Window
        window_widget = self.create_gwidget('GtkWindow')
        self.clipboard.add_widget(window_widget)

        # Add a Label
        label_widget = self.create_gwidget('GtkLabel')
        self.clipboard.add_widget(label_widget)

        # Select the Window
        self.clipboard.selected = self.clipboard.content[1]

        item = self.clipboard.get_selected_item()
        self.assertEqual(item.name, window_widget.name)

    def testGetWidgetEmpty(self):
        result = self.clipboard.get_selected_widget(self.project)

        self.assertEqual(result, None)

    def testGetWidget(self):
        # Test to add a Button and get it again
        button_widget = self.create_gwidget('GtkButton')
        self.clipboard.add_widget(button_widget)

        gwidget = self.clipboard.get_selected_widget(self.project)
        
        self.assertEqual(button_widget.name, gwidget.name)
        self.assertEqual(isinstance(gwidget, type(button_widget)), True)

        # Test to add a Label and get it again
        label_widget = self.create_gwidget('GtkLabel')
        self.clipboard.add_widget(label_widget)

        gwidget = self.clipboard.get_selected_widget(self.project)

        self.assertEqual(label_widget.name, gwidget.name)
        self.assertEqual(isinstance(gwidget, type(label_widget)), True)


    def testGetSelectedWidget(self):
        # Add three widgets
        button_widget = self.create_gwidget('GtkButton')
        self.clipboard.add_widget(button_widget)

        label_widget = self.create_gwidget('GtkLabel')
        self.clipboard.add_widget(label_widget)

        window_widget = self.create_gwidget('GtkWindow')
        self.clipboard.add_widget(window_widget)

        # Set the selected widget (Label)
        self.clipboard.selected = self.clipboard.content[1]

        gwidget = self.clipboard.get_selected_widget(self.project)

        self.assertEqual(label_widget.name, gwidget.name)
        self.assertEqual(isinstance(gwidget, type(label_widget)), True)

    # TODO test signals for remove and add
