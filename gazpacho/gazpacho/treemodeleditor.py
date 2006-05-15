import gettext

import gtk
import gobject

_ = gettext.gettext

# Supported types for Modal data
supported_types = [gobject.TYPE_INT,
                   gobject.TYPE_STRING,
                   gobject.TYPE_BOOLEAN,
                   gtk.gdk.Pixbuf.__gtype__]

supported_renderers = {gobject.TYPE_INT: gtk.CellRendererText,
                       gobject.TYPE_STRING: gtk.CellRendererText,
                       gobject.TYPE_BOOLEAN: gtk.CellRendererToggle,
                       gtk.gdk.Pixbuf.__gtype__: gtk.CellRendererPixbuf}

type_names = {gobject.TYPE_INT: _('Number'),
              gobject.TYPE_STRING: _('String'),
              gobject.TYPE_BOOLEAN: _('Toggle'),
              gtk.gdk.Pixbuf.__gtype__: _('Pixbuf'),
              gtk.TreeStore.__gtype__: _('TreeStore'),
              gtk.ListStore.__gtype__: _('ListStore')}

class TreeModelEditor(gtk.ScrolledWindow):
    """
    The Model view/editor that lets you add/edit/view the TreeModels in the project.
    """
  
    def __init__(self, app):
        """
        Initialize the Model Editor.

        @param app: the application
        @type app: gazpacho.application.Application
        """
        gtk.ScrolledWindow.__init__(self)
        self.set_shadow_type(gtk.SHADOW_IN)

        self._app = app
        self.project = None

        # Fist construct the main GUI for manipulation of TreeModels
        vbox = gtk.VBox(spacing=0)
        hbox = gtk.HBox(spacing=12)
        hbox.set_border_width(6)

        add_button = gtk.Button(stock=gtk.STOCK_ADD)
        self.main_edit_button = gtk.Button(stock=gtk.STOCK_EDIT)
        self.main_remove_button = gtk.Button(stock=gtk.STOCK_REMOVE)

        self.main_remove_button.set_sensitive(False)
        self.main_edit_button.set_sensitive(False)

        add_button.connect('clicked', self._on_main_add_button_clicked)
        self.main_remove_button.connect('clicked', self._on_main_remove_button_clicked)
        self.main_edit_button.connect('clicked', self._on_main_edit_button_clicked)

        self.main_model = gtk.TreeStore(gobject.TYPE_STRING, gobject.TYPE_STRING)
        self.main_view = gtk.TreeView(self.main_model)
        self.main_view.connect('row-activated', self._on_main_view_row_activated)
        selection = self.main_view.get_selection()
        selection.connect('changed', self._on_main_selection_changed)

        titles = [_('Name'), _('Type')]
        for i in range(0, 2):
            renderer = gtk.CellRendererText()
            column = gtk.TreeViewColumn()
            column.pack_start(renderer)
            column.set_attributes(renderer, text=i)
            column.set_property('title', titles[i])
            self.main_view.append_column(column)

        hbox.pack_start(add_button)
        hbox.pack_start(self.main_edit_button)
        hbox.pack_start(self.main_remove_button)
        
        hbox.child_set_property(add_button, 'expand', False)
        hbox.child_set_property(self.main_edit_button, 'expand', False)
        hbox.child_set_property(self.main_remove_button, 'expand', False)

        vbox.pack_start(hbox)
        vbox.child_set_property(hbox, 'expand', False)
        vbox.pack_start(self.main_view)

        self.main_view.set_headers_visible(True)
        self.add_with_viewport(vbox)
        self.set_border_width(6)

    def set_project(self, project):
        """
        Set the current project.

        @param project: the current project
        @type project: gazpacho.project.project
        """
        if self.project:
            self.main_model.clear()
        self.project = project
        if project:
            self._fill_main_model_from_project()
            project.treemodel_registry.connect('model-added', self._on_main_model_added)

    def _fill_main_model_from_project(self):
        for model in self.project.treemodel_registry.list_models():
            model_name = self.project.treemodel_registry.lookup_name(model)
            model_type_name = type_names[model.__gtype__]
            self.main_model.append(None, [model_name, model_type_name])

    def _on_main_add_button_clicked(self, button):
        table = gtk.Table(2, 2)
    
        type_label = gtk.Label(_('Type') + ':')
        table.attach(type_label, 0, 1, 0, 1)
        combo = gtk.combo_box_new_text()
        combo.append_text(type_names[gtk.TreeStore.__gtype__])
        combo.append_text(type_names[gtk.ListStore.__gtype__])
        combo.set_active(0)
        table.attach(combo, 1, 2, 0, 1)
   
        name_label = gtk.Label(_('Name') + ':')
        table.attach(name_label, 0, 1, 1, 2)
        entry = gtk.Entry()
        entry.set_activates_default(True)
        table.attach(entry, 1, 2, 1, 2)
            
        table.child_set_property(type_label, 'x-padding', 12)
        table.child_set_property(type_label, 'y-padding', 18)
        table.child_set_property(combo, 'x-padding', 12)
        table.child_set_property(combo, 'y-padding', 18)
        table.child_set_property(name_label, 'x-padding', 12)
        table.child_set_property(name_label, 'y-padding', 18)
        table.child_set_property(entry, 'x-padding', 12)
        table.child_set_property(entry, 'y-padding', 18)
        
        table.show_all()
    
        dialog = gtk.Dialog('GtkTreeModel',
                        self._app.window,
                        gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
                        (gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT,
                        gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))

        dialog.vbox.pack_start(table)
        dialog.vbox.set_property('spacing', 6)
        dialog.set_default_response(gtk.RESPONSE_ACCEPT)

        response = dialog.run()
        dialog.hide()

        if response == gtk.RESPONSE_ACCEPT:
            model_type_name = combo.get_active_text()
            model_name = entry.get_text()

            if model_type_name and model_name:
                model_type = gobject.type_from_name('Gtk' + model_type_name)
                model = gobject.new(model_type)
                model.set_data('TreeModelEditor::me', True)
                self.project.treemodel_registry.add(model, model_name)
                self.project.changed = True
                self.main_model.append(None, [model_name, model_type_name])
    
    def _on_main_model_added(self, treemodel_registry, model_name, model):
        if model.get_data('TreeModelEditor::me'):
          return
        
        model_type_name = type_names[model.__gtype__]
        self.main_model.append(None, [model_name, model_type_name])
    
    def _on_main_remove_button_clicked(self, button):
        selection = self.main_view.get_selection()
        try:
          path = selection.get_selected_rows()[1][0]
        except IndexError:
          return
        iter = self.main_model.get_iter(path)
        self.project.treemodel_registry.remove_by_name(self.main_model.get_value(iter, 0))
        self.project.changed = True
        self.main_model.remove(iter)

    def _on_renderer_toggled(self, renderer, path, view):
        model = view.get_model()
        iter = model.get_iter(path)
        position = renderer.get_data('position')
        value = not model.get_value(iter, position)
        model.set_value(iter, position, value)

    def _on_renderer_edited(self, renderer, path, new_text, view, gtype):
        model = view.get_model()
        iter = model.get_iter(path)
        position = renderer.get_data('position')

        if gtype == gobject.TYPE_INT:
            try:
                model.set_value(iter, position, int(new_text))
            except ValueError:
                pass
        else:
            model.set_value(iter, position, new_text)

    def _on_file_button_clicked(self, button, parent_dialog, entry):
        file_dialog = gtk.FileChooserDialog(title='Pixbuf Selection',
                        parent=parent_dialog,
                        buttons=(gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT,
                                gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))
        response = file_dialog.run()

        if response == gtk.RESPONSE_ACCEPT:
            response = entry.set_text(file_dialog.get_filename())
        file_dialog.destroy()

    def _on_row_activated(self, treeview, path, view_column):
        renderer = view_column.get_cell_renderers()[0]
        if isinstance(renderer, gtk.CellRendererPixbuf):
            model = treeview.get_model()
            iter = model.get_iter(path)
            position = view_column.get_data('position')
      
            table = gtk.Table(3, 2)

            stock_radio = gtk.RadioButton(None, _('From Stock') + ': ')
            table.attach(stock_radio, 0, 1, 0, 1)
     
            liststore = gtk.ListStore(gtk.gdk.Pixbuf.__gtype__)
            combobox = gtk.ComboBox(liststore)
            cell = gtk.CellRendererPixbuf()
            combobox.pack_start(cell, True)
            combobox.add_attribute(cell, 'pixbuf', 0)
            stock_list = gtk.stock_list_ids()
      
            for id in stock_list:
                pixbuf = treeview.render_icon(id, gtk.ICON_SIZE_SMALL_TOOLBAR, None)
                liststore_iter = liststore.append()
                liststore.set_value(liststore_iter, 0, pixbuf)
            table.attach(combobox, 1, 2, 0, 1)
            combobox.set_active(0)
   
            file_radio = gtk.RadioButton(stock_radio, _('From File') + ': ')
            table.attach(file_radio, 0, 1, 1, 2)
            entry = gtk.Entry()
            entry.set_activates_default(True)
            table.attach(entry, 1, 2, 1, 2)
            button = gtk.Button(stock=gtk.STOCK_OPEN)
            table.attach(button, 2, 3, 1, 2)
            
            table.child_set_property(stock_radio, 'x-padding', 12)
            table.child_set_property(stock_radio, 'y-padding', 18)
            table.child_set_property(combobox, 'x-padding', 12)
            table.child_set_property(combobox, 'y-padding', 18)
            table.child_set_property(file_radio, 'x-padding', 12)
            table.child_set_property(file_radio, 'y-padding', 18)
            table.child_set_property(entry, 'x-padding', 12)
            table.child_set_property(entry, 'y-padding', 18)
            table.child_set_property(button, 'x-padding', 12)
            table.child_set_property(button, 'y-padding', 18)
            
            table.show_all()
    
            dialog = gtk.Dialog('Pixbuf Selection',
                            self._app.window,
                            gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
                            (gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT,
                            gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))

            dialog.vbox.pack_start(table)
            dialog.vbox.set_property('spacing', 6)
            dialog.set_default_response(gtk.RESPONSE_ACCEPT)
            button.connect('clicked', self._on_file_button_clicked, dialog, entry)

            response = dialog.run()
            dialog.hide()

            if response == gtk.RESPONSE_ACCEPT:
                if stock_radio.get_active():
                    stock_id = stock_list[combobox.get_active()]
                    pixbuf = treeview.render_icon(stock_id, gtk.ICON_SIZE_SMALL_TOOLBAR, None)
                    pixbuf.set_data('gazpacho::pixbuf-save-data', stock_id)
                    model.set_value(iter, position, pixbuf)
                else:
                    filename = entry.get_text()
                    try:
                      pixbuf = gtk.gdk.pixbuf_new_from_file_at_size(filename, 32, 24)
                    except gobject.GError:
                      return
                    pixbuf.set_data('gazpacho::pixbuf-save-data', filename)
                    model.set_value(iter, position, pixbuf)

    def _on_main_view_row_activated(self, treeview, path, view_column):
        iter = self.main_model.get_iter(path)
        model_name = self.main_model.get_value(iter, 0)
        self._edit_model(model_name)
    
    def _on_main_selection_changed(self, selection):
        if selection.count_selected_rows() == 0:
            self.main_remove_button.set_sensitive(False)
            self.main_edit_button.set_sensitive(False)
        else:
            self.main_remove_button.set_sensitive(True)
            self.main_edit_button.set_sensitive(True)

    def _on_main_edit_button_clicked(self, button):
        selection = self.main_view.get_selection()
        try:
          path = selection.get_selected_rows()[1][0]
        except IndexError:
          return
        iter = self.main_model.get_iter(path)
        model_name = self.main_model.get_value(iter, 0)
        self._edit_model(model_name)
    
    def _edit_model(self, model_name):
        # Construct the GUI for TreeModel data editing
        edit_dialog = gtk.Dialog('GtkTreeModel',
                         self._app.window,
                         gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
                         (gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT,
                         gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))

        edit_vbox = gtk.VBox(spacing=18)
        edit_hbox = gtk.HBox(spacing=12)

        edit_view = gtk.TreeView()
        edit_view.connect('row-activated', self._on_row_activated)
        edit_view.set_headers_visible(True)

   
        self.column_type_combo = gtk.combo_box_new_text()
        self.column_type_combo.append_text(type_names[gobject.TYPE_INT])
        self.column_type_combo.append_text(type_names[gobject.TYPE_STRING])
        self.column_type_combo.append_text(type_names[gobject.TYPE_BOOLEAN])
        self.column_type_combo.append_text(type_names[gtk.gdk.Pixbuf.__gtype__])
        self.column_type_combo.set_active(1)
    
        column_add_button = gtk.Button(_('Add Column'))
        row_add_button = gtk.Button(_('Add Row'))
        row_remove_button = gtk.Button(_('Remove Row'))
        #dialog_apply_button = gtk.Button(_('Apply'))
    
        column_add_button.connect('clicked', self._on_column_add, edit_view)
        row_add_button.connect('clicked', self._on_row_add, edit_view)
        row_remove_button.connect('clicked', self._on_row_remove, edit_view)
        #dialog_apply_button.connect('clicked', self._on_edit_dialog_apply, edit_view)
    
        edit_hbox.pack_start(self.column_type_combo)
        edit_hbox.pack_start(column_add_button)
        edit_hbox.pack_start(row_add_button)
        edit_hbox.pack_start(row_remove_button)

        edit_hbox.child_set_property(self.column_type_combo, 'expand', False)
        edit_hbox.child_set_property(column_add_button, 'expand', False)
        edit_hbox.child_set_property(row_add_button, 'expand', False)
        edit_hbox.child_set_property(row_remove_button, 'expand', False)
        
        edit_vbox.pack_start(edit_view)
        edit_vbox.pack_start(edit_hbox)
        edit_vbox.child_set_property(edit_hbox, 'expand', False)
        edit_vbox.show_all()
        edit_dialog.vbox.pack_start(edit_vbox)
        #edit_dialog.action_area.pack_start(dialog_apply_button)
        #edit_dialog.action_area.reorder_child(dialog_apply_button, 1)
        edit_dialog.vbox.set_property('spacing', 6)
    
        model = self.project.treemodel_registry.lookup_model(model_name)
        edit_view.set_model(self._duplicate_model(model))
        edit_dialog.set_title(model_name)
  
        #edit_view.connect('drag-drop', self._on_main_treeview_drag_drop)
        if isinstance(model, gtk.TreeStore):
            edit_view.set_reorderable(True)
    
        # Create columns in the TreeView according to the model
        for i in range(0, model.get_n_columns()):
            column_type = model.get_column_type(i)
            renderer = supported_renderers[column_type]()
            column = gtk.TreeViewColumn()
            column.set_data('position', i)
            column.pack_start(renderer)
            renderer.set_data('position', i)
            if column_type == gobject.TYPE_INT or column_type == gobject.TYPE_STRING:
                column.set_attributes(renderer, text=i)
                renderer.set_property('editable', True)
                renderer.connect('edited',
                            self._on_renderer_edited, 
                            edit_view,
                            column_type)
            elif column_type == gobject.TYPE_BOOLEAN:
                column.set_attributes(renderer, active=i)
                renderer.set_property('activatable', True)
                renderer.connect('toggled', self._on_renderer_toggled, edit_view)
            elif column_type == gtk.gdk.Pixbuf.__gtype__:
                column.set_attributes(renderer, pixbuf=i)
            column_title = \
                type_names[column_type] + ':' + str(i) + '\n' + _('Remove column')
            column.set_property('title', column_title)
            column.set_property('clickable', True)
            column.connect('clicked', self._on_column_remove, edit_view)
            edit_view.append_column(column)
    
        response = edit_dialog.run()
        if response == gtk.RESPONSE_ACCEPT:
            # Update the TreeModel Registry
            self.project.treemodel_registry.replace(model, edit_view.get_model())
            self.project.changed = True
        edit_dialog.destroy()
    
    #def _on_edit_dialog_apply(self, button, edit_view):
    #    model = edit_view.get_model()
    #    self.project.treemodel_registry.replace(model, edit_view.get_model())

    def _on_column_add(self, button, edit_view):
        active = self.column_type_combo.get_active()
        if active < 0:
            return
        try:
            new_column_type = supported_types[active]
        except TypeError:
            return
        old_model = edit_view.get_model()
        new_model = self._duplicate_model(old_model, [new_column_type])
    
        # Update the TreeView
        renderer = supported_renderers[new_column_type]()
        column = gtk.TreeViewColumn()
        column.pack_start(renderer)
        position = old_model.get_n_columns()
        column.set_data('position', position)
        renderer.set_data('position', position)
        
        if new_column_type == gobject.TYPE_INT or new_column_type == gobject.TYPE_STRING:
            column.set_attributes(renderer, text=position)
            renderer.set_property('editable', True)
            renderer.connect('edited',
                        self._on_renderer_edited, 
                        edit_view,
                        new_column_type)
        elif new_column_type == gobject.TYPE_BOOLEAN:
            column.set_attributes(renderer, active=position)
            renderer.set_property('activatable', True)
            renderer.connect('toggled', self._on_renderer_toggled, edit_view)
        elif new_column_type == gtk.gdk.Pixbuf.__gtype__:
            column.set_attributes(renderer, pixbuf=position)
        column_title = \
            type_names[new_column_type] + ':' + str(position) + '\n' + _('Remove column')
        column.set_property('title', column_title)
        column.set_property('clickable', True)
        column.connect('clicked', self._on_column_remove, edit_view)
        edit_view.append_column(column)
        edit_view.set_model(new_model)
   
    def _on_column_remove(self, column, edit_view):
        old_model = edit_view.get_model()
        position = column.get_data('position')
        new_model = self._duplicate_model(old_model, [], [position])
        edit_view.set_model(new_model)

        # Update the TreeView
        edit_view.remove_column(column)
 
        for i in range(position, new_model.get_n_columns()):
            column = edit_view.get_column(i)
            column.set_data('position', i)
            renderer = column.get_cell_renderers()[0]
            renderer.set_data('position', i)
            if isinstance(renderer, gtk.CellRendererText):
                column.set_attributes(renderer, text=i)
            elif isinstance(renderer, gtk.CellRendererToggle):
                column.set_attributes(renderer, active=i)
            elif isinstance(renderer, gtk.CellRendererPixbuf):
                column.set_attributes(renderer, pixbuf=i)
       
            column_type = new_model.get_column_type(i)
            column_title = type_names[column_type] + ':' + str(position) + '\n' + _('Remove column')
            column.set_property('title', column_title)
    
    def _on_row_add(self, button, edit_view):
        model = edit_view.get_model()
        self._model_row_add(model, edit_view)

    def _model_row_add(self, model, widget, parent=None):
        if isinstance(model, gtk.ListStore):
            iter = model.append()
        else:
            iter = model.append(parent)

        for i in range(0, model.get_n_columns()):
            if model.get_column_type(i) == gobject.TYPE_STRING:
                model.set_value(iter, i, _('YOUR TEXT HERE'))
            elif model.get_column_type(i) == gtk.gdk.Pixbuf.__gtype__:
                pixbuf = widget.render_icon(gtk.STOCK_MISSING_IMAGE, gtk.ICON_SIZE_SMALL_TOOLBAR, None)
                pixbuf.set_data('gazpacho::pixbuf-save-data', gtk.STOCK_MISSING_IMAGE)
                model.set_value(iter, i, pixbuf)
        return iter

    def _on_row_remove(self, button, edit_view):
        model = edit_view.get_model()

        selection = edit_view.get_selection()
        for path in selection.get_selected_rows()[1]:
            iter = model.get_iter(path)
            model.remove(iter)

    def _duplicate_model(self, old_model, new_column_types=[], to_exclude=[]):
        column_types = []
    
        for i in range(0, old_model.get_n_columns()):
            if i not in to_exclude:
                column_types.append(old_model.get_column_type(i))

        if new_column_types:
            column_types.extend(new_column_types)

        if column_types:
            if isinstance(old_model, gtk.ListStore):
                new_model = gtk.ListStore(*column_types)
            else:
                new_model = gtk.TreeStore(*column_types)
      
            # now copy the data too
            old_iter = old_model.get_iter_first()
            self._copy_model_data(old_model, old_iter, to_exclude, new_model)
        else:
            if isinstance(old_model, gtk.ListStore):
                new_model = gobject.new(gtk.ListStore.__gtype__)
            else:
                new_model = gobject.new(gtk.TreeStore.__gtype__)
    
        return new_model

    def _copy_model_data(self, old_model, old_iter, to_exclude, new_model, new_iter_parent=None):
        if not old_iter:
            return

        new_iter = self._model_row_add(new_model, self.main_view, new_iter_parent)
        
        j=0
        for i in range(0, old_model.get_n_columns()):
            if i not in to_exclude:
                new_model.set_value(new_iter, j, old_model.get_value(old_iter, i))
                j += 1
    
        self._copy_model_data(
                        old_model,
                        old_model.iter_children(old_iter),
                        to_exclude, 
                        new_model,
                        new_iter)
   
        self._copy_model_data(
                        old_model,
                        old_model.iter_next(old_iter),
                        to_exclude, 
                        new_model,
                        new_iter_parent)

gobject.type_register(TreeModelEditor)

