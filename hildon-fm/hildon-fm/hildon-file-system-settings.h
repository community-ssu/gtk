/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Kimmo Hämäläinen <kimmo.hamalainen@nokia.com>
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
 * HildonFileSystemSettings
 *
 * Shared settings object to be used in HildonFileSystemModel.
 * Setting up dbus/gconf stuff for each model takes time, so creating
 * a single settings object is much more convenient.
 *
 * INTERNAL TO FILE SELECTION STUFF, NOT FOR APPLICATION DEVELOPERS TO USE.
 *
 */

#ifndef __HILDON_FILE_SYSTEM_SETTINGS_H__
#define __HILDON_FILE_SYSTEM_SETTINGS_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define HILDON_TYPE_FILE_SYSTEM_SETTINGS (hildon_file_system_settings_get_type())
#define HILDON_FILE_SYSTEM_SETTINGS(object) \
  (G_TYPE_CHECK_INSTANCE_CAST((object), HILDON_TYPE_FILE_SYSTEM_SETTINGS, \
  HildonFileSystemSettings))
#define HILDON_FILE_SYSTEM_SETTINGSClass(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), HILDON_TYPE_FILE_SYSTEM_SETTINGS, \
  HildonFileSystemSettingsClass))
#define HILDON_IS_FILE_SYSTEM_SETTINGS(object) \
  (G_TYPE_CHECK_INSTANCE_TYPE((object), HILDON_TYPE_FILE_SYSTEM_SETTINGS))
#define HILDON_IS_FILE_SYSTEM_SETTINGS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), HILDON_TYPE_FILE_SYSTEM_SETTINGS))
#define HILDON_FILE_SYSTEM_SETTINGS_GET_CLASS(object) \
  (G_TYPE_INSTANCE_GET_CLASS((object), HILDON_TYPE_FILE_SYSTEM_SETTINGS, \
  HildonFileSystemSettingsClass))

typedef struct _HildonFileSystemSettings HildonFileSystemSettings;
typedef struct _HildonFileSystemSettingsClass HildonFileSystemSettingsClass;
typedef struct _HildonFileSystemSettingsPrivate HildonFileSystemSettingsPrivate;

struct _HildonFileSystemSettings
{
  GObject parent;  
  HildonFileSystemSettingsPrivate *priv;
};

struct _HildonFileSystemSettingsClass
{
  GObjectClass parent_class;
};

GType hildon_file_system_settings_get_type(void);
HildonFileSystemSettings *_hildon_file_system_settings_get_instance(void);

/* Returns TRUE, if async queries have finished */
gboolean _hildon_file_system_settings_ready(HildonFileSystemSettings *self);

/* Communication with tasknavigator for displaying possible
   banner while making blocking calls */
void _hildon_file_system_prepare_banner(void);
void _hildon_file_system_cancel_banner(void);

G_END_DECLS

#endif
