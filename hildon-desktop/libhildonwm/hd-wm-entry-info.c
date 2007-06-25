/*
 * This file is part of libhildonwm
 *
 * Copyright (C) 2007 Nokia Corporation.
 *
 * Author:  Moises Martinez <moises.martinez@nokia.com>
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

#include "hd-wm-entry-info.h"
#include <gtk/gtkmarshal.h>
#include <gtk/gtkmain.h>            /* For _gtk_boolean_handled_accumulator */

#define _HD_WM_ENTRY_INFO_GET_IFACE(iface) \
  iface = HD_WM_ENTRY_INFO_GET_IFACE (info); \
  g_assert (iface != NULL);
	

static void hd_wm_entry_info_base_init (gpointer g_class);

GType 
hd_wm_entry_info_get_type (void)
{
  static GType entry_info_type = 0;

  if (!entry_info_type)
  {
    static const GTypeInfo entry_info_info =
    {
      sizeof (HDWMEntryInfoIface), /* class_size */
      hd_wm_entry_info_base_init,   /* base_init */
      NULL,		/* base_finalize */
      NULL,
      NULL,		/* class_finalize */
      NULL,		/* class_data */
      0,
      0,
      NULL
    };

    entry_info_type =
      g_type_register_static (G_TYPE_INTERFACE, "HDWMEntryInfo",
			      &entry_info_info, 0);

  }

  return entry_info_type;
}

static void
hd_wm_entry_info_base_init (gpointer g_class)
{
  static gboolean initialized = FALSE;
  /* TODO: Something to inirialize? */
  if (!initialized)
  {
    initialized = TRUE;
  }
}

gboolean 
hd_wm_entry_info_init (HDWMEntryInfo *info)
{
  HDWMEntryInfoIface *iface;

  g_return_val_if_fail (info != NULL, TRUE);

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_val_if_fail (iface->init != NULL, TRUE);

  return (* iface->init) (info);
}	

HDWMEntryInfo *
hd_wm_entry_info_get_parent (HDWMEntryInfo *info)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_val_if_fail (iface->get_parent != NULL, NULL);

  return (* iface->get_parent) (info);
}	

void 
hd_wm_entry_info_set_parent (HDWMEntryInfo *info, HDWMEntryInfo *parent)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_if_fail (iface->set_parent != NULL);

  (* iface->set_parent) (info, parent);
}	

void 
hd_wm_entry_info_add_child (HDWMEntryInfo *info, HDWMEntryInfo *child)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_if_fail (iface->add_child != NULL);

  (* iface->add_child) (info, child);
}

gboolean 
hd_wm_entry_info_remove_child (HDWMEntryInfo *info, HDWMEntryInfo *child)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_val_if_fail (iface->remove_child != NULL, FALSE);

  return (* iface->remove_child) (info, child);
}

const GList *
hd_wm_entry_info_get_children (HDWMEntryInfo *info)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_val_if_fail (iface->get_children != NULL, NULL);

  return (* iface->get_children) (info);
}

gint 
hd_wm_entry_info_get_n_children (HDWMEntryInfo *info)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_val_if_fail (iface->get_n_children != NULL,0);
  
  return (* iface->get_n_children) (info);
}	

const gchar *
hd_wm_entry_info_peek_app_name (HDWMEntryInfo *info)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_val_if_fail (iface->peek_app_name != NULL, NULL);

  return (* iface->peek_app_name) (info);
}

const gchar *
hd_wm_entry_info_peek_title (HDWMEntryInfo *info)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_val_if_fail (iface->peek_title != NULL, NULL);

  return (* iface->peek_title) (info);
}	

gchar * 
hd_wm_entry_info_get_title (HDWMEntryInfo *info)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_val_if_fail (iface->get_title != NULL, NULL);

  return (* iface->get_title) (info);
}

void 
hd_wm_entry_info_set_title (HDWMEntryInfo *info, const gchar *title)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_if_fail (iface->set_title != NULL);

  (* iface->set_title) (info, title);
}	

gchar * 
hd_wm_entry_info_get_app_name (HDWMEntryInfo *info)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_val_if_fail (iface->get_app_name != NULL, NULL);
  
  return (* iface->get_app_name) (info);
}

gchar * 
hd_wm_entry_info_get_window_name (HDWMEntryInfo *info)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_val_if_fail (iface->get_window_name != NULL, NULL);

  return (* iface->get_window_name) (info);
}	

GdkPixbuf *
hd_wm_entry_info_get_icon (HDWMEntryInfo *info)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_val_if_fail (iface->get_icon != NULL, NULL);

  return (* iface->get_icon) (info);
}

void 
hd_wm_entry_info_set_icon (HDWMEntryInfo *info, GdkPixbuf *icon)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_if_fail (iface->set_icon != NULL);

  (* iface->set_icon) (info, icon);
}

const gchar *
hd_wm_entry_info_get_app_icon_name (HDWMEntryInfo *info)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_val_if_fail (iface->get_app_icon_name != NULL, NULL);
  
  return (* iface->get_app_icon_name) (info);
}

GdkPixbuf *
hd_wm_entry_info_get_app_icon (HDWMEntryInfo *info,
			       gint           size,
			       GError     **error)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_val_if_fail (iface->get_app_icon != NULL, NULL);

  return (* iface->get_app_icon) (info, size, error);
}

void 
hd_wm_entry_info_close (HDWMEntryInfo *info)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_if_fail (iface->close != NULL);

  (* iface->close) (info);
}	

gboolean 
hd_wm_entry_info_is_urgent (HDWMEntryInfo *info)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_val_if_fail (iface->is_urgent != NULL, FALSE);

  return (* iface->is_urgent) (info);
}	

gboolean 
hd_wm_entry_info_get_ignore_urgent (HDWMEntryInfo *info)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface)
  g_return_val_if_fail (iface->get_ignore_urgent != NULL, FALSE);

  return (* iface->get_ignore_urgent) (info);
}	

void 
hd_wm_entry_info_set_ignore_urgent (HDWMEntryInfo *info,
                                    gboolean       ignore)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_if_fail (iface->set_ignore_urgent != NULL);

  (* iface->set_ignore_urgent) (info, ignore);
}

gboolean 
hd_wm_entry_info_is_active (HDWMEntryInfo *info)
{
  HDWMEntryInfoIface *iface;

  if (info == NULL)
    return FALSE;	  

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_val_if_fail (iface->is_active != NULL, FALSE);
  
  return (* iface->is_active) (info);
}	

gboolean 
hd_wm_entry_info_is_hibernating (HDWMEntryInfo *info)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_val_if_fail (iface->is_hibernating != NULL, FALSE);

  return (* iface->is_hibernating) (info);
}	

gboolean 
hd_wm_entry_info_has_extra_icon (HDWMEntryInfo *info)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_val_if_fail (iface->has_extra_icon != NULL, FALSE);

  return (* iface->has_extra_icon) (info);
}	

const gchar *
hd_wm_entry_info_get_extra_icon (HDWMEntryInfo *info)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_val_if_fail (iface->get_extra_icon != NULL, NULL);

  return (* iface->get_extra_icon) (info);
}	

Window
hd_wm_entry_info_get_x_window (HDWMEntryInfo *info)
{
  HDWMEntryInfoIface *iface;

  _HD_WM_ENTRY_INFO_GET_IFACE (iface);
  g_return_val_if_fail (iface->get_x_window != NULL, None);  

  return (* iface->get_x_window) (info);
}

