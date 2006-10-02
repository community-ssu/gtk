/* HAIL - The Hildon Accessibility Implementation Library
 * Copyright (C) 2006 Nokia Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:hailtimeeditor
 * @short_description: Implementation of the ATK interfaces for #HildonTimeEditor.
 * @see_also: #HildonTimeEditor
 *
 * #HailTimeEditor implements the required ATK interfaces of #HildonTimeEditor and its
 * children (the buttons inside the widget). It exposes:
 * <itemizedlist>
 * <listitem>An action to expose the am/pm switch </listitem>
 * <listitem>The data entries and the dialog button.</listitem>
 * <listitem>The AtkText interface showing a textual description of the selected time. It
 * should make checking current value easier (it's not required to check the children).</listitem>
 * </itemizedlist>
 */

#include <gdk/gdkkeysyms.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkentry.h>
#include <hildon-widgets/hildon-time-editor.h>
#include <libgail-util/gailtextutil.h>
#include "hailtimeeditor.h"

#define HAIL_TIME_EDITOR_DEFAULT_NAME "Time editor"

typedef struct _HailTimeEditorPrivate HailTimeEditorPrivate;

struct _HailTimeEditorPrivate {
  AtkObject * entry_frame; /* frame emulating an entry */
  AtkObject *hentry; /* hour entry */
  AtkObject *mentry; /* minute entry */
  AtkObject *sentry; /* seconds entry */
  AtkObject *iconbutton; /* dialog launch button */
  AtkObject *event_box; /* am/pm event box */
  gpointer action_idle_handler; /* action idle handler for click emulation */
  GailTextUtil * textutil; /* text ops helper */
  gint cursor_position; /* current cursor position */
  gint selection_bound; /* selection info */
  gint label_length; /* length of the exposed test */
  gchar * exposed_text; /* atk text exposed text */
};

#define HAIL_TIME_EDITOR_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), HAIL_TYPE_TIME_EDITOR, HailTimeEditorPrivate))

static void                  hail_time_editor_class_init       (HailTimeEditorClass *klass);
static void                  hail_time_editor_object_init      (HailTimeEditor      *time_editor);
static void                  hail_time_editor_finalize         (GObject             *object);

static G_CONST_RETURN gchar* hail_time_editor_get_name         (AtkObject       *obj);
static gint                  hail_time_editor_get_n_children   (AtkObject       *obj);
static AtkObject*            hail_time_editor_ref_child        (AtkObject       *obj,
								gint            i);
static void                  hail_time_editor_real_initialize  (AtkObject       *obj,
								gpointer        data);
static void       hail_time_editor_map_gtk               (GtkWidget         *widget,
							  gpointer          data);

/* AtkAction.h */
static void                  hail_time_editor_atk_action_interface_init   (AtkActionIface  *iface);
static gint                  hail_time_editor_action_get_n_actions    (AtkAction       *action);
static gboolean              hail_time_editor_action_do_action        (AtkAction       *action,
								       gint            index);
static const gchar*          hail_time_editor_action_get_name         (AtkAction       *action,
								       gint            index);
static const gchar*          hail_time_editor_action_get_description  (AtkAction       *action,
								       gint            index);
static const gchar*          hail_time_editor_action_get_keybinding   (AtkAction       *action,
								       gint            index);

static void                  add_internal_widgets              (GtkWidget       *widget,
								gpointer        data);

static void       hail_time_editor_atk_text_interface_init     (AtkTextIface      *iface);
/* atktext.h */

static void hail_time_editor_init_text_util (HailTimeEditor *time_editor,
					     GtkWidget *widget);
static gchar*	  hail_time_editor_get_text		   (AtkText	      *text,
                                                    gint	      start_pos,
						    gint	      end_pos);
static gunichar	  hail_time_editor_get_character_at_offset(AtkText	      *text,
						    gint	      offset);
