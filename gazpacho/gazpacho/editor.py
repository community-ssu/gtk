# Copyright (C) 2004,2005 by SICEm S.L. and Imendio AB
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

import gettext

import gtk
import gobject

from gazpacho.loader import tags
from gazpacho.properties import PropertyCustomEditor, UNICHAR_PROPERTIES, prop_registry 
from gazpacho.signaleditor import SignalEditor

class IncompleteFunctions(Exception):
    pass

_ = gettext.gettext

TABLE_ROW_SPACING = 2
TABLE_COLUMN_SPACING = 6

(EDITOR_INTEGER,
 EDITOR_FLOAT,
 EDITOR_DOUBLE) = range(3)

(FLAGS_COLUMN_SETTING,
 FLAGS_COLUMN_NAME,
 FLAGS_COLUMN_MASK) = range(3)

class EditorProperty:
    """ For every PropertyClass we have a EditorProperty that is basically
    an input (GtkWidget) for that PropertyClass. """

    def __init__(self, property_class, app):
        # The class this property corresponds to
        self.property_class = property_class
        
        self._app = app
        self._project = app.get_current_project()

        # The widget that modifies this property, the widget can be a
        # GtkSpinButton, a GtkEntry, GtkMenu, etc. depending on the property
        # type.
        self.input = None

        # The loaded property
        self.property = None

        # If we're in the process of loading the property
        self.property_loading = False

        # We set this flag when we are loading a new Property into this
        # EditorProperty. This flag is used so that when we receive a "changed"
        # signal we know that nothing has really changed, we just loaded a new
        # glade widget
        self.loading = False

        # the label to the left of the input
        label = gtk.Label("%s:" % property_class.label)
        label.set_alignment(1.0, 0.5)
        
        # we need to wrap the label in an event box to add tooltips
        eventbox = gtk.EventBox()
        eventbox.add(label)

        self.description = eventbox

    def load(self, widget):
        # Should be overridden
        return

    def set(self, value):
        self._app.command_manager.set_property(self.property, value)

    def set_translatable_property(self, text, engineering_english, comment_text, 
                                  translatable, has_context):
        self._app.command_manager.set_translatable_property(self.property,
                                                            text,
                                                            engineering_english,
                                                            comment_text,
                                                            translatable,
                                                            has_context)
        
    def _start_load(self, widget):
        self.property = widget.get_prop(self.property_class.name)
        assert self.property
        self.property_loading = True

    def _finish_load(self):
        self.property_loading = False

    def _internal_load(self, value):
        """This is the virtual method that actually put the data in the editor
        widget.
        Most subclasses implements this (But not EditorPropertySpecial)
        """
    
class EditorPropertyNumeric(EditorProperty):

    def __init__(self, property_class, app):
        EditorProperty.__init__(self, property_class, app)

    def load(self, widget):
        self._start_load(widget)
        self._internal_load(self.property.value)

        self.input.set_data('user_data', self)
        self._finish_load()

    def _internal_load(self, value):
        if self.property_class.name in UNICHAR_PROPERTIES:
            if isinstance(self.input, gtk.Editable):
                self.input.delete_text(0, -1)
                self.input.insert_text(value)
        #elif self.property_class.optional:
        #    button, spin = self.input.get_children()
        #    button.set_active(self.property.enabled)
        #    spin.set_sensitive(self.property.enabled)
        #    button.set_data('user_data', self)
        #    v = float(value)
        #    spin.set_value(v)
        else:
            v = float(value)
            self.input.set_value(v)
                
    def _changed_unichar(self, entry):
        if self.property_loading:
            return
        
        text = entry.get_chars(0, -1)
        if text:
            self.set(text[0])
        
    def _create_input_unichar(self):
        entry = gtk.Entry()
        # it's to prevent spirious beeps ...
        entry.set_max_length(2)
        entry.connect('changed', self._changed_unichar)
        self.input = entry

    def _changed_numeric(self, spin):
        if self.property_loading:
            return
        
        numeric_type = spin.get_data('NumericType')
        if numeric_type == EDITOR_INTEGER:
            value = spin.get_value_as_int()
        elif numeric_type == EDITOR_FLOAT:
            value = spin.get_value()
        elif numeric_type == EDITOR_DOUBLE:
            value = spin.get_value()
        else:
            value = 0

        self.set(value)

    def _changed_enabled(self, check_button):
        if self.property_loading:
            return

        l = self.input.get_children()
        spin = l[1]
        state = check_button.get_active()
        spin.set_sensitive(state)
        prop = check_button.get_data('user_data')
        prop.property.enabled = state

    def _create_input_numeric(self, numeric_type):
        if numeric_type == EDITOR_INTEGER:
            digits = 0
        else:
            digits = 2

        spin = gtk.SpinButton(self.property_class.get_adjustment(),
                              self.property_class.climb_rate,
                              self.property_class.digits)
        spin.set_data("NumericType", numeric_type)
        spin.connect('value_changed', self._changed_numeric)

        # Some numeric types are optional, for example the default window size,
        # so they have a toggle button right next to the spin button
        #if self.property_class.optional:
        #    check = gtk.CheckButton()
        #    hbox = gtk.HBox()
        #    hbox.pack_start(check, False, False)
        #    hbox.pack_start(spin)
        #    check.connect('toggled', self._changed_enabled)
        #    self.input = hbox
        #else:
        self.input = spin

