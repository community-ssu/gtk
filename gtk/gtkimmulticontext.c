/* GTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
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

#include "config.h"

#include <string.h>
#include <locale.h>

#include "gtkimmulticontext.h"
#include "gtkimmodule.h"
#include "gtkmain.h"
#include "gtkradiomenuitem.h"
#include "gtkintl.h"
#include "gtkprivate.h" /* To get redefinition of GTK_LOCALE_DIR on Win32 */
#include "gtkalias.h"

struct _GtkIMMulticontextPrivate
{
  GdkWindow *client_window;
  GdkRectangle cursor_location;

  guint use_preedit : 1;
  guint have_cursor_location : 1;
  guint focus_in : 1;
};

static void     gtk_im_multicontext_finalize           (GObject                 *object);

static void     gtk_im_multicontext_set_slave          (GtkIMMulticontext       *multicontext,
							GtkIMContext            *slave,
							gboolean                 finalizing);

static void     gtk_im_multicontext_set_client_window  (GtkIMContext            *context,
							GdkWindow               *window);
static void     gtk_im_multicontext_get_preedit_string (GtkIMContext            *context,
							gchar                  **str,
							PangoAttrList          **attrs,
							gint                   *cursor_pos);
static gboolean gtk_im_multicontext_filter_keypress    (GtkIMContext            *context,
							GdkEventKey             *event);
static void     gtk_im_multicontext_focus_in           (GtkIMContext            *context);
static void     gtk_im_multicontext_focus_out          (GtkIMContext            *context);
static void     gtk_im_multicontext_reset              (GtkIMContext            *context);
static void     gtk_im_multicontext_set_cursor_location (GtkIMContext            *context,
							GdkRectangle		*area);
static void     gtk_im_multicontext_set_use_preedit    (GtkIMContext            *context,
							gboolean                 use_preedit);
static gboolean gtk_im_multicontext_get_surrounding    (GtkIMContext            *context,
							gchar                  **text,
							gint                    *cursor_index);
static void     gtk_im_multicontext_set_surrounding    (GtkIMContext            *context,
							const char              *text,
							gint                     len,
							gint                     cursor_index);

static void     gtk_im_multicontext_preedit_start_cb        (GtkIMContext      *slave,
							     GtkIMMulticontext *multicontext);
static void     gtk_im_multicontext_preedit_end_cb          (GtkIMContext      *slave,
							     GtkIMMulticontext *multicontext);
static void     gtk_im_multicontext_preedit_changed_cb      (GtkIMContext      *slave,
							     GtkIMMulticontext *multicontext);
static void     gtk_im_multicontext_commit_cb               (GtkIMContext      *slave,
							     const gchar       *str,
							     GtkIMMulticontext *multicontext);
static gboolean gtk_im_multicontext_retrieve_surrounding_cb (GtkIMContext      *slave,
							     GtkIMMulticontext *multicontext);
static gboolean gtk_im_multicontext_delete_surrounding_cb   (GtkIMContext      *slave,
							     gint               offset,
							     gint               n_chars,
							     GtkIMMulticontext *multicontext);

#ifdef MAEMO_CHANGES
static gboolean hildon_gtk_im_multicontext_filter_event (GtkIMContext            *context,
                                                         GdkEvent                *event);

static void     gtk_im_multicontext_show                (GtkIMContext            *context);
static void     gtk_im_multicontext_hide                (GtkIMContext            *context);

static void     gtk_im_multicontext_notify              (GObject                 *object,
                                                         GParamSpec              *pspec);

static gboolean gtk_im_multicontext_has_selection_cb            (GtkIMContext                   *slave,
                                                                 GtkIMMulticontext              *multicontext);
static void     gtk_im_multicontext_clipboard_operation_cb      (GtkIMContext                   *slave,
                                                                 GtkIMContextClipboardOperation  op,
                                                                 GtkIMMulticontext              *multicontext);
