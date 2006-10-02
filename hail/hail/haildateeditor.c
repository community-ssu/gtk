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
 * SECTION:haildateeditor
 * @short_description: Implementation of the ATK interfaces for #HildonDateEditor.
 * @see_also: #HildonDateEditor
 *
 * #HailDateEditor implements the required ATK interfaces of #HildonDateEditor and its
 * children (the buttons inside the widget). It exposes:
 * <itemizedlist>
 * <listitem>An action to expose the button which launches the calendar dialog</listitem>
 * <listitem>The data entries inside their frame, with significant names</listitem>
 * <listitem>An #AtkText with a standard representation (YYYY-MM-DD) of the day</listitem>
 * </itemizedlist>
 * It also links the modification events of the children, in order to handle changes and
 * reflect them in the exposed #AtkText.
 */

#include <string.h>
#include <langinfo.h>
#include <gdk/gdkkeysyms.h>
#include <hildon-widgets/hildon-date-editor.h>
#include <libgail-util/gailtextutil.h>
#include "haildateeditor.h"


#define HAIL_DATE_EDITOR_DEFAULT_NAME "Edit date"
#define HAIL_DATE_EDITOR_YEAR_NAME "Year"
#define HAIL_DATE_EDITOR_MONTH_NAME "Month"
#define HAIL_DATE_EDITOR_DAY_NAME "Day"
#define HAIL_DATE_EDITOR_ACTION_NAME "Show calendar"
#define HAIL_DATE_EDITOR_ACTION_DESCRIPTION "Show calendar dialog"

/* the orders of fields allowed */
enum {
    MONTH_DAY_YEAR,
    DAY_MONTH_YEAR,
    YEAR_MONTH_DAY
};


typedef struct _HailDateEditorPrivate HailDateEditorPrivate;

struct _HailDateEditorPrivate {
  AtkObject * frame;  /* reference to the internal frame holding the buttons */
  guint locale_type; /* the locale type specifying the order of the fields */
  GtkWidget * internal_entry; /* one of the entries (for signaling purposes) */
  gboolean clicked;
  gpointer action_idle_handler; /* action idle handler, used to handle the dialog eventbox */
  GailTextUtil * textutil; /* text ops helper */
  gint cursor_position; /* current cursor position */
  gint selection_bound; /* selection info */
  gint label_length; /* length of the exposed test */
  gchar * exposed_text; /* atk text exposed text */
};

#define HAIL_DATE_EDITOR_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), HAIL_TYPE_DATE_EDITOR, HailDateEditorPrivate))

static void                  hail_date_editor_class_init       (HailDateEditorClass *klass);
static void                  hail_date_editor_object_init      (HailDateEditor      *date_editor);

static G_CONST_RETURN gchar* hail_date_editor_get_name         (AtkObject       *obj);
static gint                  hail_date_editor_get_n_children   (AtkObject       *obj);
static AtkObject*            hail_date_editor_ref_child        (AtkObject       *obj,
								gint            i);
static void                  hail_date_editor_real_initialize  (AtkObject       *obj,
								gpointer        data);

/* AtkAction */
static void                  hail_date_editor_atk_action_interface_init   (AtkActionIface  *iface);
static gint                  hail_date_editor_action_get_n_actions    (AtkAction       *action);
static gboolean              hail_date_editor_action_do_action        (AtkAction       *action,
								       gint            index);
static const gchar*          hail_date_editor_action_get_name         (AtkAction       *action,
								       gint            index);
static const gchar*          hail_date_editor_action_get_description  (AtkAction       *action,
								       gint            index);
static const gchar*          hail_date_editor_action_get_keybinding   (AtkAction       *action,
								       gint            index);

static void       hail_date_editor_atk_text_interface_init     (AtkTextIface      *iface);
/* atktext.h */

static void hail_date_editor_init_text_util (HailDateEditor *date_editor,
					     GtkWidget *widget);
static gchar*	  hail_date_editor_get_text		   (AtkText	      *text,
							    gint	      start_pos,
							    gint	      end_pos);
static gunichar	  hail_date_editor_get_character_at_offset(AtkText	      *text,
							   gint	      offset);
static gchar*     hail_date_editor_get_text_before_offset(AtkText	      *text,
							  gint	      offset,
							  AtkTextBoundary   boundary_type,
							  gint	      *start_offset,
							  gint	      *end_offset);