static gchar*     hail_time_editor_get_text_before_offset(AtkText	      *text,
 						    gint	      offset,
						    AtkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gchar*     hail_time_editor_get_text_at_offset    (AtkText	      *text,
 						    gint	      offset,
						    AtkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gchar*     hail_time_editor_get_text_after_offset    (AtkText	      *text,
 						    gint	      offset,
						    AtkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gint	  hail_time_editor_get_character_count   (AtkText	      *text);
static gint	  hail_time_editor_get_caret_offset	   (AtkText	      *text);
static gboolean	  hail_time_editor_set_caret_offset	   (AtkText	      *text,
                                                    gint	      offset);
static gint	  hail_time_editor_get_n_selections	   (AtkText	      *text);
static gchar*	  hail_time_editor_get_selection	   (AtkText	      *text,
                                                    gint	      selection_num,
                                                    gint	      *start_offset,
                                                    gint	      *end_offset);
static gboolean	  hail_time_editor_add_selection	   (AtkText	      *text,
                                                    gint	      start_offset,
                                                    gint	      end_offset);
static gboolean	  hail_time_editor_remove_selection	   (AtkText	      *text,
                                                    gint	      selection_num);
static gboolean	  hail_time_editor_set_selection	   (AtkText	      *text,
                                                    gint	      selection_num,
                                                    gint	      start_offset,
						    gint	      end_offset);
static void hail_time_editor_get_character_extents       (AtkText	      *text,
						    gint 	      offset,
		                                    gint 	      *x,
                    		   	            gint 	      *y,
                                		    gint 	      *width,
                                     		    gint 	      *height,
			        		    AtkCoordType      coords);
static gint hail_time_editor_get_offset_at_point         (AtkText           *text,
                                                    gint              x,
                                                    gint              y,
			                            AtkCoordType      coords);
static AtkAttributeSet* hail_time_editor_get_run_attributes 
                                                   (AtkText           *text,
              					    gint 	      offset,
                                                    gint 	      *start_offset,
					            gint	      *end_offset);
static AtkAttributeSet* hail_time_editor_get_default_attributes
                                                   (AtkText           *text);

void                    hail_time_editor_text_changed (AtkText *entry,
						       gint position,
						       gint len,
						       gpointer data);


#define HAIL_TIME_EDITOR_CLICK_ACTION_NAME "click"
#define HAIL_TIME_EDITOR_CLICK_ACTION_DESCRIPTION "Click the AM/PM button"

static GType parent_type;
static GtkAccessibleClass* parent_class = NULL;

/**
 * hail_time_editor_get_type:
 * 
 * Initialises, and returns the type of a hail time editor.
 * 
 * Return value: GType of #HailTimeEditor.
 **/
GType
hail_time_editor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
	{
	  (guint16) sizeof (HailTimeEditorClass),
	  (GBaseInitFunc) NULL, /* base init */
	  (GBaseFinalizeFunc) NULL, /* base finalize */
	  (GClassInitFunc) hail_time_editor_class_init, /* class init */
	  (GClassFinalizeFunc) NULL, /* class finalize */
	  NULL, /* class data */
	  (guint16) sizeof (HailTimeEditor), /* instance size */
	  0, /* nb preallocs */
	  (GInstanceInitFunc) hail_time_editor_object_init, /* instance init */
	  NULL /* value table */
	};

      static const GInterfaceInfo atk_action_info =
      {
        (GInterfaceInitFunc) hail_time_editor_atk_action_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      static const GInterfaceInfo atk_text_info =
      {
        (GInterfaceInitFunc) hail_time_editor_atk_text_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_CONTAINER);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;


      type = g_type_register_static (parent_type,
                                     "HailTimeEditor", &tinfo, 0);

      g_type_add_interface_static (type, ATK_TYPE_ACTION,
                                   &atk_action_info);
      g_type_add_interface_static (type, ATK_TYPE_TEXT,
                                   &atk_text_info);
    }

  return type;
}

static void
hail_time_editor_class_init (HailTimeEditorClass *klass)
{
  GObjectClass * gobject_class = G_OBJECT_CLASS(klass);
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  gobject_class->finalize = hail_time_editor_finalize;
  parent_class = g_type_class_peek_parent (klass);

  g_type_class_add_private(klass, (gsize) sizeof(HailTimeEditorPrivate));

  /* bind virtual methods for AtkObject */
  class->get_name = hail_time_editor_get_name;
  class->get_n_children = hail_time_editor_get_n_children;
  class->ref_child = hail_time_editor_ref_child;
  class->initialize = hail_time_editor_real_initialize;
}

static void
hail_time_editor_object_init (HailTimeEditor      *time_editor)
{
  GtkWidget  *widget = NULL;
  HailTimeEditorPrivate * priv = NULL;

  priv = HAIL_TIME_EDITOR_GET_PRIVATE(time_editor);
  widget = GTK_ACCESSIBLE(time_editor)->widget;

  priv->hentry = NULL;
  priv->mentry = NULL;
  priv->sentry = NULL;
  priv->entry_frame = NULL;
  priv->iconbutton = NULL;
  priv->event_box = NULL;
  priv->cursor_position = 0;
  priv->textutil = NULL;
  priv->selection_bound = 0;
  priv->label_length = 0;
  priv->exposed_text = NULL;
 
}

/* GObject map gtk event handler. Text util shouldn't be started
 * if the text is not shown yet */
static void
hail_time_editor_map_gtk (GtkWidget *widget,
			  gpointer data)
{
  HailTimeEditor *time_editor;

  time_editor = HAIL_TIME_EDITOR (data);
  hail_time_editor_init_text_util (time_editor, widget);
}

/* updates the exposed text in atk text interface */
static void
hail_time_editor_update_text (HailTimeEditor * time_editor)
{

  guint hour, minutes, seconds;
  HailTimeEditorPrivate * priv = NULL;
  GtkWidget * widget = NULL;

  g_return_if_fail (HAIL_IS_TIME_EDITOR(time_editor));
  widget = GTK_ACCESSIBLE(time_editor)->widget;
  priv = HAIL_TIME_EDITOR_GET_PRIVATE(time_editor);

  hildon_time_editor_get_time(HILDON_TIME_EDITOR(widget), &hour, &minutes, &seconds);

  if (priv->exposed_text != NULL) {
    g_free(priv->exposed_text);
    priv->exposed_text = NULL;
  }

  /* format is 24h */
  priv->exposed_text = g_strdup_printf("%02d:%02d:%02d", hour, minutes, seconds);

  gail_text_util_text_setup (priv->textutil, priv->exposed_text);

  if (priv->exposed_text == NULL)
    priv->label_length = 0;
  else
    priv->label_length = g_utf8_strlen (priv->exposed_text, -1);

}

/* start the text helper from gail */
static void
hail_time_editor_init_text_util (HailTimeEditor *time_editor,
				 GtkWidget *widget)
{
  HailTimeEditorPrivate * priv = NULL;

  g_return_if_fail (HAIL_IS_TIME_EDITOR(time_editor));
  priv = HAIL_TIME_EDITOR_GET_PRIVATE(time_editor);

  if (priv->textutil == NULL)
    priv->textutil = gail_text_util_new ();

  hail_time_editor_update_text(time_editor);
}



/**
 * hail_time_editor_new:
 * @widget: a #HildonTimeEditor casted as a #AtkObject
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonTimeEditor.
 * 
 * Return value: An #AtkObject
 **/
AtkObject* 
hail_time_editor_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_IS_TIME_EDITOR (widget), NULL);

  object = g_object_new (HAIL_TYPE_TIME_EDITOR, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/* Implementation of AtkObject method get_name() */
static G_CONST_RETURN gchar*
hail_time_editor_get_name (AtkObject *obj)
{
  G_CONST_RETURN gchar* name = NULL;

  g_return_val_if_fail (HAIL_IS_TIME_EDITOR (obj), NULL);

  name = ATK_OBJECT_CLASS (parent_class)->get_name (obj);
  if (name == NULL)
    {
      GtkWidget *widget = NULL;

      widget = GTK_ACCESSIBLE (obj)->widget;
      if (widget == NULL)
        /*
         * State is defunct
         */
        return NULL;

      g_return_val_if_fail (HILDON_IS_TIME_EDITOR (widget), NULL);

      name = HAIL_TIME_EDITOR_DEFAULT_NAME;

    }
  return name;
}


/* callback for gtk_container_forall. It's used from atk object initialize
 * to get the references to the AtkObjects of the internal widgets */
static void
add_internal_widgets (GtkWidget * widget,
		      gpointer data)
{
  HailTimeEditor * time_editor = NULL;
  HailTimeEditorPrivate * priv = NULL;

  g_return_if_fail(HAIL_IS_TIME_EDITOR(data));
  time_editor = HAIL_TIME_EDITOR(data);
  priv = HAIL_TIME_EDITOR_GET_PRIVATE(time_editor);

  if (GTK_IS_FRAME(widget)) {
    priv->entry_frame = gtk_widget_get_accessible(widget);
  } else if (GTK_IS_BUTTON(widget)) {
    priv->iconbutton = gtk_widget_get_accessible(widget);
    atk_object_set_name(priv->iconbutton, "Icon button");
  }

}

/* Implementation of AtkObject method initialize(). It gets
 * the internal objects and assign references to accessible
 * children in order to be accessed properly with get_parent
 * and ref_child methods. */
static void
hail_time_editor_real_initialize (AtkObject *obj,
				  gpointer   data)
{
  GtkWidget * time_editor = NULL;
  HailTimeEditorPrivate * priv = NULL;
  static gint num_entries = 0;
  AtkObject * frame_child = NULL;
  AtkObject * hbox = NULL;
  int frame_child_index = 0;

  ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

  obj->role = ATK_ROLE_PANEL;

  /* add children */
  time_editor = GTK_ACCESSIBLE(obj)->widget;
  gtk_container_forall(GTK_CONTAINER(time_editor), add_internal_widgets, obj);

  /* Get private */
  priv = HAIL_TIME_EDITOR_GET_PRIVATE(HAIL_TIME_EDITOR(obj));

  g_return_if_fail (ATK_IS_OBJECT(priv->entry_frame));
  g_return_if_fail (atk_object_get_n_accessible_children(priv->entry_frame)==1);
  hbox = atk_object_ref_accessible_child(priv->entry_frame, 0);
  g_return_if_fail (ATK_IS_OBJECT(hbox));

  for (frame_child_index = 0; 
       frame_child_index < atk_object_get_n_accessible_children(hbox); 
       frame_child_index++) {
    frame_child = atk_object_ref_accessible_child(hbox, frame_child_index);
    /* get entries */
    if (atk_object_get_role(frame_child)==ATK_ROLE_TEXT) {
      switch (num_entries) {
      case 0:   /* hours entry */
	priv->hentry = frame_child;
	atk_object_set_name(priv->hentry, "Hentry");
	atk_object_set_parent(priv->hentry, obj);
	g_object_ref(frame_child);
	num_entries++;
	break;
      case 1:   /* minutes entry */
	priv->mentry = frame_child;
	atk_object_set_name(priv->mentry, "Mentry");
	atk_object_set_parent(priv->mentry, obj);
	g_object_ref(frame_child);
	num_entries++;
	break;
      case 2:   /* seconds entry */
	priv->sentry = frame_child;
	atk_object_set_name(priv->sentry, "Sentry");
	atk_object_set_parent(priv->sentry, obj);
	g_object_ref(frame_child);
	num_entries = 0;
      break;
      }
    } else if (atk_object_get_role(frame_child)==ATK_ROLE_PANEL) {
      priv->event_box = frame_child;
      atk_object_set_name(priv->event_box, "Event box");
      atk_object_set_parent(priv->event_box, obj);
      g_object_ref(frame_child);
    }
    if (ATK_IS_OBJECT(frame_child)) {
      g_object_unref(frame_child);
    }
  }

  g_return_if_fail (ATK_IS_OBJECT(priv->hentry));
  g_return_if_fail (ATK_IS_OBJECT(priv->mentry));
  g_return_if_fail (ATK_IS_OBJECT(priv->sentry));
  g_return_if_fail (ATK_IS_OBJECT(priv->iconbutton));
  g_return_if_fail (ATK_IS_OBJECT(priv->event_box));

  g_return_if_fail (ATK_IS_TEXT(priv->hentry));
  g_return_if_fail (ATK_IS_TEXT(priv->mentry));
  g_return_if_fail (ATK_IS_TEXT(priv->sentry));

  g_signal_connect_after(priv->hentry, "text_changed::insert", 
			 (GCallback) hail_time_editor_text_changed,
			 NULL);
  g_signal_connect_after(priv->mentry, "text_changed::insert", 
			 (GCallback) hail_time_editor_text_changed,
			 NULL);
  g_signal_connect_after(priv->sentry, "text_changed::insert", 
			 (GCallback) hail_time_editor_text_changed,
			 NULL);

  if (GTK_WIDGET_MAPPED (time_editor))
    hail_time_editor_init_text_util (HAIL_TIME_EDITOR(obj), time_editor);
  else
    g_signal_connect (time_editor,
                      "map",
                      G_CALLBACK (hail_time_editor_map_gtk),
                      obj);

}

/* finalize handler to free the textutil helper */
static void
hail_time_editor_finalize (GObject            *object)
{
  HailTimeEditor *editor = HAIL_TIME_EDITOR (object);
  HailTimeEditorPrivate * priv = HAIL_TIME_EDITOR_GET_PRIVATE(editor);

  if (priv->textutil)
    g_object_unref (priv->textutil);
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/* Implementation of AtkObject method get_n_children() */
static gint                  
hail_time_editor_get_n_children   (AtkObject       *obj)
{
  GtkWidget *widget = NULL;
  HailTimeEditorPrivate * priv = NULL;

  g_return_val_if_fail (HAIL_IS_TIME_EDITOR (obj), 0);
  priv = HAIL_TIME_EDITOR_GET_PRIVATE(HAIL_TIME_EDITOR(obj));

  widget = GTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return 0;

  return 5;
}

/* Implementation of AtkObject method ref_child () */
static AtkObject*            
hail_time_editor_ref_child        (AtkObject       *obj,
				   gint            i)
{
  GtkWidget *widget = NULL;
  AtkObject * accessible_child = NULL;
  HailTimeEditorPrivate * priv = NULL;

  g_return_val_if_fail (HAIL_IS_TIME_EDITOR (obj), 0);
  priv = HAIL_TIME_EDITOR_GET_PRIVATE(HAIL_TIME_EDITOR(obj));
  g_return_val_if_fail ((i >= 0), NULL);
  widget = GTK_ACCESSIBLE (obj)->widget;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  switch(i) {
  case 0:
    accessible_child = priv->hentry;
    break;
  case 1:
    accessible_child = priv->mentry;
    break;
  case 2:
    accessible_child = priv->sentry;
    break;
  case 3:
    accessible_child = priv->iconbutton;
    break;
  case 4:
    accessible_child = priv->event_box;
    break;
  }
  g_return_val_if_fail (ATK_IS_OBJECT(accessible_child), NULL);

  g_object_ref(accessible_child);
  return accessible_child;
}

/* Initializes the AtkAction interface, and binds the virtual methods */
static void
hail_time_editor_atk_action_interface_init (AtkActionIface *iface)
{
  g_return_if_fail (iface != NULL);

  iface->do_action = hail_time_editor_action_do_action;
  iface->get_n_actions = hail_time_editor_action_get_n_actions;
  iface->get_name = hail_time_editor_action_get_name;
  iface->get_description = hail_time_editor_action_get_description;
  iface->get_keybinding = hail_time_editor_action_get_keybinding;
}

/* Implementation of AtkAction method get_n_actions */
static gint
hail_time_editor_action_get_n_actions    (AtkAction       *action)
{
  GtkWidget * time_editor = NULL;
  
  g_return_val_if_fail (HAIL_IS_TIME_EDITOR (action), 0);

  time_editor = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_TIME_EDITOR(time_editor), 0);

  return 1;
}

/* idle handler to perform the release button used to emulate a click in the
 * am/pm eventbox */
static gboolean 
hail_time_editor_release_button (gpointer data)
{
  HailTimeEditor *time_editor; 
  GtkWidget *widget;
  GdkEvent tmp_event;
  GdkWindow * parent_window;
  HailTimeEditorPrivate * priv = NULL;

  time_editor = HAIL_TIME_EDITOR (data);

  priv = HAIL_TIME_EDITOR_GET_PRIVATE(time_editor);

  widget = GTK_ACCESSIBLE(priv->event_box)->widget;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  if (!GTK_WIDGET_SENSITIVE (widget) || !GTK_WIDGET_VISIBLE (widget))
    return FALSE;

  /*
   * Simulate a button release event. It gets the gdk window from the widget
   */

  tmp_event.key.type = GDK_KEY_RELEASE;
  parent_window = gtk_widget_get_parent_window(widget);
  tmp_event.key.window = parent_window;
  tmp_event.key.keyval = GDK_Return;
  tmp_event.key.send_event = TRUE;
  tmp_event.key.time = GDK_CURRENT_TIME;
    
  gtk_widget_event (widget, &tmp_event);
  
  return FALSE;
}

/* idle handler to perform the press-release button used to emulate a click in the
 * am/pm eventbox */
static gboolean 
idle_do_action (gpointer data)
{
  HailTimeEditor *time_editor; 
  GtkWidget *widget;
  GdkEvent tmp_event;
  GdkWindow * parent_window;
  HailTimeEditorPrivate * priv = NULL;

  time_editor = HAIL_TIME_EDITOR (data);

  priv = HAIL_TIME_EDITOR_GET_PRIVATE(time_editor);

  widget = GTK_ACCESSIBLE(priv->event_box)->widget;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  if (!GTK_WIDGET_SENSITIVE (widget) || !GTK_WIDGET_VISIBLE (widget))
    return FALSE;

  /*
   * Simulate a button release event. It gets the gdk window from the widget
   * and sends an event to it.
   * Then it hangs an idle call to do the release once the events
   * are processed.
   */

  tmp_event.key.type = GDK_KEY_PRESS;
  parent_window = gtk_widget_get_parent_window(widget);
  tmp_event.key.window = parent_window;
  tmp_event.key.keyval = GDK_Return;
  tmp_event.key.send_event = TRUE;
  tmp_event.key.time = GDK_CURRENT_TIME;
    
  gtk_widget_event (widget, &tmp_event);
  
  g_idle_add (hail_time_editor_release_button, data);
  
  return FALSE;
}


/* Implementation of AtkAction method do_action */
static gboolean
hail_time_editor_action_do_action        (AtkAction       *action,
					  gint            index)
{
  GtkWidget * time_editor = NULL;
  HailTimeEditorPrivate * priv = NULL;

  g_return_val_if_fail (HAIL_IS_TIME_EDITOR (action), FALSE);
  g_return_val_if_fail ((index == 0), FALSE);

  time_editor = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_TIME_EDITOR(time_editor), FALSE);

  priv = HAIL_TIME_EDITOR_GET_PRIVATE(time_editor);

  if (priv->action_idle_handler)
    return FALSE;
  else {
    priv->action_idle_handler = (gpointer) g_timeout_add (500, idle_do_action, action);
    return TRUE;
  }
}

/* Implementation of AtkAction method get_name */
static const gchar*
hail_time_editor_action_get_name         (AtkAction       *action,
					  gint            index)
{
  GtkWidget * time_editor = NULL;

  g_return_val_if_fail (HAIL_IS_TIME_EDITOR (action), NULL);
  g_return_val_if_fail ((index == 0), NULL);

  time_editor = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_TIME_EDITOR(time_editor), NULL);

  return HAIL_TIME_EDITOR_CLICK_ACTION_NAME;
}

