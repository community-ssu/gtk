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
#include "gtkimcontext.h"
#include "gtkmain.h"		/* For _gtk_boolean_handled_accumulator */
#include "gtkmarshalers.h"
#include "gtkalias.h"

typedef struct _GtkIMContextPrivate GtkIMContextPrivate;

#define GTK_IM_CONTEXT_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
   GTK_TYPE_IM_CONTEXT, GtkIMContextPrivate))

enum {
  PROP_HILDON_INPUT_MODE = 1
};

enum {
  PREEDIT_START,
  PREEDIT_END,
  PREEDIT_CHANGED,
  COMMIT,
  RETRIEVE_SURROUNDING,
  DELETE_SURROUNDING,
  HAS_SELECTION,
  CLIPBOARD_OPERATION,
  LAST_SIGNAL
};

struct _GtkIMContextPrivate {
  HildonGtkInputMode mode;
};

static guint im_context_signals[LAST_SIGNAL] = { 0 };

static void gtk_im_context_class_init (GtkIMContextClass *class);
static void gtk_im_context_init (GtkIMContext *im_context);

static void     gtk_im_context_set_property            (GObject        *object,
                                                        guint           property_id,
                                                        const GValue   *value,
                                                        GParamSpec     *pspec);
static void     gtk_im_context_get_property            (GObject        *object,
                                                        guint           property_id,
                                                        GValue         *value,
                                                        GParamSpec     *pspec);
static void     gtk_im_context_real_get_preedit_string (GtkIMContext   *context,
							gchar         **str,
							PangoAttrList **attrs,
							gint           *cursor_pos);
static gboolean gtk_im_context_real_filter_keypress    (GtkIMContext   *context,
							GdkEventKey    *event);
static gboolean gtk_im_context_real_filter_event       (GtkIMContext   *context,
						        GdkEvent    *event);
static gboolean gtk_im_context_real_get_surrounding    (GtkIMContext   *context,
							gchar         **text,
							gint           *cursor_index);
static void     gtk_im_context_real_set_surrounding    (GtkIMContext   *context,
							const char     *text,
							gint            len,
							gint            cursor_index);

