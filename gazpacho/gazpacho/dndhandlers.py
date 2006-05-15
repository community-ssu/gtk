import gtk

from gazpacho import util

(DND_POS_TOP,
 DND_POS_BOTTOM,
 DND_POS_LEFT,
 DND_POS_RIGHT,
 DND_POS_BEFORE,
 DND_POS_AFTER) = range(6)

(INFO_TYPE_XML,
 INFO_TYPE_WIDGET,
 INFO_TYPE_PALETTE) = range(3)

MIME_TYPE_OBJECT_XML = 'application/x-gazpacho-object-xml'
MIME_TYPE_OBJECT     = 'application/x-gazpacho-object'
MIME_TYPE_OBJECT_PALETTE = 'application/x-gazpacho-palette'

# This is the value returned by Widget.drag_dest_find_target when no
# targets have been found. The API reference says it should return
# None (and NONE) but it seems to return the string "NONE".
DND_NO_TARGET = "NONE"

DND_WIDGET_TARGET  = (MIME_TYPE_OBJECT, gtk.TARGET_SAME_APP, INFO_TYPE_WIDGET)
DND_XML_TARGET     = (MIME_TYPE_OBJECT_XML, 0, INFO_TYPE_XML)
DND_PALETTE_TARGET = (MIME_TYPE_OBJECT_PALETTE, gtk.TARGET_SAME_APP,
                      INFO_TYPE_PALETTE)

class DnDHandler(object):

    def connect_drag_handlers(self, gtk_widget):
        targets = [DND_WIDGET_TARGET, DND_XML_TARGET]        
        gtk_widget.drag_source_set(gtk.gdk.BUTTON1_MASK,
                                   targets,
                                   gtk.gdk.ACTION_COPY |
                                   gtk.gdk.ACTION_MOVE)

        gtk_widget.connect('drag_begin', self._on_drag_begin)
        gtk_widget.connect('drag_data_get', self._on_drag_data_get)

    def _on_drag_begin(self, source_widget, drag_context):
        """Callback for the 'drag-begin' event."""
        raise NotImplementedError

    def _on_drag_data_get(self, source_widget, context, selection_data, info, time):
        """Callback for the 'drag-data-get' event."""
        raise NotImplementedError

    def connect_drop_handlers(self, gtk_widget):
        """Connect all handlers necessary for the widget to serve as a
        drag destination."""
        targets = [DND_WIDGET_TARGET, DND_XML_TARGET, DND_PALETTE_TARGET]
        gtk_widget.drag_dest_set(0, targets,
                                 gtk.gdk.ACTION_MOVE | gtk.gdk.ACTION_COPY)
         
        gtk_widget.connect('drag_motion', self._on_drag_motion)
        gtk_widget.connect('drag_leave', self._on_drag_leave)
        gtk_widget.connect('drag_drop', self._on_drag_drop)
        gtk_widget.connect('drag_data_received', self._on_drag_data_received)

    def _get_target_project(self, gtk_target):
        """Get the project for the target widget."""
        raise NotImplementedError
    
    def _set_drag_highlight(self, gtk_target, x, y):
        """Highlight the gtk_widget in an appropriate way to indicate
        that this is a valid drop zone.

        @param gtk_target: the gtk target widget
        @type gtk_target: gtk.Widget
        @param x: the mouse x position
        @type x: int 
        @param y: the mouse y position
        @type y: int
        """
        raise NotImplementedError

    def _clear_drag_highlight(self, gtk_target):
        """Clear the drag highligt.

        @param gtk_target: the gtk target widget
        @type gtk_target: gtk.Widget
        """
        raise NotImplementedError

    def _is_valid_drop_zone(self, drag_context, gtk_target):
        """Check whether the drop zone is valid or not.

        @param drag_context: the drag context
        @type drag_context: gtk.gdk.DragContext
        @param gtk_target: the target gtk widget
        @type gtk_target: gtk.Widget
        """
        from gazpacho.widget import Widget
        
        # Check if it is a valid target type
        targets = gtk_target.drag_dest_get_target_list()
        target = gtk_target.drag_dest_find_target(drag_context, targets)
        if target == DND_NO_TARGET:
            return False
        
        # Not a valid drop zone if target == source
        gtk_source = drag_context.get_source_widget()
        gsource = gtk_source and Widget.from_widget(gtk_source)
        if gsource:
            dnd_source = gsource.dnd_widget
            if not dnd_source or (dnd_source.gtk_widget == gtk_target):
                return False

            # Make sure we don't try to drop a widget onto one of its
            # children
            parent = util.get_parent(gtk_target)
            while parent:
                if parent == dnd_source:
                    return False
                parent = parent.get_parent()

        return True

    def _get_drag_action(self, drag_context, target_widget):
        """Get the drag action. If the target is in the same project
        as the source we move the widget, otherwise we copy it.

        @param gtk_source: the source gtk widget
        @type gtk_source: gtk.Widget
        @param target_project: the project for the target widget
        @type target_project: gazpacho.project.Project
        """
        from gazpacho.widget import Widget

        target_project = self._get_target_project(target_widget)
        gtk_source = drag_context.get_source_widget()
        gsource = gtk_source and Widget.from_widget(gtk_source)
        if not gsource or gsource.project != target_project:
            return gtk.gdk.ACTION_COPY
            
        return gtk.gdk.ACTION_MOVE

    def _on_drag_leave(self, target_widget, drag_context, time):
        """Callback for the 'drag-leave' event. We clear the drag
        highlight."""
        self._clear_drag_highlight(target_widget)
            
    def _on_drag_motion(self, target_widget, drag_context, x, y, time):
        """Callback for the 'drag-motion' event. If the drop zone is
        valid we set the drag highlight and the drag action."""
        if not self._is_valid_drop_zone(drag_context, target_widget):
            return False

        self._set_drag_highlight(target_widget, x, y)

        drag_action = self._get_drag_action(drag_context, target_widget)
        drag_context.drag_status(drag_action, time)
        return True

    def _on_drag_drop(self, target_widget, context, x, y, time):
        """Callback for handling the 'drag_drop' event."""
        if not context.targets:
            return False

        if context.get_source_widget():
            # For DnD within the application we request to use the
            # widget or adaptor directly
            if MIME_TYPE_OBJECT_PALETTE in context.targets:
                mime_type = MIME_TYPE_OBJECT_PALETTE
            else:
                mime_type = MIME_TYPE_OBJECT
        else:
            # otherwise we'll have request the data to be passed as XML
            mime_type = MIME_TYPE_OBJECT_XML
            
        target_widget.drag_get_data(context, mime_type, time)

        return True

    def _on_drag_data_received(self, target_widget, context,
                                  x, y, data, info, time):
        """Callback for the 'drag-data-recieved' event."""
        raise NotImplementedError

    def _get_dnd_widget(self, data, info, context, target_project):
        """Get the actual gazpacho Widget that is dragged. This is not
        necessarily the same widget as the one that recieved the drag
        event.
        """
        from gazpacho.widget import Widget, load_widget_from_xml

        dnd_widget = None        
        if info == INFO_TYPE_WIDGET:
            gtk_source = context.get_source_widget()            
            gsource = Widget.from_widget(gtk_source)
            dnd_widget = gsource.dnd_widget

        elif info == INFO_TYPE_XML:
            dnd_widget = load_widget_from_xml(data.data, target_project)

        return dnd_widget

