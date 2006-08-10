/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005,2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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

#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <X11/Xutil.h> 		/* For WMHints */
#include <X11/Xatom.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h> /* needed by hildon-navigator-main.h */
#include <libosso.h>
#include <log-functions.h>
#include <hildon-widgets/hildon-defines.h>
#include <hildon-widgets/hildon-banner.h>
#include <hildon-widgets/hildon-note.h>

#include "hn-wm-watched-window.h"
#include "hn-wm-watched-window-view.h"
#include "hn-wm-watchable-app.h"
#include "hn-entry-info.h"
#include "hn-app-switcher.h"
#include "osso-manager.h"
#include "others-menu.h"          /* needed by hildon-navigator-main.h */
#include "hildon-navigator-main.h"

#define PING_TIMEOUT_MESSAGE_STRING       _( "qgn_nc_apkil_notresponding" )
#define PING_TIMEOUT_RESPONSE_STRING      _( "qgn_ib_apkil_responded" )
#define PING_TIMEOUT_KILL_FAILURE_STRING  _( "" )

#define PING_TIMEOUT_BUTTON_OK_STRING     _( "qgn_bd_apkil_ok" )
#define PING_TIMEOUT_BUTTON_CANCEL_STRING _( "qgn_bd_apkil_cancel" )

#define HIBERNATION_TIMEMOUT 3000 /* as suggested by 31410#10 */

extern Navigator * task_nav;

extern Navigator * task_nav;

typedef char HNWMWatchedWindowFlags;

typedef enum 
{
  HNWM_WIN_URGENT           = 1 << 0,
  HNWM_WIN_NO_INITIAL_FOCUS = 1 << 1,

  /*
   *  Last dummy value for compile time check.
   *  If you need values > HNWM_WIN_LAST, you have to change the
   *  definition of HNWMWatchedWindowFlags to get more space.
   */
  HNWM_WIN_LAST           = 1 << 7
}HNWMWatchedWindowFlagsEnum;


/*
 *  compile time assert making sure that the storage space for our flags is
 *  big enough
*/
typedef char __window_compile_time_check_for_flags_size[
     (guint)(1<<(sizeof(HNWMWatchedWindowFlags)*8))>(guint)HNWM_WIN_LAST ? 1:-1
                                                       ];

#define HNWM_WIN_SET_FLAG(a,f)    ((a)->flags |= (f))
#define HNWM_WIN_UNSET_FLAG(a,f)  ((a)->flags &= ~(f))
#define HNWM_WIN_IS_SET_FLAG(a,f) ((a)->flags & (f))

struct HNWMWatchedWindow
{
  Window                  xwin;
  gchar                  *name;
  gchar                  *subname;
  GtkWidget              *menu_widget;   /* Active if no views */
  HNWMWatchableApp       *app_parent;
  GList                  *views;
  HNWMWatchedWindowView  *view_active;
  GdkPixbuf              *pixb_icon;
  Window                  xwin_group;
  gchar                  *hibernation_key;
  HNWMWatchedWindowFlags  flags;
  HNEntryInfo            *info;
};

struct xwinv
{
  Window *wins;
  gint    n_wins;
};

/* NOTE: for below caller traps x errors */
static void
hn_wm_watched_window_process_wm_state (HNWMWatchedWindow *win);

static void
hn_wm_watched_window_process_hildon_view_list (HNWMWatchedWindow *win);

static void
hn_wm_watched_window_process_wm_name (HNWMWatchedWindow *win);

static void
hn_wm_watched_window_process_hibernation_prop (HNWMWatchedWindow *win);

static void
hn_wm_watched_window_process_wm_hints (HNWMWatchedWindow *win);

static void
hn_wm_watched_window_process_net_wm_icon (HNWMWatchedWindow *win);

static void
hn_wm_watched_window_process_net_wm_user_time (HNWMWatchedWindow *win);

static void
hn_wm_watched_window_process_wm_window_role (HNWMWatchedWindow *win);

static void
pixbuf_destroy (guchar *pixels, gpointer data)
{
  /* For hn_wm_watched_window_process_net_wm_icon  */
  g_free(pixels);
}

