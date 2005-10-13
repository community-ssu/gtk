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

#include <config.h>

#include <string.h>
#include <locale.h>

#include "gtkimmulticontext.h"
#include "gtkimmodule.h"
#include "gtkmain.h"
#include "gtkradiomenuitem.h"
#include "gtkintl.h"
#include "gtkprivate.h"
#include "gtkalias.h"

struct _GtkIMMulticontextPrivate
{
  GdkWindow *client_window;
  GdkRectangle cursor_location;

  guint use_preedit : 1;
  guint have_cursor_location : 1;
  guint focus_in : 1;
};

static void     gtk_im_multicontext_class_init         (GtkIMMulticontextClass  *class);
static void     gtk_im_multicontext_init               (GtkIMMulticontext       *im_multicontext);
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
static void     gtk_im_multicontext_show               (GtkIMContext            *context);
static void     gtk_im_multicontext_hide               (GtkIMContext            *context);
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
static GtkIMContextClass *parent_class;

static gchar*
get_global_context_id(void)
{
  GdkAtom atom, type, actual_type;

  gint actual_format, actual_length;
  guchar *context_id;

  atom = gdk_atom_intern("gtk-global-immodule", FALSE);
  type = gdk_atom_intern("STRING", FALSE);
  if (!gdk_property_get(gdk_screen_get_root_window (gdk_screen_get_default ()),
			atom,
			type,
			0,
			G_MAXLONG,
			FALSE,
			&actual_type,
			&actual_format,
			&actual_length,
			&context_id)) {
    /* Fall back to default locale */
    gchar *locale = _gtk_get_lc_ctype ();
    context_id = _gtk_im_module_get_default_context_id (locale);
    g_free (locale);
  }
  
  return context_id;
}

GType
gtk_im_multicontext_get_type (void)
{
  static GType im_multicontext_type = 0;
 
  if (!im_multicontext_type)
    {
      static const GTypeInfo im_multicontext_info =
      {
        sizeof (GtkIMMulticontextClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gtk_im_multicontext_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GtkIMMulticontext),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gtk_im_multicontext_init,
      };
      
      im_multicontext_type =
	g_type_register_static (GTK_TYPE_IM_CONTEXT, "GtkIMMulticontext",
				&im_multicontext_info, 0);
    }

  return im_multicontext_type;
}

static GtkIMContext *
gtk_im_multicontext_get_slave (GtkIMMulticontext *multicontext);

enum {
  PROP_INPUT_MODE = 1,
  PROP_AUTOCAP,
  PROP_VISIBILITY,
  PROP_USE_SHOW_HIDE
};

static void gtk_im_multicontext_set_property(GObject * object,
                                                  guint property_id,
                                                  const GValue * value,
                                                  GParamSpec * pspec)
{
  GtkIMContext *slave = gtk_im_multicontext_get_slave (GTK_IM_MULTICONTEXT(object));

  GParamSpec *param_spec = g_object_class_find_property 
    (G_OBJECT_GET_CLASS(slave),
     pspec->name);

  if(param_spec != NULL)   
    g_object_set_property(G_OBJECT(slave), pspec->name, value);
}

static void gtk_im_multicontext_get_property(GObject * object,
                                                  guint property_id,
                                                  GValue * value,
                                                  GParamSpec * pspec)
{
  GtkIMContext *slave = gtk_im_multicontext_get_slave (GTK_IM_MULTICONTEXT(object));
  GParamSpec *param_spec = g_object_class_find_property 
    (G_OBJECT_GET_CLASS(slave),
     pspec->name);
  
  if(param_spec != NULL)
    g_object_get_property(G_OBJECT(slave), pspec->name, value);
  else
    {
      switch (property_id)
	{
	case PROP_INPUT_MODE:
	  /* return 0 */
	  g_value_set_int(value, 0);
	  break;
	case PROP_AUTOCAP:
	  /* return FALSE */
	  g_value_set_boolean(value, FALSE);
	  break;
	case PROP_VISIBILITY:
	  /* return TRUE */
	  g_value_set_boolean(value, TRUE);	  
	  break;
	case PROP_USE_SHOW_HIDE:
	default:
	  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	  break;	  
	}
    }
}

