/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/* 
 * This file is part of libhildonwm
 *
 * Copyright (C) 2005, 2006, 2007 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "hd-wm-window.h"

#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <X11/Xutil.h> 		/* For WMHints */
#include <X11/Xatom.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h> /* needed by hildon-navigator-main.h */

#ifdef HAVE_LIBHILDON
#include <hildon/hildon-defines.h>
#include <hildon/hildon-banner.h>
#include <hildon/hildon-note.h>
#else
#include <hildon-widgets/hildon-defines.h>
#include <hildon-widgets/hildon-banner.h>
#include <hildon-widgets/hildon-note.h>
#endif

#include "hd-wm-application.h"
#include "hd-wm-entry-info.h"

#define _(o) o

#define HD_WM_WINDOW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), HD_WM_TYPE_WINDOW, HDWMWindowPrivate))

#define PING_TIMEOUT_MESSAGE_STRING       _( "qgn_nc_apkil_notresponding" )
#define PING_TIMEOUT_RESPONSE_STRING      _( "qgn_ib_apkil_responded" )
#define PING_TIMEOUT_KILL_FAILURE_STRING  _( "" )

#define PING_TIMEOUT_BUTTON_OK_STRING     _( "qgn_bd_apkil_ok" )
#define PING_TIMEOUT_BUTTON_CANCEL_STRING _( "qgn_bd_apkil_cancel" )

#define HIBERNATION_TIMEMOUT 3000 /* as suggested by 31410#10 */

enum
{
  HDWM_WIN_CLOSE_WINDOW_SIGNAL,
  HDWM_WIN_N_SIGNALS
};  

gint hdwm_win_signals [HDWM_WIN_N_SIGNALS];

static void hd_wm_window_entry_info_init (HDWMEntryInfoIface *iface);

G_DEFINE_TYPE_WITH_CODE (HDWMWindow,hd_wm_window,G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (HD_WM_TYPE_ENTRY_INFO,
                                                hd_wm_window_entry_info_init));

typedef char HDWMWindowFlags;

typedef enum 
{
  HDWM_WIN_URGENT           = 1 << 0,
  HDWM_WIN_NO_INITIAL_FOCUS = 1 << 1,

  /*
   *  Last dummy value for compile time check.
   *  If you need values > HDWM_WIN_LAST, you have to change the
   *  definition of HDWMWindowFlags to get more space.
   */
  HDWM_WIN_LAST           = 1 << 7
}HDWMWindowFlagsEnum;


/*
 *  compile time assert making sure that the storage space for our flags is
 *  big enough
*/
typedef char __window_compile_time_check_for_flags_size[
     (guint)(1<<(sizeof(HDWMWindowFlags)*8))>(guint)HDWM_WIN_LAST ? 1:-1
                                                       ];

#define HDWM_WIN_SET_FLAG(a,f)    ((a)->priv->flags |= (f))
#define HDWM_WIN_UNSET_FLAG(a,f)  ((a)->priv->flags &= ~(f))
#define HDWM_WIN_IS_SET_FLAG(a,f) ((a)->priv->flags & (f))

struct _HDWMWindowPrivate
{
  Window                  xwin;
  gchar                  *name;
  gchar                  *subname;
  GtkWidget              *menu_widget;   /* Active if no views */
  HDWMApplication       *app_parent;
  GdkPixbuf              *pixb_icon;
  Window                  xwin_group;
  gchar                  *hibernation_key;
  HDWMWindowFlags  flags;
  HDEntryInfo            *info;
  GdkWindow              *gdk_wrapper_win;

  HDWMEntryInfo		 *parent;
  gboolean 		  ignore_urgent;
  gboolean		  info_init;
  gboolean		  override_icon_geometry;
};

struct xwinv
{
  Window *wins;
  gint    n_wins;
};

/* NOTE: for below caller traps x errors */
static void
hd_wm_window_process_wm_state (HDWMWindow *win);

static void
hd_wm_window_process_wm_name (HDWMWindow *win);

static void
hd_wm_window_process_hibernation_prop (HDWMWindow *win);

static void
hd_wm_window_process_wm_hints (HDWMWindow *win);

static void
hd_wm_window_process_net_wm_icon (HDWMWindow *win);

static void
hd_wm_window_process_net_wm_user_time (HDWMWindow *win);

static void 
hd_wm_window_process_wm_window_role (HDWMWindow *win);

static void
pixbuf_destroy (guchar *pixels, gpointer data)
{
  /* For hd_wm_window_process_net_wm_icon  */
  g_free(pixels);
}