class WidgetDnDHandler(DnDHandler):

    def _on_drag_begin(self, gtk_source, drag_context):
        """Callback for handling the 'drag_begin' event. It will just
        set a nice looking drag icon."""
        from gazpacho.widget import Widget

        gsource = Widget.from_widget(gtk_source)
        if gsource.dnd_widget:
            pixbuf = gsource.dnd_widget.adaptor.icon.get_pixbuf()
            gtk_source.drag_source_set_icon_pixbuf(pixbuf)

    def _on_drag_data_get(self, gtk_source, context, selection_data, info, time):
        """Callback for the 'drag-data-get' event."""
        from gazpacho.filewriter import XMLWriter
        from gazpacho.widget import Widget

        gsource = Widget.from_widget(gtk_source)
        dnd_widget = gsource.dnd_widget

        # If we can't get the widget we indicate this failure by
        # passing an empty string. Not sure if it's correct but it
        # works for us.
        if not dnd_widget:
            selection_data.set(selection_data.target, 8, "")
            return

        # The widget should be passed as XML
        if info == INFO_TYPE_XML:
            # Convert to XML
            xw = XMLWriter(project = dnd_widget.project)
            xml_data = xw.serialize(dnd_widget)

            selection_data.set(selection_data.target, 8, xml_data)

        # The widget can be retrieved directly and we only pass the name
        elif info == INFO_TYPE_WIDGET:
            selection_data.set(selection_data.target, 8, dnd_widget.name)
        # We don't understand the request and pass nothing
        else:
            selection_data.set(selection_data.target, 8, "")

    def _get_target_project(self, gtk_target):
        from gazpacho.widget import Widget

        gtarget = Widget.from_widget(gtk_target)
        return gtarget.project

    def _set_drag_highlight(self, gtk_target, x, y):
        from gazpacho.widget import Widget

        location, region = self._get_drop_location(gtk_target, x, y)
        gwidget = Widget.from_widget(gtk_target)
        gwidget.set_drop_region(region)

    def _clear_drag_highlight(self, gtk_target):
        from gazpacho.widget import Widget
        
        gwidget = Widget.from_widget(gtk_target)
        gwidget.clear_drop_region()
    
    def _get_drop_location(self, target_widget, x, y):
        """Calculate the drop region and also the drop location
        relative to the widget.

        The location can be one of the following constants
        - DND_POS_TOP
        - DND_POS_BOTTOM
        - DND_POS_LEFT
        - DND_POS_RIGHT
        - DND_POS_BEFORE
        - DND_POS_AFTER

        The drop region is a tuple of x,y,width and height.

        @return: (location, region)
        @rtype: tuple
        """
        parent = util.get_parent(target_widget)
        
        h_appendable = isinstance(parent.gtk_widget, gtk.HBox)
        v_appendable = isinstance(parent.gtk_widget, gtk.VBox)
        
        x_off, y_off, width, height = target_widget.allocation
        
        x_third = width / 3
        y_third = height / 3

        if x > x_third * 2:
            if h_appendable:
                location = DND_POS_AFTER
            else:
                location = DND_POS_RIGHT
            region = (x_third * 2 + x_off, 2 + y_off,
                      x_third, height - 4)
        elif x < x_third:
            if h_appendable:
                location = DND_POS_BEFORE
            else:
                location = DND_POS_LEFT
            region = (2 + x_off, 2 + y_off, x_third - 2, height - 4)
        elif y > y_third * 2:
            if v_appendable:
                location = DND_POS_AFTER
            else:
                location = DND_POS_BOTTOM
            region = (2 + x_off, y_third * 2 + y_off, width - 4, y_third - 2)
        elif y < y_third:
            if v_appendable:
                location = DND_POS_BEFORE
            else:
                location = DND_POS_TOP
            region = (2 + x_off, 2 + y_off, width - 4, y_third)
        else:
            location = None
            region = (0, 0, 0, 0) 

        return location, region

    def _on_drag_data_received(self, target_widget, context, x, y, data,
                                   info, time):
        """
        Callback for handling the 'drag_data_received' event. If
        the drag/drop is in the same project the target handles
        everything. If it's between different projects the target only
        handles the drop (paste) and the source takes care of the drag
        (cut).
        """
        from gazpacho.widget import Widget

        # If there is no data we indicate that the drop was not
        # successful
        if not data.data:
            context.finish(False, False, time)
            return

        project = self._get_target_project(target_widget)

        #if info == INFO_TYPE_PALETTE:
        #    parent = self.get_parent()
        #    
        #    if isinstance(parent.gtk_widget, gtk.VBox):
        #        pass
        #        #if adding verticly 
        #        #    add a cell to the vbox
        #        # else
        #        #    add a vbox 
        #    elif isinstance(parent.gtk_widget, gtk.HBox):
        #        pass
        #        #if adding horizontaly
        #        #    add a cell to the hbox
        #        # else
        #        #    add a hbox 
        #    else:
        #        pass
        #        #if adding verticly
        #        #    add a vbox
        #        # else
        #        #    add a hbox
        #
        #    #Add dragged widget to the created container 
        #    #mgr.create(self.project._app.add_class, self, None)
        #    context.finish(True, False, time)
        #    return

        # If we can't get the widget we indicate that the drop was not
        # successful
        dnd_widget = self._get_dnd_widget(data, info, context, project)
        if not dnd_widget:
            context.finish(False, False, time)
            return

        keep_source = (context.action != gtk.gdk.ACTION_MOVE)

        mgr = project.get_app().command_manager

        location, region = self._get_drop_location(target_widget, x, y)
        if location in [DND_POS_TOP, DND_POS_BOTTOM, DND_POS_LEFT, DND_POS_RIGHT]:
            gtarget = Widget.from_widget(target_widget)
            mgr.execute_drag_extend(dnd_widget, gtarget, location, keep_source)
            
        elif location in [DND_POS_AFTER, DND_POS_BEFORE]:
            parent = util.get_parent(target_widget)
            pos = parent.gtk_widget.get_children().index(target_widget)
            if location == DND_POS_AFTER:
                pos = pos + 1
            mgr.execute_drag_append(dnd_widget, parent, pos, keep_source)
        else:
            context.finish(False, False, time)
            return

        context.finish(True, False, time)