static gchar*     hail_date_editor_get_text_at_offset    (AtkText	      *text,
							  gint	      offset,
							  AtkTextBoundary   boundary_type,
							  gint	      *start_offset,
							  gint	      *end_offset);
static gchar*     hail_date_editor_get_text_after_offset    (AtkText	      *text,
							     gint	      offset,
							     AtkTextBoundary   boundary_type,
							     gint	      *start_offset,
							     gint	      *end_offset);
static gint	  hail_date_editor_get_character_count   (AtkText	      *text);
static gint	  hail_date_editor_get_caret_offset	   (AtkText	      *text);
static gboolean	  hail_date_editor_set_caret_offset	   (AtkText	      *text,
							    gint	      offset);
static gint	  hail_date_editor_get_n_selections	   (AtkText	      *text);
static gchar*	  hail_date_editor_get_selection	   (AtkText	      *text,
							    gint	      selection_num,
							    gint	      *start_offset,
							    gint	      *end_offset);
static gboolean	  hail_date_editor_add_selection	   (AtkText	      *text,
							    gint	      start_offset,
							    gint	      end_offset);
static gboolean	  hail_date_editor_remove_selection	   (AtkText	      *text,
							    gint	      selection_num);
static gboolean	  hail_date_editor_set_selection	   (AtkText	      *text,
							    gint	      selection_num,
							    gint	      start_offset,
							    gint	      end_offset);
static void hail_date_editor_get_character_extents       (AtkText	      *text,
							  gint 	      offset,
							  gint 	      *x,
							  gint 	      *y,
							  gint 	      *width,
							  gint 	      *height,
							  AtkCoordType      coords);
static gint hail_date_editor_get_offset_at_point         (AtkText           *text,
							  gint              x,
							  gint              y,
							  AtkCoordType      coords);
static AtkAttributeSet* hail_date_editor_get_run_attributes (AtkText           *text,
							     gint 	      offset,
							     gint 	      *start_offset,
							     gint	      *end_offset);
static AtkAttributeSet* hail_date_editor_get_default_attributes (AtkText           *text);

void                    hail_date_editor_text_changed (AtkText *entry,
						       gint position,
						       gint len,
						       gpointer data);


/* internal methods */
static void                  add_internal_widgets                  (GtkWidget * widget,
								    gpointer data);
static guint                 hail_date_editor_check_locale_settings (void);


static GType parent_type;
static GtkAccessible * parent_class = NULL;

/**
 * hail_date_editor_get_type:
 * 
 * Initialises, and returns the type of a hail date_editor.
 * 
 * Return value: GType of #HailDateEditor.
 **/
GType
hail_date_editor_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
      {
        (guint16) sizeof (HailDateEditorClass),
        (GBaseInitFunc) NULL, /* base init */
        (GBaseFinalizeFunc) NULL, /* base finalize */
        (GClassInitFunc) hail_date_editor_class_init, /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL, /* class data */
        (guint16) sizeof (HailDateEditor), /* instance size */
        0, /* nb preallocs */
        (GInstanceInitFunc) hail_date_editor_object_init, /* instance init */
        NULL /* value table */
      };

      static const GInterfaceInfo atk_action_info =
      {
        (GInterfaceInitFunc) hail_date_editor_atk_action_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      static const GInterfaceInfo atk_text_info =
      {
        (GInterfaceInitFunc) hail_date_editor_atk_text_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_CONTAINER);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      type = g_type_register_static (parent_type,
                                     "HailDateEditor", &tinfo, 0);

      g_type_add_interface_static (type, ATK_TYPE_ACTION,
                                   &atk_action_info);
      g_type_add_interface_static (type, ATK_TYPE_TEXT,
                                   &atk_text_info);

    }

  return type;
}

static void
hail_date_editor_class_init (HailDateEditorClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  g_type_class_add_private(klass, (gsize) sizeof(HailDateEditorPrivate));

  /* bind virtual methods for AtkObject */
  class->get_name = hail_date_editor_get_name;
  class->get_n_children = hail_date_editor_get_n_children;
  class->ref_child = hail_date_editor_ref_child;
  class->initialize = hail_date_editor_real_initialize;

}