class EditorPropertyInteger(EditorPropertyNumeric):
    def __init__(self, property_class, app):
        EditorPropertyNumeric.__init__(self, property_class, app)

        if self.property_class.name in UNICHAR_PROPERTIES:
            self._create_input_unichar()
        else:
            self._create_input_numeric(EDITOR_INTEGER)

class EditorPropertyFloat(EditorPropertyNumeric):
    def __init__(self, property_class, app):
        EditorPropertyNumeric.__init__(self, property_class, app)
        self._create_input_numeric(EDITOR_FLOAT)        

class EditorPropertyDouble(EditorPropertyNumeric):
    def __init__(self, property_class, app):
        EditorPropertyNumeric.__init__(self, property_class, app)
        self._create_input_numeric(EDITOR_DOUBLE)
                                
class EditorPropertyText(EditorProperty):
    def __init__(self, property_class, app):
        EditorProperty.__init__(self, property_class, app)

        if self.property_class.translatable:
            i18n_button = gtk.Button('...')
            i18n_button.connect('clicked', self._show_i18n_dialog)

        self.input = gtk.HBox()

        # If the current data contains more than one line,
        # use a GtkTextView to display the data, otherwise just use a
        # normal GtkEntry
        if property_class.multiline:
            view = gtk.TextView()
            view.set_editable(True)
            text_buffer = view.get_buffer()
            text_buffer.connect('changed', self._changed_text_view)
            self.input.pack_start(view)
        else:
            entry = gtk.Entry()
            entry.connect('changed', self._changed_text)
            self.input.pack_start(entry)

        if self.property_class.translatable:
            self.input.pack_start(i18n_button, False, False)
        
    def load(self, widget):
        self._start_load(widget)

        self._internal_load(self.property.value)

        self.input.set_data('user_data', self)

        gtk_widget = widget.gtk_widget
        hacked = bool(gtk_widget.get_data('hacked_%s' % self.property.name))
        if hacked:
            self.property.translatable = bool(gtk_widget.get_data('i18n_is_translatable_%s' % self.property.name))
            self.property.has_i18n_context = bool(gtk_widget.get_data('i18n_has_context_%s' % self.property.name))
            self.property.i18n_comment = gtk_widget.get_data('i18n_comment_%s' % self.property.name)
            self.property.engineering_english = gtk_widget.get_data('engineering_english_%s' % self.property.name)
            gtk_widget.set_data('hacked_%s' % self.property.name, False)

        self._finish_load()

    def _internal_load(self, value):
        # Text inputs are an entry or text view inside an hbox
        text_widget = self.input.get_children()[0]
        
        if isinstance(text_widget, gtk.Editable):
            pos = text_widget.get_position()
            text_widget.delete_text(0, -1)
            if value:
                text_widget.insert_text(value)

            text_widget.set_position(pos)

        elif isinstance(text_widget, gtk.TextView):
            text_buffer = text_widget.get_buffer()
            text_buffer.set_text(value)
        else:
            print ('Invalid Text Widget type')

    def _changed_text_common(self, text):
        self.set(text)

    def _changed_text(self, entry):
        if self.property_loading:
            return

        text = entry.get_chars(0, -1)
        self._changed_text_common(text)

    def _changed_text_view(self, text_buffer):
        if self.property_loading:
            return
        
        start = text_buffer.get_iter_at_offset(0)
        end = text_buffer.get_iter_at_offset(text_buffer.get_char_count())
        text = text_buffer.get_text(start, end, False)
        self._changed_text_common(text)

    def _show_i18n_dialog(self, button):
        editor = button.get_toplevel()

        translatable = self.property.translatable
        has_context = self.property.has_i18n_context
        engineering_english = self.property.engineering_english
        comment_text = self.property.i18n_comment

        dialog = gtk.Dialog(_('Edit Text Property'), editor,
                            gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
                            (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                             gtk.STOCK_OK, gtk.RESPONSE_OK))
        
        dialog.set_has_separator(False)

        vbox = gtk.VBox(False, 6)
        vbox.set_border_width(8)
        vbox.show()

        dialog.vbox.pack_start(vbox, True, True, 0)

        # The text content
        label = gtk.Label()
        label.set_markup_with_mnemonic(_('_Text:'))
        label.show()
        label.set_alignment(0, 0.5)
        vbox.pack_start(label, False, False, 0)

        scrolled_window = gtk.ScrolledWindow()
        scrolled_window.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        scrolled_window.set_shadow_type(gtk.SHADOW_IN)
        scrolled_window.set_size_request(400, 200)
        scrolled_window.show()

        vbox.pack_start(scrolled_window)

        text_view = gtk.TextView()
        text_view.show()
        label.set_mnemonic_widget(text_view)

        scrolled_window.add(text_view)

        text_buffer = text_view.get_buffer()

        text = self.property.value
        if text is not None:
            text_buffer.set_text(text)
       
        # Translatable and context prefix
        hbox = gtk.HBox(False, 12)
        hbox.show()

        vbox.pack_start(hbox, False, False, 0)

        translatable_button = gtk.CheckButton(_("T_ranslatable"))
        translatable_button.set_active(translatable)
        translatable_button.show()
        hbox.pack_start(translatable_button, False, False, 0)

        context_button = gtk.CheckButton(_("_Has context prefix"))
        context_button.set_active(has_context)
        context_button.show()
        hbox.pack_start(context_button, False, False, 0)

        # Logical ID
        hbox = gtk.HBox(False, 12)
        hbox.show()
        
        label = gtk.Label()
        label.set_markup_with_mnemonic(_("Logical translation _ID:"))
        label.show()
        label.set_alignment(0, 0.5)
        hbox.pack_start(label, False, False, 0)
        vbox.pack_start(hbox, False, False, 0)

        entry = gtk.Entry()
        entry.show()
        vbox.pack_start(entry, False, True, 0)

        # Comments
        label = gtk.Label()
        label.set_markup_with_mnemonic(_("Co_mments for translators:"))
        label.show()
        label.set_alignment(0, 0.5)
        vbox.pack_start(label, False, False, 0)

        

        scrolled_window = gtk.ScrolledWindow()
        scrolled_window.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        scrolled_window.set_shadow_type(gtk.SHADOW_IN)
        scrolled_window.set_size_request(400, 200)
        scrolled_window.show()

        vbox.pack_start(scrolled_window)

        comment_view = gtk.TextView()
        comment_view.show()
        label.set_mnemonic_widget(comment_view)

        scrolled_window.add(comment_view)

        comment_buffer = comment_view.get_buffer()

        if comment_text is not None:
            comment_buffer.set_text(comment_text)

        if engineering_english is not None:
            entry.set_text(engineering_english)

        res = dialog.run()
        if res == gtk.RESPONSE_OK:
            translatable = translatable_button.get_active()
            has_context = context_button.get_active()

            start, end = text_buffer.get_bounds()
            text = text_buffer.get_text(start, end, False)

            start, end = comment_buffer.get_bounds()
            comment_text = comment_buffer.get_text(start, end, False)

            engineering_english = entry.get_text()

            self.set_translatable_property(text, engineering_english, comment_text, translatable, has_context)
 
            # Update the entry in the property editor
            #self.load(self.property.widget)

        dialog.destroy()