static void     gtk_im_multicontext_slave_input_mode_changed_cb (GtkIMContext                   *slave,
                                                                 GParamSpec                     *pspec,
                                                                 GtkIMMulticontext              *multicontext);

static GtkIMContext *gtk_im_multicontext_get_slave              (GtkIMMulticontext              *multicontext);
#endif /* MAEMO_CHANGES */

static const gchar *user_context_id = NULL;

#ifndef MAEMO_CHANGES
static const gchar *global_context_id = NULL;
#else /* MAEMO_CHANGES */
static gchar *
get_global_context_id (void)
{
  gint actual_format, actual_length;
  gchar *context_id;
  GdkAtom atom, type, actual_type;
  gboolean succeeded;

  atom = gdk_atom_intern ("gtk-global-immodule", FALSE);
  type = gdk_atom_intern ("STRING", FALSE);

  succeeded = gdk_property_get (gdk_screen_get_root_window (gdk_screen_get_default ()),
                                atom,
                                type,
                                0,
                                G_MAXLONG,
                                FALSE,
                                &actual_type,
                                &actual_format,
                                &actual_length,
                                (guchar **) &context_id);

  if (!succeeded)
    {
      /* Fall back to default locale */
      context_id = g_strdup (_gtk_im_module_get_default_context_id (NULL));
    }

  return context_id;
}
#endif /* MAEMO_CHANGES */


G_DEFINE_TYPE (GtkIMMulticontext, gtk_im_multicontext, GTK_TYPE_IM_CONTEXT)

#ifdef MAEMO_CHANGES
static void
gtk_im_multicontext_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GtkIMContext *slave = gtk_im_multicontext_get_slave (GTK_IM_MULTICONTEXT (object));
  GParamSpec *param_spec;

  param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (slave),
                                             pspec->name);

  if (param_spec != NULL)
    g_object_set_property (G_OBJECT(slave), pspec->name, value);
}

static void
gtk_im_multicontext_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GtkIMContext *slave = gtk_im_multicontext_get_slave (GTK_IM_MULTICONTEXT(object));

  g_object_get_property (G_OBJECT (slave), pspec->name, value);
}
#endif /* MAEMO_CHANGES */

static void
gtk_im_multicontext_class_init (GtkIMMulticontextClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkIMContextClass *im_context_class = GTK_IM_CONTEXT_CLASS (class);
  
  im_context_class->set_client_window = gtk_im_multicontext_set_client_window;
  im_context_class->get_preedit_string = gtk_im_multicontext_get_preedit_string;
  im_context_class->filter_keypress = gtk_im_multicontext_filter_keypress;
  im_context_class->focus_in = gtk_im_multicontext_focus_in;
  im_context_class->focus_out = gtk_im_multicontext_focus_out;
  im_context_class->reset = gtk_im_multicontext_reset;
  im_context_class->set_cursor_location = gtk_im_multicontext_set_cursor_location;
  im_context_class->set_use_preedit = gtk_im_multicontext_set_use_preedit;
  im_context_class->set_surrounding = gtk_im_multicontext_set_surrounding;
  im_context_class->get_surrounding = gtk_im_multicontext_get_surrounding;

  gobject_class->finalize = gtk_im_multicontext_finalize;

#ifdef MAEMO_CHANGES
  im_context_class->filter_event = hildon_gtk_im_multicontext_filter_event;
  im_context_class->show = gtk_im_multicontext_show;
  im_context_class->hide = gtk_im_multicontext_hide;

  gobject_class->notify = gtk_im_multicontext_notify;

  gobject_class->set_property = gtk_im_multicontext_set_property;
  gobject_class->get_property = gtk_im_multicontext_get_property;
#endif /* MAEMO_CHANGES */

  g_type_class_add_private (gobject_class, sizeof (GtkIMMulticontextPrivate));   
}

