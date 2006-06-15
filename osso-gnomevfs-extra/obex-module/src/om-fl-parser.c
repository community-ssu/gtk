/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004-2005 Nokia Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <expat.h>

#include "om-utils.h"
#include "om-fl-parser.h"

#define d(x)

typedef struct {
	GError **error;
	GList   *elements;

	gint     depth;
} ParserData;

/* Static functions declaration */
static void       fl_parser_start_node_cb     (void                *data,
					       const char          *el,
					       const char         **attr);
static void       fl_parser_end_node_cb       (void                *data,
					       const char          *el);
static XML_Parser fl_parser_create_context    (ParserData          *data);
static gboolean   fl_parser_fill_file_info    (GnomeVFSFileInfo    *file_info,
					       const char         **attr);
static void       fl_parser_free_parser_data  (ParserData          *data, 
					       gboolean             free_list);


/* Function implementations */
static void 
fl_parser_start_node_cb (void        *user_data,
			 const char  *node_name,
			 const char **attr)
{
	ParserData       *data;
	GnomeVFSFileInfo *info;
	
	data = (ParserData *) user_data;
	
	data->depth++;
	
	d(g_print ("%d: %s\n", data->depth, node_name));

	if (data->depth > 2) {
		g_set_error (data->error,  
			     G_MARKUP_ERROR,  
			     G_MARKUP_ERROR_INVALID_CONTENT,  
			     "Don't expect node '%s' as child of 'file', 'folder' or 'parent-folder'",  
			     node_name); 
		return;
	}
	else if (data->depth == 1) {
		if (strcmp (node_name, "folder-listing") != 0) {
			g_set_error (data->error,  
				     G_MARKUP_ERROR,  
				     G_MARKUP_ERROR_INVALID_CONTENT,  
				     "Expected 'folder-listing', got '%s'",  
				     node_name);  
			return;
		}

		return;
	}

	if (strcmp (node_name, "parent-folder") == 0) {
		/* Just ignore parent-folder items */
		return;
	}
	
	info = gnome_vfs_file_info_new ();

	info->valid_fields = GNOME_VFS_FILE_INFO_FIELDS_NONE;
	info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_TYPE;
	info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE;
	
	if (strcmp (node_name, "file") == 0) {
		info->type = GNOME_VFS_FILE_TYPE_REGULAR;
	}
	else if (strcmp (node_name, "folder") == 0) {
		info->type = GNOME_VFS_FILE_TYPE_DIRECTORY;
		info->mime_type = g_strdup ("x-directory/normal");
	} else {
		g_set_error (data->error,  
			     G_MARKUP_ERROR,  
			     G_MARKUP_ERROR_UNKNOWN_ELEMENT,  
			     "Unknown element '%s'",  
			     node_name);
		return;
	}

	if (!fl_parser_fill_file_info (info, attr)) {
		d(g_print ("Failed to fill GnomeVFSFileInfo from node '%s'\n",
			   node_name));
		gnome_vfs_file_info_unref (info);
		return;
	}

	if (info->mime_type == NULL) {
		info->mime_type = g_strdup (
			gnome_vfs_mime_type_from_name (info->name));
	}

	/* Permissions on folders in OBEX has different semantics than POSIX.
	 * In POSIX, if a folder is not writable, it means that it's content
	 * can't be removed, whereas in OBEX, it just means that the folder
	 * itself can't be removed. Therefore we must set all folders to RWD and
	 * handle the error when it happens. */
	if (info->type == GNOME_VFS_FILE_TYPE_DIRECTORY) {
		info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS;
		info->permissions =
			GNOME_VFS_PERM_USER_ALL |
			GNOME_VFS_PERM_GROUP_ALL |
			GNOME_VFS_PERM_OTHER_ALL;
	}
	
	data->elements = g_list_prepend (data->elements, info);
}

static void
fl_parser_end_node_cb (void *user_data, const char *node_name)
{
	ParserData *data;

	data = (ParserData *) user_data;

	data->depth--;
	
	if (data->depth < 0) {  
		g_set_error (data->error,  
			     G_MARKUP_ERROR,  
			     G_MARKUP_ERROR_INVALID_CONTENT,  
			     "Closing non-open node '%s'",  
			     node_name);  
		return;  
	} 

	d(g_print ("%d: /%s\n", data->depth, node_name));
}

static XML_Parser
fl_parser_create_context (ParserData *data)
{
	XML_Parser parser;
	
	parser = XML_ParserCreate (NULL);
	
	XML_SetElementHandler(parser, 
			      (XML_StartElementHandler) fl_parser_start_node_cb,
			      (XML_EndElementHandler) fl_parser_end_node_cb);
	XML_SetUserData (parser, data);

	return parser;
}

