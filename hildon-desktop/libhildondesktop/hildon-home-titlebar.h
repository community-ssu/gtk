/* -*- mode:C; c-file-style:"gnu"; -*- */
/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006, 2007 Nokia Corporation.
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

#ifndef __HILDON_HOME_TITLEBAR_H__
#define __HILDON_HOME_TITLEBAR_H__

#include <gtk/gtkeventbox.h>
#include <libhildondesktop/hildon-home-area.h>

#define HILDON_TYPE_HOME_TITLEBAR		(hildon_home_titlebar_get_type ())
#define HILDON_HOME_TITLEBAR(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), HILDON_TYPE_HOME_TITLEBAR, HildonHomeTitlebar))
#define HILDON_IS_HOME_TITLEBAR(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), HILDON_TYPE_HOME_TITLEBAR))
#define HILDON_HOME_TITLEBAR_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), HILDON_TYPE_HOME_TITLEBAR, HildonHomeTitlebarClass))
#define HILDON_IS_HOME_TITLEBAR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_HOME_TITLEBAR))
#define HILDON_HOME_TITLEBAR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), HILDON_TYPE_HOME_TITLEBAR, HildonHomeTitlebarClass))

#define HILDON_TYPE_HOME_TITLEBAR_MODE		(hildon_home_titlebar_mode_get_type ())

G_BEGIN_DECLS

typedef struct _HildonHomeTitlebar		HildonHomeTitlebar;
typedef struct _HildonHomeTitlebarClass		HildonHomeTitlebarClass;
typedef struct _HildonHomeTitlebarPrivate	HildonHomeTitlebarPrivate;

struct _HildonHomeTitlebar
{
  GtkEventBox parent_instance;

  HildonHomeTitlebarPrivate *priv;
};

struct _HildonHomeTitlebarClass
{
  GtkEventBoxClass parent_class;

  void (*button1_clicked)           (HildonHomeTitlebar *titlebar);
  void (*button2_clicked)           (HildonHomeTitlebar *titlebar);

};

GType      hildon_home_titlebar_get_type        (void) G_GNUC_CONST;
GtkWidget *hildon_home_titlebar_new             (void);

void       hildon_home_titlebar_toggle_menu     (HildonHomeTitlebar     *titlebar);

void       hildon_home_titlebar_set_menu        (HildonHomeTitlebar     *titlebar,
                                                 GtkWidget              *menu);


void        hildon_home_titlebar_set_title (HildonHomeTitlebar     *titlebar,
                                            const gchar            *title);
G_END_DECLS

#endif /* __HILDON_HOME_TITLEBAR_H__ */