static void
gtk_im_multicontext_init (GtkIMMulticontext *multicontext)
{
  multicontext->slave = NULL;
  
  multicontext->priv = G_TYPE_INSTANCE_GET_PRIVATE (multicontext, GTK_TYPE_IM_MULTICONTEXT, GtkIMMulticontextPrivate);
  multicontext->priv->use_preedit = TRUE;
  multicontext->priv->have_cursor_location = FALSE;
  multicontext->priv->focus_in = FALSE;
}

/**
 * gtk_im_multicontext_new:
 *
 * Creates a new #GtkIMMulticontext.
 *
 * Returns: a new #GtkIMMulticontext.
 **/
GtkIMContext *
gtk_im_multicontext_new (void)
{
  return g_object_new (GTK_TYPE_IM_MULTICONTEXT, NULL);
}

static void
gtk_im_multicontext_finalize (GObject *object)
{
  GtkIMMulticontext *multicontext = GTK_IM_MULTICONTEXT (object);
  
  gtk_im_multicontext_set_slave (multicontext, NULL, TRUE);
  g_free (multicontext->context_id);

  G_OBJECT_CLASS (gtk_im_multicontext_parent_class)->finalize (object);
}

static void
gtk_im_multicontext_set_slave (GtkIMMulticontext *multicontext,
			       GtkIMContext      *slave,
			       gboolean           finalizing)
{
  GtkIMMulticontextPrivate *priv = multicontext->priv;
  gboolean need_preedit_changed = FALSE;
#ifdef MAEMO_CHANGES
  HildonGtkInputMode input_mode;
#endif /* MAEMO_CHANGES */
  
  if (multicontext->slave)
    {
      if (!finalizing)
	gtk_im_context_reset (multicontext->slave);
      
      g_signal_handlers_disconnect_by_func (multicontext->slave,
					    gtk_im_multicontext_preedit_start_cb,
					    multicontext);
      g_signal_handlers_disconnect_by_func (multicontext->slave,
					    gtk_im_multicontext_preedit_end_cb,
					    multicontext);
      g_signal_handlers_disconnect_by_func (multicontext->slave,
					    gtk_im_multicontext_preedit_changed_cb,
					    multicontext);
      g_signal_handlers_disconnect_by_func (multicontext->slave,
					    gtk_im_multicontext_commit_cb,
					    multicontext);

#ifdef MAEMO_CHANGES
      g_signal_handlers_disconnect_by_func (multicontext->slave,
					    gtk_im_multicontext_retrieve_surrounding_cb,
					    multicontext);
      g_signal_handlers_disconnect_by_func (multicontext->slave,
					    gtk_im_multicontext_delete_surrounding_cb,
					    multicontext);
      g_signal_handlers_disconnect_by_func (multicontext->slave,
					    gtk_im_multicontext_has_selection_cb,
					    multicontext);
      g_signal_handlers_disconnect_by_func (multicontext->slave,
					    gtk_im_multicontext_clipboard_operation_cb,
					    multicontext);
      g_signal_handlers_disconnect_by_func (multicontext->slave,
					    gtk_im_multicontext_slave_input_mode_changed_cb,
					    multicontext);
#endif /* MAEMO_CHANGES */

      g_object_unref (multicontext->slave);
      multicontext->slave = NULL;

#ifdef MAEMO_CHANGES
      g_free (multicontext->context_id);
      multicontext->context_id = NULL;
#endif /* MAEMO_CHANGES */

      if (!finalizing)
	need_preedit_changed = TRUE;
    }
  
  multicontext->slave = slave;

  if (multicontext->slave)
    {
      g_object_ref (multicontext->slave);

      g_signal_connect (multicontext->slave, "preedit-start",
			G_CALLBACK (gtk_im_multicontext_preedit_start_cb),
			multicontext);
      g_signal_connect (multicontext->slave, "preedit-end",
			G_CALLBACK (gtk_im_multicontext_preedit_end_cb),
			multicontext);
      g_signal_connect (multicontext->slave, "preedit-changed",
			G_CALLBACK (gtk_im_multicontext_preedit_changed_cb),
			multicontext);
      g_signal_connect (multicontext->slave, "commit",
			G_CALLBACK (gtk_im_multicontext_commit_cb),
			multicontext);
      g_signal_connect (multicontext->slave, "retrieve-surrounding",
			G_CALLBACK (gtk_im_multicontext_retrieve_surrounding_cb),
			multicontext);
      g_signal_connect (multicontext->slave, "delete-surrounding",
			G_CALLBACK (gtk_im_multicontext_delete_surrounding_cb),
			multicontext);

#ifdef MAEMO_CHANGES
      g_signal_connect (multicontext->slave, "has-selection",
			G_CALLBACK (gtk_im_multicontext_has_selection_cb),
			multicontext);
      g_signal_connect (multicontext->slave, "clipboard-operation",
			G_CALLBACK (gtk_im_multicontext_clipboard_operation_cb),
			multicontext);
      g_signal_connect (multicontext->slave, "notify::hildon-input-mode",
                        G_CALLBACK (gtk_im_multicontext_slave_input_mode_changed_cb),
                        multicontext);

      g_object_get(multicontext, "hildon-input-mode", &input_mode, NULL);
      g_object_set(multicontext->slave, "hildon-input-mode", input_mode, NULL);
#endif /* MAEMO_CHANGES */
      
      if (!priv->use_preedit)	/* Default is TRUE */
	gtk_im_context_set_use_preedit (slave, FALSE);
      if (priv->client_window)
	gtk_im_context_set_client_window (slave, priv->client_window);
      if (priv->have_cursor_location)
	gtk_im_context_set_cursor_location (slave, &priv->cursor_location);
      if (priv->focus_in)
	gtk_im_context_focus_in (slave);
    }

  if (need_preedit_changed)
    g_signal_emit_by_name (multicontext, "preedit-changed");
}

