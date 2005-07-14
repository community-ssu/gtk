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

#include "sapwood-proto.h"
#include <gdk/gdk.h>

G_CONST_RETURN char *
sapwood_socket_path_get_for_display (GdkDisplay *display)
{
  static GHashTable *path_table = NULL;
  char *path;

  if (!path_table)
    path_table = g_hash_table_new_full (g_direct_hash, g_direct_equal,
					NULL, g_free);

  path = g_hash_table_lookup (path_table, display);
  if (!path)
    {
      char *name;

      /* $TMPDIR/sapwood-$DISPLAY */
      name = g_strconcat ("sapwood-", gdk_display_get_name (display), NULL);
      path = g_build_filename (g_get_tmp_dir (), name, NULL);
      g_free (name);

      g_hash_table_insert (path_table, display, path);
    }

  return path;
}

G_CONST_RETURN char *
sapwood_socket_path_get_default (void)
{
  return sapwood_socket_path_get_for_display (gdk_display_get_default ());
}