/* nullify or initialize attributes */
static void
hail_date_editor_object_init (HailDateEditor      *date_editor)
{
  HailDateEditorPrivate * priv = NULL;

  priv = HAIL_DATE_EDITOR_GET_PRIVATE(date_editor);
  priv->frame = NULL;
  priv->locale_type = hail_date_editor_check_locale_settings();
  priv->internal_entry = NULL;
  priv->clicked = FALSE;
  priv->action_idle_handler = NULL;
  priv->cursor_position = 0;
  priv->textutil = NULL;
  priv->selection_bound = 0;
  priv->label_length = 0;
  priv->exposed_text = NULL;
}


/* GObject map gtk event handler. Text util shouldn't be started
 * if the text is not shown yet */
static void
hail_date_editor_map_gtk (GtkWidget *widget,
			  gpointer data)
{
  HailDateEditor *date_editor = NULL;

  date_editor = HAIL_DATE_EDITOR (data);
  hail_date_editor_init_text_util (date_editor, widget);
}

/* updates the exposed text in atk text interface */
static void
hail_date_editor_update_text (HailDateEditor * date_editor)
{

  guint year, month, day;
  HailDateEditorPrivate * priv = NULL;
  GtkWidget * widget = NULL;

  g_return_if_fail (HAIL_IS_DATE_EDITOR(date_editor));
  widget = GTK_ACCESSIBLE(date_editor)->widget;
  priv = HAIL_DATE_EDITOR_GET_PRIVATE(date_editor);

  hildon_date_editor_get_date(HILDON_DATE_EDITOR(widget), &year, &month, &day);

  if (priv->exposed_text != NULL) {
    g_free(priv->exposed_text);
    priv->exposed_text = NULL;
  }

  /* format is YYYY-MM-DD */
  priv->exposed_text = g_strdup_printf("%04d-%02d-%02d", year, month, day);

  gail_text_util_text_setup (priv->textutil, priv->exposed_text);

  if (priv->exposed_text == NULL)
    priv->label_length = 0;
  else
    priv->label_length = g_utf8_strlen (priv->exposed_text, -1);

}

/* start the text helper from gail */
static void
hail_date_editor_init_text_util (HailDateEditor *date_editor,
				 GtkWidget *widget)
{
  HailDateEditorPrivate * priv = NULL;

  g_return_if_fail (HAIL_IS_DATE_EDITOR(date_editor));
  priv = HAIL_DATE_EDITOR_GET_PRIVATE(date_editor);

  if (priv->textutil == NULL)
    priv->textutil = gail_text_util_new ();

  hail_date_editor_update_text(date_editor);
}

/**
 * hail_date_editor_new:
 * @widget: a #HildonDateEditor casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonDateEditor.
 * 
 * Return value: A #AtkObject
 **/
