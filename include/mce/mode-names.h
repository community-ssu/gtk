/**
 * @file dbus-names.h
 *
 * Copyright (C) 2006 Nokia Corporation.
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
#ifndef _MODE_NAMES_H_
#define _MODE_NAMES_H_

#define MCE_NORMAL_MODE		"normal"	/* normal mode */
#define MCE_FLIGHT_MODE		"flight"	/* flight mode */
#define MCE_OFFLINE_MODE	"offline"	/* offline mode; unsupported! */
#define MCE_INVALID_MODE	"invalid"	/* should *never* occur! */

#define MCE_DEVICE_LOCKED	"locked"	/* device locked */
#define MCE_DEVICE_UNLOCKED	"unlocked"	/* device unlocked */

#define MCE_ALARM_VISIBLE	"visible"	/* alarm visible */
#define MCE_ALARM_SNOOZED	"snoozed"	/* alarm snoozed */
#define MCE_ALARM_OFF		"off"		/* alarm off */

#endif /* _MODE_NAMES_H_ */
