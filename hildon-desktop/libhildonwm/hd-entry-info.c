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

#include "hd-entry-info.h"

#include "hd-wm-watchable-app.h"
#include "hd-wm-watched-window.h"
#include "hd-wm-watched-window-view.h"

#include "hd-wm-util.h"

typedef struct
{
  HDEntryInfoType type;

  /* parent, for child windows */  
  HDEntryInfo *parent;
  GList       *children;
  
  union {
    HDWMWatchableApp *app;
    HDWMWatchedWindow *window;
    HDWMWatchedWindowView *view;
  } d;

  GHashTable *icon_cache;
  
  guint ignore_urgent : 1;
} RealEntryInfo;

#define REAL_ENTRY_INFO(x) ((RealEntryInfo *) (x))

HDEntryInfo *
hd_entry_info_new (HDEntryInfoType type)
{
  RealEntryInfo *retval;
  
  g_return_val_if_fail (HD_ENTRY_INFO_IS_VALID_TYPE (type), NULL);
  
  retval = g_new (RealEntryInfo, 1);
  retval->type = type;
  retval->parent = NULL;
  retval->children = NULL;
  retval->ignore_urgent = FALSE;
  retval->icon_cache = g_hash_table_new_full (g_direct_hash,
                                              g_direct_equal,
                                              NULL,
                                              (GDestroyNotify) g_object_unref);
  
  return (HDEntryInfo *) retval;
}

HDEntryInfo *
hd_entry_info_new_from_app (HDWMWatchableApp *app)
{
  HDEntryInfo *retval;

  g_return_val_if_fail (app != NULL, NULL);

  retval = hd_entry_info_new (HD_ENTRY_WATCHED_APP);
  hd_entry_info_set_app (retval, app);

  return retval;
}

HDEntryInfo *
hd_entry_info_new_from_window (HDWMWatchedWindow *window)
{
  HDEntryInfo *retval;
  
  g_return_val_if_fail (window != NULL, NULL);
  
  retval = hd_entry_info_new (HD_ENTRY_WATCHED_WINDOW);
  hd_entry_info_set_window (retval, window);
  
  return retval;
}

HDEntryInfo *
hd_entry_info_new_from_view (HDWMWatchedWindowView *view)
{
  HDEntryInfo *retval;
  
  g_return_val_if_fail (view != NULL, NULL);
  
  retval = hd_entry_info_new (HD_ENTRY_WATCHED_VIEW);
  hd_entry_info_set_view (retval, view);
  
  return retval;
}

void
hd_entry_info_free (HDEntryInfo *entry_info)
{
  RealEntryInfo *real;
  
  if (!entry_info)
    return;
  
  real = REAL_ENTRY_INFO (entry_info);
  switch (real->type)
  {
    case HD_ENTRY_WATCHED_APP:
      break;
    case HD_ENTRY_WATCHED_WINDOW:
      break;
    case HD_ENTRY_WATCHED_VIEW:
      break;
    case HD_ENTRY_INVALID:
    default:
      g_assert_not_reached ();
      return;
  }
  
  g_list_free (real->children);

  g_hash_table_destroy (real->icon_cache);
  
  g_free (real);
}

void
hd_entry_info_set_app (HDEntryInfo      *entry_info,
		       HDWMWatchableApp *app)
{
  g_return_if_fail (entry_info != NULL);
  g_return_if_fail (entry_info->type == HD_ENTRY_WATCHED_APP);

  REAL_ENTRY_INFO (entry_info)->d.app = app;
}

HDWMWatchableApp *
hd_entry_info_get_app (HDEntryInfo *entry_info)
{
  RealEntryInfo         *real = NULL;
  HDWMWatchedWindow     *win = NULL;
  HDWMWatchedWindowView *view = NULL;
  
  g_return_val_if_fail (entry_info != NULL, 0);
  real = REAL_ENTRY_INFO (entry_info);

  switch (real->type)
  {
    case HD_ENTRY_WATCHED_APP:
      return REAL_ENTRY_INFO (entry_info)->d.app;
      break;
    case HD_ENTRY_WATCHED_WINDOW:
      win = hd_entry_info_get_window(entry_info);
      break;
    case HD_ENTRY_WATCHED_VIEW:
      view = hd_entry_info_get_view(entry_info);
      win = hd_wm_watched_window_view_get_parent(view);
      break;
    case HD_ENTRY_INVALID:
    default:
      g_assert_not_reached ();
      break;
  }

  return hd_wm_watched_window_get_app(win);
}

void
hd_entry_info_set_window (HDEntryInfo       *entry_info,
			  HDWMWatchedWindow *window)
{
  g_return_if_fail (entry_info != NULL);
  g_return_if_fail (entry_info->type == HD_ENTRY_WATCHED_WINDOW);
  
  REAL_ENTRY_INFO (entry_info)->d.window = window;
}

