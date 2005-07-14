/* GTK+ Sapwood Engine
 * Copyright (C) 2005 Nokia Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Written by Tommi Komulainen <tommi.komulainen@nokia.com>
 */

#ifndef PIXBUF_PROTO_H
#define PIXBUF_PROTO_H 1

#include <gdk/gdktypes.h>

G_BEGIN_DECLS

#define PIXBUF_OP_OPEN  1
#define PIXBUF_OP_CLOSE 2

typedef struct
{
  guint8  op;
  guint8  _pad1;
  guint16 length;
} PixbufBaseRequest;

typedef struct
{
  PixbufBaseRequest base;
  guint8  border_left;
  guint8  border_right;
  guint8  border_top;
  guint8  border_bottom;
  guchar  filename[0];          /* null terminated, absolute filename */
} PixbufOpenRequest;

typedef struct
{
  PixbufBaseRequest base;
  guint32 id;
} PixbufCloseRequest;

typedef struct
{
  guint32 id;
  guint16 width;
  guint16 height;
  guint32 pixmap[3][3];         /* XIDs for pixmaps and masks for each part */
  guint32 pixmask[3][3];        /* 0 if not applicable (full opacity)       */
} PixbufOpenResponse;

G_CONST_RETURN char *sapwood_socket_path_get_default (void);
G_CONST_RETURN char *sapwood_socket_path_get_for_display (GdkDisplay *display);

G_END_DECLS

#endif