static GtkIMContext *
gtk_im_multicontext_get_slave (GtkIMMulticontext *multicontext)
{
  if (!multicontext->slave)
    {
      GtkIMContext *slave;
#ifdef MAEMO_CHANGES
      gchar *global_context_id = get_global_context_id ();
#else /* !MAEMO_CHANGES */

      if (!global_context_id) 
        {
          if (user_context_id)
            global_context_id = user_context_id;
          else
            global_context_id = _gtk_im_module_get_default_context_id (multicontext->priv->client_window);
        }
#endif /* MAEMO_CHANGES */

      slave = _gtk_im_module_create (global_context_id);
      gtk_im_multicontext_set_slave (multicontext, slave, FALSE);
      g_object_unref (slave);

      g_free (multicontext->context_id);
      multicontext->context_id = g_strdup (global_context_id);

#ifdef MAEMO_CHANGES
      g_free (global_context_id);
#endif /* MAEMO_CHANGES */
    }

  return multicontext->slave;
}

static void
im_module_setting_changed (GtkSettings *settings, 
                           gpointer     data)
{
#ifndef MAEMO_CHANGES
  global_context_id = NULL;
#endif
}


static void
gtk_im_multicontext_set_client_window (GtkIMContext *context,
				       GdkWindow    *window)
{
  GtkIMMulticontext *multicontext = GTK_IM_MULTICONTEXT (context);
  GtkIMContext *slave; 
  GdkScreen *screen; 
  GtkSettings *settings;
  gboolean connected;

  multicontext->priv->client_window = window;

  slave = gtk_im_multicontext_get_slave (multicontext);

  if (slave)
    gtk_im_context_set_client_window (slave, window);

  if (window == NULL) 
    return;
   
  screen = gdk_drawable_get_screen (GDK_DRAWABLE (window));
  if (screen)
    settings = gtk_settings_get_for_screen (screen);
  else
    settings = gtk_settings_get_default ();

  connected = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (settings),
                                                  "gtk-im-module-connected"));
  if (!connected) 
    {
      g_signal_connect (settings, "notify::gtk-im-module",
                        G_CALLBACK (im_module_setting_changed), NULL);
      g_object_set_data (G_OBJECT (settings), "gtk-im-module-connected",
                         GINT_TO_POINTER (TRUE));

#ifndef MAEMO_CHANGES
      global_context_id = NULL;  
#endif
    }
}

