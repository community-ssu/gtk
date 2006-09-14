/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/* hn-entry-info.c
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
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

#include <string.h>
#include <stdlib.h>
#include <glib/gi18n.h>

#include "hn-entry-info.h"

#include "hn-wm-watchable-app.h"
#include "hn-wm-watched-window.h"
#include "hn-wm-watched-window-view.h"

#include "hn-wm-util.h"

typedef struct
{
  HNEntryInfoType type;

  /* parent, for child windows */  
  HNEntryInfo *parent;
  GList       *children;
  
  union {
    HNWMWatchableApp *app;
    HNWMWatchedWindow *window;
    HNWMWatchedWindowView *view;
  } d;
  
  guint ignore_urgent : 1;
} RealEntryInfo;

#define REAL_ENTRY_INFO(x) ((RealEntryInfo *) (x))

HNEntryInfo *
hn_entry_info_new (HNEntryInfoType type)
{
  RealEntryInfo *retval;
  
  g_return_val_if_fail (HN_ENTRY_INFO_IS_VALID_TYPE (type), NULL);
  
  retval = g_new (RealEntryInfo, 1);
  retval->type = type;
  retval->parent = NULL;
  retval->children = NULL;
  retval->ignore_urgent = FALSE;
  
  return (HNEntryInfo *) retval;
}

HNEntryInfo *
hn_entry_info_new_from_app (HNWMWatchableApp *app)
{
  HNEntryInfo *retval;

  g_return_val_if_fail (app != NULL, NULL);

  retval = hn_entry_info_new (HN_ENTRY_WATCHED_APP);
  hn_entry_info_set_app (retval, app);

  return retval;
}

HNEntryInfo *
hn_entry_info_new_from_window (HNWMWatchedWindow *window)
{
  HNEntryInfo *retval;
  
  g_return_val_if_fail (window != NULL, NULL);
  
  retval = hn_entry_info_new (HN_ENTRY_WATCHED_WINDOW);
  hn_entry_info_set_window (retval, window);
  
  return retval;
}

HNEntryInfo *
hn_entry_info_new_from_view (HNWMWatchedWindowView *view)
{
  HNEntryInfo *retval;
  
  g_return_val_if_fail (view != NULL, NULL);
  
  retval = hn_entry_info_new (HN_ENTRY_WATCHED_VIEW);
  hn_entry_info_set_view (retval, view);
  
  return retval;
}

void
hn_entry_info_free (HNEntryInfo *entry_info)
{
  RealEntryInfo *real;
  
  if (!entry_info)
    return;
  
  real = REAL_ENTRY_INFO (entry_info);
  switch (real->type)
    {
    case HN_ENTRY_WATCHED_APP:
     break;
    case HN_ENTRY_WATCHED_WINDOW:
      break;
    case HN_ENTRY_WATCHED_VIEW:
      break;
    case HN_ENTRY_INVALID:
    default:
      g_assert_not_reached ();
      return;
    }
  
  g_list_free (real->children);
  
  g_free (real);
}

void
hn_entry_info_set_app (HNEntryInfo      *entry_info,
		       HNWMWatchableApp *app)
{
  g_return_if_fail (entry_info != NULL);
  g_return_if_fail (entry_info->type == HN_ENTRY_WATCHED_APP);

  REAL_ENTRY_INFO (entry_info)->d.app = app;
}

HNWMWatchableApp *
hn_entry_info_get_app (HNEntryInfo *entry_info)
{
  RealEntryInfo         *real = NULL;
  HNWMWatchedWindow     *win = NULL;
  HNWMWatchedWindowView *view = NULL;
  
  g_return_val_if_fail (entry_info != NULL, 0);
  real = REAL_ENTRY_INFO (entry_info);

  switch (real->type)
    {
    case HN_ENTRY_WATCHED_APP:
      return REAL_ENTRY_INFO (entry_info)->d.app;
      break;
    case HN_ENTRY_WATCHED_WINDOW:
      win = hn_entry_info_get_window(entry_info);
      break;
    case HN_ENTRY_WATCHED_VIEW:
      view = hn_entry_info_get_view(entry_info);
      win = hn_wm_watched_window_view_get_parent(view);
      break;
    case HN_ENTRY_INVALID:
    default:
      g_assert_not_reached ();
      break;
    }

  return hn_wm_watched_window_get_app(win);
}

