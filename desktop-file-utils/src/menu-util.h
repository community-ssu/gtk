/* Random utility functions for menu code */

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

#ifndef MENU_UTIL_H
#define MENU_UTIL_H

#include <glib.h>

gboolean g_create_dir (const char    *dir,
                       unsigned int   mode,
                       GError       **err);

/* random utility function */
#ifdef DFU_MENU_DISABLE_VERBOSE
#ifdef G_HAVE_ISO_VARARGS
#define menu_verbose(...)
#elif defined(G_HAVE_GNUC_VARARGS)
#define menu_verbose(format...)
#else
#error "Cannot disable verbose mode due to lack of varargs macros"
#endif
#else
void menu_verbose    (const char *format,
                      ...) G_GNUC_PRINTF (1, 2);
#endif

gboolean g_file_save_atomically (const char *filename,
                                 const char *str,
                                 int         len,
                                 GError    **error);

void g_string_append_random_bytes (GString *str,
                                   int      n_bytes);
void g_string_append_random_ascii (GString *str,
                                   int      n_bytes);



typedef struct XdgPathInfo XdgPathInfo;

struct XdgPathInfo
{
  char  *data_home;
  char  *config_home;
  char **data_dirs;
  char **config_dirs;
  const char *first_system_data;
  const char *first_system_config;
};

void init_xdg_paths (XdgPathInfo *info_p);
     
#endif /* MENU_UTIL_H */