/* Implementation of AtkAction method get_description */
static const gchar*
hail_time_editor_action_get_description   (AtkAction       *action,
					   gint            index)
{
  GtkWidget * time_editor = NULL;

  g_return_val_if_fail (HAIL_IS_TIME_EDITOR (action), NULL);
  g_return_val_if_fail ((index == 0), NULL);

  time_editor = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_TIME_EDITOR(time_editor), NULL);

  return HAIL_TIME_EDITOR_CLICK_ACTION_DESCRIPTION;
}

/* Implementation of AtkAction method get_keybinding */
static const gchar*
hail_time_editor_action_get_keybinding   (AtkAction       *action,
					  gint            index)
{
  GtkWidget * time_editor = NULL;

  g_return_val_if_fail (HAIL_IS_TIME_EDITOR (action), NULL);
  g_return_val_if_fail ((index == 0), NULL);

  time_editor = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_TIME_EDITOR(time_editor), NULL);

  return NULL;
}

/* atktext.h */

static void
hail_time_editor_atk_text_interface_init (AtkTextIface *iface)
{
  g_return_if_fail (iface != NULL);
  iface->get_text = hail_time_editor_get_text;
  iface->get_character_at_offset = hail_time_editor_get_character_at_offset;
  iface->get_text_before_offset = hail_time_editor_get_text_before_offset;
  iface->get_text_at_offset = hail_time_editor_get_text_at_offset;
  iface->get_text_after_offset = hail_time_editor_get_text_after_offset;
  iface->get_character_count = hail_time_editor_get_character_count;
  iface->get_caret_offset = hail_time_editor_get_caret_offset;
  iface->set_caret_offset = hail_time_editor_set_caret_offset;
  iface->get_n_selections = hail_time_editor_get_n_selections;
  iface->get_selection = hail_time_editor_get_selection;
  iface->add_selection = hail_time_editor_add_selection;
  iface->remove_selection = hail_time_editor_remove_selection;
  iface->set_selection = hail_time_editor_set_selection;
  iface->get_character_extents = hail_time_editor_get_character_extents;
  iface->get_offset_at_point = hail_time_editor_get_offset_at_point;
  iface->get_run_attributes = hail_time_editor_get_run_attributes;
  iface->get_default_attributes = hail_time_editor_get_default_attributes;
}