void
hn_entry_info_set_window (HNEntryInfo       *entry_info,
			  HNWMWatchedWindow *window)
{
  g_return_if_fail (entry_info != NULL);
  g_return_if_fail (entry_info->type == HN_ENTRY_WATCHED_WINDOW);
  
  REAL_ENTRY_INFO (entry_info)->d.window = window;
}

HNWMWatchedWindow *
hn_entry_info_get_window (HNEntryInfo *entry_info)
{
  HNWMWatchedWindowView *view;
  g_return_val_if_fail (entry_info != NULL, NULL);

  switch(entry_info->type)
    {
    case HN_ENTRY_WATCHED_WINDOW:
      return REAL_ENTRY_INFO (entry_info)->d.window;
    case HN_ENTRY_WATCHED_VIEW:
      view = hn_entry_info_get_view(entry_info);
      return hn_wm_watched_window_view_get_parent(view);
    default:
      return hn_wm_watchable_app_get_active_window(
                                           hn_entry_info_get_app(entry_info));
    }
}

void
hn_entry_info_set_view (HNEntryInfo           *entry_info,
			HNWMWatchedWindowView *view)
{
  g_return_if_fail (entry_info != NULL);
  g_return_if_fail (entry_info->type == HN_ENTRY_WATCHED_VIEW);
  
  REAL_ENTRY_INFO (entry_info)->d.view = view;
}

HNWMWatchedWindowView *
hn_entry_info_get_view (HNEntryInfo *entry_info)
{
  g_return_val_if_fail (entry_info != NULL, NULL);
  g_return_val_if_fail (entry_info->type == HN_ENTRY_WATCHED_VIEW, NULL);

  return REAL_ENTRY_INFO (entry_info)->d.view;
}

HNEntryInfo *
hn_entry_info_get_parent (HNEntryInfo *entry_info)
{
  RealEntryInfo *real;
  
  g_return_val_if_fail (entry_info != NULL, NULL);
  real = REAL_ENTRY_INFO (entry_info);
  
  return real->parent;
}

gint
hn_entry_info_get_n_children (HNEntryInfo *entry_info)
{
  RealEntryInfo *real;

  g_return_val_if_fail (entry_info != NULL, 0);
  real = REAL_ENTRY_INFO (entry_info);

  if(!real->children)
    return 0;

  return g_list_length(real->children);
}

const gchar *
hn_entry_info_peek_title (HNEntryInfo *entry_info)
{
  RealEntryInfo *real;
  const gchar *retval;
  
  g_return_val_if_fail (entry_info != NULL, NULL);

  real = REAL_ENTRY_INFO (entry_info);
  switch (real->type)
    {
    case HN_ENTRY_WATCHED_APP:
      {
        HNWMWatchedWindow *win;

	win = hn_wm_watchable_app_get_active_window (real->d.app);
	if (win)
          retval = hn_wm_watched_window_get_name (win);
	else
          retval = hn_wm_watchable_app_get_name (real->d.app);
      }
      break;
    case HN_ENTRY_WATCHED_WINDOW:
      retval = hn_wm_watched_window_get_name (real->d.window);
      break;
    case HN_ENTRY_WATCHED_VIEW:
      retval = hn_wm_watched_window_view_get_name (real->d.view);
      break;
    case HN_ENTRY_DESKTOP:
      retval = _("tana_fi_home");
      break;
    case HN_ENTRY_INVALID:
    default:
      g_assert_not_reached ();
      retval = NULL;
      break;
    }
 
  return retval;
}

gchar *
hn_entry_info_get_title (HNEntryInfo *info)
{
  return g_strdup (hn_entry_info_peek_title (info));
}

gchar *
hn_entry_info_get_app_name (HNEntryInfo *info)
{
  gchar *title, *sep;
  
  g_return_val_if_fail (info != NULL, NULL);

  if (info->type == HN_ENTRY_DESKTOP)
    return g_strdup (_("tana_fi_home"));
  
  title = hn_entry_info_get_title (info);
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
  else
    return title;
}