class EditorPropertyEnum(EditorProperty):
    def __init__(self, property_class, app):
        EditorProperty.__init__(self, property_class, app)

        model = gtk.ListStore(str, object)
        for name, value in self.property_class.get_choices():
            model.append((name, value))

        combo_box = gtk.ComboBox(model)
        renderer = gtk.CellRendererText()
        combo_box.pack_start(renderer)
        combo_box.set_attributes(renderer, text=0)
        combo_box.connect('changed', self._changed_enum)
        
        self.input = combo_box

    def load(self, widget):
        self._start_load(widget)
        self._internal_load(self.property.value)
        self._finish_load()

    def _internal_load(self, value):
        idx = 0
        for name, enum in self.property.get_choices():
            if enum == value:
                break
            idx += 1
        self.input.set_active(idx)
        
    def _changed_enum(self, combo_box):
        if self.property_loading:
            return
        
        active_iter = combo_box.get_active_iter()
        model = combo_box.get_model()
        value = model[active_iter][1]
        self.set(value)
   
class EditorPropertyFlags(EditorProperty):
    def __init__(self, property_class, app):
        EditorProperty.__init__(self, property_class, app)

        scrolled_window = gtk.ScrolledWindow()
        scrolled_window.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        scrolled_window.set_shadow_type(gtk.SHADOW_IN)
        scrolled_window.show()
        # create the treeview using a simple list model with 3 columns
        model = gtk.ListStore(bool, str, int)
        treeview = gtk.TreeView(model)
        treeview.set_property("allow-checkbox-mode", False)
        treeview.set_headers_visible(False)
        treeview.show()
        scrolled_window.add(treeview)

        column = gtk.TreeViewColumn()
        renderer = gtk.CellRendererToggle()
        renderer.connect('toggled', self._flag_toggled, model)
        column.pack_start(renderer, False)
        column.set_attributes(renderer, active=FLAGS_COLUMN_SETTING)
        renderer = gtk.CellRendererText()
        column.pack_start(renderer)
        column.set_attributes(renderer, text=FLAGS_COLUMN_NAME)

        treeview.append_column(column)
        
        # Populate the model with the flags 
        for name, mask in property_class.get_choices():
            model.append((False, name, mask))

        self.input = scrolled_window

    def load(self, widget):
        self._start_load(widget)
        self._internal_load(self.property.value)
        self._finish_load()

    def _internal_load(self, value):
        model = self.input.child.get_model()
        value = self.property.value
        for i in range(len(model)):
            setting, name, mask = model[i]
            model[i] = bool((value & mask) == mask), name, mask

    def _flag_toggled(self, cell, path_string, model):
        row = model[path_string]
        row[FLAGS_COLUMN_SETTING] = not row[FLAGS_COLUMN_SETTING]

        # set the property
        new_value = 0
        for i in range(len(model)):
            setting, name, mask = model[i]
            if setting:
                new_value |= mask

        # if the new value is different from the old value, we need to
        # update the property
        if new_value != self.property.value:
            self.set(new_value)
                