GType
gtk_im_context_get_type (void)
{
  static GType im_context_type = 0;

  if (!im_context_type)
    {
      static const GTypeInfo im_context_info =
      {
        sizeof (GtkIMContextClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gtk_im_context_class_init,
        NULL,		/* class_finalize */
        NULL,		/* class_data */
        sizeof (GtkIMContext),
        0,		/* n_preallocs */
        (GInstanceInitFunc) gtk_im_context_init,
        NULL,		/* value_table */
      };
      
      im_context_type =
	g_type_register_static (G_TYPE_OBJECT, "GtkIMContext",
				&im_context_info, G_TYPE_FLAG_ABSTRACT);
    }

  return im_context_type;
}

static void
gtk_im_context_class_init (GtkIMContextClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  klass->get_preedit_string = gtk_im_context_real_get_preedit_string;
  klass->filter_keypress = gtk_im_context_real_filter_keypress;
  klass->filter_event = gtk_im_context_real_filter_event;
  klass->get_surrounding = gtk_im_context_real_get_surrounding;
  klass->set_surrounding = gtk_im_context_real_set_surrounding;

  gobject_class->set_property = gtk_im_context_set_property;
  gobject_class->get_property = gtk_im_context_get_property;

  g_type_class_add_private (klass, sizeof(GtkIMContextPrivate));

  im_context_signals[PREEDIT_START] =
    g_signal_new ("preedit_start",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GtkIMContextClass, preedit_start),
		  NULL, NULL,
		  _gtk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  
  im_context_signals[PREEDIT_END] =
    g_signal_new ("preedit_end",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GtkIMContextClass, preedit_end),
		  NULL, NULL,
		  _gtk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  
  im_context_signals[PREEDIT_CHANGED] =
    g_signal_new ("preedit_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GtkIMContextClass, preedit_changed),
		  NULL, NULL,
		  _gtk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  
  im_context_signals[COMMIT] =
    g_signal_new ("commit",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GtkIMContextClass, commit),
		  NULL, NULL,
		  _gtk_marshal_VOID__STRING,
		  G_TYPE_NONE, 1,
		  G_TYPE_STRING);

  im_context_signals[RETRIEVE_SURROUNDING] =
    g_signal_new ("retrieve_surrounding",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GtkIMContextClass, retrieve_surrounding),
                  _gtk_boolean_handled_accumulator, NULL,
                  _gtk_marshal_BOOLEAN__VOID,
                  G_TYPE_BOOLEAN, 0);
  im_context_signals[DELETE_SURROUNDING] =
    g_signal_new ("delete_surrounding",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GtkIMContextClass, delete_surrounding),
                  _gtk_boolean_handled_accumulator, NULL,
                  _gtk_marshal_BOOLEAN__INT_INT,
                  G_TYPE_BOOLEAN, 2,
                  G_TYPE_INT,
		  G_TYPE_INT);
  /**
   * GtkIMContext:has-selection:
   *
   * This signal is emitted when input context needs to know if there is
   * any text selected in the widget. Return TRUE if there is.
   *
   * Since: maemo 2.0
   **/
  im_context_signals[HAS_SELECTION] =
    g_signal_new ("has_selection",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GtkIMContextClass, has_selection),
                  NULL, NULL,
                  _gtk_marshal_BOOLEAN__VOID,
                  G_TYPE_BOOLEAN, 0);
  /**
   * GtkIMContext:clipboard-operation:
   *
   * This signal is emitted when input context wants to copy, cut or paste
   * text. The widget needs to implement these operations.
   *
   * Since: maemo 2.0
   **/
  im_context_signals[CLIPBOARD_OPERATION] =
    g_signal_new ("clipboard_operation",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GtkIMContextClass, clipboard_operation),
                  NULL, NULL,
                  _gtk_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1, GTK_TYPE_IM_CONTEXT_CLIPBOARD_OPERATION);

  /**
   * GtkIMContext:hildon-input-mode:
   *
   * Allowed characters and input mode for this IM context.
   * See #HildonGtkInputMode.
   *
   * Since: maemo 2.0
   **/
  g_object_class_install_property(gobject_class, PROP_HILDON_INPUT_MODE,
        g_param_spec_flags("hildon-input-mode", "Hildon input mode",
          "Allowed characters and input mode", GTK_TYPE_GTK_INPUT_MODE,
          HILDON_GTK_INPUT_MODE_FULL | HILDON_GTK_INPUT_MODE_AUTOCAP,
          G_PARAM_READWRITE));
}

static void
gtk_im_context_init (GtkIMContext *im_context)
{
  GtkIMContextPrivate *priv = GTK_IM_CONTEXT_GET_PRIVATE (im_context);

  priv->mode = HILDON_GTK_INPUT_MODE_FULL | HILDON_GTK_INPUT_MODE_AUTOCAP;
}

static void 
gtk_im_context_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GtkIMContextPrivate *priv = GTK_IM_CONTEXT_GET_PRIVATE (object);

  switch (property_id) 
  {
    case PROP_HILDON_INPUT_MODE:
      priv->mode = g_value_get_flags(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
  }
}

static void 
gtk_im_context_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GtkIMContextPrivate *priv = GTK_IM_CONTEXT_GET_PRIVATE (object);

  switch (property_id) 
  {
    case PROP_HILDON_INPUT_MODE:
      g_value_set_flags(value, priv->mode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
  }
}

static void
gtk_im_context_real_get_preedit_string (GtkIMContext       *context,
					gchar             **str,
					PangoAttrList     **attrs,
					gint               *cursor_pos)
{
  if (str)
    *str = g_strdup ("");
  if (attrs)
    *attrs = pango_attr_list_new ();
  if (cursor_pos)
    *cursor_pos = 0;
}

static gboolean
gtk_im_context_real_filter_keypress (GtkIMContext       *context,
				     GdkEventKey        *event)
{
  return FALSE;
}

static gboolean
gtk_im_context_real_filter_event (GtkIMContext       *context,
				  GdkEvent        *event)
{
  return FALSE;
}

typedef struct
{
  gchar *text;
  gint cursor_index;
} SurroundingInfo;

