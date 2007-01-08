/* Vfolder parsing */

/*
 * Copyright (C) 2002 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef VFOLDER_PARSER_H
#define VFOLDER_PARSER_H

#include <glib.h>

typedef enum
{
  /* Internal parser use */
  VFOLDER_QUERY_ROOT,
  /* Exported */
  VFOLDER_QUERY_OR,
  VFOLDER_QUERY_AND,
  VFOLDER_QUERY_CATEGORY
} VfolderQueryType;

typedef struct _VfolderQuery VfolderQuery;
typedef struct _Vfolder Vfolder;

Vfolder* vfolder_load (const char  *filename,
                       GError     **err);
void     vfolder_free (Vfolder *folder);

GSList*       vfolder_get_subfolders       (Vfolder *folder);
const char*   vfolder_get_name             (Vfolder *folder);
const char*   vfolder_get_desktop_file     (Vfolder *folder);
gboolean      vfolder_get_show_if_empty    (Vfolder *folder);
gboolean      vfolder_get_only_unallocated (Vfolder *folder);
GSList*       vfolder_get_excludes         (Vfolder *folder);
GSList*       vfolder_get_includes         (Vfolder *folder);
GSList*       vfolder_get_merge_dirs       (Vfolder *folder);
GSList*       vfolder_get_desktop_dirs     (Vfolder *folder);
VfolderQuery* vfolder_get_query            (Vfolder *folder);


VfolderQueryType vfolder_query_get_type       (VfolderQuery *query);
GSList*          vfolder_query_get_subqueries (VfolderQuery *query);
const char*      vfolder_query_get_category   (VfolderQuery *query);
gboolean         vfolder_query_get_negated    (VfolderQuery *query);

#endif
