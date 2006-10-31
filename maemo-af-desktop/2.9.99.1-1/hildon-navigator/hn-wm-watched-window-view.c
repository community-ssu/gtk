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

#include "hn-wm-watched-window-view.h"
#include "hn-wm-watched-window.h"
#include "hn-wm-watchable-app.h"
#include "hn-entry-info.h"
#include "hn-app-switcher.h"

/* Watched Window views */

struct HNWMWatchedWindowView
{
  GtkWidget         *menu_widget;
  int                id;
  HNWMWatchedWindow *win_parent;
  char              *name;
  HNEntryInfo       *info;
};

HNWMWatchedWindowView*
hn_wm_watched_window_view_new (HNWMWatchedWindow *win,
			       int                view_id)
{
  HNWMWatchedWindowView *view;
  
  view = g_new0 (HNWMWatchedWindowView, 1);  

  if (!view) 	 /* FIXME: Handle OOM */
    return NULL;

  view->id         = view_id;
  view->win_parent = win; 

  return view;
}

HNWMWatchedWindow*
hn_wm_watched_window_view_get_parent (HNWMWatchedWindowView *view)
{
  return view->win_parent;
}

gint
hn_wm_watched_window_view_get_id (HNWMWatchedWindowView *view)
{
  return view->id;
}

GtkWidget*
hn_wm_watched_window_view_get_menu (HNWMWatchedWindowView *view)
{
  return view->menu_widget;
}

void
hn_wm_watched_window_view_set_name (HNWMWatchedWindowView *view,
				    const gchar           *name)
{
  if (name == NULL)
    return;

  if (view->name)
    g_free(view->name);

  view->name = g_strdup(name);
}

const gchar*
hn_wm_watched_window_view_get_name (HNWMWatchedWindowView *view)
{
  if (view->name != NULL)
    return view->name;

  if (view->win_parent && hn_wm_watched_window_get_name (view->win_parent))
    return hn_wm_watched_window_get_name (view->win_parent);

  return "unknown";
}

void
hn_wm_watched_window_view_destroy (HNWMWatchedWindowView *view)
{
  HNWMWatchableApp *app;

  app = hn_wm_watched_window_get_app(view->win_parent);

  /* Only remove the widget if the app is *really* killed */
  if (app && hn_wm_watchable_app_is_hibernating(app))
    {
      HN_DBG("### Aborting destroying view '%s' as hibernating ###",
             view->name);
      return;
    }


  if (hn_wm_watched_window_get_active_view(view->win_parent) == view)
    hn_wm_watched_window_set_active_view(view->win_parent, NULL);

  if (view->name)
    g_free(view->name);

  /*
   * NB -- this info might have been removed earlier if views are being removed
   * because the whole app is shutting down; make sure AS can handle this
   */
  HN_DBG("removing view info from AS");
  
  hn_app_switcher_remove (hn_wm_get_app_switcher (), view->info);
  hn_entry_info_free (view->info);

  g_free (view);
}

void
hn_wm_watched_window_view_close_window (HNWMWatchedWindowView *view)
{
  g_return_if_fail(view);
  hn_wm_watched_window_close(hn_wm_watched_window_view_get_parent (view));
}

void
hn_wm_watched_window_view_set_info (HNWMWatchedWindowView *view,
				    HNEntryInfo           *info)
{
  g_return_if_fail (view);

  if (view->info)
    hn_entry_info_free (view->info);

  view->info = info;
}

HNEntryInfo *
hn_wm_watched_window_view_get_info (HNWMWatchedWindowView *view)
{
  g_return_val_if_fail (view != NULL, NULL);

  if (!view->info)
    view->info = hn_entry_info_new_from_view (view);

  return view->info;
}

gboolean
hn_wm_watched_window_view_is_active (HNWMWatchedWindowView *view)
{
  g_return_val_if_fail(view, FALSE);

  if(hn_wm_watched_window_is_active(view->win_parent) &&
     view == hn_wm_watched_window_get_active_view (view->win_parent))
    return TRUE;

  return FALSE;
}