HDWMWatchedWindow *
hd_entry_info_get_window (HDEntryInfo *entry_info)
{
  HDWMWatchedWindowView *view;
  g_return_val_if_fail (entry_info != NULL, NULL);

  switch (entry_info->type)
  {
    case HD_ENTRY_WATCHED_WINDOW:
      return REAL_ENTRY_INFO (entry_info)->d.window;
    case HD_ENTRY_WATCHED_VIEW:
      view = hd_entry_info_get_view (entry_info);
      return hd_wm_watched_window_view_get_parent (view);
    default:
      return hd_wm_watchable_app_get_active_window (hd_entry_info_get_app (entry_info));
    }
}

void
hd_entry_info_set_view (HDEntryInfo           *entry_info,
			HDWMWatchedWindowView *view)
{
  g_return_if_fail (entry_info != NULL);
  g_return_if_fail (entry_info->type == HD_ENTRY_WATCHED_VIEW);
  
  REAL_ENTRY_INFO (entry_info)->d.view = view;
}

HDWMWatchedWindowView *
hd_entry_info_get_view (HDEntryInfo *entry_info)
{
  g_return_val_if_fail (entry_info != NULL, NULL);
  g_return_val_if_fail (entry_info->type == HD_ENTRY_WATCHED_VIEW, NULL);

  return REAL_ENTRY_INFO (entry_info)->d.view;
}

HDEntryInfo *
hd_entry_info_get_parent (HDEntryInfo *entry_info)
{
  RealEntryInfo *real;
  
  g_return_val_if_fail (entry_info != NULL, NULL);
  real = REAL_ENTRY_INFO (entry_info);
  
  return real->parent;
}

gint
hd_entry_info_get_n_children (HDEntryInfo *entry_info)
{
  RealEntryInfo *real;

  g_return_val_if_fail (entry_info != NULL, 0);
  real = REAL_ENTRY_INFO (entry_info);

  if(!real->children)
    return 0;

  return g_list_length(real->children);
}

const gchar *
hd_entry_info_peek_app_name (HDEntryInfo *entry_info)
{
  RealEntryInfo *real;
  const gchar *retval;
  
  g_return_val_if_fail (entry_info != NULL, NULL);

  real = REAL_ENTRY_INFO (entry_info);
  switch (real->type)
  {
    case HD_ENTRY_WATCHED_APP:
      retval = hd_wm_watchable_app_get_name (real->d.app);
      break;
    case HD_ENTRY_WATCHED_WINDOW:
      retval = hd_wm_watched_window_get_name (real->d.window);
      break;
    case HD_ENTRY_WATCHED_VIEW:
      retval = hd_wm_watched_window_view_get_name (real->d.view);
      break;
    case HD_ENTRY_DESKTOP:
      retval = _("tana_fi_home");
      break;
    case HD_ENTRY_INVALID:
    default:
      g_assert_not_reached ();
      retval = NULL;
      break;
  }
 
  return retval;
}
const gchar *
hd_entry_info_peek_title (HDEntryInfo *entry_info)
{
  RealEntryInfo *real;
  const gchar *retval;
  
  g_return_val_if_fail (entry_info != NULL, NULL);

  real = REAL_ENTRY_INFO (entry_info);
  switch (real->type)
  {
    case HD_ENTRY_WATCHED_APP:
      {
      HDWMWatchedWindow *win;

      win = hd_wm_watchable_app_get_active_window (real->d.app);
	
      if (win)
        retval = hd_wm_watched_window_get_name (win);
      else
        retval = hd_wm_watchable_app_get_name (real->d.app);
      }
      break;
    
    case HD_ENTRY_WATCHED_WINDOW:
      retval = hd_wm_watched_window_get_name (real->d.window);
      break;
      
    case HD_ENTRY_WATCHED_VIEW:
      retval = hd_wm_watched_window_view_get_name (real->d.view);
      break;
      
    case HD_ENTRY_DESKTOP:
      retval = _("tana_fi_home");
      break;
      
    case HD_ENTRY_INVALID:
    default:
      g_assert_not_reached ();
      retval = NULL;
      break;
  }
 
  return retval;
}

gchar *
hd_entry_info_get_title (HDEntryInfo *info)
{
  return g_strdup (hd_entry_info_peek_title (info));
}