static void
hn_wm_watched_window_process_net_wm_icon (HNWMWatchedWindow *win)
{
  gulong *data;
  gint    len = 0, offset, w, h, i;
  guchar *rgba_data, *p;
  HNEntryInfo *info;

  rgba_data = p = NULL;

  data = hn_wm_util_get_win_prop_data_and_validate 
                 (hn_wm_watched_window_get_x_win (win),
                  hn_wm_get_atom(HN_ATOM_NET_WM_ICON),
                  XA_CARDINAL,
                  0,
                  0,
                  &len);

  HN_DBG("#### grabbing NET icon ####");

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

      HN_DBG("got w:%i, h:%im offset is %i\n", w, h, offset);

      if (w == 26 && h == 26)
	break;

      offset += ((w*h) + 2);
    }
  while (offset < len);

  if (offset >= len)
    {
      HN_DBG("w,h not found");
      goto out;
    }

  HN_DBG("#### w,h ok ####");

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

  if (win->pixb_icon)
    g_object_unref(win->pixb_icon);

  win->pixb_icon = gdk_pixbuf_new_from_data (rgba_data,
					     GDK_COLORSPACE_RGB,
					     TRUE,
					     8,
					     w, h, w * 4,
					     pixbuf_destroy,
					     NULL);

  if (win->pixb_icon == NULL)
    {
      HN_DBG("#### win->pixb_icon == NULL ####");
      g_free(rgba_data);
      goto out;
    }

  /* FIXME: need to just update icon, also could be broke for views */
  info = hn_wm_watched_window_peek_info (win);

  if(info)
    hn_app_switcher_changed (hn_wm_get_app_switcher(), info);

out:
  if (data)
    XFree(data);
}

static void
hn_wm_watched_window_process_hildon_view_active (HNWMWatchedWindow *win)
{
  Window                *new_active_view_id;
  HNWMWatchedWindowView *current_active_view;
  GList                 *iter = NULL;
  HNWMWatchableApp      *app = NULL;

  if (hn_wm_watched_window_get_views (win) == NULL)
    return;

  app = hn_wm_watched_window_get_app (win);
  
  if (!app)
    return;
  
  new_active_view_id = hn_wm_util_get_win_prop_data_and_validate 
                             (hn_wm_watched_window_get_x_win (win),
                              hn_wm_get_atom(HN_ATOM_HILDON_VIEW_ACTIVE),
                              XA_WINDOW,
                              32,
                              0,
                              NULL);

  if (!new_active_view_id)
    return;

  current_active_view = hn_wm_watched_window_get_active_view (win);

  /* Check the prop value is valid and not alreday active */

  if (current_active_view
      && hn_wm_watched_window_view_get_id (current_active_view) 
            == *new_active_view_id)
    goto out;

  iter = hn_wm_watched_window_get_views (win);
  
  /* Find what the view id matches for this window's views and
   * update.
   */
  while (iter != NULL)
    {
      HNWMWatchedWindowView *view;

      view = (HNWMWatchedWindowView *)iter->data;

      if (hn_wm_watched_window_view_get_id (view) == *new_active_view_id)
	{
          HNEntryInfo *info;

	  info = hn_wm_watched_window_view_get_info (view);
	  hn_wm_watched_window_set_active_view (win, view);
	  hn_app_switcher_changed_stack (hn_wm_get_app_switcher (), info);
	  goto out;
	}
      iter  = g_list_next(iter);
    }

 out:

  if (new_active_view_id)
    XFree(new_active_view_id);

  return;
}

static void
hn_wm_watched_window_process_wm_name (HNWMWatchedWindow *win)
{
  HNWMWatchedWindowView *view;
  int                    n_items = 0;

  if (win->name)
    XFree(win->name);

  win->name = NULL;

  /* Attempt to get UTF8 name */
  win->name = hn_wm_util_get_win_prop_data_and_validate 
                    (win->xwin,
                     hn_wm_get_atom(HN_ATOM_NET_WM_NAME),
                     hn_wm_get_atom(HN_ATOM_UTF8_STRING),
                     8,
                     0,
                     &n_items);
  
  /* If that fails grab it basic way */
  if (win->name == NULL || n_items == 0 
      || !g_utf8_validate (win->name, n_items, NULL))
    XFetchName(GDK_DISPLAY(), win->xwin, &win->name);

  if (win->name == NULL)
    win->name = g_strdup("unknown");

  /* Handle sub naming */

  if (win->subname)
    XFree(win->subname);

  win->subname = NULL;

  win->subname = hn_wm_util_get_win_prop_data_and_validate 
                      (win->xwin,
                       hn_wm_get_atom(HN_ATOM_MB_WIN_SUB_NAME),
                       XA_STRING,
                       8,
                       0,
                       NULL);
  
  view = hn_wm_watched_window_get_active_view(win);
  
  /* Duplicate to topped view also */
  if (view)
    hn_wm_watched_window_view_set_name (view, win->name);
  
  if(win->info)
    hn_app_switcher_changed (hn_wm_get_app_switcher (), win->info);
}

