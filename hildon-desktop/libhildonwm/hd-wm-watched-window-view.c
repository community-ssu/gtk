/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/* 
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005, 2006 Nokia Corporation.
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

#include "hd-wm-watched-window-view.h"
#include "hd-wm-watched-window.h"
#include "hd-wm-watchable-app.h"
#include "hd-entry-info.h"

/* Watched Window views */

struct HDWMWatchedWindowView
{
  GtkWidget         *menu_widget;
  int                id;
  HDWMWatchedWindow *win_parent;
  char              *name;
  HDEntryInfo       *info;
};

HDWMWatchedWindowView*
hd_wm_watched_window_view_new (HDWMWatchedWindow *win,
			       int                view_id)
{
  HDWMWatchedWindowView *view;
  
  view = g_new0 (HDWMWatchedWindowView, 1);  

  if (!view) 	 /* FIXME: Handle OOM */
    return NULL;

  view->id         = view_id;
  view->win_parent = win; 

  return view;
}

HDWMWatchedWindow*
hd_wm_watched_window_view_get_parent (HDWMWatchedWindowView *view)
{
  return view->win_parent;
}

gint
hd_wm_watched_window_view_get_id (HDWMWatchedWindowView *view)
{
  return view->id;
}

GtkWidget*
hd_wm_watched_window_view_get_menu (HDWMWatchedWindowView *view)
{
  return view->menu_widget;
}

void
hd_wm_watched_window_view_set_name (HDWMWatchedWindowView *view,
				    const gchar           *name)
{
  if (name == NULL)
    return;

  if (view->name)
    g_free(view->name);

  view->name = g_strdup(name);
}

const gchar*
hd_wm_watched_window_view_get_name (HDWMWatchedWindowView *view)
{
  if (view->name != NULL)
    return view->name;

  if (view->win_parent && hd_wm_watched_window_get_name (view->win_parent))
    return hd_wm_watched_window_get_name (view->win_parent);

  return "unknown";
}

void
hd_wm_watched_window_view_destroy (HDWMWatchedWindowView *view)
{
  HDWMWatchableApp *app;
  HDWM 		   *hdwm = hd_wm_get_singleton ();

  app = hd_wm_watched_window_get_app(view->win_parent);

  /* Only remove the widget if the app is *really* killed */
  if (app && hd_wm_watchable_app_is_hibernating(app))
    {
      HN_DBG("### Aborting destroying view '%s' as hibernating ###",
             view->name);
      return;
    }


  if (hd_wm_watched_window_get_active_view(view->win_parent) == view)
    hd_wm_watched_window_set_active_view(view->win_parent, NULL);

  if (view->name)
    g_free(view->name);

  /*
   * NB -- this info might have been removed earlier if views are being removed
   * because the whole app is shutting down; make sure AS can handle this
   */
  HN_DBG("removing view info from AS");

  gboolean removed_app = hd_wm_remove_applications (hdwm, view->info);

  g_signal_emit_by_name (hdwm,"entry_info_removed",removed_app,view->info);
  /* FIXME: We cant free the info before the callback returns  
  hd_entry_info_free (view->info);*/ /*FIXME: MEMORY LEAKKKK*/

  g_free (view);
}

void
hd_wm_watched_window_view_close_window (HDWMWatchedWindowView *view)
{
  g_return_if_fail(view);
  hd_wm_watched_window_close(hd_wm_watched_window_view_get_parent (view));
}

void
hd_wm_watched_window_view_set_info (HDWMWatchedWindowView *view,
				    HDEntryInfo           *info)
{
  g_return_if_fail (view);

  if (view->info)
    hd_entry_info_free (view->info);

  view->info = info;
}

HDEntryInfo *
hd_wm_watched_window_view_get_info (HDWMWatchedWindowView *view)
{
  g_return_val_if_fail (view != NULL, NULL);

  if (!view->info)
    view->info = hd_entry_info_new_from_view (view);

  return view->info;
}

gboolean
hd_wm_watched_window_view_is_active (HDWMWatchedWindowView *view)
{
  g_return_val_if_fail(view, FALSE);

  if(hd_wm_watched_window_is_active(view->win_parent) &&
     view == hd_wm_watched_window_get_active_view (view->win_parent))
    return TRUE;

  return FALSE;
}