/* Implementation of AtkText method get_text() */
static gchar*
hail_time_editor_get_text (AtkText *text,
			   gint    start_pos,
			   gint    end_pos)
{
  GtkWidget *widget = NULL;
  HailTimeEditorPrivate *priv = NULL;

  g_return_val_if_fail (HAIL_IS_TIME_EDITOR(text), NULL);
  widget = GTK_ACCESSIBLE (text)->widget;
  priv = HAIL_TIME_EDITOR_GET_PRIVATE(text);
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  hail_time_editor_update_text(HAIL_TIME_EDITOR(text));
  return gail_text_util_get_substring (priv->textutil, 
				       start_pos, end_pos);
}

/* Implementation of AtkText method get_text_before_offset() */
static gchar*
hail_time_editor_get_text_before_offset (AtkText         *text,
					 gint            offset,
					 AtkTextBoundary boundary_type,
					 gint            *start_offset,
					 gint            *end_offset)
{
  GtkWidget *widget = NULL;
  HailTimeEditorPrivate * priv = NULL;
  
  g_return_val_if_fail (HAIL_IS_TIME_EDITOR(text), NULL);
  widget = GTK_ACCESSIBLE (text)->widget;
  priv = HAIL_TIME_EDITOR_GET_PRIVATE(text);
  
  if (widget == NULL)
    /* State is defunct */
    return NULL;
  
  hail_time_editor_update_text(HAIL_TIME_EDITOR(text));

  return gail_text_util_get_text (priv->textutil,
                           NULL, GAIL_BEFORE_OFFSET, 
                           boundary_type, offset, start_offset, end_offset); 
}

