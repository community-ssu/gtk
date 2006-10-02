# Gazpacho Hildon widget bindings
#
# Copyright (C) 2005, 2006 Nokia Corporation.
# Contact: Fernando Herrera <fernando.herrera-de-las-heras@nokia.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation
# version 2.1 of the License.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

import gtk
import hildon

from gazpacho.gadget import Gadget
from gazpacho.widgets.base.base import ContainerAdaptor
from gazpacho.widgets.base.dialog import DialogAdaptor
from gazpacho.placeholder import Placeholder
from gazpacho.properties import prop_registry
from gazpacho.widgetregistry import widget_registry

root_library = 'hildon'

widget_prefix = 'Hildon'

class WindowAdaptor(ContainerAdaptor):
    def post_create(self, context, window, interactive=True):
        window.set_size_request(672, 396)

class CaptionAdaptor(ContainerAdaptor):

    def post_create (self, context, caption, interactive=True):
        caption.set_label("Caption")
        caption.set_separator(":")

# Boxed properties are not yet supported in Gazpacho
# Also note that there is no support in GtkObjectBuilder for storing
# GdkColor objects, so we better disable the color properties
prop_registry.override_simple('HildonColorButton::color', enabled=False)
prop_registry.override_simple('HildonColorSelector::color', enabled=False)
prop_registry.override_simple('HildonFontSelectionDialog::color',
                              enabled=False)

class ContainerWithNoChildrenAdaptor(ContainerAdaptor):
    """ This Adaptor does not allow children inside the container

    Some widgets (DateEditor, WeekdayPicker, ...) inherits from
    GtkContainer for implementation purposes but users are not supposed to
    store other widgets inside them.
    """

    def fill_empty(self, context, date_editor):
        pass

    def get_children(self, context, widget):
        return []

class FindToolbarAdaptor(ContainerWithNoChildrenAdaptor):
    """FindToolbar is not like a regular Toolbar so it should have a
    different adaptor than ToolbarAdaptor or weird things happen"""

prop_registry.override_simple('HildonFindToolbar::list', disabled=True)

class ModalDialogAdaptor(ContainerWithNoChildrenAdaptor):

    def post_create(self, context, dialog, interactive=True):
        dialog.set_modal(False)
        dialog.set_type_hint(gtk.gdk.WINDOW_TYPE_HINT_NORMAL)
        dialog.set_property("decorated", True)
        dialog.realize()
        dialog.window.set_decorations(gtk.gdk.DECOR_ALL)

# See https://maemo.org/bugzilla/show_bug.cgi?id=767
prop_registry.override_simple('HildonGetPasswordDialog::password',
                              enabled=False)
# See https://maemo.org/bugzilla/show_bug.cgi?id=769
prop_registry.override_simple('HildonGetPasswordDialog::numbers-only',
                              enabled=False)

# The following is a construct-only property
prop_registry.override_simple('HildonSetPasswordDialog::modify-protection',
                              enabled=False)

class FileChooserDialogAdaptor(ModalDialogAdaptor):
    def post_create(self, context, dialog, interactive=True):
        dialog.set_property('action', gtk.FILE_CHOOSER_ACTION_OPEN)

    def load(self, context, widget):
        project = context.get_project()
        gadget = Gadget.load(widget, project)

        # don't create the children
        # if the widget is a toplevel we need to attach the accel groups
        # of the application
        gadget.setup_toplevel()

        action = gadget.widget.get_property('action')
        if action == gtk.FILE_CHOOSER_ACTION_OPEN: # this is the default
            gadget.widget.set_property('action', action)
        return gadget

# All these properties make the test suite fails with a core
prop_registry.override_simple('HildonFileChooserDialog::preview-widget-active',
                              enabled=False)
prop_registry.override_simple('HildonFileChooserDialog::use-preview-label',
                              enabled=False)
prop_registry.override_simple('HildonFileChooserDialog::extra-widget',
                              enabled=False)
prop_registry.override_simple('HildonFileChooserDialog::preview-widget',
                              enabled=False)
prop_registry.override_simple('HildonFileChooserDialog::file-system-model',
                              enabled=False)

class WizardDialogAdaptor(ContainerAdaptor):
    def post_create(self, context, wizard, interactive=True):
        notebook = gtk.Notebook()
        notebook.append_page(Placeholder())
        wizard.set_property('wizard-notebook', notebook)
        self._setup_internal_children(wizard, context.get_project())

    def fill_empty(self, context, wizard):
        pass # no more children, thanks

    def _setup_internal_children(self, wizard, project):
        child_class = widget_registry.get_by_name('GtkVBox')
        vbox_gadget = Gadget(child_class, project)
        vbox_gadget.setup_internal_widget(wizard.vbox, 'vbox',
                                          wizard.name or '')

        hbox = wizard.vbox.get_children()[0]
        child_class = widget_registry.get_by_name('GtkHBox')
        hbox_gadget = Gadget(child_class, project)
        hbox_gadget.setup_internal_widget(hbox, 'hbox',
                                          wizard.name or '')

        notebook = hbox.get_children()[1]
        child_class = widget_registry.get_by_name('GtkNotebook')
        notebook_gadget = Gadget(child_class, project)
        notebook_gadget.setup_internal_widget(notebook, 'notebook-wizard',
                                              wizard.name or '')

# It would be nice to put the following code inside the hildon package
# so the loader can work with WizardDialogs properly. Probably its __init__.py
# file is a good place to put it.
from gazpacho import loader
class WizardDialogLoaderAdapter(loader.custom.DialogAdapter):
    """This is a *loader* adapter and is used by gazpacho loader library"""
    object_type = hildon.WizardDialog

    def construct(self, name, gtype, properties):
        gobj = super(WizardDialogLoaderAdapter, self).construct(name, gtype,
                                                                properties)
        gobj.set_property('wizard-notebook', gtk.Notebook())
        return gobj

    def find_internal_child(self, gobj, name):
        if name == 'vbox':
            return gobj.vbox
        elif name == 'hbox':
            hbox = gobj.vbox.get_children()[0]
            return hbox
        elif name == 'notebook-wizard':
            hbox = gobj.vbox.get_children()[0]
            notebook = hbox.get_children()[1]
            return notebook

loader.custom.adapter_registry.register_adapter(WizardDialogLoaderAdapter)