static void
gtk_im_multicontext_get_preedit_string (GtkIMContext   *context,
					gchar         **str,
					PangoAttrList **attrs,
					gint           *cursor_pos)
{
  GtkIMMulticontext *multicontext = GTK_IM_MULTICONTEXT (context);
  GtkIMContext *slave = gtk_im_multicontext_get_slave (multicontext);

  if (slave)
    gtk_im_context_get_preedit_string (slave, str, attrs, cursor_pos);
  else
    {
      if (str)
	*str = g_strdup ("");
      if (attrs)
	*attrs = pango_attr_list_new ();
    }
}

static gboolean
gtk_im_multicontext_filter_keypress (GtkIMContext *context,
				     GdkEventKey  *event)
{
  GtkIMMulticontext *multicontext = GTK_IM_MULTICONTEXT (context);
  GtkIMContext *slave = gtk_im_multicontext_get_slave (multicontext);

  if (slave)
    return gtk_im_context_filter_keypress (slave, event);
  else
    return FALSE;
}

#ifdef MAEMO_CHANGES
static gboolean
hildon_gtk_im_multicontext_filter_event (GtkIMContext *context,
					 GdkEvent *event)
{
  GtkIMMulticontext *multicontext = GTK_IM_MULTICONTEXT (context);
  GtkIMContext *slave = gtk_im_multicontext_get_slave (multicontext);

  if (slave)
    return hildon_gtk_im_context_filter_event (slave, event);
  else
    return FALSE;
}
#endif /* MAEMO_CHANGES */

static void
gtk_im_multicontext_focus_in (GtkIMContext   *context)
{
  GtkIMMulticontext *multicontext = GTK_IM_MULTICONTEXT (context);
  GtkIMContext *slave;
#ifdef MAEMO_CHANGES
  gchar *global_context_id = get_global_context_id ();
#endif /* MAEMO_CHANGES */

  /* If the global context type is different from the context we were
   * using before, get rid of the old slave and create a new one
   * for the new global context type.
   */
  if (multicontext->context_id == NULL || 
      global_context_id == NULL ||
      strcmp (global_context_id, multicontext->context_id) != 0)
    gtk_im_multicontext_set_slave (multicontext, NULL, FALSE);

  slave = gtk_im_multicontext_get_slave (multicontext);
  
  multicontext->priv->focus_in = TRUE;
  
  if (slave)
    gtk_im_context_focus_in (slave);

#ifdef MAEMO_CHANGES
  g_free (global_context_id);
#endif /* MAEMO_CHANGES */
}

static void
gtk_im_multicontext_focus_out (GtkIMContext   *context)
{
  GtkIMMulticontext *multicontext = GTK_IM_MULTICONTEXT (context);
  GtkIMContext *slave = gtk_im_multicontext_get_slave (multicontext);

  multicontext->priv->focus_in = FALSE;
  
  if (slave)
    gtk_im_context_focus_out (slave);
}

static void
gtk_im_multicontext_reset (GtkIMContext   *context)
{
  GtkIMMulticontext *multicontext = GTK_IM_MULTICONTEXT (context);
  GtkIMContext *slave = gtk_im_multicontext_get_slave (multicontext);

  if (slave)
    gtk_im_context_reset (slave);
}

static void
gtk_im_multicontext_set_cursor_location (GtkIMContext   *context,
					 GdkRectangle   *area)
{
  GtkIMMulticontext *multicontext = GTK_IM_MULTICONTEXT (context);
  GtkIMContext *slave = gtk_im_multicontext_get_slave (multicontext);

  multicontext->priv->have_cursor_location = TRUE;
  multicontext->priv->cursor_location = *area;

  if (slave)
    gtk_im_context_set_cursor_location (slave, area);
}