/* Implementation of AtkText method get_text_at_offset() */
static gchar*
hail_time_editor_get_text_at_offset (AtkText         *text,
				     gint            offset,
				     AtkTextBoundary boundary_type,
				     gint            *start_offset,
				     gint            *end_offset)
{
  GtkWidget *widget = NULL;
  HailTimeEditorPrivate * priv = NULL;
 
  g_return_val_if_fail(HAIL_IS_TIME_EDITOR(text), NULL);
  widget = GTK_ACCESSIBLE (text)->widget;
  priv = HAIL_TIME_EDITOR_GET_PRIVATE(text);
  
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  hail_time_editor_update_text(HAIL_TIME_EDITOR(text));
  
  return gail_text_util_get_text (priv->textutil,
                              NULL, GAIL_AT_OFFSET, 
                              boundary_type, offset, start_offset, end_offset);
}

/* Implementation of AtkText method get_text_after_offset() */
static gchar*
hail_time_editor_get_text_after_offset (AtkText         *text,
					gint            offset,
					AtkTextBoundary boundary_type,
					gint            *start_offset,
					gint            *end_offset)
{
  GtkWidget *widget = NULL;
  HailTimeEditorPrivate * priv = NULL;

  g_return_val_if_fail (HAIL_IS_TIME_EDITOR(text), NULL);
  widget = GTK_ACCESSIBLE (text)->widget;
  priv = HAIL_TIME_EDITOR_GET_PRIVATE(text);
  
  if (widget == NULL)
  {
    /* State is defunct */
    return NULL;
  }
  
  hail_time_editor_update_text(HAIL_TIME_EDITOR(text));
  
  return gail_text_util_get_text (priv->textutil,
                           NULL, GAIL_AFTER_OFFSET, 
                           boundary_type, offset, start_offset, end_offset);
}

