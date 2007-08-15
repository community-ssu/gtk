/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/* 
 * This file is part of libhildonwm
 *
 * Copyright (C) 2005, 2006, 2007 Nokia Corporation.
 *
 * Author: Moises Martinez <moises.martinez@nokia.com>
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
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "hd-wm-application.h"
#include "hd-wm-entry-info.h"

#define HD_WM_DESKTOP_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), HD_WM_TYPE_DESKTOP, HDWMDesktopPrivate))

static void hd_wm_desktop_entry_info_init (HDWMEntryInfoIface *iface);

G_DEFINE_TYPE_WITH_CODE (HDWMDesktop,hd_wm_desktop,G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (HD_WM_TYPE_ENTRY_INFO,
                                                hd_wm_desktop_entry_info_init));



#define HDWM_WIN_SET_FLAG(a,f)    ((a)->priv->flags |= (f))
#define HDWM_WIN_UNSET_FLAG(a,f)  ((a)->priv->flags &= ~(f))
#define HDWM_WIN_IS_SET_FLAG(a,f) ((a)->priv->flags & (f))

struct _HDWMDesktopPrivate
{
  Window                  xwin;

  GdkPixbuf		 *icon;
};

static const gchar *
hd_wm_desk_info_peek_app_name (HDWMEntryInfo *info)
{
  return _("tana_fi_home");
}

static const gchar *
hd_wm_desk_info_peek_title (HDWMEntryInfo *info)
{
  return _("tana_fi_home");
}

static gchar *
hd_wm_desk_info_get_title (HDWMEntryInfo *info)
{
  return g_strdup (_("tana_fi_home"));
}

static gchar * 
hd_wm_desk_info_get_app_name (HDWMEntryInfo *info)
{
  return g_strdup (_("tana_fi_home"));
}

static gchar * 
hd_wm_desk_info_get_window_name (HDWMEntryInfo *info)
{
  return g_strdup (_("tana_fi_home_thumb"));
}  

static const gchar *
hd_wm_desk_info_get_app_icon_name (HDWMEntryInfo *info)
{
  return "qgn_list_home";	
}

static GdkPixbuf *
hd_wm_desk_info_get_icon (HDWMEntryInfo *info)
{
  return NULL;	
}

static gboolean 
hd_wm_desk_info_is_hibernating (HDWMEntryInfo *info)
{
  return FALSE;	
}

static gboolean 
hd_wm_desk_info_is_urgent (HDWMEntryInfo *info)
{
  return FALSE;	
}

static GdkPixbuf *
hd_wm_desk_info_get_app_icon (HDWMEntryInfo *info,
                               gint          size,
                               GError     **error)
{
  GdkPixbuf *retval;	
  GError *load_error = NULL;
  HDWMDesktop *desktop = (HDWMDesktop *) info;

  if (desktop->priv->icon != NULL)
    return desktop->priv->icon;	  

  retval = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                     "qgn_list_home",
                                     size,
                                     GTK_ICON_LOOKUP_NO_SVG,
                                     &load_error);
  if (load_error)
  {
    g_propagate_error (error, load_error);
    return NULL;
  }

  desktop->priv->icon = NULL;
  
  return retval;
}

static gboolean
hd_wm_desk_info_is_active (HDWMEntryInfo *info)
{
  return TRUE;
}

static Window
hd_wm_desk_info_get_x_window (HDWMEntryInfo *info)
{
  HDWMDesktop *desktop = (HDWMDesktop *) info;

  return desktop->priv->xwin;
}

static void 
hd_wm_desktop_entry_info_init (HDWMEntryInfoIface *iface)
{
  iface->init		   = NULL;
  iface->get_parent        = NULL;
  iface->set_parent        = NULL;
  iface->add_child         = NULL;
  iface->remove_child      = NULL;
  iface->get_children      = NULL;
  iface->get_n_children    = NULL;
  iface->peek_app_name     = hd_wm_desk_info_peek_app_name;
  iface->peek_title        = hd_wm_desk_info_peek_title;
  iface->get_title         = hd_wm_desk_info_get_title;
  iface->set_title         = NULL;
  iface->get_app_name      = hd_wm_desk_info_get_app_name;
  iface->get_window_name   = hd_wm_desk_info_get_window_name;
  iface->get_icon          = hd_wm_desk_info_get_icon;
  iface->set_icon          = NULL;
  iface->get_app_icon_name = hd_wm_desk_info_get_app_icon_name;
  iface->get_app_icon      = hd_wm_desk_info_get_app_icon;
  iface->close             = NULL;
  iface->is_urgent         = hd_wm_desk_info_is_urgent;
  iface->get_ignore_urgent = NULL;
  iface->set_ignore_urgent = NULL;
  iface->is_active         = hd_wm_desk_info_is_active;
  iface->is_hibernating    = hd_wm_desk_info_is_hibernating;
  iface->has_extra_icon    = NULL;
  iface->get_extra_icon    = NULL;
  iface->get_x_window      = hd_wm_desk_info_get_x_window;
}

static void
hd_wm_desktop_finalize (GObject *object)
{
  HDWMDesktop *desktop = HD_WM_DESKTOP (object);	

  g_object_unref (G_OBJECT (desktop->priv->icon));
}

static void 
hd_wm_desktop_init (HDWMDesktop *desktop)
{
  desktop->priv = HD_WM_DESKTOP_GET_PRIVATE (desktop);	

  desktop->priv->xwin = None;
  
  desktop->priv->icon = NULL;
}

static void 
hd_wm_desktop_class_init (HDWMDesktopClass *desktop_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (desktop_class);

  object_class->finalize = hd_wm_desktop_finalize;
	
  g_type_class_add_private (desktop_class, sizeof (HDWMDesktopPrivate));
}

HDWMDesktop *
hd_wm_desktop_new (void)
{
  return 
    HD_WM_DESKTOP (g_object_new (HD_WM_TYPE_DESKTOP, NULL));
}  

void 
hd_wm_desktop_set_x_window (HDWMDesktop *desktop, Window win)
{
  g_assert (HD_WM_IS_DESKTOP (desktop));

  desktop->priv->xwin = win;
}