static gboolean 
hd_wm_win_info_init (HDWMEntryInfo *info)
{
  HDWMWindow *win = (HDWMWindow *) info;
  
  if (win->priv->info_init)
    return TRUE;

  win->priv->info_init = TRUE;

  return FALSE;  
}

static HDWMEntryInfo *
hd_wm_win_info_get_parent (HDWMEntryInfo *info)
{
  HDWMWindow *win = (HDWMWindow *) info;

  return win->priv->parent;
}

static void 
hd_wm_win_info_set_parent (HDWMEntryInfo *info, HDWMEntryInfo *parent)
{
  HDWMWindow *win = (HDWMWindow *) info;

  win->priv->parent = parent;
}

static const gchar *
hd_wm_win_info_peek_app_name (HDWMEntryInfo *info)
{
  HDWMWindow *win = (HDWMWindow *) info;

  return hd_wm_window_get_name (win);
}

static const gchar *
hd_wm_win_info_peek_title (HDWMEntryInfo *info)
{
  HDWMWindow *win = (HDWMWindow *) info;

  return hd_wm_window_get_name (win);
}

static gchar *
hd_wm_win_info_get_title (HDWMEntryInfo *info)
{
  HDWMWindow *win = (HDWMWindow *) info;

  return g_strdup (hd_wm_window_get_name (win));
}

static void 
hd_wm_win_info_set_title (HDWMEntryInfo *info, const gchar *title)
{
  HDWMWindow *win = (HDWMWindow *) info;

  hd_wm_window_set_name (win, title);
}	

static gchar * 
hd_wm_win_info_get_app_name (HDWMEntryInfo *info)
{
  gchar *title, *sep;

  title = hd_wm_entry_info_get_title (info);

  if (!title)
    return NULL;

  sep = strstr (title, " - ");
  if (sep)
  {
    gchar *retval;

    *sep = 0;
    retval = g_strdup (title);

    g_free (title);

    return retval;
  }
 
  return title;	
}

static gchar * 
hd_wm_win_info_get_window_name (HDWMEntryInfo *info)
{
  gchar *title, *sep;

  title = hd_wm_entry_info_get_title (info);
  if (!title)
    return NULL;

  sep = strstr (title, " - ");
  if (sep)
  {
    gchar *retval;

    *sep = 0;
    retval = g_strdup (sep + 3);

    g_free (title);

    return retval;
  }
  else
    return NULL;
}

static GdkPixbuf *
hd_wm_win_info_get_icon (HDWMEntryInfo *info)
{
  HDWMWindow *win = (HDWMWindow *) info;

  return hd_wm_window_get_custom_icon (win);
}  

static const gchar *
hd_wm_win_info_get_app_icon_name (HDWMEntryInfo *info)
{
  HDWMWindow *win = (HDWMWindow *) info;
  HDWMApplication *app;
  
  app = hd_wm_window_get_application (win);

  if (app)
    return hd_wm_application_get_icon_name (app);

  return NULL;
}

static GdkPixbuf *
hd_wm_win_info_get_app_icon (HDWMEntryInfo *info,
                               gint           size,
                               GError     **error)
{
  GdkPixbuf *retval;	
  const gchar *icon_name;
  GError *load_error = NULL;
  HDWMWindow *win = (HDWMWindow *) info;
  HDWMApplication *app;

  app = hd_wm_window_get_application (win);

  if (!app)
    return NULL;	  

  retval = g_hash_table_lookup (hd_wm_application_get_icon_cache (app),
                                GINT_TO_POINTER (size));
  if (retval)
    return g_object_ref (retval);

  icon_name = hd_wm_entry_info_get_app_icon_name (info);
  if (!icon_name || icon_name[0] == '\0')
    return NULL;

  load_error = NULL;
  retval = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                     icon_name,
                                     size,
                                     GTK_ICON_LOOKUP_NO_SVG,
                                     &load_error);
  if (load_error)
  {
    g_propagate_error (error, load_error);
    return NULL;
  }

  hd_wm_debug (G_STRLOC ": adding cache entry (size:%d)", size);
  g_hash_table_insert (hd_wm_application_get_icon_cache (app),
                       GINT_TO_POINTER (size),
                       g_object_ref (retval));

  return retval;
}

static void 
hd_wm_win_info_close (HDWMEntryInfo *info)
{
  HDWMWindow *win = (HDWMWindow *) info;

  hd_wm_window_close (win);
}  

static gboolean
hd_wm_win_info_is_urgent (HDWMEntryInfo *info)
{
  HDWMWindow *win = (HDWMWindow *) info;

  return hd_wm_window_is_urgent (win);
}

static gboolean
hd_wm_win_info_get_ignore_urgent (HDWMEntryInfo *info)
{
  HDWMWindow *win = (HDWMWindow *) info;

  return win->priv->ignore_urgent;
}