AtkObject* 
hail_date_editor_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_IS_DATE_EDITOR (widget), NULL);

  object = g_object_new (HAIL_TYPE_DATE_EDITOR, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/* Implementation of AtkObject method get_name() */
static G_CONST_RETURN gchar*
hail_date_editor_get_name (AtkObject *obj)
{
  G_CONST_RETURN gchar* name = NULL;

  g_return_val_if_fail (HAIL_IS_DATE_EDITOR (obj), NULL);

  name = ATK_OBJECT_CLASS (parent_class)->get_name (obj);
  if (name == NULL)
    {
      /*
       * Get the text on the label
       */
      GtkWidget *widget = NULL;

      widget = GTK_ACCESSIBLE (obj)->widget;
      if (widget == NULL)
        /*
         * State is defunct
         */
        return NULL;

      g_return_val_if_fail (HILDON_IS_DATE_EDITOR (widget), NULL);

      /* It gets the name from the date_editor label */
      name = HAIL_DATE_EDITOR_DEFAULT_NAME;

    }
  return name;
}

/* callback for gtk_container_forall. It's used from atk object initialize
 * to get the references to the AtkObjects of the internal buttons */
static void
add_internal_widgets (GtkWidget * widget,
		      gpointer data)
{
  HailDateEditor * picker = NULL;
  AtkObject * accessible_child = NULL;
  HailDateEditorPrivate *priv = NULL;
  AtkObject * date_container = NULL;
  AtkObject * embedded = NULL;

  g_return_if_fail (HAIL_IS_DATE_EDITOR(data));
  picker = HAIL_DATE_EDITOR(data);
  priv = HAIL_DATE_EDITOR_GET_PRIVATE(picker);
  if (GTK_IS_FRAME(widget)) {
    accessible_child = gtk_widget_get_accessible(widget);
    priv->frame = accessible_child;
    date_container = atk_object_ref_accessible_child(accessible_child, 0);
    g_return_if_fail(ATK_IS_OBJECT(date_container));

    embedded = atk_object_ref_accessible_child(date_container,0);
    priv->internal_entry = GTK_ACCESSIBLE(embedded)->widget;
    g_object_unref(embedded);

    /* depending on the order, we have to get the children in the correct order and
       give them the proper name */
    if (priv->locale_type ==DAY_MONTH_YEAR) {
      embedded = atk_object_ref_accessible_child(date_container,0);
      atk_object_set_name(embedded, HAIL_DATE_EDITOR_DAY_NAME);
      g_object_unref(embedded);
      embedded = atk_object_ref_accessible_child(date_container,2);
      atk_object_set_name(embedded, HAIL_DATE_EDITOR_MONTH_NAME);
      g_object_unref(embedded);
      embedded = atk_object_ref_accessible_child(date_container,4);
      atk_object_set_name(embedded, HAIL_DATE_EDITOR_YEAR_NAME);
      g_object_unref(embedded);
      } else  if (priv->locale_type == MONTH_DAY_YEAR) {
      embedded = atk_object_ref_accessible_child(date_container,0);
      atk_object_set_name(embedded, HAIL_DATE_EDITOR_MONTH_NAME);
      g_object_unref(embedded);
      embedded = atk_object_ref_accessible_child(date_container,2);
      atk_object_set_name(embedded, HAIL_DATE_EDITOR_DAY_NAME);
      g_object_unref(embedded);
      embedded = atk_object_ref_accessible_child(date_container,4);
      atk_object_set_name(embedded, HAIL_DATE_EDITOR_YEAR_NAME);
      g_object_unref(embedded);
      } else {
      embedded = atk_object_ref_accessible_child(date_container,0);
      atk_object_set_name(embedded, HAIL_DATE_EDITOR_YEAR_NAME);
      g_object_unref(embedded);
      embedded = atk_object_ref_accessible_child(date_container,2);
      atk_object_set_name(embedded, HAIL_DATE_EDITOR_MONTH_NAME);
      g_object_unref(embedded);
      embedded = atk_object_ref_accessible_child(date_container,4);
      atk_object_set_name(embedded, HAIL_DATE_EDITOR_DAY_NAME);
      g_object_unref(embedded);
    }

    embedded = atk_object_ref_accessible_child(date_container,0);
    g_signal_connect_after(embedded, "text_changed::insert", 
			   (GCallback) hail_date_editor_text_changed,
			   NULL);
    g_object_unref(embedded);
    embedded = atk_object_ref_accessible_child(date_container,2);
    g_signal_connect_after(embedded, "text_changed::insert", 
			   (GCallback) hail_date_editor_text_changed,
			   NULL);
    g_object_unref(embedded);
    embedded = atk_object_ref_accessible_child(date_container,4);
    g_signal_connect_after(embedded, "text_changed::insert", 
			   (GCallback) hail_date_editor_text_changed,
			   NULL);
    g_object_unref(embedded);
      
    g_object_unref(date_container);
  }
}

/* Implementation of AtkObject method initialize() */
static void
hail_date_editor_real_initialize (AtkObject *obj,
                             gpointer   data)
{
  GtkWidget * widget = NULL;
  ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

  obj->role = ATK_ROLE_PANEL;

  widget = GTK_ACCESSIBLE (obj)->widget;
  gtk_container_forall(GTK_CONTAINER(widget), add_internal_widgets, obj);

  if (GTK_WIDGET_MAPPED (widget))
    hail_date_editor_init_text_util (HAIL_DATE_EDITOR(obj), widget);
  else
    g_signal_connect (widget,
                      "map",
                      G_CALLBACK (hail_date_editor_map_gtk),
                      obj);

}

/* Implementation of AtkObject method get_n_children () */
static gint                  
hail_date_editor_get_n_children   (AtkObject       *obj)
{
  GtkWidget *widget = NULL;

  g_return_val_if_fail (HAIL_IS_DATE_EDITOR (obj), 0);

  widget = GTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return 0;

  /* The internal frame */
  return 1;

}

/* Implementation of AtkObject method ref_child() */
static AtkObject*            
hail_date_editor_ref_child        (AtkObject       *obj,
gint            i)
{
  GtkWidget *widget = NULL;
  AtkObject * accessible_child = NULL;
  HailDateEditorPrivate * priv = NULL;
  
  g_return_val_if_fail (HAIL_IS_DATE_EDITOR (obj), 0);
  priv = HAIL_DATE_EDITOR_GET_PRIVATE(HAIL_DATE_EDITOR(obj));
  g_return_val_if_fail (i == 0, NULL);
  widget = GTK_ACCESSIBLE (obj)->widget;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;
  
  
  /* get the internal frame */
  accessible_child = priv->frame;
  g_return_val_if_fail (ATK_IS_OBJECT(accessible_child), NULL);
  
  g_object_ref(accessible_child);
  return accessible_child;
}

/* internal utility function to get the current locale setting for day-month-year order */
static guint
hail_date_editor_check_locale_settings()
{
  gchar *dfmt = NULL;
  
  dfmt = nl_langinfo(D_FMT);

  if (!strncmp(dfmt, "%d", 2))
    return DAY_MONTH_YEAR;
  else if (!strncmp(dfmt, "%m", 2))
        return MONTH_DAY_YEAR;
  else
    return YEAR_MONTH_DAY;
}

/* Initializes the AtkAction interface, and binds the virtual methods */
static void
hail_date_editor_atk_action_interface_init (AtkActionIface *iface)
{
  g_return_if_fail (iface != NULL);
  
  iface->do_action = hail_date_editor_action_do_action;
  iface->get_n_actions = hail_date_editor_action_get_n_actions;
  iface->get_name = hail_date_editor_action_get_name;
  iface->get_description = hail_date_editor_action_get_description;
  iface->get_keybinding = hail_date_editor_action_get_keybinding;
}

/* Implementation of AtkAction method get_n_actions */
static gint
hail_date_editor_action_get_n_actions    (AtkAction       *action)
{
  GtkWidget * date_editor = NULL;

  g_return_val_if_fail (HAIL_IS_DATE_EDITOR (action), 0);
  
  date_editor = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_DATE_EDITOR(date_editor), 0);
  
  return 1;
}