gchar *
hd_entry_info_get_app_name (HDEntryInfo *info)
{
  gchar *title, *sep;
  
  g_return_val_if_fail (info != NULL, NULL);

  if (info->type == HD_ENTRY_DESKTOP)
    return g_strdup (_("tana_fi_home"));
  
  title = hd_entry_info_get_title (info);
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
hd_entry_info_get_window_name (HDEntryInfo *info)
{
  gchar *title, *sep;

  g_return_val_if_fail (info != NULL, NULL);

  if (info->type == HD_ENTRY_DESKTOP)
    return g_strdup (_("tana_fi_home_thumb"));
  
  title = hd_entry_info_get_title (info);
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
hd_entry_info_set_title (HDEntryInfo *entry_info,
			 const gchar *title)
{
  RealEntryInfo *real;
  
  g_return_if_fail (entry_info != NULL);
  
  real = REAL_ENTRY_INFO (entry_info);
  
  switch (real->type)
  {
    case HD_ENTRY_WATCHED_APP:
      /* TODO */
      break;
      
    case HD_ENTRY_WATCHED_WINDOW:
      hd_wm_watched_window_set_name (real->d.window, title);
      break;
      
    case HD_ENTRY_WATCHED_VIEW:
      hd_wm_watched_window_view_set_name (real->d.view, title);
      break;
      
    case HD_ENTRY_INVALID:
    default:
      g_assert_not_reached ();
      break;
  }
}

GdkPixbuf *
hd_entry_info_get_icon (HDEntryInfo *entry_info)
{
  RealEntryInfo *real;
  GdkPixbuf *retval = NULL;
  
  g_return_val_if_fail (entry_info != NULL, NULL);
  
  real = REAL_ENTRY_INFO (entry_info);
  
  switch (real->type)
  {
    case HD_ENTRY_WATCHED_APP:
      /* TODO */
      break;
    case HD_ENTRY_WATCHED_WINDOW:
      retval = hd_wm_watched_window_get_custom_icon (real->d.window);
      break;
    case HD_ENTRY_WATCHED_VIEW:
      retval = hd_entry_info_get_icon (hd_entry_info_get_parent (entry_info));
      break;
    case HD_ENTRY_DESKTOP:
      retval = NULL;
      break;
    case HD_ENTRY_INVALID:
    default:
      g_assert_not_reached ();
      break;
  }
  
  return retval;
}

void
hd_entry_info_set_icon (HDEntryInfo *entry_info,
			GdkPixbuf   *icon)
{
  RealEntryInfo *real;
  
  g_return_if_fail (entry_info != NULL);
  
  real = REAL_ENTRY_INFO (entry_info);
  
  switch (real->type)
  {
    case HD_ENTRY_WATCHED_APP:
      /* TODO */
      break;
    case HD_ENTRY_WATCHED_WINDOW:
      break;
    case HD_ENTRY_WATCHED_VIEW:
      break;
    case HD_ENTRY_INVALID:
    default:
      g_assert_not_reached ();
      break;
  }
}

void
hd_entry_info_close (HDEntryInfo *info)
{
  RealEntryInfo *real;

  g_return_if_fail (info != NULL);
  
  real = REAL_ENTRY_INFO (info);
  switch (real->type)
  {
    case HD_ENTRY_WATCHED_APP:
      /* TODO */
      break;
      
    case HD_ENTRY_WATCHED_WINDOW:
      hd_wm_watched_window_close (real->d.window);
      break;
      
    case HD_ENTRY_WATCHED_VIEW:
      hd_wm_watched_window_view_close_window (real->d.view);
      break;
      
    case HD_ENTRY_INVALID:
    default:
      g_assert_not_reached ();
      break;
  }
}

const gchar *
hd_entry_info_get_app_icon_name (HDEntryInfo *info)
{
  RealEntryInfo     *real;
  HDWMWatchedWindow *win = NULL;
  HDWMWatchableApp  *app = NULL;
  
  g_return_val_if_fail (info != NULL, NULL);
  
  real = REAL_ENTRY_INFO (info);
  switch (real->type)
  {
    case HD_ENTRY_WATCHED_APP:
      app = hd_entry_info_get_app (info);
      break;
    case HD_ENTRY_WATCHED_WINDOW:
      win = real->d.window;
      break;
    case HD_ENTRY_WATCHED_VIEW:
      win = hd_wm_watched_window_view_get_parent (real->d.view);
      break;
    case HD_ENTRY_DESKTOP:
      return "qgn_list_home";
    case HD_ENTRY_INVALID:
    default:
      g_assert_not_reached ();
      break;
  }

  if (!app)
  {
    if (!win)
      return NULL;

    app = hd_wm_watched_window_get_app (win);

    if (!app)
      return NULL;
  }
  
  return hd_wm_watchable_app_get_icon_name (app);
}

GdkPixbuf *
hd_entry_info_get_app_icon (HDEntryInfo  *info,
                            gint          size,
                            GError      **error)
{
  const gchar *icon_name;
  GdkPixbuf *retval;
  GError *load_error;
  RealEntryInfo *real_info;

  real_info = REAL_ENTRY_INFO (info);
 
  retval = g_hash_table_lookup (real_info->icon_cache,
                                GINT_TO_POINTER (size));
  if (retval)
    return g_object_ref (retval);

  icon_name = hd_entry_info_get_app_icon_name (info);
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
  
  g_debug (G_STRLOC ": adding cache entry (size:%d)", size);
  g_hash_table_insert (real_info->icon_cache,
                       GINT_TO_POINTER (size),
                       g_object_ref (retval));
  
  return retval;  
}

void
hd_entry_info_add_child (HDEntryInfo *info,
		         HDEntryInfo *child)
{
  RealEntryInfo     *real;
  RealEntryInfo     *real_child;
  GList		    *iter;
  
  g_return_if_fail (info && child);
  
  real       = REAL_ENTRY_INFO (info);
  real_child = REAL_ENTRY_INFO (child);

  for (iter=real->children;iter;iter = iter->next)
  {
    if (child == (HDEntryInfo *)iter->data)
      return;
  }
  
  real->children = g_list_prepend (real->children, child);

  real_child->parent = info;
}

/* returns true if this parent has children left */
gboolean
hd_entry_info_remove_child (HDEntryInfo *info,
	                    HDEntryInfo *child)
{
  RealEntryInfo     *real;
  
  g_return_val_if_fail (info && child, FALSE);
  
  real = REAL_ENTRY_INFO (info);
  real->children = g_list_remove (real->children, child);
  
  return (real->children != NULL);
}

const GList*
hd_entry_info_get_children (HDEntryInfo *info)
{
  RealEntryInfo     *real;
  
  g_return_val_if_fail (info, NULL);
  
  real = REAL_ENTRY_INFO (info);
  return real->children;
}

gboolean
hd_entry_info_is_urgent (HDEntryInfo *info)
{
  HDWMWatchedWindow * win;
  
  g_return_val_if_fail(info, FALSE);
  
  if (info->type == HD_ENTRY_DESKTOP)
    return FALSE;
  
  win = hd_entry_info_get_window (info);

  if (win)
    return hd_wm_watched_window_is_urgent(win);

  return FALSE;
}

gboolean
hd_entry_info_get_ignore_urgent (HDEntryInfo *info)
{
  RealEntryInfo     *real;
  g_return_val_if_fail (info, FALSE);
  
  real = REAL_ENTRY_INFO (info);
  return real->ignore_urgent;
}

void
hd_entry_info_set_ignore_urgent (HDEntryInfo *info, gboolean ignore)
{
  RealEntryInfo     *real;
  g_return_if_fail (info);
  
  real = REAL_ENTRY_INFO (info);
  real->ignore_urgent = ignore;
}

gboolean
hd_entry_info_is_active (HDEntryInfo *info)
{
  RealEntryInfo     *real;
  
  g_return_val_if_fail (info, FALSE);
  
  real = REAL_ENTRY_INFO (info);
  switch (info->type)
  {
    case HD_ENTRY_WATCHED_APP:
      {
      GList *l;

      for (l = real->children; l != NULL; l = l->next)
         if (hd_entry_info_is_active (l->data))
           return TRUE;
      }
      return FALSE;
      
    case HD_ENTRY_WATCHED_WINDOW:
      return hd_wm_watched_window_is_active(real->d.window);
      
    case HD_ENTRY_WATCHED_VIEW:
      return hd_wm_watched_window_view_is_active(real->d.view);
	
    case HD_ENTRY_DESKTOP:
      return TRUE;
     
    case HD_ENTRY_INVALID:
    default:
      g_critical("Invalid Entry type");
  }

  return FALSE;
}

gboolean
hd_entry_info_is_hibernating (HDEntryInfo *info)
{
  g_return_val_if_fail (info != NULL, FALSE);

  if (info->type == HD_ENTRY_DESKTOP)
    return FALSE;
  
  return hd_wm_watchable_app_is_hibernating (hd_entry_info_get_app (info));
}

const gchar *
hd_entry_info_get_extra_icon (HDEntryInfo *info)
{
  g_return_val_if_fail (info, NULL);
  
  switch (info->type)
  {
    case HD_ENTRY_WATCHED_APP:
      return hd_wm_watchable_app_get_extra_icon(hd_entry_info_get_app(info));
        
    case HD_ENTRY_WATCHED_WINDOW:
    case HD_ENTRY_WATCHED_VIEW:
    case HD_ENTRY_INVALID:
    default:
      g_critical("Invalid Entry type");
  }

  return NULL;
}

gboolean
hd_entry_info_has_extra_icon (HDEntryInfo *info)
{
  g_return_val_if_fail (info != NULL, FALSE);

  return (NULL != hd_entry_info_get_extra_icon (info));
}