class EditorPropertyBoolean(EditorProperty):
    def __init__(self, property_class, app):
        EditorProperty.__init__(self, property_class, app)

        button = gtk.ToggleButton(_('No'))
        button.connect('toggled', self._changed_boolean)

        self.input = button

    def load(self, widget):
        self._start_load(widget)
        self._internal_load(self.property.value)
        
        self.input.set_data('user_data', self)
        self._finish_load()

    def _internal_load(self, state):
        label = self.input.get_child()

        if state:
            value = tags.YES
        else:
            value = tags.NO
        label.set_text(value)
            
        self.input.set_active(state)
        
    def _changed_boolean(self, button):
        if self.property_loading:
            return
        
        if button.get_active():
            value = tags.YES
        else:
            value = tags.NO
            
        label = button.get_child()
        label.set_text(value)
        self.set(button.get_active())

class EditorPropertySpecial(EditorProperty):
    def __init__(self, property_class, app):
        EditorProperty.__init__(self, property_class, app)

        self.custom_editor = self.property_class.editor()
        self.input = self.custom_editor.get_editor_widget()
        
    def load(self, widget):
        self._start_load(widget)
        
        self.widget = widget
        
        proxy = EditorValueProxy()
        proxy.connect('value-set', self._value_changed, widget)
        proxy.connect('property-set', self._property_set, widget)

        context = widget.project.context
        self.custom_editor.update(context, widget.gtk_widget, proxy)
        self._finish_load()

    def _value_changed(self, proxy, value, widget):
        if self.property_loading:
            return

        self.set(value)

    def _property_set(self, proxy, prop_id, value, widget):
        property = widget.get_prop(prop_id)

        if not property or self.property_loading: 
            return

        self.set(value)