static void
gtk_im_multicontext_class_init (GtkIMMulticontextClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkIMContextClass *im_context_class = GTK_IM_CONTEXT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);

  im_context_class->set_client_window = gtk_im_multicontext_set_client_window;
  im_context_class->get_preedit_string = gtk_im_multicontext_get_preedit_string;
  im_context_class->filter_keypress = gtk_im_multicontext_filter_keypress;
  im_context_class->focus_in = gtk_im_multicontext_focus_in;
  im_context_class->focus_out = gtk_im_multicontext_focus_out;
  im_context_class->reset = gtk_im_multicontext_reset;
  im_context_class->show = gtk_im_multicontext_show;
  im_context_class->hide = gtk_im_multicontext_hide;
  im_context_class->set_cursor_location = gtk_im_multicontext_set_cursor_location;
  im_context_class->set_use_preedit = gtk_im_multicontext_set_use_preedit;
  im_context_class->set_surrounding = gtk_im_multicontext_set_surrounding;
  im_context_class->get_surrounding = gtk_im_multicontext_get_surrounding;

  gobject_class->finalize = gtk_im_multicontext_finalize;

  gobject_class->set_property = gtk_im_multicontext_set_property;
  gobject_class->get_property = gtk_im_multicontext_get_property;

  g_object_class_install_property(gobject_class, PROP_INPUT_MODE,
        g_param_spec_int("input-mode", "Input mode",
          "Specifies the set of allowed characters",
          0, 9, 0,  /* We don't move symbolic definitions here. See hildon-input-mode.h */
          G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class, PROP_AUTOCAP,
        g_param_spec_boolean("autocap", "Autocap",
          "Whether the client wants the first character in a sentense to be automatic upper case",
          FALSE, G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class, PROP_VISIBILITY,
        g_param_spec_boolean("visibility", "Visibility",
          "FALSE displays the \"invisible char\"instead of the actual text (password mode)",
          TRUE, G_PARAM_READABLE | G_PARAM_WRITABLE));

  g_object_class_install_property(gobject_class, PROP_VISIBILITY,
        g_param_spec_boolean("use-show-hide", "Use show/hide functions",
          "Use show/hide functions to show/hide IM instead of focus_in/focus_out",
          FALSE, G_PARAM_READABLE | G_PARAM_WRITABLE));
}

static void
gtk_im_multicontext_init (GtkIMMulticontext *multicontext)
{
  multicontext->slave = NULL;
  
  multicontext->priv = g_new0 (GtkIMMulticontextPrivate, 1);
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
  g_free (multicontext->priv);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gtk_im_multicontext_set_slave (GtkIMMulticontext *multicontext,
			       GtkIMContext      *slave,
			       gboolean           finalizing)
{
  GtkIMMulticontextPrivate *priv = multicontext->priv;
  gboolean need_preedit_changed = FALSE;
  
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

      g_object_unref (multicontext->slave);
      multicontext->slave = NULL;

      g_free (multicontext->context_id);
      multicontext->context_id = NULL;

      if (!finalizing)
	need_preedit_changed = TRUE;
    }
  
  multicontext->slave = slave;

  if (multicontext->slave)
    {
      g_object_ref (multicontext->slave);

      g_signal_connect (multicontext->slave, "preedit_start",
			G_CALLBACK (gtk_im_multicontext_preedit_start_cb),
			multicontext);
      g_signal_connect (multicontext->slave, "preedit_end",
			G_CALLBACK (gtk_im_multicontext_preedit_end_cb),
			multicontext);
      g_signal_connect (multicontext->slave, "preedit_changed",
			G_CALLBACK (gtk_im_multicontext_preedit_changed_cb),
			multicontext);
      g_signal_connect (multicontext->slave, "commit",
			G_CALLBACK (gtk_im_multicontext_commit_cb),
			multicontext);
      g_signal_connect (multicontext->slave, "retrieve_surrounding",
			G_CALLBACK (gtk_im_multicontext_retrieve_surrounding_cb),
			multicontext);
      g_signal_connect (multicontext->slave, "delete_surrounding",
			G_CALLBACK (gtk_im_multicontext_delete_surrounding_cb),
			multicontext);
      
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
    g_signal_emit_by_name (multicontext, "preedit_changed");
}

static GtkIMContext *
gtk_im_multicontext_get_slave (GtkIMMulticontext *multicontext)
{
  if (!multicontext->slave)
    {
      GtkIMContext *slave;
      gchar *global_context_id = get_global_context_id();

      slave = _gtk_im_module_create (global_context_id);
      gtk_im_multicontext_set_slave (multicontext, slave, FALSE);
      g_object_unref (slave);

      g_free (multicontext->context_id);
      multicontext->context_id = global_context_id;
    }

  return multicontext->slave;
}

static void
gtk_im_multicontext_set_client_window (GtkIMContext *context,
				       GdkWindow    *window)
{
  GtkIMMulticontext *multicontext = GTK_IM_MULTICONTEXT (context);
  
  GtkIMContext *slave = gtk_im_multicontext_get_slave (multicontext);

  multicontext->priv->client_window = window;
  
  if (slave)
    gtk_im_context_set_client_window (slave, window);
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

static void
gtk_im_multicontext_focus_in (GtkIMContext   *context)
{
  GtkIMMulticontext *multicontext = GTK_IM_MULTICONTEXT (context);
  GtkIMContext *slave;
  gchar *global_context_id = get_global_context_id();

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
    gtk_im_context_focus_in (slave);

  g_free (global_context_id);
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

static void
gtk_im_multicontext_preedit_start_cb   (GtkIMContext      *slave,
					GtkIMMulticontext *multicontext)
{
  g_signal_emit_by_name (multicontext, "preedit_start");
}

static void
gtk_im_multicontext_preedit_end_cb (GtkIMContext      *slave,
				    GtkIMMulticontext *multicontext)
{
  g_signal_emit_by_name (multicontext, "preedit_end");
}

static void
gtk_im_multicontext_preedit_changed_cb (GtkIMContext      *slave,
					GtkIMMulticontext *multicontext)
{
  g_signal_emit_by_name (multicontext, "preedit_changed");
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
  
  g_signal_emit_by_name (multicontext, "retrieve_surrounding", &result);

  return result;
}

static gboolean
gtk_im_multicontext_delete_surrounding_cb (GtkIMContext      *slave,
					   gint               offset,
					   gint               n_chars,
					   GtkIMMulticontext *multicontext)
{
  gboolean result;
  
  g_signal_emit_by_name (multicontext, "delete_surrounding",
			 offset, n_chars, &result);

  return result;
}

static void
activate_cb (GtkWidget         *menuitem,
	     GtkIMMulticontext *context)
{
  if (GTK_CHECK_MENU_ITEM (menuitem)->active)
    {
      gtk_im_context_reset (GTK_IM_CONTEXT (context));
      
      gtk_im_multicontext_set_slave (context, NULL, FALSE);
    }
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
  gchar *global_context_id = get_global_context_id();
  
  _gtk_im_module_list (&contexts, &n_contexts);

  for (i=0; i < n_contexts; i++)
    {
      GtkWidget *menuitem;
      const gchar *translated_name;
#ifdef ENABLE_NLS
      if (contexts[i]->domain && contexts[i]->domain_dirname &&
	  contexts[i]->domain[0] && contexts[i]->domain_dirname[0])
	{
	  if (strcmp (contexts[i]->domain, GETTEXT_PACKAGE) == 0 &&
	      strcmp (contexts[i]->domain_dirname, GTK_LOCALEDIR) == 0)
	    /* Input method may have a name in the GTK+ message catalog */
	    translated_name = _(contexts[i]->context_name);
	  else
	    /* Input method has own message catalog */
	    {
	      bindtextdomain (contexts[i]->domain,
			      contexts[i]->domain_dirname);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	      bind_textdomain_codeset (contexts[i]->domain, "UTF-8");
#endif
	      translated_name = dgettext (contexts[i]->domain, contexts[i]->context_name);
	    }
	}
      else
	/* Either domain or domain_dirname is NULL or "". We assume that
	 * input method does not want a translated name in this case
	 */
	translated_name = contexts[i]->context_name;
#else
      translated_name = contexts[i]->context_name;
#endif
      menuitem = gtk_radio_menu_item_new_with_label (group,
						     translated_name);
      
      if ((global_context_id == NULL && group == NULL) ||
          (global_context_id &&
           strcmp (contexts[i]->context_id, global_context_id) == 0))
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem),
                                        TRUE);
      
      group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menuitem));
      
      g_object_set_data (G_OBJECT (menuitem), "gtk-context-id",
			 (char *)contexts[i]->context_id);
      g_signal_connect (menuitem, "activate",
			G_CALLBACK (activate_cb), context);

      gtk_widget_show (menuitem);
      gtk_menu_shell_append (menushell, menuitem);
    }

  g_free (global_context_id);
  g_free (contexts);
}

static void
gtk_im_multicontext_show (GtkIMContext   *context)
{
  GtkIMMulticontext *multicontext = GTK_IM_MULTICONTEXT (context);
  GtkIMContext *slave;
  gchar *global_context_id = get_global_context_id();

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
gtk_im_multicontext_hide (GtkIMContext   *context)
{
  GtkIMMulticontext *multicontext = GTK_IM_MULTICONTEXT (context);
  GtkIMContext *slave = gtk_im_multicontext_get_slave (multicontext);

  multicontext->priv->focus_in = FALSE;
  
  if (slave)
    gtk_im_context_hide (slave);
}

#define __GTK_IM_MULTICONTEXT_C__
#include "gtkaliasdef.c"
