/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
 *
 * Author:  Johan Bilien <johan bilien@nokia.com>
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

#ifndef __HILDON_DESKTOP_PICTURE_H__
#define __HILDON_DESKTOP_PICTURE_H__

#include <X11/extensions/Xrender.h>

G_BEGIN_DECLS

void    hildon_desktop_picture_and_mask_from_file (const gchar *filename,
                                                   Picture     *picture,
                                                   Picture     *mask,
                                                   gint        *width,
                                                   gint        *height);

void    hildon_desktop_picture_and_mask_from_pixbuf (GdkDisplay        *display,
                                                     const GdkPixbuf   *pixbuf,
                                                     Picture           *picture,
                                                     Picture           *mask);

Picture hildon_desktop_picture_from_drawable      (GdkDrawable *drawable);

G_END_DECLS

#endif /* __HILDON_DESKTOP_TOGGLE_BUTTON_H__ */