static void 
hd_wm_win_info_set_ignore_urgent (HDWMEntryInfo *info, gboolean ignore_urgent)
{
  HDWMWindow *win = (HDWMWindow *) info;

  win->priv->ignore_urgent = ignore_urgent;
}

static gboolean
hd_wm_win_info_is_active (HDWMEntryInfo *info)
{
  HDWMWindow *win = (HDWMWindow *) info;

  return hd_wm_window_is_active (win);;
}

static gboolean
hd_wm_win_info_is_hibernating (HDWMEntryInfo *info)
{
  HDWMWindow *win = (HDWMWindow *) info;
  HDWMApplication *app;

  app = hd_wm_window_get_application (win);

  if (app)
    return hd_wm_application_is_hibernating (app);

  return FALSE;
}

static gboolean
hd_wm_win_info_has_extra_icon (HDWMEntryInfo *info)
{
  HDWMWindow *win = (HDWMWindow *) info;
  HDWMApplication *app;

  app = hd_wm_window_get_application (win);

  if (app)
    return (hd_wm_application_get_extra_icon (app) != NULL);

  return FALSE;
}

static Window
hd_wm_win_info_get_x_window (HDWMEntryInfo *info)
{
  HDWMWindow *win = (HDWMWindow *) info;

  return hd_wm_window_get_x_win (win);
}

static void 
hd_wm_window_entry_info_init (HDWMEntryInfoIface *iface)
{
  iface->init		   = hd_wm_win_info_init;	
  iface->get_parent        = hd_wm_win_info_get_parent;
  iface->set_parent        = hd_wm_win_info_set_parent;
  iface->add_child         = NULL;
  iface->remove_child      = NULL;
  iface->get_children      = NULL;
  iface->get_n_children    = NULL;
  iface->peek_app_name     = hd_wm_win_info_peek_app_name;
  iface->peek_title        = hd_wm_win_info_peek_title;
  iface->get_title         = hd_wm_win_info_get_title;
  iface->set_title         = hd_wm_win_info_set_title;
  iface->get_app_name      = hd_wm_win_info_get_app_name;
  iface->get_window_name   = hd_wm_win_info_get_window_name;
  iface->get_icon          = hd_wm_win_info_get_icon;
  iface->set_icon          = NULL;
  iface->get_app_icon_name = hd_wm_win_info_get_app_icon_name;
  iface->get_app_icon      = hd_wm_win_info_get_app_icon;
  iface->close             = hd_wm_win_info_close;
  iface->is_urgent         = hd_wm_win_info_is_urgent;
  iface->get_ignore_urgent = hd_wm_win_info_get_ignore_urgent;
  iface->set_ignore_urgent = hd_wm_win_info_set_ignore_urgent;
  iface->is_active         = hd_wm_win_info_is_active;
  iface->is_hibernating    = hd_wm_win_info_is_hibernating;
  iface->has_extra_icon    = hd_wm_win_info_has_extra_icon;
  iface->get_extra_icon    = NULL;
  iface->get_x_window      = hd_wm_win_info_get_x_window;
}

static void
hd_wm_window_finalize (GObject *object)
{
  HDWMWindow	*win = HD_WM_WINDOW (object);	
  GObject       *note;
  HDWM		*hdwm = hd_wm_get_singleton ();
  
  hd_wm_debug("Removing '%s'", win->priv->name);

  g_signal_emit_by_name (object, "close-window", win);

  /* Dont destroy windows that are hiberating */
  if (hd_wm_window_is_hibernating (win))
    {
      hd_wm_debug ("### Aborting destroying '%s' as hibernating ###", win->priv->name);
      return;
    }
  
  /* If ping timeout note is displayed.. */
  note = hd_wm_application_get_ping_timeout_note (win->priv->app_parent);

  if (note)
  {
    /* Show infobanner */
    gchar *response_message = 
      g_strdup_printf (PING_TIMEOUT_RESPONSE_STRING, win->priv->name);
  
      /* Show the infoprint */
    hd_wm_debug ("TODO: %s hildon_banner_show_information (NULL, NULL, response_message);",
	     response_message);

    g_free (response_message);
      
    /* .. destroy the infonote */
    g_object_unref (note);

    hd_wm_application_set_ping_timeout_note (win->priv->app_parent, NULL);
  }
  
  if (hd_wm_entry_info_init (HD_WM_ENTRY_INFO (win)))
  {
    /* only windows of multiwindow apps have their own info */
    hd_wm_debug ("TO BE REMOVED a window of multiwindow application; removing info from AS");

    gboolean removed_app = hd_wm_remove_applications (hdwm,HD_WM_ENTRY_INFO (win));
 
    g_signal_emit_by_name (hdwm,"entry_info_removed",removed_app,HD_WM_ENTRY_INFO (win));
      
    /*hd_entry_info_free (win->priv->info);*/
  }
  
  if (win->priv->name)
    XFree (win->priv->name);

  if (win->priv->subname)
    XFree (win->priv->subname);

  if (win->priv->hibernation_key)
    g_free (win->priv->hibernation_key);

  if (win->priv->pixb_icon)
    g_object_unref (win->priv->pixb_icon);

  if(win->priv->app_parent && 
     win == hd_wm_application_get_active_window (win->priv->app_parent))
    {
      hd_wm_application_set_active_window (win->priv->app_parent, NULL);
    }

  if (hd_wm_get_active_window() == win)
    hd_wm_reset_active_window();

  if (hd_wm_get_last_active_window() == win)
    hd_wm_reset_last_active_window();

  if (win->priv->gdk_wrapper_win)
    g_object_unref (win->priv->gdk_wrapper_win);

  g_signal_emit_by_name (hdwm, "fullscreen", hd_wm_fullscreen_mode ());
}