class EditorValueProxy(gobject.GObject):
    __gsignals__ = {
        'value-set':(gobject.SIGNAL_RUN_LAST, None, (object,)),
        'property-set':(gobject.SIGNAL_RUN_LAST, None, (str, object))
        }

    def set_value(self, value):
        self.emit('value-set', value)

    def set_property(self, prop, value):
        self.emit('property-set', prop, value)
        
gobject.type_register(EditorValueProxy)

# BUGS:
#   the cellrenderers show up twice
#   no way to remove the widget, eg set it to empty
#
class EditorPropertyObject(EditorProperty):
    def __init__(self, property_class, app):
        EditorProperty.__init__(self, property_class, app)

        self.property_class = property_class
        self.property_type = property_class.type
        self.project = app.get_current_project()

        self.model = gtk.ListStore(str, object)

        combo_box = gtk.ComboBoxEntry(self.model)
        renderer = gtk.CellRendererText()
        combo_box.pack_start(renderer)
        combo_box.set_attributes(renderer, text=0)
        combo_box.connect('changed', self._changed_enum)

        self.input = combo_box

    def load(self, widget):
        self._start_load(widget)
        current = self.property.get()

        self.model.clear()
        for pwidget in self.project.widgets:
            if isinstance(pwidget, gtk.Window):
                continue

            if not gobject.type_is_a(pwidget, self.property_type):
                continue
            
            iter = self.model.append((pwidget.name, pwidget))
            if pwidget == current:
                self.input.set_active_iter(iter)

        self._finish_load()

    def _changed_enum(self, combo_box):
        if self.property_loading:
            return

        active_iter = combo_box.get_active_iter()
        model = combo_box.get_model()
        widget = model[active_iter][1]
        self.set(widget)

class PropertyEditorDialog(gtk.Dialog):
    """This is a base abstract class for those editors that need to be
    opened in a dialog.
    """

    def __init__(self):
        """The widgets of the dialog are created in the virtual method
        _create_widgets.
        """
        gtk.Dialog.__init__(self,
                            title='',
                            parent=None,
                            flags=gtk.DIALOG_DESTROY_WITH_PARENT,
                            buttons=(gtk.STOCK_CLOSE, gtk.RESPONSE_CLOSE))
        self.widget = None
        self.gwidget = None
        self.proxy = None
        
        # avoid destrcution when clicking the x button
        self.connect('delete-event', self._on_delete_event)
        self.connect('response', self._on_response)

        self.set_border_width(6)
        self.set_resizable(False)
        self.set_has_separator(False)
        self.vbox.set_spacing(6)
        
        self._create_widgets()

        self.vbox.show_all()

    def _create_widgets(self):
        """Subclasses should override this."""

    def set_widget(self, widget, proxy):
        """Every time the user clicks on a custom property editor button
        we are called with a different widget and proxy.
        """
        from gazpacho.widget import Widget

        self.widget = widget
        self.proxy = proxy
        self.gwidget = Widget.from_widget(widget)

    def _on_delete_event(self, dialog, event):
        self.hide()
        return True
    
    def _on_response(self, dialog, response_id):
        self.hide()