static void
hn_wm_watched_window_process_wm_window_role (HNWMWatchedWindow *win)
{
  gchar *new_key = NULL;

  g_return_if_fail(win);

  new_key = hn_wm_compute_watched_window_hibernation_key 
                      (win->xwin,
                       hn_wm_watched_window_get_app (win));

  if (new_key)
    {
      if (win->hibernation_key)
	g_free(win->hibernation_key);
      win->hibernation_key = new_key;
    }
}


static void
hn_wm_watched_window_process_hibernation_prop (HNWMWatchedWindow *win)
{
  Atom             *foo = NULL;
  HNWMWatchableApp *app;

  app = hn_wm_watched_window_get_app (win);

  if (!app)
    return;

  /* NOTE: foo has no 'value' if set the app is killable (hibernatable),
   *       deletes to unset
   */
  foo = hn_wm_util_get_win_prop_data_and_validate 
                     (win->xwin,
                      hn_wm_get_atom(HN_ATOM_HILDON_APP_KILLABLE),
                      XA_STRING,
                      8,
                      0,
                      NULL);

  if (!foo)
    {
      /*try the alias*/
      foo = hn_wm_util_get_win_prop_data_and_validate 
                    (win->xwin,
                     hn_wm_get_atom(HN_ATOM_HILDON_ABLE_TO_HIBERNATE),
                     XA_STRING,
                     8,
                     0,
                     NULL);
    }
  
  hn_wm_watchable_app_set_able_to_hibernate (app,
					     (foo != NULL) ? TRUE : FALSE );
  if (foo)
    XFree(foo);
}

static void
hn_wm_watched_window_process_wm_state (HNWMWatchedWindow *win)
{
  HNWMWatchableApp *app;
  Atom             *state = NULL;

  app = hn_wm_watched_window_get_app (win);

  state = hn_wm_util_get_win_prop_data_and_validate 
                  (win->xwin,
                   hn_wm_get_atom(HN_ATOM_WM_STATE),
                   hn_wm_get_atom(HN_ATOM_WM_STATE),
                   0, /* FIXME: format */
                   0,
                   NULL);
  
  if (!state
      || (hn_wm_watchable_app_is_minimised(app) && state[0] == IconicState) )
    goto out;
  
  if (state[0] == IconicState)
    {
      hn_wm_watchable_app_set_minimised (app, TRUE);
    }
  else /* Assume non minimised state */
    {
      hn_wm_watchable_app_set_minimised (app, FALSE);
    }

 out:

  if (state)
    XFree(state);
}

static void
hn_wm_watched_window_process_wm_hints (HNWMWatchedWindow *win)
{
  HNWMWatchableApp *app;
  XWMHints         *wm_hints;
  gboolean          need_icon_sync = FALSE;

  app = hn_wm_watched_window_get_app (win);

  wm_hints = XGetWMHints (GDK_DISPLAY(), win->xwin);

  if (!wm_hints)
    return;

  win->xwin_group = wm_hints->window_group;

  if (HNWM_WIN_IS_SET_FLAG(win,HNWM_WIN_URGENT) 
                != (wm_hints->flags & XUrgencyHint))
    need_icon_sync = TRUE;

  if(wm_hints->flags & XUrgencyHint)
    {
      HNWM_WIN_SET_FLAG(win,HNWM_WIN_URGENT);
    }
  else
    {
      HNWM_WIN_UNSET_FLAG(win,HNWM_WIN_URGENT);
    }

  if (need_icon_sync)
    {
      HNEntryInfo *info = hn_wm_watched_window_peek_info (win);

      if(info)
        hn_app_switcher_changed (hn_wm_get_app_switcher (), info);
    }
  
  XFree(wm_hints);
}

static void
hn_wm_watched_window_process_net_wm_user_time (HNWMWatchedWindow *win)
{
  gulong *data;

  data = hn_wm_util_get_win_prop_data_and_validate 
                (hn_wm_watched_window_get_x_win (win),
                 hn_wm_get_atom(HN_ATOM_NET_WM_USER_TIME),
                 XA_CARDINAL,
                 0,
                 0,
                 NULL);
  
  HN_DBG("#### processing _NET_WM_USER_TIME ####");

  if (data == NULL)
    return;

  if (*data == 0)
    {
      HNWM_WIN_SET_FLAG(win,HNWM_WIN_NO_INITIAL_FOCUS);
    }
  
  if (data)
    XFree(data);
}

