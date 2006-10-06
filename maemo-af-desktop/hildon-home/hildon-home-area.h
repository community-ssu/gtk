/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 * Author: Johan Bilien <johan.bilien@nokia.com>
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

#ifndef __HILDON_HOME_AREA_H__
#define __HILDON_HOME_AREA_H__

#include <gtk/gtkfixed.h>
#include "hildon-plugin-list.h"

G_BEGIN_DECLS


#define HILDON_TYPE_HOME_AREA (hildon_home_area_get_type())
#define HILDON_HOME_AREA(obj) \
    (GTK_CHECK_CAST (obj, HILDON_TYPE_HOME_AREA, HildonHomeArea))
#define HILDON_HOME_AREA_CLASS(klass) \
    (GTK_CHECK_CLASS_CAST ((klass),\
     HILDON_TYPE_HOME_AREA, HildonHomeAreaClass))
#define HILDON_IS_HOME_AREA(obj) (GTK_CHECK_TYPE (obj, HILDON_TYPE_HOME_AREA))
#define HILDON_IS_HOME_AREA_CLASS(klass) \
    (GTK_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_HOME_AREA))


typedef struct _HildonHomeArea
{
  GtkFixed          parent;

} HildonHomeArea;

typedef struct _HildonHomeAreaClass
{
  GtkFixedClass     parent_class;

  void (* layout_mode_start)    (HildonHomeArea *area);
  void (* layout_mode_started)  (HildonHomeArea *area);
  void (* layout_mode_end)      (HildonHomeArea *area);
  void (* layout_mode_ended)    (HildonHomeArea *area);
  void (* layout_changed)       (HildonHomeArea *area);
  
  void (* applet_added)           (HildonHomeArea *area, GtkWidget *w);

} HildonHomeAreaClass;

GType       hildon_home_area_get_type           (void);

GtkWidget * hildon_home_area_new                (void);

void        hildon_home_area_set_layout_mode    (HildonHomeArea *area,
                                                 gboolean layout_mode);

gboolean    hildon_home_area_get_layout_mode    (HildonHomeArea *area);

gint        hildon_home_area_save_configuration (HildonHomeArea *area,
                                                 const gchar *filename);
void        hildon_home_area_load_configuration (HildonHomeArea *area,
                                                 const gchar *filename);

gint        hildon_home_area_sync_from_list     (HildonHomeArea *area,
                                                 HildonPluginList *list);

void        hildon_home_area_sync_to_list       (HildonHomeArea *area,
                                                 HildonPluginList *list);

gboolean    hildon_home_area_get_layout_changed (HildonHomeArea *area);

gboolean    hildon_home_area_get_overlaps       (HildonHomeArea *area);


G_END_DECLS


#endif