/* idle handler to perform the release button used to emulate a click in the
 * dialog eventbox */
static gboolean 
hail_date_editor_release_button (gpointer data)
{
  HailDateEditor *date_editor = NULL; 
  GtkWidget *widget = NULL;
  GdkEvent tmp_event;
  GdkWindow * parent_window = NULL;
  HailDateEditorPrivate * priv = NULL;

  date_editor = HAIL_DATE_EDITOR (data);

  priv = HAIL_DATE_EDITOR_GET_PRIVATE(date_editor);

  widget = priv->internal_entry;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  if (!GTK_WIDGET_SENSITIVE (widget) || !GTK_WIDGET_VISIBLE (widget))
    return FALSE;

  /*
   * Simulate a button release event. It gets the gdk window from one
   * of the entries, and the same entry is used to send the event.
   */

  tmp_event.key.type = GDK_KEY_RELEASE;
  parent_window = gtk_widget_get_parent_window(priv->internal_entry);
  tmp_event.key.window = parent_window;
  tmp_event.key.keyval = GDK_Return;
  tmp_event.key.send_event = TRUE;
  tmp_event.key.time = GDK_CURRENT_TIME;
    
  gtk_widget_event (widget, &tmp_event);
  
  return FALSE;
}

/* idle handler to perform the press-release button used to emulate a click in the
 * dialog eventbox */