static void
hn_wm_watched_window_process_hildon_view_list (HNWMWatchedWindow *win)
{
  struct xwinv xwins;
  int          i;
  GList       *iter = NULL, *next_iter;
  HNEntryInfo *info;
  
  if (hn_wm_watched_window_is_hibernating(win))
    return;

  xwins.wins = hn_wm_util_get_win_prop_data_and_validate 
                    (win->xwin,
                     hn_wm_get_atom(HN_ATOM_HILDON_VIEW_LIST),
                     XA_WINDOW,
                     32,
                     0,
                     &xwins.n_wins);

  if (G_UNLIKELY(xwins.wins == NULL))
    return;

  HN_DBG("_HILDON_VIEW_LIST change with %i wins", xwins.n_wins);

  iter = hn_wm_watched_window_get_views (win);

  /* Delete an views we have listed, but are not listed in prop */

  while (iter != NULL)
    {
      HNWMWatchedWindowView *view;
      gboolean               view_found;
      
      view       = (HNWMWatchedWindowView *)iter->data;
      view_found = FALSE;
      
      next_iter  = g_list_next(iter);
      
      for (i=0; i < xwins.n_wins; i++)
	if (xwins.wins[i] == hn_wm_watched_window_view_get_id (view))
	  {
	    view_found = TRUE;
	    break;
	  }
      
      if (!view_found)
	{
	  /* view is not listed on client - delete the list entry */
	  hn_wm_watched_window_remove_view  (win, view);
	  hn_wm_watched_window_view_destroy (view);
	}
      
      iter = next_iter;
    }
  
  /* Now add any new views in prop that we dont have listed */

  for (i=0; i < xwins.n_wins; i++)
    {
      gboolean view_found;

      iter       = hn_wm_watched_window_get_views (win);
      view_found = FALSE;

      while (iter != NULL)
	{
	  HNWMWatchedWindowView *view;
		  
	  view = (HNWMWatchedWindowView *)iter->data;
		  
	  if (hn_wm_watched_window_view_get_id (view) == xwins.wins[i])
	    {
	      view_found = TRUE;
	      break;
	    }
	  iter = g_list_next(iter);
	}

      if (!view_found)
        {
          HNWMWatchedWindowView *new_view;

          /* Not in internal list so its new, add it */
          new_view = hn_wm_watched_window_view_new (win, xwins.wins[i]);
          
          if (new_view)
            hn_wm_watched_window_add_view (win, new_view);

          HN_DBG("adding view info to AS");
          info = hn_wm_watched_window_view_get_info (new_view);
          hn_app_switcher_add (hn_wm_get_app_switcher (), info);
  
          /* The window may have been 'viewless' before this 
           * view was created to we need to remove the widget 
           * ref for a viewless window  
           */
          if (hn_wm_watched_window_peek_info (win))
            {
              HN_DBG("adding first view; removing window info from AS");
              
              hn_app_switcher_remove(hn_wm_get_app_switcher (),
                                     hn_wm_watched_window_peek_info (win));

              /*
               * since the window of multiviewed app does not figure in the AS,
               * tell it to get rid of the info
               */
              hn_wm_watched_window_destroy_info (win);
            }
        }
    }

  if (xwins.wins)
    XFree(xwins.wins);
}

HNWMWatchedWindow*
hn_wm_watched_window_new (Window            xid,
			  HNWMWatchableApp *app)
{
  HNWMWatchedWindow *win = NULL;
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
  hkey = hn_wm_compute_watched_window_hibernation_key(xid, app);

  HN_DBG("^^^^ new watched window, key %s ^^^^", hkey);
  
  g_return_val_if_fail(hkey, NULL);

  win_found = g_hash_table_lookup_extended (hn_wm_get_hibernating_windows(),
                                            (gconstpointer)hkey,
                                            & orig_key_ptr,
                                            & win_ptr);
  
  HN_DBG("^^^^ win 0x%x ^^^^", (int)win);

  if (win_found)
    {
      HNEntryInfo *info = NULL;

      HN_DBG("New Window is from hibernation");

      win = (HNWMWatchedWindow*)win_ptr;
      
      /* Window already exists in our hibernating window hash.
       * There for we can reuse by just updating its var with
       * new window values
       */
      g_hash_table_steal(hn_wm_get_hibernating_windows(),
			 hkey);

      /* We already have this value */
      g_free(hkey); 
      hkey = NULL;

      /* free the hash key */
      g_free(orig_key_ptr);

      /* mark the application as no longer hibernating */
      hn_wm_watchable_app_set_hibernate (app, FALSE);

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
      if (win->views)
        info = hn_wm_watched_window_view_get_info (win->views->data);
      else
        info = hn_wm_watched_window_peek_info (win);

      if (info)
        hn_app_switcher_changed_stack (hn_wm_get_app_switcher(), info);
    }
  else
    win = g_new0 (HNWMWatchedWindow, 1);

  if (!win)
    {
      if (hkey)
	g_free(hkey); 
      return NULL;
    }

  win->xwin       = xid;
  win->app_parent = app;

  if(hkey)
    win->hibernation_key = hkey;

  /* Grab some initial props */
  hn_wm_watched_window_props_sync (win,
				   HN_WM_SYNC_NAME
				   |HN_WM_SYNC_WMHINTS
				   |HN_WM_SYNC_ICON
				   |HN_WM_SYNC_HILDON_APP_KILLABLE
				   |HN_WM_SYNC_USER_TIME);
  return win;
}