static void
gtk_im_context_real_set_surrounding (GtkIMContext  *context,
				     const gchar   *text,
				     gint           len,
				     gint           cursor_index)
{
  SurroundingInfo *info = g_object_get_data (G_OBJECT (context), "gtk-im-surrounding-info");

  if (info)
    {
      g_free (info->text);
      info->text = g_strndup (text, len);
      info->cursor_index = cursor_index;
    }
}

static gboolean
gtk_im_context_real_get_surrounding (GtkIMContext *context,
				     gchar       **text,
				     gint         *cursor_index)
{
  gboolean result;
  gboolean info_is_local = FALSE;
  SurroundingInfo local_info = { NULL, 0 };
  SurroundingInfo *info;
  
  info = g_object_get_data (G_OBJECT (context), "gtk-im-surrounding-info");
  if (!info)
    {
      info = &local_info;
      g_object_set_data (G_OBJECT (context), "gtk-im-surrounding-info", info);
      info_is_local = TRUE;
    }
  
  g_signal_emit (context,
		 im_context_signals[RETRIEVE_SURROUNDING], 0,
		 &result);

  if (result)
    {
      *text = g_strdup (info->text ? info->text : "");
      *cursor_index = info->cursor_index;
    }
  else
    {
      *text = NULL;
      *cursor_index = 0;
    }

  if (info_is_local)
    {
      g_free (info->text);
      g_object_set_data (G_OBJECT (context), "gtk-im-surrounding-info", NULL);
    }
  
  return result;
}

/**
 * gtk_im_context_set_client_window:
 * @context: a #GtkIMContext
 * @window:  the client window. This may be %NULL to indicate
 *           that the previous client window no longer exists.
 * 
 * Set the client window for the input context; this is the
 * #GdkWindow in which the input appears. This window is
 * used in order to correctly position status windows, and may
 * also be used for purposes internal to the input method.
 **/
void
gtk_im_context_set_client_window (GtkIMContext *context,
				  GdkWindow    *window)
{
  GtkIMContextClass *klass;
  
  g_return_if_fail (GTK_IS_IM_CONTEXT (context));

  klass = GTK_IM_CONTEXT_GET_CLASS (context);
  if (klass->set_client_window)
    klass->set_client_window (context, window);
}

/**
 * gtk_im_context_get_preedit_string:
 * @context:    a #GtkIMContext
 * @str:        location to store the retrieved string. The
 *              string retrieved must be freed with g_free ().
 * @attrs:      location to store the retrieved attribute list.
 *              When you are done with this list, you must
 *              unreference it with pango_attr_list_unref().
 * @cursor_pos: location to store position of cursor (in characters)
 *              within the preedit string.  
 * 
 * Retrieve the current preedit string for the input context,
 * and a list of attributes to apply to the string.
 * This string should be displayed inserted at the insertion
 * point.
 **/
void
gtk_im_context_get_preedit_string (GtkIMContext   *context,
				   gchar         **str,
				   PangoAttrList **attrs,
				   gint           *cursor_pos)
{
  GtkIMContextClass *klass;
  
  g_return_if_fail (GTK_IS_IM_CONTEXT (context));
  
  klass = GTK_IM_CONTEXT_GET_CLASS (context);
  klass->get_preedit_string (context, str, attrs, cursor_pos);
  g_return_if_fail (str == NULL || g_utf8_validate (*str, -1, NULL));
}

/**
 * gtk_im_context_filter_keypress:
 * @context: a #GtkIMContext
 * @event: the key event
 * 
 * Allow an input method to internally handle a key press event.
 * If this function returns %TRUE, then no further processing
 * should be done for this keystroke.
 * 
 * Return value: %TRUE if the input method handled the keystroke.
 *
 **/
gboolean
gtk_im_context_filter_keypress (GtkIMContext *context,
				GdkEventKey  *key)
{
  GtkIMContextClass *klass;
  
  g_return_val_if_fail (GTK_IS_IM_CONTEXT (context), FALSE);
  g_return_val_if_fail (key != NULL, FALSE);

  klass = GTK_IM_CONTEXT_GET_CLASS (context);
  return klass->filter_keypress (context, key);
}

