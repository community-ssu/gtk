/* hn-entry-info.h
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

#ifndef __HD_ENTRY_INFO_H__
#define __HD_ENTRY_INFO_H__

#include <glib-object.h>
#include <gdk/gdkpixbuf.h>

#include <X11/Xlib.h>

#include <libhildonwm/hd-wm-types.h>

G_BEGIN_DECLS

/* HDEntryInfo - opaque wrapper type for watchable applications
 *
 * This object is used to be passed around between the WM and
 * the AS, providing a commodity API for accessing common meta-data
 * about the windows without having the AS to know the subtleties
 * of the actual window types.
 */

typedef enum {
  HD_ENTRY_INVALID,        /* only used for debugging */

  HD_ENTRY_DESKTOP,
  
  HD_ENTRY_WATCHED_APP,
  HD_ENTRY_WATCHED_WINDOW,
  HD_ENTRY_WATCHED_VIEW
} HDEntryInfoType;

#define HD_ENTRY_INFO_IS_VALID_TYPE(x) (((x) > HD_ENTRY_INVALID) && ((x) <= HD_ENTRY_WATCHED_VIEW))

struct _HDEntryInfo
{
  HDEntryInfoType type;
};

HDEntryInfo *hd_entry_info_new             (HDEntryInfoType        type);
HDEntryInfo *hd_entry_info_new_from_app    (HDWMWatchableApp      *app);
HDEntryInfo *hd_entry_info_new_from_window (HDWMWatchedWindow     *window);
HDEntryInfo *hd_entry_info_new_from_view   (HDWMWatchedWindowView *view);
void         hd_entry_info_free            (HDEntryInfo           *info);

void                   hd_entry_info_set_app    (HDEntryInfo           *info,
						 HDWMWatchableApp      *app);
HDWMWatchableApp *     hd_entry_info_get_app    (HDEntryInfo           *info);
void                   hd_entry_info_set_window (HDEntryInfo           *info,
						 HDWMWatchedWindow     *window);
HDWMWatchedWindow *    hd_entry_info_get_window (HDEntryInfo           *info);
void                   hd_entry_info_set_view   (HDEntryInfo           *info,
						 HDWMWatchedWindowView *view);
HDWMWatchedWindowView *hd_entry_info_get_view   (HDEntryInfo           *info);

HDEntryInfo *hd_entry_info_get_parent      (HDEntryInfo *info);

void         hd_entry_info_add_child       (HDEntryInfo *info,
					    HDEntryInfo *child);
gboolean     hd_entry_info_remove_child    (HDEntryInfo *info,
					    HDEntryInfo *child);
const GList* hd_entry_info_get_children    (HDEntryInfo *info);
gint         hd_entry_info_get_n_children  (HDEntryInfo *info);

const gchar *hd_entry_info_peek_app_name   (HDEntryInfo *info);
const gchar *hd_entry_info_peek_title      (HDEntryInfo *info);
gchar *      hd_entry_info_get_title       (HDEntryInfo *info);
void         hd_entry_info_set_title       (HDEntryInfo *info,
				            const gchar *title);
gchar *      hd_entry_info_get_app_name    (HDEntryInfo *info);
gchar *      hd_entry_info_get_window_name (HDEntryInfo *info);
GdkPixbuf *  hd_entry_info_get_icon        (HDEntryInfo *info);
void         hd_entry_info_set_icon        (HDEntryInfo *info,
				            GdkPixbuf   *icon);

const gchar *hd_entry_info_get_app_icon_name (HDEntryInfo *info);
GdkPixbuf *  hd_entry_info_get_app_icon      (HDEntryInfo *info,
                                              gint         size,
                                              GError     **error);

void         hd_entry_info_close             (HDEntryInfo *info);

gboolean     hd_entry_info_is_urgent         (HDEntryInfo *info);
gboolean     hd_entry_info_get_ignore_urgent (HDEntryInfo *info);
void         hd_entry_info_set_ignore_urgent (HDEntryInfo *info,
		                              gboolean     ignore);

gboolean     hd_entry_info_is_active         (HDEntryInfo *info);

gboolean     hd_entry_info_is_hibernating    (HDEntryInfo *info);

gboolean     hd_entry_info_has_extra_icon    (HDEntryInfo *info);
const gchar *hd_entry_info_get_extra_icon    (HDEntryInfo *info);
Window       hd_entry_info_get_x_window      (HDEntryInfo *info);

G_END_DECLS

#endif /*__HD_ENTRY_INFO_H__*/