HNWMWatchableApp*
hn_wm_watched_window_get_app (HNWMWatchedWindow *win)
{
  return win->app_parent;
}

Window
hn_wm_watched_window_get_x_win (HNWMWatchedWindow *win)
{
  return win->xwin;
}

gboolean
hn_wm_watched_window_is_urgent (HNWMWatchedWindow *win)
{
  return HNWM_WIN_IS_SET_FLAG(win,HNWM_WIN_URGENT);

}

gboolean
hn_wm_watched_window_wants_no_initial_focus (HNWMWatchedWindow *win)
{
  return HNWM_WIN_IS_SET_FLAG(win,HNWM_WIN_NO_INITIAL_FOCUS);
}

const gchar*
hn_wm_watched_window_get_name (HNWMWatchedWindow *win)
{
  return win->name;
}

const gchar*
hn_wm_watched_window_get_sub_name (HNWMWatchedWindow *win)
{
  return win->subname;
}

const gchar*
hn_wm_watched_window_get_hibernation_key (HNWMWatchedWindow *win)
{
  return win->hibernation_key;
}

void
hn_wm_watched_window_set_name (HNWMWatchedWindow *win,
			       const gchar             *name)
{
  if (win->name) g_free(win->name);
  win->name = g_strdup(name);
}


GtkWidget*
hn_wm_watched_window_get_menu (HNWMWatchedWindow *win)
{
  return win->menu_widget;
}

GdkPixbuf*
hn_wm_watched_window_get_custom_icon (HNWMWatchedWindow *win)
{
  if (win->pixb_icon == NULL)
    return NULL;

  return gdk_pixbuf_copy(win->pixb_icon);
}

void
hn_wm_watched_window_set_menu (HNWMWatchedWindow *win,
			       GtkWidget         *menu)
{
  win->menu_widget = menu;
}

GList*
hn_wm_watched_window_get_views (HNWMWatchedWindow *win)
{
  g_return_val_if_fail (win != NULL, NULL);
  
  return win->views;
}

gint
hn_wm_watched_window_get_n_views (HNWMWatchedWindow *win)
{
  g_return_val_if_fail (win != NULL, 0);
  
  return g_list_length (win->views);
}

void
hn_wm_watched_window_add_view (HNWMWatchedWindow     *win,
			       HNWMWatchedWindowView *view)
{
  win->views = g_list_append(win->views, view);
}

void
hn_wm_watched_window_remove_view (HNWMWatchedWindow     *win,
				  HNWMWatchedWindowView *view)
{
  GList *view_link;

  view_link = g_list_find (win->views, view);

  if (view_link)
    win->views = g_list_delete_link(win->views, view_link);
}

void
hn_wm_watched_window_set_active_view (HNWMWatchedWindow     *win,
				      HNWMWatchedWindowView *view)
{
  win->view_active = view;
}

HNWMWatchedWindowView*
hn_wm_watched_window_get_active_view (HNWMWatchedWindow     *win)
{
  g_return_val_if_fail (win, NULL);

  if (win->view_active)
  return win->view_active;

  /* We have no active view set atm so just return the first one. 
   * Works around some issues with hildon_app_new_with_view() not 
   * dispatching VIEW_ACTIVE 
  */
  if (win->views)
    return g_list_first(win->views)->data;

  return NULL;
}