static gboolean 
idle_do_action (gpointer data)
{
  HailDateEditor *date_editor = NULL; 
  GtkWidget *widget = NULL;
  GdkEvent tmp_event;
  GdkWindow * parent_window = NULL;
  HailDateEditorPrivate * priv = NULL;

  date_editor = HAIL_DATE_EDITOR (data);

  priv = HAIL_DATE_EDITOR_GET_PRIVATE(date_editor);

  widget = priv->internal_entry;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  if (!GTK_WIDGET_SENSITIVE (widget) || !GTK_WIDGET_VISIBLE (widget))
    return FALSE;

  /*
   * Simulate a button press event. It gets the gdk window from one
   * of the entries, and the same entry is used to send the event.
   * Then it hangs an idle call to do the release once the events
   * are processed.
   */

  tmp_event.key.type = GDK_KEY_PRESS;
  parent_window = gtk_widget_get_parent_window(priv->internal_entry);
  tmp_event.key.window = parent_window;
  tmp_event.key.keyval = GDK_Return;
  tmp_event.key.send_event = TRUE;
  tmp_event.key.time = GDK_CURRENT_TIME;
    
  gtk_widget_event (widget, &tmp_event);
  
  g_idle_add (hail_date_editor_release_button, data);
  
  return FALSE;
}

/* Implementation of AtkAction method do_action */
static gboolean
hail_date_editor_action_do_action        (AtkAction       *action,
					  gint            index)
{
  GtkWidget * date_editor = NULL;
  HailDateEditorPrivate * priv = NULL;
  
  g_return_val_if_fail (HAIL_IS_DATE_EDITOR (action), FALSE);
  g_return_val_if_fail ((index == 0), FALSE);
  priv = HAIL_DATE_EDITOR_GET_PRIVATE(HAIL_DATE_EDITOR(action));
  
  date_editor = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_DATE_EDITOR(date_editor), FALSE);
  
  if (priv->action_idle_handler)
    return FALSE;
  else {
    priv->clicked = FALSE;
    priv->action_idle_handler = (gpointer) g_timeout_add (500, idle_do_action, action);
    return TRUE;
  }

}

/* Implementation of AtkAction method get_name */
static const gchar*
hail_date_editor_action_get_name         (AtkAction       *action,
					  gint            index)
{
  GtkWidget * date_editor = NULL;
  
  g_return_val_if_fail (HAIL_IS_DATE_EDITOR (action), NULL);
  g_return_val_if_fail ((index == 0), NULL);
  
  date_editor = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_DATE_EDITOR(date_editor), NULL);
  
  return HAIL_DATE_EDITOR_ACTION_NAME;
}

/* Implementation of AtkAction method get_description */
static const gchar*
hail_date_editor_action_get_description   (AtkAction       *action,
					   gint            index)
{
  GtkWidget * date_editor = NULL;
  
  g_return_val_if_fail (HAIL_IS_DATE_EDITOR (action), NULL);
  g_return_val_if_fail ((index == 0), NULL);
  
  date_editor = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_DATE_EDITOR(date_editor), NULL);
  
  return HAIL_DATE_EDITOR_ACTION_DESCRIPTION;
}

/* Implementation of AtkAction method get_keybinding */
static const gchar*
hail_date_editor_action_get_keybinding   (AtkAction       *action,
					  gint            index)
{
  GtkWidget * date_editor = NULL;
  
  g_return_val_if_fail (HAIL_IS_DATE_EDITOR (action), NULL);
  g_return_val_if_fail ((index == 0), NULL);
  
  date_editor = GTK_ACCESSIBLE (action)->widget;
  g_return_val_if_fail (HILDON_IS_DATE_EDITOR(date_editor), NULL);
  
  return NULL;
}

/* atktext.h */

static void
hail_date_editor_atk_text_interface_init (AtkTextIface *iface)
{
  g_return_if_fail (iface != NULL);
  iface->get_text = hail_date_editor_get_text;
  iface->get_character_at_offset = hail_date_editor_get_character_at_offset;
  iface->get_text_before_offset = hail_date_editor_get_text_before_offset;
  iface->get_text_at_offset = hail_date_editor_get_text_at_offset;
  iface->get_text_after_offset = hail_date_editor_get_text_after_offset;
  iface->get_character_count = hail_date_editor_get_character_count;
  iface->get_caret_offset = hail_date_editor_get_caret_offset;
  iface->set_caret_offset = hail_date_editor_set_caret_offset;
  iface->get_n_selections = hail_date_editor_get_n_selections;
  iface->get_selection = hail_date_editor_get_selection;
  iface->add_selection = hail_date_editor_add_selection;
  iface->remove_selection = hail_date_editor_remove_selection;
  iface->set_selection = hail_date_editor_set_selection;
  iface->get_character_extents = hail_date_editor_get_character_extents;
  iface->get_offset_at_point = hail_date_editor_get_offset_at_point;
  iface->get_run_attributes = hail_date_editor_get_run_attributes;
  iface->get_default_attributes = hail_date_editor_get_default_attributes;
}