static void
gtk_im_multicontext_set_use_preedit (GtkIMContext   *context,
				     gboolean	    use_preedit)
{
  GtkIMMulticontext *multicontext = GTK_IM_MULTICONTEXT (context);
  GtkIMContext *slave = gtk_im_multicontext_get_slave (multicontext);

  use_preedit = use_preedit != FALSE;

  multicontext->priv->use_preedit = use_preedit;

  if (slave)
    gtk_im_context_set_use_preedit (slave, use_preedit);
}

static gboolean
gtk_im_multicontext_get_surrounding (GtkIMContext  *context,
				     gchar        **text,
				     gint          *cursor_index)
{
  GtkIMMulticontext *multicontext = GTK_IM_MULTICONTEXT (context);
  GtkIMContext *slave = gtk_im_multicontext_get_slave (multicontext);

  if (slave)
    return gtk_im_context_get_surrounding (slave, text, cursor_index);
  else
    {
      if (text)
	*text = NULL;
      if (cursor_index)
	*cursor_index = 0;

      return FALSE;
    }
}

static void
gtk_im_multicontext_set_surrounding (GtkIMContext *context,
				     const char   *text,
				     gint          len,
				     gint          cursor_index)
{
  GtkIMMulticontext *multicontext = GTK_IM_MULTICONTEXT (context);
  GtkIMContext *slave = gtk_im_multicontext_get_slave (multicontext);

  if (slave)
    gtk_im_context_set_surrounding (slave, text, len, cursor_index);
}

#ifdef MAEMO_CHANGES
static void
gtk_im_multicontext_notify (GObject      *object,
                            GParamSpec   *pspec)
{
  GtkIMMulticontext *multicontext = GTK_IM_MULTICONTEXT (object);
  HildonGtkInputMode input_mode_slave, input_mode_multi;

  if (multicontext->slave != NULL &&
      strcmp (pspec->name, "hildon-input-mode") == 0)
    {
      g_object_get (multicontext->slave,
		    "hildon-input-mode", &input_mode_slave,
		    NULL);
      g_object_get (multicontext,
		    "hildon-input-mode", &input_mode_multi,
		    NULL);

      /* don't change without comparing, or we'll get to infinite loop */
      if (input_mode_slave != input_mode_multi)
        g_object_set (multicontext->slave,
		      "hildon-input-mode", input_mode_multi,
		      NULL);
    }
}
#endif /* MAEMO_CHANGES */

static void
gtk_im_multicontext_preedit_start_cb   (GtkIMContext      *slave,
					GtkIMMulticontext *multicontext)
{
  g_signal_emit_by_name (multicontext, "preedit-start");
}

static void
gtk_im_multicontext_preedit_end_cb (GtkIMContext      *slave,
				    GtkIMMulticontext *multicontext)
{
  g_signal_emit_by_name (multicontext, "preedit-end");
}

static void
gtk_im_multicontext_preedit_changed_cb (GtkIMContext      *slave,
					GtkIMMulticontext *multicontext)
{
  g_signal_emit_by_name (multicontext, "preedit-changed");
}

static void
gtk_im_multicontext_commit_cb (GtkIMContext      *slave,
			       const gchar       *str,
			       GtkIMMulticontext *multicontext)
{
  g_signal_emit_by_name (multicontext, "commit", str);;
}

static gboolean
gtk_im_multicontext_retrieve_surrounding_cb (GtkIMContext      *slave,
					     GtkIMMulticontext *multicontext)
{
  gboolean result;
  
  g_signal_emit_by_name (multicontext, "retrieve-surrounding", &result);

  return result;
}

static gboolean
gtk_im_multicontext_delete_surrounding_cb (GtkIMContext      *slave,
					   gint               offset,
					   gint               n_chars,
					   GtkIMMulticontext *multicontext)
{
  gboolean result;
  
  g_signal_emit_by_name (multicontext, "delete-surrounding",
			 offset, n_chars, &result);

  return result;
}