class PlaceholderDnDHandler(DnDHandler):

    def _get_target_project(self, gtk_target):
        return gtk_target.get_project()

    def _set_drag_highlight(self, gtk_target, x, y):
        gtk_target.drag_highlight()

    def _clear_drag_highlight(self, gtk_target):
        gtk_target.drag_unhighlight()

    def _on_drag_data_received(self, target_widget, context, x, y, data,
                               info, time):
        """Callback for handling the 'drag_data_received' event. If
        the drag/drop is in the same project the target handles
        everything. If it's between different projects the target only
        handles the drop (paste) and the source takes care of the drag
        (cut)."""        
        from gazpacho.widget import copy_widget
        # If there is no data we indicate that the drop was not
        # successful
        if not data.data:
            context.finish(False, False, time)
            return

        project = self._get_target_project(target_widget)
        mgr = project.get_app().command_manager

        if info in [INFO_TYPE_WIDGET, INFO_TYPE_XML]:
            
            dnd_widget = self._get_dnd_widget(data, info, context, project)
            if not dnd_widget:
                context.finish(False, False, time)
                return

            if context.action == gtk.gdk.ACTION_MOVE:
                mgr.execute_drag_drop(dnd_widget, target_widget)
            else:
                dnd_widget_copy = copy_widget(dnd_widget, project)
                mgr.execute_drop(dnd_widget_copy, target_widget)
            
            context.finish(True, False, time)

        elif info == INFO_TYPE_PALETTE:
            mgr.create(project.get_app().add_class, target_widget, None)
            context.finish(True, False, time)
            
        # We don't know how to handle the data and ndicate that the
        # drop was not successful
        else:
            context.finish(False, False, time)