static gboolean
hn_wm_watched_window_sigterm_timeout_cb (gpointer data)
{
  pid_t pid;
  
  g_return_val_if_fail (data, FALSE);

  pid = (pid_t) GPOINTER_TO_INT (data);
  
  
  HN_DBG ("Checking if pid %d is still around", pid);
  
  if(pid && !kill (pid, 0))
    {
      /* app did not exit in response to SIGTERM, kill it */
      HN_DBG ("App still around, sending SIGKILL");

      if(kill (pid, SIGKILL))
        {
	  /* Something went wrong, perhaps we do not have sufficient
	   * permissions to kill this process
	   */
	  HN_DBG ("SIGKILL failed");
        }
    }
  
  return FALSE;
}

gboolean
hn_wm_watched_window_attempt_signal_kill (HNWMWatchedWindow *win,
                                          int sig,
                                          gboolean ensure)
{
  guint32 *pid_result = NULL;

  pid_result = hn_wm_util_get_win_prop_data_and_validate 
                     (win->xwin,
                      hn_wm_get_atom(HN_ATOM_NET_WM_PID),
                      XA_CARDINAL,
                      32,
                      0,
                      NULL);

  if (pid_result == NULL)
    return FALSE;

  if(!pid_result[0])
    {
      g_warning("PID was 0");
      XFree(pid_result);
      return FALSE;
    }

  HN_DBG("Attempting to kill pid %d with signal %d",
         pid_result[0], sig);

  if (sig == SIGTERM && ensure)
    {
      /* install timeout to check that the SIGTERM was handled */
      g_timeout_add (HIBERNATION_TIMEMOUT,
                     (GSourceFunc)hn_wm_watched_window_sigterm_timeout_cb,
                     GINT_TO_POINTER (pid_result[0]));
    }
  
  /* we would like to use -1 to indicate that children should be
   *  killed too, but it does not work for some reason
   */
  if(kill((pid_t)(pid_result[0]), sig) != 0)
    {
      HN_DBG("... failed.");
      
      osso_log(LOG_ERR, "Failed to kill pid %d with signal %d",
	       pid_result[0], sig);

      XFree(pid_result);
      return FALSE;
    }

  XFree(pid_result);
  return TRUE;
}

gboolean
hn_wm_watched_window_is_hibernating (HNWMWatchedWindow *win)
{
  HNWMWatchableApp      *app;

  app = hn_wm_watched_window_get_app(win);

  if (app && hn_wm_watchable_app_is_hibernating(app))
    return TRUE;

  return FALSE;
}

void
hn_wm_watched_window_awake (HNWMWatchedWindow *win)
{
  HNWMWatchableApp      *app;
  osso_manager_t        *osso_man;

  app = hn_wm_watched_window_get_app(win);

  if (app)
    {
      /* Relaunch it with RESTORED */
      hn_wm_watchable_app_set_launching (app, TRUE);
      osso_man = osso_manager_singleton_get_instance();
      osso_manager_launch(osso_man, hn_wm_watchable_app_get_service (app),
			  RESTORED);

      /* do not reset the hibernating flag yet -- we will do that when the app
       * creates its first window
      */
    }
}

void
hn_wm_watched_window_destroy (HNWMWatchedWindow *win)
{
  HNWMWatchedWindowView *view;
  GtkWidget             *note;

  HN_DBG("Removing '%s'", win->name);

  /* Dont destroy windows that are hiberating */
  if (hn_wm_watched_window_is_hibernating(win))
    {
      HN_DBG("### Aborting destroying '%s' as hibernating ###", win->name);
      return;
    }
  
  /* If ping timeout note is displayed.. */
  note = hn_wm_watchable_app_get_ping_timeout_note (win->app_parent);

  if (note)
    {
      /* Show infobanner */
      gchar *response_message
            = g_strdup_printf (PING_TIMEOUT_RESPONSE_STRING,
                    win->name );
  
      /* Show the infoprint */
      hildon_banner_show_information (NULL, NULL, response_message );

      g_free (response_message);
      
      /* .. destroy the infonote */
      gtk_widget_destroy( note );
      hn_wm_watchable_app_set_ping_timeout_note (win->app_parent, NULL);
    }
  
  if(win->info)
    {
      /* only windows of multiwindow apps have their own info */
      HN_DBG("a window of multiwindow application; removing info from AS");
      hn_app_switcher_remove(hn_wm_get_app_switcher(),
                             win->info);
      hn_entry_info_free (win->info);
      win->info = NULL;
    }
  
  /* Destroy the views too */
  while (win->views)
    {
      view = (HNWMWatchedWindowView *)win->views->data;

      hn_wm_watched_window_view_destroy (view);

      win->views = g_list_delete_link(win->views, win->views);
    }

  if (win->name)
    XFree(win->name);

  if (win->subname)
    XFree(win->subname);

  if (win->hibernation_key)
    g_free(win->hibernation_key);

  if (win->pixb_icon)
    g_object_unref(win->pixb_icon);

  if(win->app_parent &&
     win == hn_wm_watchable_app_get_active_window(win->app_parent))
    {
      hn_wm_watchable_app_set_active_window(win->app_parent, NULL);
    }

  if(hn_wm_get_active_window() == win)
    hn_wm_reset_active_window();
  
  g_free(win);
}