/**
 * hildon_gtk_im_context_filter_event:
 * @context: a #GtkIMContext
 * @event: the event
 * 
 * Allow an input method to internally handle an event.
 * If this function returns %TRUE, then no further processing
 * should be done for this event. The widget should at least
 * pass all events which are directly generated by user action,
 * such as key and button press and release events, to this function.
 * 
 * Return value: %TRUE if the input method handled the event.
 *
 * Since: maemo 2.0
 *
 **/
gboolean
hildon_gtk_im_context_filter_event (GtkIMContext *context,
				    GdkEvent     *event)
{
  GtkIMContextClass *klass;
  
  g_return_val_if_fail (GTK_IS_IM_CONTEXT (context), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  klass = GTK_IM_CONTEXT_GET_CLASS (context);
  return klass->filter_event (context, event);
}

/**
 * gtk_im_context_focus_in:
 * @context: a #GtkIMContext
 *
 * Notify the input method that the widget to which this
 * input context corresponds has gained focus. The input method
 * may, for example, change the displayed feedback to reflect
 * this change.
 **/
void
gtk_im_context_focus_in (GtkIMContext   *context)
{
  GtkIMContextClass *klass;
  
  g_return_if_fail (GTK_IS_IM_CONTEXT (context));
  
  klass = GTK_IM_CONTEXT_GET_CLASS (context);
  if (klass->focus_in)
    klass->focus_in (context);
}

/**
 * gtk_im_context_focus_out:
 * @context: a #GtkIMContext
 *
 * Notify the input method that the widget to which this
 * input context corresponds has lost focus. The input method
 * may, for example, change the displayed feedback or reset the contexts
 * state to reflect this change.
 **/
void
gtk_im_context_focus_out (GtkIMContext   *context)
{
  GtkIMContextClass *klass;
  
  g_return_if_fail (GTK_IS_IM_CONTEXT (context));

  klass = GTK_IM_CONTEXT_GET_CLASS (context);
  if (klass->focus_out)
    klass->focus_out (context);
}

/**
 * gtk_im_context_show:
 * @context: a #GtkIMContext
 *
 * Notify the input method that widget thinks the actual
 * input method show be opened.
 *
 * Since: maemo 1.0
 *
 * Deprecated: Use hildon_gtk_im_context_show() instead.
 **/
void
gtk_im_context_show (GtkIMContext   *context)
{
  hildon_gtk_im_context_show (context);
}

/**
 * gtk_im_context_hide:
 * @context: a #GtkIMContext
 *
 * Notify the input method that widget thinks the actual
 * input method show be closed.
 *
 * Since: maemo 1.0
 *
 * Deprecated: Use hildon_gtk_im_context_hide() instead.
 **/
void
gtk_im_context_hide (GtkIMContext   *context)
{
  hildon_gtk_im_context_hide (context);
}

/**
 * hildon_gtk_im_context_show:
 * @context: a #GtkIMContext
 *
 * Notify the input method that widget thinks the actual
 * input method show be opened.
 *
 * Since: maemo 2.0
 **/
void
hildon_gtk_im_context_show (GtkIMContext   *context)
{
  GtkIMContextClass *klass;
  
  g_return_if_fail (GTK_IS_IM_CONTEXT (context));
  
  klass = GTK_IM_CONTEXT_GET_CLASS (context);
  if (klass->show)
    klass->show (context);
}

/**
 * hildon_gtk_im_context_hide:
 * @context: a #GtkIMContext
 *
 * Notify the input method that widget thinks the actual
 * input method show be closed.
 *
 * Since: maemo 2.0
 **/
void
hildon_gtk_im_context_hide (GtkIMContext   *context)
{
  GtkIMContextClass *klass;
  
  g_return_if_fail (GTK_IS_IM_CONTEXT (context));

  klass = GTK_IM_CONTEXT_GET_CLASS (context);
  if (klass->hide)
    klass->hide (context);
}

/**
 * gtk_im_context_reset:
 * @context: a #GtkIMContext
 *
 * Notify the input method that a change such as a change in cursor
 * position has been made. This will typically cause the input
 * method to clear the preedit state.
 **/
void
gtk_im_context_reset (GtkIMContext   *context)
{
  GtkIMContextClass *klass;
  
  g_return_if_fail (GTK_IS_IM_CONTEXT (context));

  klass = GTK_IM_CONTEXT_GET_CLASS (context);
  if (klass->reset)
    klass->reset (context);
}


/**
 * gtk_im_context_set_cursor_location:
 * @context: a #GtkIMContext
 * @area: new location
 *
 * Notify the input method that a change in cursor 
 * position has been made. The location is relative to the client
 * window.
 **/
void
gtk_im_context_set_cursor_location (GtkIMContext *context,
				    GdkRectangle *area)
{
  GtkIMContextClass *klass;
  
  g_return_if_fail (GTK_IS_IM_CONTEXT (context));

  klass = GTK_IM_CONTEXT_GET_CLASS (context);
  if (klass->set_cursor_location)
    klass->set_cursor_location (context, area);
}

/**
 * gtk_im_context_set_use_preedit:
 * @context: a #GtkIMContext
 * @use_preedit: whether the IM context should use the preedit string.
 * 
 * Sets whether the IM context should use the preedit string
 * to display feedback. If @use_preedit is FALSE (default
 * is TRUE), then the IM context may use some other method to display
 * feedback, such as displaying it in a child of the root window.
 **/
void
gtk_im_context_set_use_preedit (GtkIMContext *context,
				gboolean      use_preedit)
{
  GtkIMContextClass *klass;
  
  g_return_if_fail (GTK_IS_IM_CONTEXT (context));

  klass = GTK_IM_CONTEXT_GET_CLASS (context);
  if (klass->set_use_preedit)
    klass->set_use_preedit (context, use_preedit);
}

/**
 * gtk_im_context_set_surrounding:
 * @context: a #GtkIMContext 
 * @text: text surrounding the insertion point, as UTF-8.
 *        the preedit string should not be included within
 *        @text.
 * @len: the length of @text, or -1 if @text is nul-terminated
 * @cursor_index: the byte index of the insertion cursor within @text.
 * 
 * Sets surrounding context around the insertion point and preedit
 * string. This function is expected to be called in response to the
 * GtkIMContext::retrieve_surrounding signal, and will likely have no
 * effect if called at other times.
 **/
void
gtk_im_context_set_surrounding (GtkIMContext  *context,
				const gchar   *text,
				gint           len,
				gint           cursor_index)
{
  GtkIMContextClass *klass;
  
  g_return_if_fail (GTK_IS_IM_CONTEXT (context));
  g_return_if_fail (text != NULL || len == 0);

  if (text == NULL && len == 0)
    text = "";
  if (len < 0)
    len = strlen (text);

  g_return_if_fail (cursor_index >= 0 && cursor_index <= len);

  klass = GTK_IM_CONTEXT_GET_CLASS (context);
  if (klass->set_surrounding)
    klass->set_surrounding (context, text, len, cursor_index);
}

/**
 * gtk_im_context_get_surrounding:
 * @context: a #GtkIMContext
 * @text: location to store a UTF-8 encoded string of text
 *        holding context around the insertion point.
 *        If the function returns %TRUE, then you must free
 *        the result stored in this location with g_free().
 * @cursor_index: location to store byte index of the insertion cursor
 *        within @text.
 * 
 * Retrieves context around the insertion point. Input methods
 * typically want context in order to constrain input text based on
 * existing text; this is important for languages such as Thai where
 * only some sequences of characters are allowed.
 *
 * This function is implemented by emitting the
 * GtkIMContext::retrieve_surrounding signal on the input method; in
 * response to this signal, a widget should provide as much context as
 * is available, up to an entire paragraph, by calling
 * gtk_im_context_set_surrounding(). Note that there is no obligation
 * for a widget to respond to the ::retrieve_surrounding signal, so input
 * methods must be prepared to function without context.
 *
 * Return value: %TRUE if surrounding text was provided; in this case
 *    you must free the result stored in *text.
 **/
gboolean
gtk_im_context_get_surrounding (GtkIMContext *context,
				gchar       **text,
				gint         *cursor_index)
{
  GtkIMContextClass *klass;
  gchar *local_text = NULL;
  gint local_index;
  gboolean result = FALSE;
  
  g_return_val_if_fail (GTK_IS_IM_CONTEXT (context), FALSE);

  klass = GTK_IM_CONTEXT_GET_CLASS (context);
  if (klass->get_surrounding)
    result = klass->get_surrounding (context,
				     text ? text : &local_text,
				     cursor_index ? cursor_index : &local_index);

  if (result)
    g_free (local_text);

  return result;
}

/**
 * gtk_im_context_delete_surrounding:
 * @context: a #GtkIMContext
 * @offset: offset from cursor position in chars;
 *    a negative value means start before the cursor.
 * @n_chars: number of characters to delete.
 * 
 * Asks the widget that the input context is attached to to delete
 * characters around the cursor position by emitting the
 * GtkIMContext::delete_surrounding signal. Note that @offset and @n_chars
 * are in characters not in bytes which differs from the usage other
 * places in #GtkIMContext.
 *
 * In order to use this function, you should first call
 * gtk_im_context_get_surrounding() to get the current context, and
 * call this function immediately afterwards to make sure that you
 * know what you are deleting. You should also account for the fact
 * that even if the signal was handled, the input context might not
 * have deleted all the characters that were requested to be deleted.
 *
 * This function is used by an input method that wants to make
 * subsitutions in the existing text in response to new input. It is
 * not useful for applications.
 * 
 * Return value: %TRUE if the signal was handled.
 **/
gboolean
gtk_im_context_delete_surrounding (GtkIMContext *context,
				   gint          offset,
				   gint          n_chars)
{
  gboolean result;
  
  g_return_val_if_fail (GTK_IS_IM_CONTEXT (context), FALSE);

  g_signal_emit (context,
		 im_context_signals[DELETE_SURROUNDING], 0,
		 offset, n_chars, &result);

  return result;
}

/**
 * hildon_gtk_im_context_has_selection:
 * @context: a #GtkIMContext
 * 
 * Returns TRUE if the widget attached to this input context has some
 * text selected in it.
 *
 * Since: maemo 2.0
 **/
gboolean
hildon_gtk_im_context_has_selection(GtkIMContext *context)
{
  gboolean result = FALSE;

  g_return_val_if_fail (GTK_IS_IM_CONTEXT (context), 0);

  g_signal_emit (context,
		 im_context_signals[HAS_SELECTION], 0,
		 &result);
  return result;
}

/**
 * hildon_gtk_im_context_copy:
 * @context: a #GtkIMContext
 * 
 * Requests from the widget attached to this input context that the
 * selected text in it is copied to clipboard.
 *
 * Since: maemo 2.0
 **/
void
hildon_gtk_im_context_copy(GtkIMContext *context)
{
  g_return_if_fail (GTK_IS_IM_CONTEXT (context));

  g_signal_emit (context, im_context_signals[CLIPBOARD_OPERATION], 0,
                 GTK_IM_CONTEXT_CLIPBOARD_OP_COPY);
}

/**
 * hildon_gtk_im_context_cut:
 * @context: a #GtkIMContext
 * 
 * Requests from the widget attached to this input context that the
 * selected text is cut and copied to clipboard.
 *
 * Since: maemo 2.0
 **/
void
hildon_gtk_im_context_cut(GtkIMContext *context)
{
  g_return_if_fail (GTK_IS_IM_CONTEXT (context));

  g_signal_emit (context, im_context_signals[CLIPBOARD_OPERATION], 0,
                 GTK_IM_CONTEXT_CLIPBOARD_OP_CUT);
}

/**
 * hildon_gtk_im_context_paste:
 * @context: a #GtkIMContext
 * 
 * Requests from the widget attached to this input context that the
 * text in clipboard is pasted to it.
 *
 * Since: maemo 2.0
 **/
void
hildon_gtk_im_context_paste(GtkIMContext *context)
{
  g_return_if_fail (GTK_IS_IM_CONTEXT (context));

  g_signal_emit (context, im_context_signals[CLIPBOARD_OPERATION], 0,
                 GTK_IM_CONTEXT_CLIPBOARD_OP_PASTE);
}

#define __GTK_IM_CONTEXT_C__
#include "gtkaliasdef.c"