#ifdef MAEMO_CHANGES
static gboolean
gtk_im_multicontext_has_selection_cb (GtkIMContext      *slave,
                                      GtkIMMulticontext *multicontext)
{
  gboolean result;

  g_signal_emit_by_name (multicontext, "has_selection",
                         &result);

  return result;
}

static void
gtk_im_multicontext_clipboard_operation_cb (GtkIMContext                   *slave,
                                            GtkIMContextClipboardOperation  op,
                                            GtkIMMulticontext              *multicontext)
{
  g_signal_emit_by_name (multicontext, "clipboard_operation", op);
}

static void
gtk_im_multicontext_slave_input_mode_changed_cb (GtkIMContext      *slave,
                                                 GParamSpec        *pspec,
                                                 GtkIMMulticontext *multicontext)
{
  HildonGtkInputMode input_mode_slave, input_mode_multi;

  g_object_get (slave, "hildon-input-mode", &input_mode_slave, NULL);
  g_object_get (multicontext, "hildon-input-mode", &input_mode_multi, NULL);

  /* don't change without comparing, or we'll get to infinite loop */
  if (input_mode_slave != input_mode_multi)
    g_object_set (multicontext, "hildon-input-mode", input_mode_slave, NULL);
}
#endif /* MAEMO_CHANGES */

static void
activate_cb (GtkWidget         *menuitem,
	     GtkIMMulticontext *context)
{
  if (GTK_CHECK_MENU_ITEM (menuitem)->active)
    {
#ifndef MAEMO_CHANGES
      const gchar *id = g_object_get_data (G_OBJECT (menuitem), "gtk-context-id");
#endif /* MAEMO_CHANGES */

      gtk_im_context_reset (GTK_IM_CONTEXT (context));

#ifndef MAEMO_CHANGES
      user_context_id = id;
      global_context_id = NULL;
#endif /* MAEMO_CHANGES */
      gtk_im_multicontext_set_slave (context, NULL, FALSE);
    }
}

static int
pathnamecmp (const char *a,
	     const char *b)
{
#ifndef G_OS_WIN32
  return strcmp (a, b);
#else
  /* Ignore case insensitivity, probably not that relevant here. Just
   * make sure slash and backslash compare equal.
   */
  while (*a && *b)
    if ((G_IS_DIR_SEPARATOR (*a) && G_IS_DIR_SEPARATOR (*b)) ||
	*a == *b)
      a++, b++;
    else
      return (*a - *b);
  return (*a - *b);
#endif
}

/**
 * gtk_im_multicontext_append_menuitems:
 * @context: a #GtkIMMultiContext
 * @menushell: a #GtkMenuShell
 * 
 * Add menuitems for various available input methods to a menu;
 * the menuitems, when selected, will switch the input method
 * for the context and the global default input method.
 **/