/* Implementation of AtkText method get_character_count() */
static gint
hail_time_editor_get_character_count (AtkText *text)
{
  GtkWidget *widget = NULL;
  HailTimeEditorPrivate * priv = NULL;

  g_return_val_if_fail (HAIL_IS_TIME_EDITOR(text), 0);
  widget = GTK_ACCESSIBLE (text)->widget;
  priv = HAIL_TIME_EDITOR_GET_PRIVATE(text);

  if (widget == NULL)
    /* State is defunct */
    return 0;

  hail_time_editor_update_text(HAIL_TIME_EDITOR(text));
  
  return g_utf8_strlen (priv->exposed_text, -1);
}

/* Implementation of AtkText method get_caret_offset() */
static gint
hail_time_editor_get_caret_offset (AtkText *text)
{
  HailTimeEditorPrivate * priv = NULL;

  g_return_val_if_fail (HAIL_IS_TIME_EDITOR(text), 0);
  priv = HAIL_TIME_EDITOR_GET_PRIVATE(text);
  
  return priv->cursor_position;
}

/* Implementation of AtkText method set_caret_offset() */
static gboolean
hail_time_editor_set_caret_offset (AtkText *text, 
				   gint    offset)
{
  /* no text selection support */
  return FALSE;
}

/* Implementation of AtkText method get_n_selections() */
static gint
hail_time_editor_get_n_selections (AtkText *text)
{
  /* no text selection support */
  return 0;
}