gboolean
hn_wm_watched_window_props_sync (HNWMWatchedWindow *win, gulong props)
{
  gdk_error_trap_push();

  if (props & HN_WM_SYNC_NAME)
    {
      hn_wm_watched_window_process_wm_name (win);
    }

  if (props & HN_WM_SYNC_HILDON_APP_KILLABLE)
    {
      hn_wm_watched_window_process_hibernation_prop (win);
    }

  if (props & HN_WM_SYNC_WM_STATE)
    {
      hn_wm_watched_window_process_wm_state (win);
    }

  if (props & HN_WM_SYNC_HILDON_VIEW_LIST)
    {
      hn_wm_watched_window_process_hildon_view_list (win);
    }

  if (props & HN_WM_SYNC_HILDON_VIEW_ACTIVE)
    {
      hn_wm_watched_window_process_hildon_view_active (win);
    }

  if (props & HN_WM_SYNC_WMHINTS)
    {
      hn_wm_watched_window_process_wm_hints (win);
    }

  if (props & HN_WM_SYNC_ICON)
    {
      hn_wm_watched_window_process_net_wm_icon (win);
    }

  if (props & HN_WM_SYNC_USER_TIME)
    {
      hn_wm_watched_window_process_net_wm_user_time (win);
    }

  if (props & HN_WM_SYNC_WINDOW_ROLE)
    {
      hn_wm_watched_window_process_wm_window_role (win);
    }

  gdk_error_trap_pop();

  return TRUE;
}

void
hn_wm_watched_window_reset_x_win (HNWMWatchedWindow * win)
{
  g_return_if_fail (win);
  win->xwin = None;
}

static void
hn_wm_ping_timeout_dialog_response (GtkDialog *note, gint ret, gpointer data)
{
  HNWMWatchedWindow *win = (HNWMWatchedWindow *)data;
  HNWMWatchableApp *app = hn_wm_watched_window_get_app (win);

  gtk_widget_destroy (GTK_WIDGET(note));
  hn_wm_watchable_app_set_ping_timeout_note (app, NULL);

  if (ret == GTK_RESPONSE_OK)
    {
      /* Kill the app */
      if (!hn_wm_watched_window_attempt_signal_kill (win, SIGKILL, FALSE))
        {
          HN_DBG ("hn_wm_ping_timeout: "
                  "failed to kill application '%s'.",
                  win->name);
        }
    }
}

void
hn_wm_ping_timeout (HNWMWatchedWindow *win)
{
  GtkWidget *note;

  HNWMWatchableApp *app = hn_wm_watched_window_get_app (win);

  gchar *timeout_message
    = g_strdup_printf (PING_TIMEOUT_MESSAGE_STRING, win->name );

  /* FIXME: Do we need to check if the note already exists? */
  note = hn_wm_watchable_app_get_ping_timeout_note( app );
  
  if (note && GTK_IS_WIDGET(note))
    {
      HN_DBG( "hn_wm_ping_timeout: "
              "the note already exists. ");

      goto cleanup_and_exit;
    }

  note = hildon_note_new_confirmation (NULL, timeout_message);

  hn_wm_watchable_app_set_ping_timeout_note (app, note);

  hildon_note_set_button_texts (HILDON_NOTE(note),
                                PING_TIMEOUT_BUTTON_OK_STRING,
                                PING_TIMEOUT_BUTTON_CANCEL_STRING);

  g_signal_connect (G_OBJECT (note),
                    "response",
                    G_CALLBACK (hn_wm_ping_timeout_dialog_response),
                    win);

  gtk_widget_show_all (note);

cleanup_and_exit:

  g_free( timeout_message );
}


void
hn_wm_ping_timeout_cancel (HNWMWatchedWindow *win)
{
  HNWMWatchableApp *app = hn_wm_watched_window_get_app( win );

  GtkWidget *note = hn_wm_watchable_app_get_ping_timeout_note(app);

  gchar *response_message
    = g_strdup_printf (PING_TIMEOUT_RESPONSE_STRING,
                       win->name );

  if (note && GTK_IS_WIDGET (note)) {
    gtk_dialog_response (GTK_DIALOG(note), GTK_RESPONSE_CANCEL);
  }

  /* Show the infoprint */
  hildon_banner_show_information (NULL, NULL, response_message );

  g_free (response_message);
}


