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

/**
 * SECTION:hildon-finger
 * @short_description: Helper functions for finger functionality
 *
 */

#include <gdk/gdk.h>
#include "hildon-finger.h"

#define HILDON_FINGER_BUTTON 8
#define HILDON_FINGER_ALT_BUTTON 1
#define HILDON_FINGER_ALT_MASK GDK_MOD4_MASK
#define HILDON_FINGER_SIMULATE_BUTTON 2

/**
 * hildon_button_event_is_finger:
 * 
 * Check if the event is a finger event
 **/
gboolean hildon_button_event_is_finger (GdkEventButton *event)
{
  gdouble pressure;

  if (gdk_event_get_axis ((GdkEvent*)event, GDK_AXIS_PRESSURE, &pressure) &&
      pressure > HILDON_FINGER_PRESSURE_THRESHOLD)
    return TRUE;

  if (event->button == HILDON_FINGER_BUTTON)
    return TRUE;

  if (event->button == HILDON_FINGER_ALT_BUTTON &&
      event->state & HILDON_FINGER_ALT_MASK)
    return TRUE;

  if (event->button == HILDON_FINGER_SIMULATE_BUTTON)
    return TRUE;

  return FALSE;
}