gchar *
hn_entry_info_get_window_name (HNEntryInfo *info)
{
  gchar *title, *sep;

  g_return_val_if_fail (info != NULL, NULL);

  if (info->type == HN_ENTRY_DESKTOP)
    return g_strdup (_("tana_fi_home_thumb"));
  
  title = hn_entry_info_get_title (info);
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

void
hn_entry_info_set_title (HNEntryInfo *entry_info,
			 const gchar *title)
{
  RealEntryInfo *real;
  
  g_return_if_fail (entry_info != NULL);
  
  real = REAL_ENTRY_INFO (entry_info);
  
  switch (real->type)
    {
    case HN_ENTRY_WATCHED_APP:
      /* TODO */
      break;
    case HN_ENTRY_WATCHED_WINDOW:
      hn_wm_watched_window_set_name (real->d.window, title);
      break;
    case HN_ENTRY_WATCHED_VIEW:
      hn_wm_watched_window_view_set_name (real->d.view, title);
      break;
    case HN_ENTRY_INVALID:
    default:
      g_assert_not_reached ();
      break;
    }
}

GdkPixbuf *
hn_entry_info_get_icon (HNEntryInfo *entry_info)
{
  RealEntryInfo *real;
  GdkPixbuf *retval = NULL;
  
  g_return_val_if_fail (entry_info != NULL, NULL);
  
  real = REAL_ENTRY_INFO (entry_info);
  
  switch (real->type)
    {
    case HN_ENTRY_WATCHED_APP:
      /* TODO */
      break;
    case HN_ENTRY_WATCHED_WINDOW:
      retval = hn_wm_watched_window_get_custom_icon (real->d.window);
      break;
    case HN_ENTRY_WATCHED_VIEW:
      retval = hn_entry_info_get_icon (hn_entry_info_get_parent (entry_info));
      break;
    case HN_ENTRY_DESKTOP:
      retval = NULL;
      break;
    case HN_ENTRY_INVALID:
    default:
      g_assert_not_reached ();
      break;
    }
  
  return retval;
}

void
hn_entry_info_set_icon (HNEntryInfo *entry_info,
			GdkPixbuf   *icon)
{
  RealEntryInfo *real;
  
  g_return_if_fail (entry_info != NULL);
  
  real = REAL_ENTRY_INFO (entry_info);
  
  switch (real->type)
    {
    case HN_ENTRY_WATCHED_APP:
      /* TODO */
      break;
    case HN_ENTRY_WATCHED_WINDOW:
      break;
    case HN_ENTRY_WATCHED_VIEW:
      break;
    case HN_ENTRY_INVALID:
    default:
      g_assert_not_reached ();
      break;
    }
}

void
hn_entry_info_close (HNEntryInfo *info)
{
  RealEntryInfo *real;

  g_return_if_fail (info != NULL);
  
  real = REAL_ENTRY_INFO (info);
  switch (real->type)
    {
    case HN_ENTRY_WATCHED_APP:
      /* TODO */
      break;
    case HN_ENTRY_WATCHED_WINDOW:
      hn_wm_watched_window_close (real->d.window);
      break;
    case HN_ENTRY_WATCHED_VIEW:
      hn_wm_watched_window_view_close_window (real->d.view);
      break;
    case HN_ENTRY_INVALID:
    default:
      g_assert_not_reached ();
      break;
    }
}

const gchar *
hn_entry_info_get_app_icon_name (HNEntryInfo *info)
{
  RealEntryInfo     *real;
  HNWMWatchedWindow *win = NULL;
  HNWMWatchableApp  *app = NULL;
  
  g_return_val_if_fail (info != NULL, NULL);
  
  real = REAL_ENTRY_INFO (info);
  switch (real->type)
    {
    case HN_ENTRY_WATCHED_APP:
      app = hn_entry_info_get_app (info);
      break;
    case HN_ENTRY_WATCHED_WINDOW:
      win = real->d.window;
      break;
    case HN_ENTRY_WATCHED_VIEW:
      win = hn_wm_watched_window_view_get_parent (real->d.view);
      break;
    case HN_ENTRY_DESKTOP:
      return "qgn_list_home";
    case HN_ENTRY_INVALID:
    default:
      g_assert_not_reached ();
      break;
    }

  if (!app)
    {
      if (!win)
        return NULL;

      app = hn_wm_watched_window_get_app (win);

      if (!app)
	return NULL;
    }
  
  return hn_wm_watchable_app_get_icon_name (app);
}

void
hn_entry_info_add_child (HNEntryInfo *info,
		         HNEntryInfo *child)
{
  RealEntryInfo     *real;
  RealEntryInfo     *real_child;
  
  g_return_if_fail (info && child);
  
  real       = REAL_ENTRY_INFO (info);
  real_child = REAL_ENTRY_INFO (child);
  
  real->children = g_list_prepend (real->children, child);

  real_child->parent = info;
}

/* returns true if this parent has children left */
gboolean
hn_entry_info_remove_child (HNEntryInfo *info,
	                    HNEntryInfo *child)
{
  RealEntryInfo     *real;
  
  g_return_val_if_fail (info && child, FALSE);
  
  real = REAL_ENTRY_INFO (info);
  real->children = g_list_remove (real->children, child);
  
  return (real->children != NULL);
}

const GList*
hn_entry_info_get_children (HNEntryInfo *info)
{
  RealEntryInfo     *real;
  
  g_return_val_if_fail (info, NULL);
  
  real = REAL_ENTRY_INFO (info);
  return real->children;
}

gboolean
hn_entry_info_is_urgent (HNEntryInfo *info)
{
  HNWMWatchedWindow * win;
  
  g_return_val_if_fail(info, FALSE);
  
  if (info->type == HN_ENTRY_DESKTOP)
    return FALSE;
  
  win = hn_entry_info_get_window (info);

  if (win)
    return hn_wm_watched_window_is_urgent(win);

  return FALSE;
}

gboolean
hn_entry_info_get_ignore_urgent (HNEntryInfo *info)
{
  RealEntryInfo     *real;
  g_return_val_if_fail (info, FALSE);
  
  real = REAL_ENTRY_INFO (info);
  return real->ignore_urgent;
}

void
hn_entry_info_set_ignore_urgent (HNEntryInfo *info, gboolean ignore)
{
  RealEntryInfo     *real;
  g_return_if_fail (info);
  
  real = REAL_ENTRY_INFO (info);
  real->ignore_urgent = ignore;
}

gboolean
hn_entry_info_is_active (HNEntryInfo *info)
{
  RealEntryInfo     *real;
  
  g_return_val_if_fail (info, FALSE);
  
  real = REAL_ENTRY_INFO (info);
  switch (info->type)
    {
      case HN_ENTRY_WATCHED_APP:
        {
        GList *l;

	for (l = real->children; l != NULL; l = l->next)
          if (hn_entry_info_is_active (l->data))
            return TRUE;
        }
        return FALSE;
      case HN_ENTRY_WATCHED_WINDOW:
        return hn_wm_watched_window_is_active(real->d.window);
      case HN_ENTRY_WATCHED_VIEW:
        return hn_wm_watched_window_view_is_active(real->d.view);
      case HN_ENTRY_DESKTOP:
        return TRUE;
      case HN_ENTRY_INVALID:
      default:
        g_critical("Invalid Entry type");
    }

  return FALSE;
}

gboolean
hn_entry_info_is_hibernating (HNEntryInfo *info)
{
  g_return_val_if_fail (info != NULL, FALSE);

  if (info->type == HN_ENTRY_DESKTOP)
    return FALSE;
  
  return hn_wm_watchable_app_is_hibernating (hn_entry_info_get_app (info));
}

const gchar *
hn_entry_info_get_extra_icon (HNEntryInfo *info)
{
  g_return_val_if_fail (info, NULL);
  
  switch (info->type)
    {
      case HN_ENTRY_WATCHED_APP:
        return hn_wm_watchable_app_get_extra_icon(hn_entry_info_get_app(info));
        
      case HN_ENTRY_WATCHED_WINDOW:
      case HN_ENTRY_WATCHED_VIEW:
      case HN_ENTRY_INVALID:
      default:
        g_critical("Invalid Entry type");
    }

  return NULL;
}

gboolean
hn_entry_info_has_extra_icon (HNEntryInfo *info)
{
  g_return_val_if_fail (info != NULL, FALSE);

  return (NULL != hn_entry_info_get_extra_icon (info));
}