/*
 * Closes window and associated views (if any), handling hibernated
 * applications according to the UI spec.
*/
void
hn_wm_watched_window_close (HNWMWatchedWindow *win)
{
  XEvent ev;

  g_return_if_fail(win);

  if(!hn_wm_watched_window_is_hibernating(win))
    {
      /*
       * To close the window, we dispatch _NET_CLOSE_WINDOW event to Matchbox.
       * This will simulate the pressing of the close button on the app window,
       * the app will do its thing, and in turn we will be notified about 
       * changed client list, triggering the necessary updates of AS, etc.
       */
      memset(&ev, 0, sizeof(ev));

      ev.xclient.type         = ClientMessage;
      ev.xclient.window       = hn_wm_watched_window_get_x_win (win);
      ev.xclient.message_type = hn_wm_get_atom(HN_ATOM_NET_CLOSE_WINDOW);
      ev.xclient.format       = 32;
      ev.xclient.data.l[0]    = CurrentTime;
      ev.xclient.data.l[1]    = GDK_WINDOW_XID(gtk_widget_get_parent_window(task_nav->main_window));
  
      gdk_error_trap_push();
      XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
                 SubstructureRedirectMask, &ev);

      XSync(GDK_DISPLAY(),FALSE);
      gdk_error_trap_pop();
    }
#if 0
  /*
   *  Disabled util the infrastructure outside of TN for handling this is in
   *  place
   */
  else if(/*have_dirty_data test comes here */)
    {
      /*
       * If the window is hibernating and has dirty data, we need to remove the
       * appropriate items from the AS, but we leave the window in our hash, to
       * allow for the data to be recovered, and to comply with UI spec 1.0,
       * paragraph 6.3
      */
      GList * views = hn_wm_watched_window_get_views (win);
      
      if(views)
        {
          while(views)
            {
              app_switcher_remove_item(hn_wm_get_app_switcher(),
                                       hn_wm_watched_window_view_get_menu(
                                       (HNWMWatchedWindowView*)views->data));

              views = g_list_next(views);
            }
        }
      else
        {
          app_switcher_remove_item(hn_wm_get_app_switcher(),
                                   hn_wm_watched_window_get_menu (win));
        }
    }
#endif
  else /* hibernating window with no dirty data */
    {
      /* turn off the hibernation flag in our app to force full destruction */
      HNWMWatchableApp * app = hn_wm_watched_window_get_app (win);

      g_return_if_fail(app);

      /* Set hibernate to FALSE and remove from hibernation hash as not
       * hibernating anymore. Note g_hash_table_remove will call
       * hn_wm_watched_window_destroy()
      */
      hn_wm_watchable_app_set_hibernate(app, FALSE);

      g_hash_table_remove (hn_wm_get_hibernating_windows(), 
                           hn_wm_watched_window_get_hibernation_key(win));

      /* If the app has any windows left, we need to reset the hibernation 
       * flag back to TRUE
       */
      if(hn_wm_watchable_app_has_hibernating_windows (app))
        hn_wm_watchable_app_set_hibernate(app, TRUE);
    }
}

void
hn_wm_watched_window_set_info (HNWMWatchedWindow *win,
			       HNEntryInfo       *info)
{
  if (win->info)
    hn_entry_info_free (win->info);
  
  win->info = info;
}

HNEntryInfo *
hn_wm_watched_window_peek_info (HNWMWatchedWindow *win)
{
  return win->info;
}

HNEntryInfo *
hn_wm_watched_window_create_new_info (HNWMWatchedWindow *win)
{
  if(win->info)
    {
      /*
       * refuse to creat a new info, in case the old one is already referenced
       * by AS
       */
      g_warning("Window info alread exists");
    }
  else
    {
      win->info = hn_entry_info_new_from_window (win);
    }
  
  return win->info;
}

void
hn_wm_watched_window_destroy_info (HNWMWatchedWindow *win)
{
  if(win->info)
    {
      hn_entry_info_free (win->info);
      win->info = NULL;
    }
}

gboolean
hn_wm_watched_window_is_active (HNWMWatchedWindow *win)
{
  if(win && win == hn_wm_get_active_window())
    return TRUE;

  return FALSE;
}