class PropertyCustomEditorWithDialog(PropertyCustomEditor):
    """A custom editor which is just a button that opens up a dialog"""

    # subclasses should probably want to override the dialog_class
    dialog_class = PropertyEditorDialog
    button_text = _('Edit...')
    
    def __init__(self):
        self.dialog = self.dialog_class()
        self.input = gtk.Button(self.button_text)
        self.input.connect('clicked', self.on_button_clicked)
        
        self.application_window = None
        self.proxy = None
        self.edited_widget = None

    def get_editor_widget(self):
        return self.input

    def update(self, context, gtk_widget, proxy):
        self.application_window = context.get_application_window()
        self.edited_widget = gtk_widget
        self.proxy = proxy
        
    def on_button_clicked(self, button):
        self.dialog.set_transient_for(self.application_window)
        self.dialog.set_widget(self.edited_widget, self.proxy)
        self.dialog.present()

# this maps property types with EditorProperty classes
property_map = {
    gobject.TYPE_INT: EditorPropertyInteger,
    gobject.TYPE_UINT: EditorPropertyInteger,
    gobject.TYPE_FLOAT: EditorPropertyFloat,
    gobject.TYPE_DOUBLE: EditorPropertyDouble,
    gobject.TYPE_ENUM: EditorPropertyEnum,
    gobject.TYPE_FLAGS: EditorPropertyFlags,
    gobject.TYPE_BOOLEAN: EditorPropertyBoolean,
    gobject.TYPE_STRING: EditorPropertyText
    }

class EditorPage(gtk.Table):
    """
    For each widget type that we have modified, we create an EditorPage.
    An EditorPage is a gtk.Table subclass with all the widgets to edit
    a particular widget type
    """
    def __init__(self):
        gtk.Table.__init__(self, 1, 1, False)

        self._rows = 0

        self.set_border_width(6)
        self.set_row_spacings(TABLE_ROW_SPACING)
        self.set_col_spacings(TABLE_COLUMN_SPACING)

    def _attach_row(self, label, editable):
        row = self._rows
        if label:
            self.attach(label, 0, 1, row, row+1,
                        xoptions=gtk.FILL, yoptions=gtk.FILL)
            self.attach(editable, 1, 2, row, row+1,
                        xoptions=gtk.EXPAND|gtk.FILL,
                        yoptions=gtk.EXPAND|gtk.FILL)
        else:
            self.attach(editable, 0, 2, row, row+1)
        self._rows += 1

    def _create_editor_property(self, property_class, app):
        if property_class.editor != PropertyCustomEditor:
            return EditorPropertySpecial(property_class, app)

        # Special case GObject subclasses
        if gobject.type_is_a(property_class.type, gobject.GObject):
            return EditorPropertyObject(property_class, app)

        if property_map.has_key(property_class.base_type):
            return property_map[property_class.base_type](property_class, app)
    
    def append_item(self, property_class, tooltips, app):
        editor = self._create_editor_property(property_class, app)
        
        if editor and editor.input:
            #tooltips.set_tip(editor.description,
            #                 property_class.tooltip)

            description = editor.description
            if property_class.editor.wide_editor:
                # no label for this editor since it need both columns
                # of the table
                description = None
                
            self._attach_row(description, editor.input)
        
        return editor
    
    def append_name_field(self, widget_adaptor):
        # Class
        label = gtk.Label(_('Class')+':')
        label.set_alignment(1.0, 0.5)
        class_label = gtk.Label(widget_adaptor.editor_name)
        class_label.set_alignment(0.0, 0.5)
        self._attach_row(label, class_label)