void
gtk_im_multicontext_append_menuitems (GtkIMMulticontext *context,
				      GtkMenuShell      *menushell)
{
  const GtkIMContextInfo **contexts;
  guint n_contexts, i;
  GSList *group = NULL;
  GtkWidget *menuitem;
#ifdef MAEMO_CHANGES
  gchar *global_context_id = get_global_context_id();
#endif /* MAEMO_CHANGES */

  menuitem = gtk_radio_menu_item_new_with_label (group, Q_("input method menu|System"));
  if (!user_context_id)
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem), TRUE);
  group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menuitem));
  g_object_set_data (G_OBJECT (menuitem), I_("gtk-context-id"), NULL);
  g_signal_connect (menuitem, "activate", G_CALLBACK (activate_cb), context);

  gtk_widget_show (menuitem);
  gtk_menu_shell_append (menushell, menuitem);

  _gtk_im_module_list (&contexts, &n_contexts);

  for (i = 0; i < n_contexts; i++)
    {
      const gchar *translated_name;
#ifdef ENABLE_NLS
      if (contexts[i]->domain && contexts[i]->domain[0])
	{
	  if (strcmp (contexts[i]->domain, GETTEXT_PACKAGE) == 0)
	    {
	      /* Same translation domain as GTK+ */
	      if (!(contexts[i]->domain_dirname && contexts[i]->domain_dirname[0]) ||
		  pathnamecmp (contexts[i]->domain_dirname, GTK_LOCALEDIR) == 0)
		{
		  /* Empty or NULL, domain directory, or same as
		   * GTK+. Input method may have a name in the GTK+
		   * message catalog.
		   */
		  translated_name = _(contexts[i]->context_name);
		}
	      else
		{
		  /* Separate domain directory but the same
		   * translation domain as GTK+. We can't call
		   * bindtextdomain() as that would make GTK+ forget
		   * its own messages.
		   */
		  g_warning ("Input method %s should not use GTK's translation domain %s",
			     contexts[i]->context_id, GETTEXT_PACKAGE);
		  /* Try translating the name in GTK+'s domain */
		  translated_name = _(contexts[i]->context_name);
		}
	    }
	  else if (contexts[i]->domain_dirname && contexts[i]->domain_dirname[0])
	    /* Input method has own translation domain and message catalog */
	    {
	      bindtextdomain (contexts[i]->domain,
			      contexts[i]->domain_dirname);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	      bind_textdomain_codeset (contexts[i]->domain, "UTF-8");
#endif
	      translated_name = g_dgettext (contexts[i]->domain, contexts[i]->context_name);
	    }
	  else
	    {
	      /* Different translation domain, but no domain directory */
	      translated_name = contexts[i]->context_name;
	    }
	}
      else
	/* Empty or NULL domain. We assume that input method does not
	 * want a translated name in this case.
	 */
	translated_name = contexts[i]->context_name;
#else
      translated_name = contexts[i]->context_name;
#endif
      menuitem = gtk_radio_menu_item_new_with_label (group,
						     translated_name);
      
      if ((user_context_id &&
           strcmp (contexts[i]->context_id, user_context_id) == 0))
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem), TRUE);
      
      group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menuitem));
      
      g_object_set_data (G_OBJECT (menuitem), I_("gtk-context-id"),
			 (char *)contexts[i]->context_id);
      g_signal_connect (menuitem, "activate",
			G_CALLBACK (activate_cb), context);

      gtk_widget_show (menuitem);
      gtk_menu_shell_append (menushell, menuitem);
    }

#ifdef MAEMO_CHANGES
  g_free (global_context_id);
#endif /* MAEMO_CHANGES */
  g_free (contexts);
}

#ifdef MAEMO_CHANGES
static void
gtk_im_multicontext_show (GtkIMContext *context)
{
  GtkIMMulticontext *multicontext = GTK_IM_MULTICONTEXT (context);
  GtkIMContext *slave;
  gchar *global_context_id = get_global_context_id ();

  /* If the global context type is different from the context we were
   * using before, get rid of the old slave and create a new one
   * for the new global context type.
   */
  if (!multicontext->context_id ||
      strcmp (global_context_id, multicontext->context_id) != 0)
    gtk_im_multicontext_set_slave (multicontext, NULL, FALSE);

  slave = gtk_im_multicontext_get_slave (multicontext);

  multicontext->priv->focus_in = TRUE;

  if (slave)
    gtk_im_context_show (slave);

  g_free (global_context_id);
}

static void
gtk_im_multicontext_hide (GtkIMContext *context)
{
  GtkIMMulticontext *multicontext = GTK_IM_MULTICONTEXT (context);
  GtkIMContext *slave = gtk_im_multicontext_get_slave (multicontext);
  
  multicontext->priv->focus_in = FALSE;

  if (slave)
    gtk_im_context_hide (slave);
}
#endif /* MAEMO_CHANGES */

#define __GTK_IM_MULTICONTEXT_C__
#include "gtkaliasdef.c"