/* Implementation of AtkText method get_text() */
static gchar*
hail_date_editor_get_text (AtkText *text,
			   gint    start_pos,
			   gint    end_pos)
{
  GtkWidget *widget = NULL;
  HailDateEditorPrivate *priv = NULL;

  g_return_val_if_fail (HAIL_IS_DATE_EDITOR(text), NULL);
  widget = GTK_ACCESSIBLE (text)->widget;
  priv = HAIL_DATE_EDITOR_GET_PRIVATE(text);
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  hail_date_editor_update_text(HAIL_DATE_EDITOR(text));
  return gail_text_util_get_substring (priv->textutil, 
				       start_pos, end_pos);
}

/* Implementation of AtkText method get_text_before_offset() */
static gchar*
hail_date_editor_get_text_before_offset (AtkText         *text,
					 gint            offset,
					 AtkTextBoundary boundary_type,
					 gint            *start_offset,
					 gint            *end_offset)
{
  GtkWidget *widget = NULL;
  HailDateEditorPrivate * priv = NULL;
  
  g_return_val_if_fail (HAIL_IS_DATE_EDITOR(text), NULL);
  widget = GTK_ACCESSIBLE (text)->widget;
  priv = HAIL_DATE_EDITOR_GET_PRIVATE(text);
  
  if (widget == NULL)
    /* State is defunct */
    return NULL;
  
  hail_date_editor_update_text(HAIL_DATE_EDITOR(text));

  return gail_text_util_get_text (priv->textutil,
                           NULL, GAIL_BEFORE_OFFSET, 
                           boundary_type, offset, start_offset, end_offset); 
}

/* Implementation of AtkText method get_text_at_offset() */
static gchar*
hail_date_editor_get_text_at_offset (AtkText         *text,
				     gint            offset,
				     AtkTextBoundary boundary_type,
				     gint            *start_offset,
				     gint            *end_offset)
{
  GtkWidget *widget = NULL;
  HailDateEditorPrivate * priv = NULL;
 
  g_return_val_if_fail(HAIL_IS_DATE_EDITOR(text), NULL);
  widget = GTK_ACCESSIBLE (text)->widget;
  priv = HAIL_DATE_EDITOR_GET_PRIVATE(text);
  
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  hail_date_editor_update_text(HAIL_DATE_EDITOR(text));
  
  return gail_text_util_get_text (priv->textutil,
                              NULL, GAIL_AT_OFFSET, 
                              boundary_type, offset, start_offset, end_offset);
}

/* Implementation of AtkText method get_text_after_offset() */
static gchar*
hail_date_editor_get_text_after_offset (AtkText         *text,
					gint            offset,
					AtkTextBoundary boundary_type,
					gint            *start_offset,
					gint            *end_offset)
{
  GtkWidget *widget = NULL;
  HailDateEditorPrivate * priv = NULL;

  g_return_val_if_fail (HAIL_IS_DATE_EDITOR(text), NULL);
  widget = GTK_ACCESSIBLE (text)->widget;
  priv = HAIL_DATE_EDITOR_GET_PRIVATE(text);
  
  if (widget == NULL)
  {
    /* State is defunct */
    return NULL;
  }
  
  hail_date_editor_update_text(HAIL_DATE_EDITOR(text));
  
  return gail_text_util_get_text (priv->textutil,
                           NULL, GAIL_AFTER_OFFSET, 
                           boundary_type, offset, start_offset, end_offset);
}

/* Implementation of AtkText method get_character_count() */
static gint
hail_date_editor_get_character_count (AtkText *text)
{
  GtkWidget *widget = NULL;
  HailDateEditorPrivate * priv = NULL;

  g_return_val_if_fail (HAIL_IS_DATE_EDITOR(text), 0);
  widget = GTK_ACCESSIBLE (text)->widget;
  priv = HAIL_DATE_EDITOR_GET_PRIVATE(text);

  if (widget == NULL)
    /* State is defunct */
    return 0;

  hail_date_editor_update_text(HAIL_DATE_EDITOR(text));
  
  return g_utf8_strlen (priv->exposed_text, -1);
}