class Editor(gtk.Notebook):

    __gsignals__ = {
        'add_signal':(gobject.SIGNAL_RUN_LAST, None,
                      (str, long, int, str))
        }
    
    def __init__(self, app):
        gtk.Notebook.__init__(self)
        self.set_size_request(350, 300)
        
        self._app = app

        # The editor has (at this moment) four tabs this are pointers to the
        # vboxes inside each tab.
        self._vbox_widget = self._notebook_page(_('Widget'))
        self._vbox_packing = self._notebook_page(_('Packing'))
        self._vbox_common = self._notebook_page(_('Common'))
        self._vbox_signals = self._notebook_page(_('Signals'))

        # A handy reference to the widget that is loaded in the editor. None
        # if no widgets are selected
        self._loaded_widget = None

        # A list of properties for the current widget
        # XXX: We might want to cache this
        self._widget_properties = []
        
        self._signal_editor = None

        self._tooltips = gtk.Tooltips()

        # Display some help
        help_label = gtk.Label(_("Select a widget to edit its properties"))
        help_label.show()
        self._vbox_widget.pack_start(help_label)
        
    def _notebook_page(self, name):
        vbox = gtk.VBox()

        scrolled_window = gtk.ScrolledWindow()
        scrolled_window.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        scrolled_window.add_with_viewport(vbox)
        scrolled_window.set_shadow_type(gtk.SHADOW_NONE)

        self.insert_page(scrolled_window, gtk.Label(name), -1)

        return vbox
        
    def do_add_signal(self, id_widget, type_widget, id_signal, callback_name):
        pass

    def _create_property_pages(self, widget, adaptor):
        # Create the pages, one for widget properties and
        # one for the common ones
        widget_table = EditorPage()
        widget_table.append_name_field(adaptor)
        self._vbox_widget.pack_start(widget_table, False)
        
        common_table = EditorPage()
        self._vbox_common.pack_start(common_table, False)

        packing_table = EditorPage()
        self._vbox_packing.pack_start(packing_table, False)
        
        # Put the property widgets on the right page
        widget_properties = []
        parent = widget.gtk_widget.get_parent()
        for property_class in prop_registry.list(adaptor.type_name,
                                                 parent):

            if not property_class.editable:
                continue
            
            if property_class.child:
                page = packing_table
            elif property_class.owner_type in (gtk.Object.__gtype__,
                                               gtk.Widget.__gtype__):
                page = common_table
            else:
                page = widget_table

            property_widget = page.append_item(property_class,
                                               self._tooltips,
                                               self._app)
            if property_widget:
                widget_properties.append(property_widget)
                
        self._widget_properties = widget_properties

        # XXX: Remove this, show all widgets individually instead
        widget_table.show_all()
        common_table.show_all()
        packing_table.show_all()
        
    def _create_signal_page(self):
        if self._signal_editor:
            return self._signal_editor
        
        self._signal_editor = SignalEditor(self, self._app)
        self._vbox_signals.pack_start(self._signal_editor)

    def _get_parent_types(self, widget):
        retval = [type(widget)]
        while True:
            parent = widget.get_parent()
            if not parent:
                return retval
            retval.append(type(parent))
            widget = parent
    
    def _needs_rebuild(self, widget):
        """
        Return True if we need to rebuild the current property pages, False
        if it's not require.
        """
        
        if not self._loaded_widget:
            return True

        # Check if we need to rebuild the interface, otherwise we might end
        # up with a (child) properties which does not belong to us
        # FIXME: This implementation is not optimal, in some cases it'll
        # rebuild even when it doesn't need to, a better way would be
        # to compare child properties.
        if (self._get_parent_types(self._loaded_widget.gtk_widget) !=
            self._get_parent_types(widget.gtk_widget)):
            return True

        return False
        
    def _load_widget(self, widget):
        if self._needs_rebuild(widget):
            self.clear()
            self._create_property_pages(widget, widget.adaptor)
            self._create_signal_page()

        for widget_property in self._widget_properties:
            widget_property.load(widget)
        self._signal_editor.load_widget(widget)

        self._loaded_widget = widget

    def clear(self):
        "Clear the content of the editor"
        for vbox in (self._vbox_widget,
                     self._vbox_common,
                     self._vbox_packing):
            map(vbox.remove, vbox.get_children())
        self._loaded_widget = None
 
    def display(self, widget):
        "Display a widget in the editor"
        
        # Skip widget if it's already loaded or None
        if self._loaded_widget == widget or not widget:
            return

        self._load_widget(widget)

    def refresh(self):
        "Reread properties and update the editor"
        if self._loaded_widget:
            self._load_widget(self._loaded_widget)

        
gobject.type_register(Editor)
