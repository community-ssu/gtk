/* Overrides for .desktop files in a menu */

/*
 * Copyright (C) 2003 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef MENU_OVERRIDES_H
#define MENU_OVERRIDES_H

#include "menu-util.h"

/* A tricky thing here is that name_to_override/name_to_unoverride
 * can be a long name with dir separators, like "foo/bar.desktop"
 * i.e. it's anything that can be in an include/exclude
 */

typedef struct MenuOverrideDir MenuOverrideDir;

MenuOverrideDir* menu_override_dir_create      (const char       *path,
                                                GError          **error);
void             menu_override_dir_ref         (MenuOverrideDir  *override);
void             menu_override_dir_unref       (MenuOverrideDir  *override);
gboolean         menu_override_dir_add         (MenuOverrideDir  *override,
                                                const char       *menu_path,
                                                const char       *name_to_override,
                                                const char       *based_on_fs_path,
                                                GError          **error);
gboolean         menu_override_dir_remove      (MenuOverrideDir  *override,
                                                const char       *menu_path,
                                                const char       *name_to_unoverride,
                                                GError          **error);
char*            menu_override_dir_get_fs_path (MenuOverrideDir  *override,
                                                const char       *menu_path,
                                                const char       *name_to_unoverride);

gboolean g_create_dir (const char    *dir,
                       unsigned int   mode,
                       GError       **err);

#endif /* MENU_OVERRIDES_H */
