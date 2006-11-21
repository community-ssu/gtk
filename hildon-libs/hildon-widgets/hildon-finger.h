/*
 * This file is part of hildon-libs
 *
 * Copyright (C) 2006 Nokia Corporation, all rights reserved.
 *
 * Contact: Michael Dominic Kostrzewa <michael.kostrzewa@nokia.com>
 *  Author: Tomas Junnonen <tomas.junnonen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1 of
 * the License.
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

#ifndef __HILDON_FINGER_H__
#define __HILDON_FINGER_H__

#include <gdk/gdkevents.h>

G_BEGIN_DECLS

#define HILDON_FINGER_PRESSURE_THRESHOLD 0.4

gboolean hildon_button_event_is_finger (GdkEventButton *event);

G_END_DECLS
#endif /* HILDON_FINGER_H */
