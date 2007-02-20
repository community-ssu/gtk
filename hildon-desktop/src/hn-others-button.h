/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

#ifndef __HN_OTHERS_BUTTON_H__
#define __HN_OTHERS_BUTTON_H__

#include <glib.h>
#include <glib-object.h>
#include <libhildonwm/hd-wm.h>
#include <libhildondesktop/libhildondesktop.h>

G_BEGIN_DECLS

typedef struct _HNOthersButton HNOthersButton;
typedef struct _HNOthersButtonClass HNOthersButtonClass;
typedef struct _HNOthersButtonPrivate HNOthersButtonPrivate;

#define HN_TYPE_OTHERS_BUTTON            (hn_others_button_get_type ())
#define HN_OTHERS_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HN_TYPE_OTHERS_BUTTON, HNOthersButton))
#define HN_IS_OTHERS_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HN_TYPE_OTHERS_BUTTON))
#define HN_OTHERS_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HN_TYPE_OTHERS_BUTTON, HNOthersButtonClass))
#define HN_IS_OTHERS_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HN_TYPE_OTHERS_BUTTON))
#define HN_OTHERS_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HN_TYPE_OTHERS_BUTTON, HNOthersButtonClass))

struct _HNOthersButton
{
  TaskNavigatorItem parent_instance;
  
  HNOthersButtonPrivate *priv;
};

struct _HNOthersButtonClass
{
  TaskNavigatorItemClass parent_class;
};

GType        hn_others_button_get_type             (void);

GtkWidget   *hn_others_button_new                  (void);

void         hn_others_button_dnotify_register     (HNOthersButton *button);

void         hn_others_button_close_menu	   (HNOthersButton *button);

G_END_DECLS

#endif /* __HN_OTHERS_BUTTON_H__ */