/* Implementation of AtkText method get_selection() */
static gchar*
hail_time_editor_get_selection (AtkText *text,
				gint    selection_num,
				gint    *start_pos,
				gint    *end_pos)
{
  /* no text selection support */
  return NULL;
}

/* Implementation of AtkText method add_selection() */
static gboolean
hail_time_editor_add_selection (AtkText *text,
				gint    start_pos,
				gint    end_pos)
{
  /* no text selection support */
  return FALSE;
}

/* Implementation of AtkText method remove_selection() */
static gboolean
hail_time_editor_remove_selection (AtkText *text,
				   gint    selection_num)
{
  /* no text selection support */
  return FALSE;
}

/* Implementation of AtkText method set_selection() */
static gboolean
hail_time_editor_set_selection (AtkText *text,
				gint	  selection_num,
				gint    start_pos,
				gint    end_pos)
{
  /* no text selection support */
  return FALSE;
}

/* Implementation of AtkText method get_character_extents() */
static void
hail_time_editor_get_character_extents (AtkText      *text,
					gint         offset,
					gint         *x,
					gint 	     *y,
					gint 	     *width,
					gint 	     *height,
					AtkCoordType coords)
{
  /* no complex layout info */
  return;
} 

/* Implementation of AtkText method get_offset_at_point() */
static gint 
hail_time_editor_get_offset_at_point (AtkText      *text,
				      gint         x,
				      gint         y,
				      AtkCoordType coords)
{ 
  /* no complex layout info */
  return -1;
}