static gboolean
fl_parser_fill_file_info (GnomeVFSFileInfo  *info, const char **attr)
{
	gint i;
	
	for (i = 0; attr[i]; ++i) {
		const gchar *name;
		const gchar *value;

		name  = attr[i];
		value = attr[++i];
		
		if (strcmp (name, "name") == 0) {
			/* Apparently someone decided it was a good idea
			 * to send name="" mem-type="MMC" 
			 */
			if (!value || strcmp (value, "") == 0) {
				return FALSE;
			}

			info->name = g_strdup (value);
			d(g_print ("Name: '%s'\n", info->name));
		}
		else if (strcmp (name, "size") == 0) {
			info->size = strtoll (value, NULL, 10);
			info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_SIZE;
			d(g_print ("Size: '%lld'\n", info->size));
		}
		else if (strcmp (name, "modified") == 0) {
			info->mtime = om_utils_parse_iso8601 (value);
			if (info->mtime < 0) {
				continue;
			}
			
			info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_MTIME;
			d(g_print ("Modified: '%s' = '%d'\n", 
				   value, (int)info->mtime));
		}
		else if (strcmp (name, "created") == 0) {
			info->ctime = om_utils_parse_iso8601 (value);
			if (info->ctime < 0) {
				continue;
			}
			
			info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_CTIME;
			d(g_print ("Created: '%s' = '%d'\n", 
				   value, (int)info->ctime));
		}
		else if (strcmp (name, "accessed") == 0) {
			info->atime = om_utils_parse_iso8601 (value);
			if (info->atime < 0) {
				continue;
			} 
			
			info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_ATIME;
			d(g_print ("Accessed: '%s' = '%d'\n", 
				   value, (int)info->atime));
		}
		else if (strcmp (name, "user-perm") == 0) {
			/* The permissions don't map well to unix semantics,
			 * since the user is most likely not the same on both
			 * sides. We map the user permissions to "other" on the
			 * local side. D is treated as write, otherwise files
			 * can't be deleted through the module, even if it
			 * should be possible.
			 */
			info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS;

			if (strstr (value, "R")) {
				info->permissions |= GNOME_VFS_PERM_USER_READ;
				info->permissions |= GNOME_VFS_PERM_GROUP_READ;
				info->permissions |= GNOME_VFS_PERM_OTHER_READ;
			}
			if (strstr (value, "W") || strstr (value, "D")) {
				info->permissions |= GNOME_VFS_PERM_USER_WRITE;
				info->permissions |= GNOME_VFS_PERM_GROUP_WRITE;
				info->permissions |= GNOME_VFS_PERM_OTHER_WRITE;
			}
		}
		else if (strcmp (name, "group-perm") == 0) {
			/* Ignore for now */
			d(g_print ("Group permissions: '%s'\n", value));
		}
		else if (strcmp (name, "other-perm") == 0) {
			/* Ignore for now */
			d(g_print ("Other permissions: '%s'\n", value));
		}
		else if (strcmp (name, "owner") == 0) {
			/* Ignore for now */
			d(g_print ("Owner: '%s'\n", value));
		}
		else if (strcmp (name, "group") == 0) {
			/* Ignore for now */
			d(g_print ("Group: '%s'\n", value));
		}
		else if (strcmp (name, "type") == 0) {
			info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE;
			info->mime_type = g_strdup (value);
			d(g_print ("Mime-Type: '%s'\n", info->mime_type));
		}
		else if (strcmp (name, "xml:lang") == 0) {
			d(g_print ("Lang: '%s'\n", value));
		} 
		else {
			d(g_print ("Unknown Attribute: %s = %s\n", 
				   name, value));
		}
	}

	if (!info->name) { /* Required attribute */
		/* Set error */
		return FALSE;
	}
	
	return TRUE;
}

static void
fl_parser_free_parser_data (ParserData *data, gboolean free_list)
{
	if (free_list) {
		gnome_vfs_file_info_list_free (data->elements);
		data->elements = NULL;
	}

	g_free (data);
}

gboolean
om_fl_parser_parse (const gchar *buf, gint len, GList **elements,
		    GError **error)
{
	ParserData *data;
	XML_Parser  parser;

	data = g_new0 (ParserData, 1);
	data->error = error;
	data->elements = NULL;
	data->depth = 0;

	parser = fl_parser_create_context (data);
	if (!parser) {
		g_free (data);
		return FALSE;
	}

	if (XML_Parse (parser, buf, len, TRUE) == 0) {
		XML_ParserFree (parser);
		fl_parser_free_parser_data (data, TRUE);

		if (*error == NULL) {
			g_set_error (error,
				     G_MARKUP_ERROR,
				     G_MARKUP_ERROR_INVALID_CONTENT,
				     "Couldn't parse the incoming data");
		}
		return FALSE;
	}

	XML_ParserFree (parser);
	
	*elements = data->elements;

	fl_parser_free_parser_data (data, FALSE);
		
	return TRUE;
}

