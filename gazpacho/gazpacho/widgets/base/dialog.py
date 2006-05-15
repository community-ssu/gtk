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

import gtk

from gazpacho.properties import prop_registry
from gazpacho.widget import Widget
from gazpacho.widgetregistry import widget_registry
from gazpacho.widgets.base.base import ContainerAdaptor

class DialogAdaptor(ContainerAdaptor):
    def child_property_applies(self, context, ancestor, gtk_widget,
                               property_id):
        if property_id == 'response-id' and \
           isinstance(gtk_widget.parent, gtk.HButtonBox) and \
           isinstance(gtk_widget.parent.parent, gtk.VBox) and \
           gtk_widget.parent.parent.parent == ancestor:
            return True
        elif gtk_widget.parent == ancestor:
            return True

        return False
    
    def post_create(self, context, dialog, interactive=True):
        gwidget = Widget.from_widget(dialog)
        if not gwidget: return
        
        # create the GladeWidgets for internal children
        self._setup_internal_children(gwidget)
                
        dialog.action_area.pack_start(context.create_placeholder())
        dialog.action_area.pack_start(context.create_placeholder())

        dialog.vbox.pack_start(context.create_placeholder())
        
        # set a reasonable default size for a dialog
        dialog.set_default_size(320, 260)

    def _setup_internal_children(self, gwidget):
        child_class = widget_registry.get_by_name('GtkVBox')
        vbox_widget = Widget(child_class, gwidget.project)
        vbox_widget.setup_internal_widget(gwidget.gtk_widget.vbox, 'vbox',
                                          gwidget.name or '')
        child_class = widget_registry.get_by_name('GtkHButtonBox')
        action_area_widget = Widget(child_class, gwidget.project)
        action_area_widget.setup_internal_widget(gwidget.gtk_widget.action_area,
                                                 'action_area',
                                                 gwidget.name or '')
        
class MessageDialogAdaptor(DialogAdaptor):
    def post_create(self, context, dialog, interactive=True):
        dialog.set_default_size(400, 115)

# GtkDialog
prop_registry.override_simple('GtkDialog::has-separator', default=False)