static void 
hd_wm_window_init (HDWMWindow *watched)
{
  watched->priv = HD_WM_WINDOW_GET_PRIVATE (watched);	

  watched->priv->parent = NULL;
  
  watched->priv->info_init =
  watched->priv->ignore_urgent =
  watched->priv->override_icon_geometry = FALSE;
}

static void 
hd_wm_window_class_init (HDWMWindowClass *watched_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (watched_class);

  object_class->finalize = hd_wm_window_finalize;
	
  g_type_class_add_private (watched_class, sizeof (HDWMWindowPrivate));

  hdwm_win_signals[HDWM_WIN_CLOSE_WINDOW_SIGNAL] = 
	g_signal_new("close-window",
		     G_OBJECT_CLASS_TYPE(object_class),
		     G_SIGNAL_RUN_FIRST,
		     0,
		     NULL, NULL,
		     g_cclosure_marshal_VOID__POINTER,
		     G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void
hd_wm_window_process_net_wm_icon (HDWMWindow *win)
{
  gulong *data;
  gint    len = 0, offset, w, h, i;
  guchar *rgba_data, *p;
  HDWMEntryInfo *info;
  HDWM	      *hdwm = hd_wm_get_singleton ();

  rgba_data = p = NULL;

  data = hd_wm_util_get_win_prop_data_and_validate 
                 (hd_wm_window_get_x_win (win),
                  hd_wm_get_atom(HD_ATOM_NET_WM_ICON),
                  XA_CARDINAL,
                  0,
                  0,
                  &len);

  if (data == NULL || len < 2)
    goto out;

  offset = 0;

  /* Do we have an icon of required size ?
   * NOTE: ICON_SIZE set in application-switcher.h, defaults to 26
   * FIXME: figure out best way to handle scaling here
   */
  do
    {
      w = data[offset];
      h = data[offset+1];

      hd_wm_debug("got w:%i, h:%im offset is %i\n", w, h, offset);

      if (w == 26 && h == 26)
	break;

      offset += ((w*h) + 2);
    }
  while (offset < len);

  if (offset >= len)
    {
      hd_wm_debug("w,h not found");
      goto out;
    }

  p = rgba_data = g_new (guchar, w * h * 4);

  i = offset+2;

  while (i < (w*h + offset + 2))
    {
      *p++ = (data[i] >> 16) & 0xff;
      *p++ = (data[i] >> 8) & 0xff;
      *p++ = data[i] & 0xff;
      *p++ = data[i] >> 24;
      i++;
    }

  if (win->priv->pixb_icon)
    g_object_unref (win->priv->pixb_icon);

  win->priv->pixb_icon = gdk_pixbuf_new_from_data (rgba_data,
						   GDK_COLORSPACE_RGB,
					     	   TRUE,
					     	   8,
					     	   w, h, w * 4,
					     	   pixbuf_destroy,
					     	   NULL);

  if (win->priv->pixb_icon == NULL)
  {
    g_free (rgba_data);
    goto out;
  }

  /* FIXME: need to just update icon, also could be broke for views */
  info = HD_WM_ENTRY_INFO (win);

  if (info)
    g_signal_emit_by_name (hdwm,"entry_info_changed",info);

out:
  if (data)
    XFree (data);
}

	static void
hd_wm_window_process_wm_name (HDWMWindow *win)
{
  int                    n_items = 0;
  HDWM			*hdwm = hd_wm_get_singleton ();

  if (win->priv->name)
    XFree (win->priv->name);

  win->priv->name = NULL;

  /* Attempt to get UTF8 name */
  win->priv->name = hd_wm_util_get_win_prop_data_and_validate 
                     (win->priv->xwin,
                      hd_wm_get_atom(HD_ATOM_NET_WM_NAME),
                      hd_wm_get_atom(HD_ATOM_UTF8_STRING),
                      8,
                      0,
                      &n_items);
  
  /* If that fails grab it basic way */
  if (win->priv->name == NULL || n_items == 0 
      || !g_utf8_validate (win->priv->name, n_items, NULL))
    XFetchName(GDK_DISPLAY(), win->priv->xwin, &win->priv->name);

  if (win->priv->name == NULL)
    win->priv->name = g_strdup ("unknown");

  /* Handle sub naming */

  if (win->priv->subname)
    XFree (win->priv->subname);

  win->priv->subname = NULL;

  win->priv->subname = hd_wm_util_get_win_prop_data_and_validate 
                        (win->priv->xwin,
                         hd_wm_get_atom(HD_ATOM_MB_WIN_SUB_NAME),
                         XA_STRING,
                         8,
                         0,
                         NULL);
  
  if (win->priv->info)
    g_signal_emit_by_name (hdwm,"entry_info_changed",win->priv->info);
}  

static void
hd_wm_window_process_wm_window_role (HDWMWindow *win)
{
  gchar *new_key = NULL;

  g_return_if_fail(win);

  new_key = hd_wm_compute_window_hibernation_key 
                      (win->priv->xwin,
                       hd_wm_window_get_application (win));

  if (new_key)
  {
    if (win->priv->hibernation_key)
      g_free (win->priv->hibernation_key);
    win->priv->hibernation_key = new_key;
  }
}

static void
hd_wm_window_process_hibernation_prop (HDWMWindow *win)
{
  Atom             *foo = NULL;
  HDWMApplication *app;

  app = hd_wm_window_get_application (win);

  if (!app)
    return;

  /* NOTE: foo has no 'value' if set the app is killable (hibernatable),
   *       deletes to unset
   */
  foo = hd_wm_util_get_win_prop_data_and_validate 
                     (win->priv->xwin,
                      hd_wm_get_atom(HD_ATOM_HILDON_APP_KILLABLE),
                      XA_STRING,
                      8,
                      0,
                      NULL);

  if (!foo)
    {
      /*try the alias*/
      foo = hd_wm_util_get_win_prop_data_and_validate 
                    (win->priv->xwin,
                     hd_wm_get_atom(HD_ATOM_HILDON_ABLE_TO_HIBERNATE),
                     XA_STRING,
                     8,
                     0,
                     NULL);
    }
  
  hd_wm_application_set_able_to_hibernate (app,
					     (foo != NULL) ? TRUE : FALSE );
  if (foo)
    XFree(foo);
}

static void
hd_wm_window_process_wm_state (HDWMWindow *win)
{
  HDWMApplication *app;
  Atom             *state = NULL;

  app = hd_wm_window_get_application (win);

  state = hd_wm_util_get_win_prop_data_and_validate 
                  (win->priv->xwin,
                   hd_wm_get_atom(HD_ATOM_WM_STATE),
                   hd_wm_get_atom(HD_ATOM_WM_STATE),
                   0, /* FIXME: format */
                   0,
                   NULL);
  
  if (!state
      || (hd_wm_application_is_minimised(app) && state[0] == IconicState) )
    goto out;
  
  if (state[0] == IconicState)
    hd_wm_application_set_minimised (app, TRUE);
  else /* Assume non minimised state */
    hd_wm_application_set_minimised (app, FALSE);

 out:

  if (state)
    XFree(state);
}

static void
hd_wm_window_process_wm_hints (HDWMWindow *win)
{
  HDWMApplication *app;
  XWMHints         *wm_hints;
  gboolean          need_icon_sync = FALSE;
  HDWM		   *hdwm = hd_wm_get_singleton ();

  app = hd_wm_window_get_application (win);

  wm_hints = XGetWMHints (GDK_DISPLAY(), win->priv->xwin);

  if (!wm_hints)
    return;

  win->priv->xwin_group = wm_hints->window_group;

  if (HDWM_WIN_IS_SET_FLAG(win,HDWM_WIN_URGENT) 
                != (wm_hints->flags & XUrgencyHint))
    need_icon_sync = TRUE;

  if (wm_hints->flags & XUrgencyHint)
    HDWM_WIN_SET_FLAG(win,HDWM_WIN_URGENT);
  else
    HDWM_WIN_UNSET_FLAG(win,HDWM_WIN_URGENT);
 
  if (need_icon_sync)
  {
    HDWMEntryInfo *info = HD_WM_ENTRY_INFO (win);

    if (hd_wm_entry_info_init (info))
      g_signal_emit_by_name (hdwm,"entry_info_changed",info);
  }
  
  XFree(wm_hints);
}

static void
hd_wm_window_process_net_wm_user_time (HDWMWindow *win)
{
  gulong *data;

  data = hd_wm_util_get_win_prop_data_and_validate 
                (hd_wm_window_get_x_win (win),
                 hd_wm_get_atom(HD_ATOM_NET_WM_USER_TIME),
                 XA_CARDINAL,
                 0,
                 0,
                 NULL);
  
  hd_wm_debug("#### processing _NET_WM_USER_TIME ####");

  if (data == NULL)
    return;

  if (*data == 0)
    HDWM_WIN_SET_FLAG (win,HDWM_WIN_NO_INITIAL_FOCUS);
  
  if (data)
    XFree(data);
}

HDWMWindow*
hd_wm_window_new (Window            xid,
			  HDWMApplication *app)
{
  HDWMWindow *win = NULL;
  gchar             *hkey;
  gboolean           win_found = FALSE;
  gpointer           win_ptr = NULL;
  gpointer           orig_key_ptr = NULL;


  /*  Check if this window is actually one coming out
   *  of 'hibernation', we use its WM_CLASS[+WM_WINDOW_ROLE] 
   *  to identify.
   *
   *  WM_WINDOW_ROLE should be unique for grouped/HildonProgram 
   *  windows.
   *
  */
  hkey = hd_wm_compute_window_hibernation_key(xid, app);

  g_return_val_if_fail(hkey, NULL);

  win_found = g_hash_table_lookup_extended (hd_wm_get_hibernating_windows(),
                                            (gconstpointer)hkey,
                                            & orig_key_ptr,
                                            & win_ptr);
  
  if (win_found)
  {
    HDEntryInfo *info = NULL;
    HDWM	  *hdwm = hd_wm_get_singleton ();

    win = (HDWMWindow*)win_ptr;
      
      /* Window already exists in our hibernating window hash.
       * There for we can reuse by just updating its var with
       * new window values
       */
    g_hash_table_steal (hd_wm_get_hibernating_windows(), hkey);

      /* We already have this value */
    g_free(hkey); 
    hkey = NULL;

    /* free the hash key */
    g_free(orig_key_ptr);

    /* mark the application as no longer hibernating */
    hd_wm_application_set_hibernate (app, FALSE);

      /* As window has come out of hibernation and we still have all
       * resources ( views etc ) view creation etc wont fire
       * needed app_switcher updates. Therefore explicitly push
       * window up our selves.
       * Note, win->view_active will be NULL for viewless windows.
       *
       * We have to use a view from the list here, as the active view is
       * not set for single-view apps, and does not have to be valid for
       * multi-view apps that just woken up.
       */
    info = hd_wm_window_peek_info (win);

    if (info)
      g_signal_emit_by_name (hdwm,"entry_info_stack_changed", info);
  }
  else
    win = HD_WM_WINDOW (g_object_new (HD_WM_TYPE_WINDOW, NULL));

  if (!win)
  {
    if (hkey)
      g_free(hkey); 
    return NULL;
  }

  win->priv->xwin       = xid;
  win->priv->app_parent = app;

  if (hkey)
    win->priv->hibernation_key = hkey;

  /* Grab some initial props */
  hd_wm_window_props_sync (win,
				   HD_WM_SYNC_NAME | 
				   HD_WM_SYNC_WMHINTS | 
				   HD_WM_SYNC_ICON | 
				   HD_WM_SYNC_HILDON_APP_KILLABLE | 
				   HD_WM_SYNC_USER_TIME);
  return win;
}


HDWMApplication*
hd_wm_window_get_application (HDWMWindow *win)
{
  return win->priv->app_parent;
}

Window
hd_wm_window_get_x_win (HDWMWindow *win)
{
  return win->priv->xwin;
}

gboolean
hd_wm_window_is_urgent (HDWMWindow *win)
{
  return HDWM_WIN_IS_SET_FLAG(win,HDWM_WIN_URGENT);

}

gboolean
hd_wm_window_wants_no_initial_focus (HDWMWindow *win)
{
  return HDWM_WIN_IS_SET_FLAG(win,HDWM_WIN_NO_INITIAL_FOCUS);
}

const gchar*
hd_wm_window_get_name (HDWMWindow *win)
{
  return win->priv->name;
}

const gchar*
hd_wm_window_get_sub_name (HDWMWindow *win)
{
  return win->priv->subname;
}

const gchar*
hd_wm_window_get_hibernation_key (HDWMWindow *win)
{
  return win->priv->hibernation_key;
}

void
hd_wm_window_set_name (HDWMWindow *win,
			       const gchar             *name)
{
  if (win->priv->name) 
    g_free (win->priv->name);
  
  win->priv->name = g_strdup (name);
}

void
hd_wm_window_set_gdk_wrapper_win (HDWMWindow *win,
                                          GdkWindow         *wrapper_win)
{
  if (win->priv->gdk_wrapper_win)
    g_object_unref (win->priv->gdk_wrapper_win);
  win->priv->gdk_wrapper_win = wrapper_win;
}

GdkWindow *
hd_wm_window_get_gdk_wrapper_win (HDWMWindow *win)
{
  return win->priv->gdk_wrapper_win;
}

GtkWidget*
hd_wm_window_get_menu (HDWMWindow *win)
{
  return win->priv->menu_widget;
}

GdkPixbuf*
hd_wm_window_get_custom_icon (HDWMWindow *win)
{
  if (win->priv->pixb_icon == NULL)
    return NULL;

  return gdk_pixbuf_copy (win->priv->pixb_icon);
}

void
hd_wm_window_set_menu (HDWMWindow *win,
			       GtkWidget         *menu)
{
  win->priv->menu_widget = menu;
}

static gboolean
hd_wm_window_sigterm_timeout_cb (gpointer data)
{
  pid_t pid;
  
  g_return_val_if_fail (data, FALSE);

  pid = (pid_t) GPOINTER_TO_INT (data);
  
  if (pid && !kill (pid, 0))
  {
    /* app did not exit in response to SIGTERM, kill it */

    if (kill (pid, SIGKILL))
    {
      /* Something went wrong, perhaps we do not have sufficient
       * permissions to kill this process
       */
	  hd_wm_debug ("%s %d: SIGKILL failed",__FILE__,__LINE__);
        }
    }
  
  return FALSE;
}

gboolean
hd_wm_window_attempt_signal_kill (HDWMWindow *win,
                                          int sig,
                                          gboolean ensure)
{
  guint32 *pid_result = NULL;

  pid_result = hd_wm_util_get_win_prop_data_and_validate 
                     (win->priv->xwin,
                      hd_wm_get_atom(HD_ATOM_NET_WM_PID),
                      XA_CARDINAL,
                      32,
                      0,
                      NULL);

  if (pid_result == NULL)
    return FALSE;

  if (!pid_result[0])
  {
    g_warning("%s %d: PID was 0",__FILE__,__LINE__);
    XFree (pid_result);
    return FALSE;
  }

  hd_wm_debug ("Attempting to kill pid %d with signal %d", pid_result[0], sig);

  if (sig == SIGTERM && ensure)
  {
    /* install timeout to check that the SIGTERM was handled */
    g_timeout_add (HIBERNATION_TIMEMOUT,
                   (GSourceFunc)hd_wm_window_sigterm_timeout_cb,
                   GINT_TO_POINTER (pid_result[0]));
  }
  
  /* we would like to use -1 to indicate that children should be
   *  killed too, but it does not work for some reason
   */
  if (kill((pid_t)(pid_result[0]), sig) != 0)
  {
    g_warning ("Failed to kill pid %d with signal %d",
 	       pid_result[0], sig);

    XFree (pid_result);
    return FALSE;
  }

  XFree (pid_result);
  return TRUE;
}

gboolean
hd_wm_window_is_hibernating (HDWMWindow *win)
{
  HDWMApplication      *app;

  app = hd_wm_window_get_application (win);

  if (app && hd_wm_application_is_hibernating (app))
    return TRUE;

  return FALSE;
}

void
hd_wm_window_awake (HDWMWindow *win)
{
  HDWMApplication      *app;

  app = hd_wm_window_get_application(win);

  if (app)
  {
    /* Relaunch it with RESTORED */
    hd_wm_application_set_launching (app, TRUE);
    hd_wm_activate_service (hd_wm_application_get_service (app),
                            RESTORED);

    /* do not reset the hibernating flag yet -- we will do that when the app
     * creates its first window
    */
  }
}



gboolean
hd_wm_window_props_sync (HDWMWindow *win, gulong props)
{
  gdk_error_trap_push();

  if (props & HD_WM_SYNC_NAME)
    hd_wm_window_process_wm_name (win);

  if (props & HD_WM_SYNC_HILDON_APP_KILLABLE)
    hd_wm_window_process_hibernation_prop (win);

  if (props & HD_WM_SYNC_WM_STATE)
    hd_wm_window_process_wm_state (win);

  if (props & HD_WM_SYNC_WMHINTS)
    hd_wm_window_process_wm_hints (win);

  if (props & HD_WM_SYNC_ICON)
    hd_wm_window_process_net_wm_icon (win);

  if (props & HD_WM_SYNC_USER_TIME)
    hd_wm_window_process_net_wm_user_time (win);

  if (props & HD_WM_SYNC_WINDOW_ROLE)
    hd_wm_window_process_wm_window_role (win);

  gdk_error_trap_pop();

  return TRUE;
}

void
hd_wm_window_reset_x_win (HDWMWindow * win)
{
  g_return_if_fail (win);
  win->priv->xwin = None;
}

/*
 * Closes window and associated views (if any), handling hibernated
 * applications according to the UI spec.
*/
void
hd_wm_window_close (HDWMWindow *win)
{
  XEvent ev;

  g_return_if_fail(win);

  if(!hd_wm_window_is_hibernating(win))
  {
    /*
     * To close the window, we dispatch _NET_CLOSE_WINDOW event to Matchbox.
     * This will simulate the pressing of the close button on the app window,
     * the app will do its thing, and in turn we will be notified about 
     * changed client list, triggering the necessary updates of AS, etc.
     */
    memset(&ev, 0, sizeof(ev));

    ev.xclient.type         = ClientMessage;
    ev.xclient.window       = hd_wm_window_get_x_win (win);
    ev.xclient.message_type = hd_wm_get_atom (HD_ATOM_NET_CLOSE_WINDOW);
    ev.xclient.format       = 32;
    ev.xclient.data.l[0]    = CurrentTime;
    ev.xclient.data.l[1]    = GDK_WINDOW_XID (gdk_get_default_root_window ());
      /*ev.xclient.data.l[1]    = GDK_WINDOW_XID(gtk_widget_get_parent_window (GTK_WIDGET (tasknav)));*/
  
    gdk_error_trap_push();
    XSendEvent (GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,SubstructureRedirectMask, &ev);

    XSync(GDK_DISPLAY(),FALSE);
    gdk_error_trap_pop();
  }
  else /* hibernating window with no dirty data */
  {
    /* turn off the hibernation flag in our app to force full destruction */
    HDWMApplication * app = hd_wm_window_get_application (win);

    g_return_if_fail (app);

    /* Set hibernate to FALSE and remove from hibernation hash as not
     * hibernating anymore. Note g_hash_table_remove will call
     * hd_wm_window_destroy()
     */
    hd_wm_application_set_hibernate (app, FALSE);

    g_hash_table_remove (hd_wm_get_hibernating_windows(), 
                         hd_wm_window_get_hibernation_key (win));

    /* If the app has any windows left, we need to reset the hibernation 
     * flag back to TRUE
     */
    if (hd_wm_application_has_hibernating_windows (app))
        hd_wm_application_set_hibernate(app, TRUE);
  }
}

void
hd_wm_window_set_info (HDWMWindow *win,
			       HDWMEntryInfo       *info)
{
}

HDEntryInfo *
hd_wm_window_peek_info (HDWMWindow *win)
{
  return win->priv->info;
}

HDEntryInfo *
hd_wm_window_create_new_info (HDWMWindow *win)
{
  return win->priv->info;
}

void
hd_wm_window_destroy_info (HDWMWindow *win)
{
  if (win->priv->info)
  {
    win->priv->info = NULL;
  }
}

gboolean
hd_wm_window_is_active (HDWMWindow *win)
{
  if (win && win == hd_wm_get_active_window())
    return TRUE;

  return FALSE;
}

void         
hd_wm_window_set_icon_geometry (HDWMWindow *win,
                                gint x,
                                gint y,
                                gint width,
                                gint height,
                                gboolean override)
{
  gulong data[4];
  gboolean old_override_flag;
  GdkDisplay *display = gdk_x11_lookup_xdisplay (GDK_DISPLAY ());

  g_return_if_fail (win != NULL && HD_WM_IS_WINDOW (win));

  old_override_flag = win->priv->override_icon_geometry;

  win->priv->override_icon_geometry = override;

  if (!old_override_flag && !override)
    return;

  data[0] = x;
  data[1] = y;
  data[2] = width;
  data[3] = height;

  gdk_error_trap_push ();

  XChangeProperty (GDK_DISPLAY (),
                   hd_wm_window_get_x_win (win),
                   gdk_x11_get_xatom_by_name_for_display (display,"_NET_WM_ICON_GEOMETRY"),
                   XA_CARDINAL, 32, PropModeReplace,
                   (guchar *)&data, 4);

  gdk_error_trap_pop ();
}