/* Implementation of AtkText method get_caret_offset() */
static gint
hail_date_editor_get_caret_offset (AtkText *text)
{
  HailDateEditorPrivate * priv = NULL;

  g_return_val_if_fail (HAIL_IS_DATE_EDITOR(text), 0);
  priv = HAIL_DATE_EDITOR_GET_PRIVATE(text);
  
  return priv->cursor_position;
}

/* Implementation of AtkText method set_caret_offset() */
static gboolean
hail_date_editor_set_caret_offset (AtkText *text, 
				   gint    offset)
{
  /* no text selection support */
  return FALSE;
}

/* Implementation of AtkText method get_n_selections() */
static gint
hail_date_editor_get_n_selections (AtkText *text)
{
  /* no text selection support */
  return 0;
}

/* Implementation of AtkText method get_selection() */
static gchar*
hail_date_editor_get_selection (AtkText *text,
				gint    selection_num,
				gint    *start_pos,
				gint    *end_pos)
{
  /* no text selection support */
  return NULL;
}

/* Implementation of AtkText method add_selection() */
static gboolean
hail_date_editor_add_selection (AtkText *text,
				gint    start_pos,
				gint    end_pos)
{
  /* no text selection support */
  return FALSE;
}

/* Implementation of AtkText method remove_selection() */
static gboolean
hail_date_editor_remove_selection (AtkText *text,
				   gint    selection_num)
{
  /* no text selection support */
  return FALSE;
}

/* Implementation of AtkText method set_selection() */
static gboolean
hail_date_editor_set_selection (AtkText *text,
				gint	  selection_num,
				gint    start_pos,
				gint    end_pos)
{
  /* no text selection support */
  return FALSE;
}

/* Implementation of AtkText method get_character_extents() */
static void
hail_date_editor_get_character_extents (AtkText      *text,
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
hail_date_editor_get_offset_at_point (AtkText      *text,
				      gint         x,
				      gint         y,
				      AtkCoordType coords)
{ 
  /* no complex layout info */
  return -1;
}

/* Implementation of AtkText method get_run_attributes() */
static AtkAttributeSet*
hail_date_editor_get_run_attributes (AtkText        *text,
				     gint 	      offset,
				     gint 	      *start_offset,
				     gint	      *end_offset)
{
  /* no attributes */
  return NULL;
}

/* Implementation of AtkText method get_default_attributes() */
static AtkAttributeSet*
hail_date_editor_get_default_attributes (AtkText        *text)
{
  /* no attributes */
  return NULL;
}

/* Implementation of AtkText method get_character_at_offset() */
static gunichar 
hail_date_editor_get_character_at_offset (AtkText	         *text,
					  gint	         offset)
{
  GtkWidget *widget = NULL;
  HailDateEditorPrivate *priv = NULL;
  gchar *string = NULL;
  gchar *index = NULL;

  g_return_val_if_fail (HAIL_IS_DATE_EDITOR(text), (gunichar) '\0');
  widget = GTK_ACCESSIBLE (text)->widget;
  priv = HAIL_DATE_EDITOR_GET_PRIVATE(text);

  if (widget == NULL)
    /* State is defunct */
    return (gunichar) '\0';

  hail_date_editor_update_text(HAIL_DATE_EDITOR(text));
  
  string = priv->exposed_text;
  if (offset >= g_utf8_strlen (string, -1))
    return (gunichar) '\0';
  index = g_utf8_offset_to_pointer (string, offset);

  return g_utf8_get_char (index);
}

/* this event handler is for updating the hail date editor value when
 * the entries are modified */
void
hail_date_editor_text_changed (AtkText *entry,
			       gint position,
			       gint len,
			       gpointer data)
{
  GtkWidget * widget = NULL;

  g_return_if_fail (ATK_IS_TEXT(entry));
  g_return_if_fail (GTK_IS_ACCESSIBLE(entry));

  widget = GTK_ACCESSIBLE(entry)->widget;
  g_return_if_fail (GTK_IS_EDITABLE(widget));

  g_signal_emit_by_name(widget, "changed");

}
