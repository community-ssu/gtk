/* hn-entry-info.h
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
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

#ifndef HN_ENTRY_INFO_H
#define HN_ENTRY_INFO_H

#include <glib-object.h>
#include <gdk/gdkpixbuf.h>

#include "hn-wm-types.h"

G_BEGIN_DECLS

/* HNEntryInfo - opaque wrapper type for watchable applications
 *
 * This object is used to be passed around between the WM and
 * the AS, providing a commodity API for accessing common meta-data
 * about the windows without having the AS to know the subtleties
 * of the actual window types.
 */

typedef enum {
  HN_ENTRY_INVALID,        /* only used for debugging */

  HN_ENTRY_WATCHED_APP,
  HN_ENTRY_WATCHED_WINDOW,
  HN_ENTRY_WATCHED_VIEW
} HNEntryInfoType;

#define HN_ENTRY_INFO_IS_VALID_TYPE(x) (((x) > HN_ENTRY_INVALID) && ((x) <= HN_ENTRY_WATCHED_VIEW))

struct _HNEntryInfo
{
  HNEntryInfoType type;
};

HNEntryInfo *hn_entry_info_new             (HNEntryInfoType        type);
HNEntryInfo *hn_entry_info_new_from_app    (HNWMWatchableApp      *app);
HNEntryInfo *hn_entry_info_new_from_window (HNWMWatchedWindow     *window);
HNEntryInfo *hn_entry_info_new_from_view   (HNWMWatchedWindowView *view);
void         hn_entry_info_free            (HNEntryInfo           *info);

void                   hn_entry_info_set_app    (HNEntryInfo           *info,
						 HNWMWatchableApp      *app);
HNWMWatchableApp *     hn_entry_info_get_app    (HNEntryInfo           *info);
void                   hn_entry_info_set_window (HNEntryInfo           *info,
						 HNWMWatchedWindow     *window);
HNWMWatchedWindow *    hn_entry_info_get_window (HNEntryInfo           *info);
void                   hn_entry_info_set_view   (HNEntryInfo           *info,
						 HNWMWatchedWindowView *view);
HNWMWatchedWindowView *hn_entry_info_get_view   (HNEntryInfo           *info);

HNEntryInfo *hn_entry_info_get_parent      (HNEntryInfo *info);

void         hn_entry_info_add_child       (HNEntryInfo *info,
					    HNEntryInfo *child);
gboolean     hn_entry_info_remove_child    (HNEntryInfo *info,
					    HNEntryInfo *child);
const GList* hn_entry_info_get_children    (HNEntryInfo *info);
gint         hn_entry_info_get_n_children  (HNEntryInfo *info);

gchar *      hn_entry_info_get_title       (HNEntryInfo *info);
void         hn_entry_info_set_title       (HNEntryInfo *info,
				            const gchar *title);
GdkPixbuf *  hn_entry_info_get_icon        (HNEntryInfo *info);
void         hn_entry_info_set_icon        (HNEntryInfo *info,
				            GdkPixbuf   *icon);

const gchar *hn_entry_info_get_app_icon_name (HNEntryInfo *info);

void         hn_entry_info_close             (HNEntryInfo *info);

gboolean     hn_entry_info_is_urgent         (HNEntryInfo *info);
gboolean     hn_entry_info_get_ignore_urgent (HNEntryInfo *info);
void         hn_entry_info_set_ignore_urgent (HNEntryInfo *info,
		                              gboolean     ignore);

gboolean     hn_entry_info_is_active         (HNEntryInfo *info);

gboolean     hn_entry_info_is_hibernating    (HNEntryInfo *info);

gboolean     hn_entry_info_has_extra_icon    (HNEntryInfo *info);
const gchar *hn_entry_info_get_extra_icon    (HNEntryInfo *info);

G_END_DECLS

#endif /* HN_ENTRY_INFO_H */
