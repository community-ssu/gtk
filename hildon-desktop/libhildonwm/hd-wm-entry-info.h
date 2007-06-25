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

#ifndef __HD_WM_ENTRY_INFO_H__
#define __HD_WM_ENTRY_INFO_H__

#include <glib-object.h>
#include <gdk/gdk.h>
#include <gdk/gdkpixbuf.h>
#include <X11/Xlib.h>

G_BEGIN_DECLS

#define HD_WM_TYPE_ENTRY_INFO           (hd_wm_entry_info_get_type ())
#define HD_WM_ENTRY_INFO(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), HD_WM_TYPE_ENTRY_INFO, HDWMEntryInfo))
#define HD_WM_IS_ENTRY_INFO(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HD_WM_TYPE_ENTRY_INFO))
#define HD_WM_ENTRY_INFO_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), HD_WM_TYPE_ENTRY_INFO, HDWMEntryInfoIface))

typedef struct _HDWMEntryInfo HDWMEntryInfo;
typedef struct _HDWMEntryInfoIface HDWMEntryInfoIface;

struct _HDWMEntryInfoIface
{
  GTypeInterface g_iface;

  gboolean       (*init)	    (HDWMEntryInfo *info);

  HDWMEntryInfo *(*get_parent)      (HDWMEntryInfo *info);
  
  void		 (*set_parent)      (HDWMEntryInfo *info,
		  		     HDWMEntryInfo *parent);

  void           (*add_child)       (HDWMEntryInfo *info,
	                             HDWMEntryInfo *child);

  gboolean       (*remove_child)   (HDWMEntryInfo *info,
                           	    HDWMEntryInfo *child);

  const GList    *(*get_children)   (HDWMEntryInfo *info);

  gint           (*get_n_children) (HDWMEntryInfo *info);

  const gchar   *(*peek_app_name)  (HDWMEntryInfo *info);

  const gchar   *(*peek_title)     (HDWMEntryInfo *info);

  gchar         *(*get_title)      (HDWMEntryInfo *info);

  void           (*set_title)      (HDWMEntryInfo *info,
                                            	     const gchar *title);

  gchar         *(*get_app_name)   (HDWMEntryInfo *info);

  gchar         *(*get_window_name) (HDWMEntryInfo *info);

  GdkPixbuf     *(*get_icon)       (HDWMEntryInfo *info);

  void           (*set_icon)       (HDWMEntryInfo *info,
                                            	     GdkPixbuf   *icon);

  const gchar   *(*get_app_icon_name) (HDWMEntryInfo *info);

  GdkPixbuf     *(*get_app_icon)      (HDWMEntryInfo *info,
                                              		gint         size,
                                              		GError     **error);

  void           (*close)          (HDWMEntryInfo *info);

  gboolean       (*is_active)      (HDWMEntryInfo *info);

  gboolean       (*is_hibernating) (HDWMEntryInfo *info);

  gboolean       (*has_extra_icon)    (HDWMEntryInfo *info);
  const gchar   *(*get_extra_icon)    (HDWMEntryInfo *info);
  
  Window         (*get_x_window)   (HDWMEntryInfo *info);

  gboolean       (*is_urgent)      (HDWMEntryInfo *info);

  gboolean       (*get_ignore_urgent) (HDWMEntryInfo *info);

  void           (*set_ignore_urgent) (HDWMEntryInfo *info,
                                              		gboolean     ignore); 
	
};

GType hd_wm_entry_info_get_type (void);

gboolean       hd_wm_entry_info_init          (HDWMEntryInfo *info);

HDWMEntryInfo *hd_wm_entry_info_get_parent      (HDWMEntryInfo *info);
void 	       hd_wm_entry_info_set_parent    (HDWMEntryInfo *info,
						 HDWMEntryInfo *parent);

void         hd_wm_entry_info_add_child       (HDWMEntryInfo *info,
                                               HDWMEntryInfo *child);

gboolean     hd_wm_entry_info_remove_child    (HDWMEntryInfo *info,
                                               HDWMEntryInfo *child);

const GList* hd_wm_entry_info_get_children    (HDWMEntryInfo *info);
gint         hd_wm_entry_info_get_n_children  (HDWMEntryInfo *info);

const gchar *hd_wm_entry_info_peek_app_name   (HDWMEntryInfo *info);
const gchar *hd_wm_entry_info_peek_title      (HDWMEntryInfo *info);
gchar *      hd_wm_entry_info_get_title       (HDWMEntryInfo *info);
void         hd_wm_entry_info_set_title       (HDWMEntryInfo *info,
                                              const gchar *title);

gchar *      hd_wm_entry_info_get_app_name    (HDWMEntryInfo *info);
gchar *      hd_wm_entry_info_get_window_name (HDWMEntryInfo *info);
GdkPixbuf *  hd_wm_entry_info_get_icon        (HDWMEntryInfo *info);

void         hd_wm_entry_info_set_icon        (HDWMEntryInfo *info,
                                               GdkPixbuf   *icon);

const gchar *hd_wm_entry_info_get_app_icon_name (HDWMEntryInfo *info);
GdkPixbuf *  hd_wm_entry_info_get_app_icon      (HDWMEntryInfo *info,
                                              gint         size,
                                              GError     **error);

void         hd_wm_entry_info_close             (HDWMEntryInfo *info);

gboolean     hd_wm_entry_info_is_urgent         (HDWMEntryInfo *info);
gboolean     hd_wm_entry_info_get_ignore_urgent (HDWMEntryInfo *info);
void         hd_wm_entry_info_set_ignore_urgent (HDWMEntryInfo *info,
                                              gboolean     ignore);

gboolean     hd_wm_entry_info_is_active         (HDWMEntryInfo *info);

gboolean     hd_wm_entry_info_is_hibernating    (HDWMEntryInfo *info);

gboolean     hd_wm_entry_info_has_extra_icon    (HDWMEntryInfo *info);
const gchar *hd_wm_entry_info_get_extra_icon    (HDWMEntryInfo *info);
Window       hd_wm_entry_info_get_x_window      (HDWMEntryInfo *info);

G_END_DECLS

#endif/*__HD_WM_ENTRY_INFO_H__*/
