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
#ifndef _DBUS_NAMES_H_
#define _DBUS_NAMES_H_

#define MCE_SERVICE			"com.nokia.mce"

#define MCE_REQUEST_IF			"com.nokia.mce.request"
#define MCE_SIGNAL_IF			"com.nokia.mce.signal"
#define MCE_REQUEST_PATH		"/com/nokia/mce/request"
#define MCE_SIGNAL_PATH			"/com/nokia/mce/signal"

#define MCE_ERROR_FATAL			"com.nokia.mce.error.fatal"
#define MCE_ERROR_INVALID_ARGS		"org.freedesktop.DBus.Error.InvalidArgs"

#define MCE_DISPLAY_ON_REQ		"req_display_state_on"
#define MCE_PREVENT_BLANK_REQ		"req_display_blanking_pause"
#define MCE_DEVICE_MODE_GET		"get_device_mode"
#define MCE_DEVLOCK_MODE_GET		"get_devicelock_mode"
#define MCE_VERSION_GET			"get_version"
#define MCE_DEVICE_MODE_CHANGE_REQ	"req_device_mode_change"
#define MCE_POWERUP_REQ			"req_powerup"
#define MCE_REBOOT_REQ			"req_reboot"
#define MCE_SHUTDOWN_REQ		"req_shutdown"
#define MCE_DEVLOCK_VALIDATE_CODE_REQ	"validate_devicelock_code"
#define MCE_ALARM_MODE_CHANGE_REQ	"set_alarm_mode"

#define MCE_SHUTDOWN_SIG		"shutdown_ind"
#define MCE_DEVLOCK_MODE_SIG		"devicelock_mode_ind"
#define MCE_DATA_SAVE_SIG		"save_unsaved_data_ind"
#define MCE_INACTIVITY_SIG		"system_inactivity_ind"
#define MCE_DEVICE_MODE_SIG		"sig_device_mode_ind"
#define MCE_HOME_KEY_SIG		"sig_home_key_pressed_ind"
#define MCE_HOME_KEY_LONG_SIG		"sig_home_key_pressed_long_ind"

#endif /* _DBUS_NAMES_H_ */