/* Implementation of AtkText method get_run_attributes() */
static AtkAttributeSet*
hail_time_editor_get_run_attributes (AtkText        *text,
				     gint 	      offset,
				     gint 	      *start_offset,
				     gint	      *end_offset)
{
  /* no attributes */
  return NULL;
}

/* Implementation of AtkText method get_default_attributes() */
static AtkAttributeSet*
hail_time_editor_get_default_attributes (AtkText        *text)
{
  /* no attributes */
  return NULL;
}

/* Implementation of AtkText method get_character_at_offset() */
static gunichar 
hail_time_editor_get_character_at_offset (AtkText	         *text,
					  gint	         offset)
{
  GtkWidget *widget = NULL;
  HailTimeEditorPrivate *priv = NULL;
  const gchar *string;
  gchar *index;

  g_return_val_if_fail (HAIL_IS_TIME_EDITOR(text), (gunichar) '\0');
  widget = GTK_ACCESSIBLE (text)->widget;
  priv = HAIL_TIME_EDITOR_GET_PRIVATE(text);

  if (widget == NULL)
    /* State is defunct */
    return (gunichar) '\0';

  hail_time_editor_update_text(HAIL_TIME_EDITOR(text));
  
  string = priv->exposed_text;
  if (offset >= g_utf8_strlen (string, -1))
    return (gunichar) '\0';
  index = g_utf8_offset_to_pointer (string, offset);

  return g_utf8_get_char (index);
}

/* this event handler is for updating the hail time editor value when
 * the entries are modified */
void
hail_time_editor_text_changed (AtkText *entry,
			       gint position,
			       gint len,
			       gpointer data)
{
  GtkWidget * widget = NULL;

  g_return_if_fail (ATK_IS_TEXT(entry));
  g_return_if_fail (GTK_IS_ACCESSIBLE(entry));

  widget = GTK_ACCESSIBLE(entry)->widget;
  g_return_if_fail (GTK_IS_EDITABLE(widget));

  g_signal_emit_by_name(widget, "focus-out-event");

}
