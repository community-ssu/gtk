/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005 Nokia Corporation.
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

/* 
 * Simple blinking pixbuf animation class. 
 *   Authored by Chris Lord <chris@o-hand.com> 
 *               Matthew Allum <mallum@o-hand.com>
 */

#ifndef HILDON_PIXBUF_ANIM_BLINKER_H
#define HILDON_PIXBUF_ANIM_BLINKER_H

#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

typedef struct _HildonPixbufAnimBlinker HildonPixbufAnimBlinker;
typedef struct _HildonPixbufAnimBlinkerClass HildonPixbufAnimBlinkerClass;

#define TYPE_HILDON_PIXBUF_ANIM_BLINKER              (hildon_pixbuf_anim_blinker_get_type ())
#define HILDON_PIXBUF_ANIM_BLINKER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), TYPE_HILDON_PIXBUF_ANIM_BLINKER, HildonPixbufAnimBlinker))
#define IS_HILDON_PIXBUF_ANIM_BLINKER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), TYPE_HILDON_PIXBUF_ANIM_BLINKER))

#define HILDON_PIXBUF_ANIM_BLINKER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_HILDON_PIXBUF_ANIM_BLINKER, HildonPixbufAnimBlinkerClass))
#define IS_HILDON_PIXBUF_ANIM_BLINKER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_HILDON_PIXBUF_ANIM_BLINKER))
#define HILDON_PIXBUF_ANIM_BLINKER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_HILDON_PIXBUF_ANIM_BLINKER, HildonPixbufAnimBlinkerClass))

GType hildon_pixbuf_anim_blinker_get_type (void) G_GNUC_CONST;
GType hildon_pixbuf_anim_blinker_iter_get_type (void) G_GNUC_CONST;

HildonPixbufAnimBlinker *hildon_pixbuf_anim_blinker_new (GdkPixbuf *pixbuf, 
							 gint       period,
							 gint       length);

G_END_DECLS

#endif
