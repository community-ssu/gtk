/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* 
 * vfolder-info.c - Loading of .vfolder-info files.  External interface 
 *                  defined in vfolder-common.h
 *
 * Copyright (C) 2002 Ximian, Inc.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Alex Graveley <alex@ximian.com>
 *         Based on original code by George Lebl <jirka@5z.com>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>

#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-module-shared.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <sys/time.h>

#include "vfolder-common.h"
#include "vfolder-util.h"

#define DOT_GNOME ".gnome2"

typedef enum {
	ITEM_DIR = 1,
	MERGE_DIR
} ItemDirType;

typedef struct {
	VFolderInfo    *info;
	gint            weight;
	gchar          *uri;
	GSList         *monitors;
	ItemDirType     type;
} ItemDir;

/* .vfolder-info format example:
 * <VFolderInfo>
 *   <!-- Merge dirs optional -->
 *   <MergeDir>/etc/X11/applnk</MergeDir>
 *   <!-- Only specify if it should override standard location -->
 *   <ItemDir>/usr/share/applications</ItemDir>
 *   <!-- This is where the .directories are -->
 *   <DesktopDir>/etc/X11/gnome/vfolders</DesktopDir>
 *   <!-- Root folder -->
 *   <Folder>
 *     <Name>Root</Name>
 *
 *     <Include>important.desktop</Include>
 *
 *     <!-- Other folders -->
 *     <Folder>
 *       <Name>SomeFolder</Name>
 *       <ParentLink>http:///mywebdav.com/homedir</ParentLink>
 *     </Folder>
 *     <Folder>
 *       <Name>Test_Folder</Name>
 *       <Parent>file:///a_readonly_path</Parent>
 *       <!-- could also be absolute -->
 *       <Desktop>Test_Folder.directory</Desktop>
 *       <Query>
 *         <Or>
 *           <And>
 *             <Keyword>Application</Keyword>
 *             <Keyword>Game</Keyword>
 *           </And>
 *           <Keyword>Clock</Keyword>
 *         </Or>
 *       </Query>
 *       <Include>somefile.desktop</Include>
 *       <Include>someotherfile.desktop</Include>
 *       <Exclude>yetanother.desktop</Exclude>
 *     </Folder>
 *   </Folder>
 * </VFolderInfo>
 */


/* 
 * XML VFolder description writing
 */
static void
add_xml_tree_from_query (xmlNode *parent, Query *query)
{
	xmlNode *real_parent;

	if (query->not)
		real_parent = xmlNewChild (parent /* parent */,
					   NULL /* ns */,
					   "Not" /* name */,
					   NULL /* content */);
	else
		real_parent = parent;

	if (query->type == QUERY_KEYWORD) {
		const char *string = g_quark_to_string (query->val.keyword);

		xmlNewChild (real_parent /* parent */,
			     NULL /* ns */,
			     "Keyword" /* name */,
			     string /* content */);
	} else if (query->type == QUERY_FILENAME) {
		xmlNewChild (real_parent /* parent */,
			     NULL /* ns */,
			     "Filename" /* name */,
			     query->val.filename /* content */);
	} else if (query->type == QUERY_PARENT) {
		xmlNewChild (real_parent   /* parent */,
			     NULL          /* ns */,
			     "ParentQuery" /* name */,
			     NULL          /* content */);
	} else if (query->type == QUERY_OR ||
		   query->type == QUERY_AND) {
		xmlNode *node;
		const char *name;
		GSList *li;

		if (query->type == QUERY_OR)
			name = "Or";
		else /* QUERY_AND */
			name = "And";

		node = xmlNewChild (real_parent /* parent */,
				    NULL /* ns */,
				    name /* name */,
				    NULL /* content */);

		for (li = query->val.queries; li != NULL; li = li->next) {
			Query *subquery = li->data;
			add_xml_tree_from_query (node, subquery);
		}
	} else {
		g_assert_not_reached ();
	}
}

static void
add_excludes_to_xml (gpointer key, gpointer value, gpointer user_data)
{
	const char *filename = key;
	xmlNode *folder_node = user_data;

	xmlNewChild (folder_node /* parent */,
		     NULL /* ns */,
		     "Exclude" /* name */,
		     filename /* content */);
}

static void
add_xml_tree_from_folder (xmlNode *parent, Folder *folder)
{
	const GSList *li;
	xmlNode *folder_node;
	const gchar *extend_uri;

	/* 
	 * return if this folder hasn't been modified by the user, 
	 * and contains no modified subfolders.
	 */
	if (!folder->user_private && !folder->has_user_private_subfolders)
		return;

	folder_node = xmlNewChild (parent /* parent */,
				   NULL /* ns */,
				   "Folder" /* name */,
				   NULL /* content */);

	xmlNewChild (folder_node /* parent */,
		     NULL /* ns */,
		     "Name" /* name */,
		     folder_get_name (folder) /* content */);

	extend_uri = folder_get_extend_uri (folder);
	if (extend_uri) {
		xmlNewChild (folder_node /* parent */,
			     NULL /* ns */,
			     folder->is_link ? "ParentLink" : "Parent",
			     extend_uri /* content */);
	}

	if (folder->user_private) {
		const gchar *desktop_file;

		if (folder->read_only)
			xmlNewChild (folder_node /* parent */,
				     NULL /* ns */,
				     "ReadOnly" /* name */,
				     NULL /* content */);
		if (folder->dont_show_if_empty)
			xmlNewChild (folder_node /* parent */,
				     NULL /* ns */,
				     "DontShowIfEmpty" /* name */,
				     NULL /* content */);
		if (folder->only_unallocated)
			xmlNewChild (folder_node /* parent */,
				     NULL /* ns */,
				     "OnlyUnallocated" /* name */,
				     NULL /* content */);

		if (folder->desktop_file != NULL) {
			desktop_file = folder_get_desktop_file (folder);
			if (desktop_file)
				xmlNewChild (folder_node /* parent */,
					     NULL /* ns */,
					     "Desktop" /* name */,
					     desktop_file);
		}

		for (li = folder->includes; li != NULL; li = li->next) {
			const char *include = li->data;
			xmlNewChild (folder_node /* parent */,
				     NULL /* ns */,
				     "Include" /* name */,
				     include /* content */);
		}

		if (folder->excludes) {
			g_hash_table_foreach (folder->excludes,
					      add_excludes_to_xml,
					      folder_node);
		}

		if (folder->query) {
			xmlNode *query_node;
			query_node = xmlNewChild (folder_node /* parent */,
						  NULL /* ns */,
						  "Query" /* name */,
						  NULL /* content */);
			add_xml_tree_from_query (query_node, 
						 folder_get_query (folder));
		}
	}

	for (li = folder_list_subfolders (folder); li != NULL; li = li->next) {
		Folder *subfolder = li->data;
		add_xml_tree_from_folder (folder_node, subfolder);
	}
}

static xmlDoc *
xml_tree_from_vfolder (VFolderInfo *info)
{
	xmlDoc *doc;
	xmlNode *topnode;
	GSList *li;

	doc = xmlNewDoc ("1.0");

	topnode = xmlNewDocNode (doc /* doc */,
				 NULL /* ns */,
				 "VFolderInfo" /* name */,
				 NULL /* content */);
	doc->xmlRootNode = topnode;

	if (info->write_dir != NULL) {
		xmlNewChild (topnode /* parent */,
			     NULL /* ns */,
			     "WriteDir" /* name */,
			     info->write_dir /* content */);
	}

	/* Deprecated */
	if (info->desktop_dir != NULL) {
		xmlNewChild (topnode /* parent */,
			     NULL /* ns */,
			     "DesktopDir" /* name */,
			     info->desktop_dir /* content */);
	}
	
	for (li = info->item_dirs; li != NULL; li = li->next) {
		ItemDir *item_dir = li->data;

		switch (item_dir->type) {
		case MERGE_DIR:
			xmlNewChild (topnode /* parent */,
				     NULL /* ns */,
				     "MergeDir" /* name */,
				     item_dir->uri /* content */);
			break;
		case ITEM_DIR:
			xmlNewChild (topnode /* parent */,
				     NULL /* ns */,
				     "ItemDir" /* name */,
				     item_dir->uri /* content */);
			break;
		}
	}

	if (info->root != NULL)
		add_xml_tree_from_folder (topnode, info->root);

	return doc;
}

/* FIXME: what to do about errors */
void
vfolder_info_write_user (VFolderInfo *info)
{
	xmlDoc *doc;
	GnomeVFSResult result;
	gchar *tmpfile;
	struct timeval tv;

	if (info->loading || !info->dirty)
		return;

	if (!info->filename)
		return;

	info->loading = TRUE;

	/* FIXME: errors, anyone? */
	result = vfolder_make_directory_and_parents (info->filename, 
						     TRUE, 
						     0700);
	if (result != GNOME_VFS_OK) {
		g_warning ("Unable to create parent directory for "
			   "vfolder-info file: %s",
			   info->filename);
		return;
	}

	doc = xml_tree_from_vfolder (info);
	if (!doc)
		return;

	gettimeofday (&tv, NULL);
	tmpfile = g_strdup_printf ("%s.tmp-%d", 
				   info->filename,
				   (int) (tv.tv_sec ^ tv.tv_usec));

	/* Write to temporary file */
	xmlSaveFormatFile (tmpfile, doc, TRUE /* format */);

	/* Avoid being notified of move, since we're performing it */
	if (info->filename_monitor)
		vfolder_monitor_freeze (info->filename_monitor);

	/* Move temp file over to real filename */
	result = gnome_vfs_move (tmpfile, 
				 info->filename, 
				 TRUE /*force_replace*/);
	if (result != GNOME_VFS_OK) {
		g_warning ("Error writing vfolder configuration "
			   "file \"%s\": %s.",
			   info->filename,
			   gnome_vfs_result_to_string (result));
	}

	/* Start listening to changes again */
	if (info->filename_monitor)
		vfolder_monitor_thaw (info->filename_monitor);

	xmlFreeDoc(doc);
	g_free (tmpfile);

	info->modification_time = time (NULL);
	info->dirty = FALSE;
	info->loading = FALSE;
}


/* 
 * XML VFolder description reading
 */
static Query *
single_query_read (xmlNode *qnode)
{
	Query *query;
	xmlNode *node;

	if (qnode->type != XML_ELEMENT_NODE || qnode->name == NULL)
		return NULL;

	query = NULL;

	if (g_ascii_strcasecmp (qnode->name, "Not") == 0 &&
	    qnode->xmlChildrenNode != NULL) {
		xmlNode *iter;

		for (iter = qnode->xmlChildrenNode;
		     iter != NULL && query == NULL;
		     iter = iter->next)
			query = single_query_read (iter);
		if (query != NULL) {
			query->not = ! query->not;
		}
		return query;
	} 
	else if (g_ascii_strcasecmp (qnode->name, "Keyword") == 0) {
		xmlChar *word = xmlNodeGetContent (qnode);

		if (word != NULL) {
			query = query_new (QUERY_KEYWORD);
			query->val.keyword = g_quark_from_string (word);
			xmlFree (word);
		}
		return query;
	} 
	else if (g_ascii_strcasecmp (qnode->name, "Filename") == 0) {
		xmlChar *file = xmlNodeGetContent (qnode);

		if (file != NULL) {
			query = query_new (QUERY_FILENAME);
			query->val.filename = g_strdup (file);
			xmlFree (file);
		}
		return query;
	} 
	else if (g_ascii_strcasecmp (qnode->name, "ParentQuery") == 0) {
		query = query_new (QUERY_PARENT);
	}
	else if (g_ascii_strcasecmp (qnode->name, "And") == 0) {
		query = query_new (QUERY_AND);
	} 
	else if (g_ascii_strcasecmp (qnode->name, "Or") == 0) {
		query = query_new (QUERY_OR);
	} 
	else {
		/* We don't understand */
		return NULL;
	}

	/* This must be OR or AND */
	g_assert (query != NULL);

	for (node = qnode->xmlChildrenNode; node; node = node->next) {
		Query *new_query = single_query_read (node);

		if (new_query != NULL)
			query->val.queries = 
				g_slist_prepend (query->val.queries, new_query);
	}

	query->val.queries = g_slist_reverse (query->val.queries);

	return query;
}

static void
add_or_set_query (Query **query, Query *new_query)
{
	if (*query == NULL) {
		*query = new_query;
	} else {
		Query *old_query = *query;
		*query = query_new (QUERY_OR);
		(*query)->val.queries = 
			g_slist_append ((*query)->val.queries, old_query);
		(*query)->val.queries = 
			g_slist_append ((*query)->val.queries, new_query);
	}
}

static Query *
query_read (xmlNode *qnode)
{
	Query *query;
	xmlNode *node;

	query = NULL;

	for (node = qnode->xmlChildrenNode; node != NULL; node = node->next) {
		if (node->type != XML_ELEMENT_NODE ||
		    node->name == NULL)
			continue;

		if (g_ascii_strcasecmp (node->name, "Not") == 0 &&
		    node->xmlChildrenNode != NULL) {
			xmlNode *iter;
			Query *new_query = NULL;

			for (iter = node->xmlChildrenNode;
			     iter != NULL && new_query == NULL;
			     iter = iter->next)
				new_query = single_query_read (iter);
			if (new_query != NULL) {
				new_query->not = ! new_query->not;
				add_or_set_query (&query, new_query);
			}
		} else {
			Query *new_query = single_query_read (node);
			if (new_query != NULL)
				add_or_set_query (&query, new_query);
		}
	}

	return query;
}

static Folder *
folder_read (VFolderInfo *info, gboolean user_private, xmlNode *fnode)
{
	Folder *folder;
	xmlNode *node;

	folder = folder_new (info, NULL, user_private);

	for (node = fnode->xmlChildrenNode; node != NULL; node = node->next) {
		if (node->type != XML_ELEMENT_NODE ||
		    node->name == NULL)
			continue;

		if (g_ascii_strcasecmp (node->name, "Name") == 0) {
			xmlChar *name = xmlNodeGetContent (node);

			if (name) {
				g_free (folder->name);
				folder_set_name (folder, name);
				xmlFree (name);
			}
		} 
		else if (g_ascii_strcasecmp (node->name, "Parent") == 0) {
			xmlChar *parent = xmlNodeGetContent (node);

			if (parent) {
				gchar *esc_parent;

				esc_parent = vfolder_escape_home (parent);
				folder_set_extend_uri (folder, esc_parent);
				folder->is_link = FALSE;

				xmlFree (parent);
				g_free (esc_parent);
			}
		} 
		else if (g_ascii_strcasecmp (node->name, "ParentLink") == 0) {
			xmlChar *parent = xmlNodeGetContent (node);

			if (parent) {
				gchar *esc_parent;

				esc_parent = vfolder_escape_home (parent);
				folder_set_extend_uri (folder, esc_parent);
				folder->is_link = TRUE;
				
				xmlFree (parent);
				g_free (esc_parent);
			}
		} 
		else if (g_ascii_strcasecmp (node->name, "Desktop") == 0) {
			xmlChar *desktop = xmlNodeGetContent (node);

			if (desktop) {
				folder_set_desktop_file (folder, desktop);
				xmlFree (desktop);
			}
		} 
		else if (g_ascii_strcasecmp (node->name, "Include") == 0) {
			xmlChar *file = xmlNodeGetContent (node);

			if (file) {
				gchar *esc_file;

				esc_file = vfolder_escape_home (file);
				folder_add_include (folder, esc_file);

				xmlFree (file);
				g_free (esc_file);
			}
		}
		else if (g_ascii_strcasecmp (node->name, "Exclude") == 0) {
			xmlChar *file = xmlNodeGetContent (node);

			if (file) {
				gchar *esc_file;

				esc_file = vfolder_escape_home (file);
				folder_add_exclude (folder, esc_file);

				xmlFree (file);
				g_free (esc_file);
			}
		} 
		else if (g_ascii_strcasecmp (node->name, "Query") == 0) {
			Query *query;

			query = query_read (node);
			if (query)
				folder_set_query (folder, query);
		} 
		else if (g_ascii_strcasecmp (node->name, "Folder") == 0) {
			Folder *new_folder = folder_read (info, 
							  user_private,
							  node);

			if (new_folder != NULL) {
				folder_add_subfolder (folder, new_folder);
				folder_unref (new_folder);
			}
		} 
		else if (g_ascii_strcasecmp (node->name, 
					     "OnlyUnallocated") == 0) {
			folder->only_unallocated = TRUE;
			info->has_unallocated_folder = TRUE;
		} 
		else if (g_ascii_strcasecmp (node->name, "ReadOnly") == 0) {
			folder->read_only = TRUE;
		} 
		else if (g_ascii_strcasecmp (node->name,
					     "DontShowIfEmpty") == 0) {
			folder->dont_show_if_empty = TRUE;
		}
	}

	/* Name is required */
	if (!folder_get_name (folder)) {
		folder_unref (folder);
		return NULL;
	}

	return folder;
}

static void itemdir_monitor_cb (GnomeVFSMonitorHandle    *handle,
				const gchar              *monitor_uri,
				const gchar              *info_uri,
				GnomeVFSMonitorEventType  event_type,
				gpointer                  user_data);

static void writedir_monitor_cb (GnomeVFSMonitorHandle    *handle,
				 const gchar              *monitor_uri,
				 const gchar              *info_uri,
				 GnomeVFSMonitorEventType  event_type,
				 gpointer                  user_data);

static void desktopdir_monitor_cb (GnomeVFSMonitorHandle    *handle,
				   const gchar              *monitor_uri,
				   const gchar              *info_uri,
				   GnomeVFSMonitorEventType  event_type,
				   gpointer                  user_data);


static char *
remove_double_slashes (const char *uri)
{
	const char *src;
	char *dest;
	char *result;
	gboolean slash;

	if (uri == NULL) {
		return NULL;
	}

	result = malloc (strlen (uri) + 1);
	if (result == NULL) {
		return NULL;
	}

	src = uri;
	dest = result;
	slash = FALSE;

	while (*src != '\0') {
		/* Don't do anything if current char is a / and slash is TRUE*/
		if ((*src == '/') && (slash != FALSE)) {
			src++;
			continue;
		}

		if ((*src == '/') && (slash == FALSE)) {
			slash = TRUE;

		} else {
			slash = FALSE;
		}

		*dest = *src;
		dest++;
		src++;
	}
	*dest = '\0';

	return result;
}

static ItemDir *
itemdir_new (VFolderInfo *info, 
	     const gchar *uri, 
	     ItemDirType  type,
	     gint         weight)
{
	ItemDir *ret;
	gchar *tmp_uri;

	ret = g_new0 (ItemDir, 1);
	ret->info   = info;
	ret->weight = weight;
	tmp_uri = vfolder_escape_home (uri);
	ret->uri    = remove_double_slashes (tmp_uri);
	g_free (tmp_uri);
	ret->type   = type;

	info->item_dirs = g_slist_append (info->item_dirs, ret);

	return ret;
}

static void
itemdir_free (ItemDir *itemdir)
{
	GSList *iter;

	for (iter = itemdir->monitors; iter; iter = iter->next) {
		VFolderMonitor *monitor = iter->data;
		vfolder_monitor_cancel (monitor);
	}

	g_slist_free (itemdir->monitors);
	g_free (itemdir->uri);
	g_free (itemdir);
}

static gboolean
read_vfolder_from_file (VFolderInfo     *info,
			const gchar     *filename,
			gboolean         user_private,
			GnomeVFSResult  *result,
			GnomeVFSContext *context)
{
	xmlDoc *doc;
	xmlNode *node;
	GnomeVFSResult my_result;
	gint weight = 700;

	if (result == NULL)
		result = &my_result;

	/* Fail silently if filename does not exist */
	if (access (filename, F_OK) != 0)
		return TRUE;

	doc = xmlParseFile (filename); 
	if (doc == NULL
	    || doc->xmlRootNode == NULL
	    || doc->xmlRootNode->name == NULL
	    || g_ascii_strcasecmp (doc->xmlRootNode->name, 
				   "VFolderInfo") != 0) {
		*result = GNOME_VFS_ERROR_WRONG_FORMAT;
		xmlFreeDoc(doc);
		return FALSE;
	}

	if (context != NULL && 
	    gnome_vfs_context_check_cancellation (context)) {
		xmlFreeDoc(doc);
		*result = GNOME_VFS_ERROR_CANCELLED;
		return FALSE;
	}

	for (node = doc->xmlRootNode->xmlChildrenNode; 
	     node != NULL; 
	     node = node->next) {
		if (node->type != XML_ELEMENT_NODE ||
		    node->name == NULL)
			continue;

		if (context != NULL && 
		    gnome_vfs_context_check_cancellation (context)) {
			xmlFreeDoc(doc);
			*result = GNOME_VFS_ERROR_CANCELLED;
			return FALSE;
		}

		if (g_ascii_strcasecmp (node->name, "MergeDir") == 0) {
			xmlChar *dir = xmlNodeGetContent (node);

			if (dir != NULL) {
				itemdir_new (info, dir, MERGE_DIR, weight--);
				xmlFree (dir);
			}
		} 
		else if (g_ascii_strcasecmp (node->name, "ItemDir") == 0) {
			xmlChar *dir = xmlNodeGetContent (node);

			if (dir != NULL) {
				itemdir_new (info, dir, ITEM_DIR, weight--);
				xmlFree (dir);
			}
		} 
		else if (g_ascii_strcasecmp (node->name, "WriteDir") == 0) {
			xmlChar *dir = xmlNodeGetContent (node);

			if (dir != NULL) {
				g_free (info->write_dir);
				info->write_dir = vfolder_escape_home (dir);
				xmlFree (dir);
			}
		} 
		else if (g_ascii_strcasecmp (node->name, "DesktopDir") == 0) {
			xmlChar *dir = xmlNodeGetContent (node);

			if (dir != NULL) {
				g_free (info->desktop_dir);
				info->desktop_dir = vfolder_escape_home (dir);
				xmlFree (dir);
			}
		} 
		else if (g_ascii_strcasecmp (node->name, "Folder") == 0) {
			Folder *folder = folder_read (info, 
						      user_private,
						      node);

			if (folder != NULL) {
				if (info->root != NULL)
					folder_unref (info->root);

				info->root = folder;
			}
		} 
		else if (g_ascii_strcasecmp (node->name, "ReadOnly") == 0) {
			info->read_only = TRUE;
		}
	}

	xmlFreeDoc(doc);

	return TRUE;
}


/*
 * MergeDir/ItemDir entry pool reading 
 */
struct {
	const gchar *dirname;
	const gchar *keyword;
} mergedir_keywords[] = {
	 /*Parent Dir*/  /*Keyword to add*/

	/* Gnome Menus */
	{ "Development",  "Development" },
	{ "Editors",      "TextEditor" },
	{ "Games",        "Game" },
	{ "Graphics",     "Graphics" },
	{ "Internet",     "Network" },
	{ "Multimedia",   "AudioVideo" },
	{ "Office",       "Office" },
	{ "Settings",     "Settings" },
	{ "System",       "System" },
	{ "Utilities",    "Utility" },

	/* Ximian Menus */
	{ "Addressbook",  "Office" },
	{ "Audio",        "AudioVideo" },
	{ "Calendar",     "Office" },
	{ "Finance",      "Office" },

	/* KDE Menus */
	{ "WordProcessing", "Office" },
	{ "Toys",           "Utility" },
};

static GQuark
get_mergedir_keyword (const gchar *dirname)
{
	gint i;

	for (i = 0; i < G_N_ELEMENTS (mergedir_keywords); i++) {
		if (g_ascii_strcasecmp (mergedir_keywords [i].dirname, 
					dirname) == 0) {
			return g_quark_from_static_string (
					mergedir_keywords [i].keyword);
		}
	}

	return 0;
}

static Entry *
create_itemdir_entry (ItemDir          *id, 
		      const gchar      *rel_path,
		      GnomeVFSFileInfo *file_info)
{
	Entry *new_entry = NULL;
	gchar *file_uri;
	
	if (!vfolder_check_extension (file_info->name, ".desktop")) 
		return NULL;

	if (vfolder_info_lookup_entry (id->info, file_info->name)) {
		D (g_print ("EXCLUDING DUPLICATE ENTRY: %s\n", 
			    file_info->name));
		return NULL;
	}

	file_uri = vfolder_build_uri (id->uri, rel_path, NULL);

	/* Ref belongs to the VFolderInfo */
	new_entry = entry_new (id->info, 
			       file_uri        /*filename*/, 
			       file_info->name /*displayname*/, 
			       FALSE           /*user_private*/,
			       id->weight      /*weight*/);

	g_free (file_uri);

	return new_entry;
}

static void
add_keywords_from_relative_path (Entry *new_entry, const gchar *rel_path)
{
	gchar **pelems;
	GQuark keyword;
	gint i;

	pelems = g_strsplit (rel_path, "/", -1);
	if (!pelems)
		return;

	for (i = 0; pelems [i]; i++) {
		keyword = get_mergedir_keyword (pelems [i]);
		if (keyword)
			entry_add_implicit_keyword (new_entry, keyword);
	}

	g_strfreev (pelems);
}

static void
set_mergedir_entry_keywords (Entry *new_entry, const gchar *rel_path)
{
	static GQuark merged = 0, application = 0, core_quark = 0;

	if (!merged) {
		merged = g_quark_from_static_string ("Merged");
		application = g_quark_from_static_string("Application");
		core_quark = g_quark_from_static_string ("Core");
	}

	/* 
	 * Mergedirs have the 'Merged' and 'Appliction' keywords added.
	 */
	entry_add_implicit_keyword (new_entry, merged);
	entry_add_implicit_keyword (new_entry, application);

	if (!strcmp (rel_path, entry_get_displayname (new_entry)))
		entry_add_implicit_keyword (new_entry, core_quark);
	else
		add_keywords_from_relative_path (new_entry, rel_path);
}

static Entry *
create_mergedir_entry (ItemDir          *id,
		       const gchar      *rel_path,
		       GnomeVFSFileInfo *file_info)
{
	Entry *new_entry;

	new_entry = create_itemdir_entry (id, rel_path, file_info);
	if (new_entry)
		set_mergedir_entry_keywords (new_entry, rel_path);

	return new_entry;
}

static Entry *
create_entry_or_add_dir_monitor (ItemDir          *id,
				 const gchar      *rel_path,
				 GnomeVFSFileInfo *file_info)
{
	VFolderMonitor *dir_monitor;
	Entry *ret = NULL;
	gchar *file_uri;	

	if (file_info->type == GNOME_VFS_FILE_TYPE_DIRECTORY) {
		/* Add monitor for subdirectory of this MergeDir/ItemDir */
		file_uri = vfolder_build_uri (id->uri, rel_path, NULL);
		dir_monitor = vfolder_monitor_dir_new (file_uri, 
						       itemdir_monitor_cb, 
						       id);
		if (dir_monitor)
			id->monitors = g_slist_prepend (id->monitors, 
							dir_monitor);
		g_free (file_uri);
	} 
	else {
		switch (id->type) {
		case MERGE_DIR:
			ret = create_mergedir_entry (id, rel_path, file_info);
			break;
		case ITEM_DIR:
			ret = create_itemdir_entry (id, rel_path, file_info);
			break;
		}
	}

	return ret;
}

static gboolean
create_entry_directory_visit_cb (const gchar      *rel_path,
				 GnomeVFSFileInfo *file_info,
				 gboolean          recursing_will_loop,
				 gpointer          user_data,
				 gboolean         *recurse)
{
	ItemDir *id = user_data;

	create_entry_or_add_dir_monitor (id, rel_path, file_info);

	*recurse = !recursing_will_loop;
	return TRUE;
}

static gboolean
vfolder_info_read_info (VFolderInfo     *info,
			GnomeVFSResult  *result,
			GnomeVFSContext *context)
{
	gboolean ret = FALSE;
	GSList *iter;

	if (!info->filename)
		return FALSE;

	/* Don't let set_dirty write out the file */
	info->loading = TRUE;

	ret = read_vfolder_from_file (info, 
				      info->filename, 
				      TRUE,
				      result, 
				      context);
	if (ret) {
		if (info->write_dir)
			info->write_dir_monitor = 
				vfolder_monitor_dir_new (info->write_dir,
							 writedir_monitor_cb,
							 info);

		if (info->desktop_dir)
			info->desktop_dir_monitor = 
				vfolder_monitor_dir_new (info->desktop_dir,
							 desktopdir_monitor_cb,
							 info);

		/* Load ItemDir/MergeDirs in order of appearance. */
		for (iter = info->item_dirs; iter; iter = iter->next) {
			ItemDir *id = iter->data;
			VFolderMonitor *dir_monitor;

			/* Add a monitor for the root directory */
			dir_monitor = 
				vfolder_monitor_dir_new (id->uri, 
							 itemdir_monitor_cb, 
							 id);
			if (dir_monitor)
				id->monitors = g_slist_prepend (id->monitors, 
								dir_monitor);

			gnome_vfs_directory_visit (
				id->uri,
				GNOME_VFS_FILE_INFO_DEFAULT,
				GNOME_VFS_DIRECTORY_VISIT_DEFAULT,
				create_entry_directory_visit_cb,
				id);
		}
	}

	/* Allow set_dirty to write config file again */
	info->loading = FALSE;

	return ret;
}		     

static void
vfolder_info_reset (VFolderInfo *info)
{
	GSList *iter;

	info->loading = TRUE;
	
	if (info->filename_monitor) {
		vfolder_monitor_cancel (info->filename_monitor);
		info->filename_monitor = NULL;
	}

	if (info->write_dir_monitor) {
		vfolder_monitor_cancel (info->write_dir_monitor);
		info->write_dir_monitor = NULL;
	}

	for (iter = info->item_dirs; iter; iter = iter->next) {
		ItemDir *dir = iter->data;
		itemdir_free (dir);
	}
	g_slist_free (info->item_dirs);
	info->item_dirs = NULL;

	g_free (info->filename);
	g_free (info->write_dir);
	g_free (info->desktop_dir);

	info->filename = NULL;
	info->desktop_dir = NULL;
	info->write_dir = NULL;

	folder_unref (info->root);
	info->root = NULL;

	g_slist_foreach (info->entries, (GFunc) entry_unref, NULL);
	g_slist_free (info->entries);
	info->entries = NULL;

	if (info->entries_ht) {
		g_hash_table_destroy (info->entries_ht);
		info->entries_ht = NULL;
	}

	/* Clear flags */
	info->read_only =
		info->dirty = 
		info->loading =
		info->has_unallocated_folder = FALSE;
}


/* 
 * VFolder ItemDir/MergeDir/WriteDir/DesktopDir directory monitoring
 */
static void
integrate_entry (Folder *folder, Entry *entry, gboolean do_add)
{
	const GSList *subs;
	Entry *existing;
	Query *query;
	gboolean matches = FALSE;

	for (subs = folder_list_subfolders (folder); subs; subs = subs->next) {
		Folder *asub = subs->data;
		integrate_entry (asub, entry, do_add);
	}

	if (folder->only_unallocated)
		return;

	query = folder_get_query (folder);
	if (query)
		matches = query_try_match (query, folder, entry);

	existing = folder_get_entry (folder, entry_get_displayname (entry));
	if (existing) {
		/* 
		 * Do nothing if the existing entry has a higher weight than the
		 * one we wish to add.
		 */
		if (entry_get_weight (existing) > entry_get_weight (entry))
			return;
		
		folder_remove_entry (folder, existing);

		if (do_add && matches) {
			folder_add_entry (folder, entry);

			folder_emit_changed (folder, 
					     entry_get_displayname (entry),
					     GNOME_VFS_MONITOR_EVENT_CHANGED);
		} else 
			folder_emit_changed (folder, 
					     entry_get_displayname (entry),
					     GNOME_VFS_MONITOR_EVENT_DELETED);
	} 
	else if (do_add && matches) {
		folder_add_entry (folder, entry);

		folder_emit_changed (folder, 
				     entry_get_displayname (entry),
				     GNOME_VFS_MONITOR_EVENT_CREATED);
	}
}

static void
integrate_itemdir_entry_createupdate (ItemDir                  *id,
				      GnomeVFSURI              *full_uri,
				      const gchar              *full_uristr,
				      const gchar              *displayname,
				      GnomeVFSMonitorEventType  event_type)
{
	Entry *entry;
	GnomeVFSURI *real_uri;
	const gchar *rel_path;

	rel_path  = strstr (full_uristr, id->uri);
	g_assert (rel_path != NULL);
	rel_path += strlen (id->uri);

	/* Look for an existing entry with the same displayname */
	entry = vfolder_info_lookup_entry (id->info, displayname);
	if (entry) {
		real_uri = entry_get_real_uri (entry);

		if (gnome_vfs_uri_equal (full_uri, real_uri)) {
			/* Refresh */
			entry_set_dirty (entry);
		} 
		else if (entry_get_weight (entry) < id->weight) {
			/* 
			 * Existing entry is less important than the new
			 * one, so replace.
			 */
			entry_set_filename (entry, full_uristr);
			entry_set_weight (entry, id->weight);
			
			if (id->type == MERGE_DIR) {
				/* Add keywords from relative path */
				set_mergedir_entry_keywords (entry, rel_path);
			}
		}

		gnome_vfs_uri_unref (real_uri);
	} 
	else if (event_type == GNOME_VFS_MONITOR_EVENT_CREATED) {
		GnomeVFSFileInfo *file_info;
		GnomeVFSResult result;

		file_info = gnome_vfs_file_info_new ();

		result = 
			gnome_vfs_get_file_info_uri (
				full_uri,
				file_info,
				GNOME_VFS_FILE_INFO_DEFAULT);

		if (result == GNOME_VFS_OK)
			entry = create_entry_or_add_dir_monitor (id,
								 rel_path,
								 file_info);
		
		gnome_vfs_file_info_unref (file_info);
	}

	if (entry) {
		entry_ref (entry);
		integrate_entry (id->info->root, 
				 entry, 
				 TRUE /* do_add */);
		entry_unref (entry);

		id->info->modification_time = time (NULL);
	}
}

static gboolean
find_replacement_for_delete (ItemDir *id, Entry *entry)
{
	GSList *iter, *miter;
	gint idx;
	
	idx = g_slist_index (id->info->item_dirs, id);
	if (idx < 0)
		return FALSE;

	iter = g_slist_nth (id->info->item_dirs, idx + 1);

	for (; iter; iter = iter->next) {
		ItemDir *id_next = iter->data;

		for (miter = id_next->monitors; miter; miter = miter->next) {
			VFolderMonitor *monitor = miter->data;
			GnomeVFSURI *check_uri;
			gchar *uristr, *rel_path;
			gboolean exists;

			uristr = 
				vfolder_build_uri (
					monitor->uri,
					entry_get_displayname (entry),
					NULL);

			check_uri = gnome_vfs_uri_new (uristr);
			exists = gnome_vfs_uri_exists (check_uri);
			gnome_vfs_uri_unref (check_uri);

			if (!exists) {
				g_free (uristr);
				continue;
			}

			entry_set_filename (entry, uristr);
			entry_set_weight (entry, id_next->weight);

			if (id_next->type == MERGE_DIR) {
				rel_path  = strstr (uristr, id_next->uri);
				rel_path += strlen (id_next->uri);

				/* Add keywords based on relative path */
				set_mergedir_entry_keywords (entry, rel_path);
			}

			g_free (uristr);
			return TRUE;
		}
	}

	return FALSE;
}

static void
integrate_itemdir_entry_delete (ItemDir                  *id,
				GnomeVFSURI              *full_uri,
				const gchar              *displayname)
{
	Entry *entry;
	GnomeVFSURI *real_uri;
	gboolean replaced, equal;

	entry = vfolder_info_lookup_entry (id->info, displayname);
	if (!entry)
		return;

	real_uri = entry_get_real_uri (entry);
	equal = gnome_vfs_uri_equal (full_uri, real_uri);
	gnome_vfs_uri_unref (real_uri);

	/* Only care if its the currently visible entry being deleted */
	if (!equal)
		return;

	replaced = find_replacement_for_delete (id, entry);

	entry_ref (entry);
	integrate_entry (id->info->root, entry, replaced /* do_add */);
	entry_unref (entry);

	id->info->modification_time = time (NULL);
}

static void
itemdir_monitor_cb (GnomeVFSMonitorHandle    *handle,
		    const gchar              *monitor_uri,
		    const gchar              *info_uri,
		    GnomeVFSMonitorEventType  event_type,
		    gpointer                  user_data)
{
	ItemDir *id = user_data;
	gchar *filename;
	GnomeVFSURI *uri;

	D (g_print ("*** Itemdir '%s' monitor %s%s%s called! ***\n", 
		    info_uri,
		    event_type == GNOME_VFS_MONITOR_EVENT_CREATED ? "CREATED":"",
		    event_type == GNOME_VFS_MONITOR_EVENT_DELETED ? "DELETED":"",
		    event_type == GNOME_VFS_MONITOR_EVENT_CHANGED ? "CHANGED":""));

	/* Operating on the whole directory, ignore */
	if (!strcmp (monitor_uri, info_uri) ||
	    !vfolder_check_extension (info_uri, ".desktop"))
		return;

	uri = gnome_vfs_uri_new (info_uri);
	filename = gnome_vfs_uri_extract_short_name (uri);

	switch (event_type) {
	case GNOME_VFS_MONITOR_EVENT_CREATED:
	case GNOME_VFS_MONITOR_EVENT_CHANGED:
		VFOLDER_INFO_WRITE_LOCK (id->info);
		integrate_itemdir_entry_createupdate (id,
						      uri,
						      info_uri,
						      filename,
						      event_type);
		VFOLDER_INFO_WRITE_UNLOCK (id->info);
		break;
	case GNOME_VFS_MONITOR_EVENT_DELETED:
		VFOLDER_INFO_WRITE_LOCK (id->info);
		integrate_itemdir_entry_delete (id, uri, filename);
		VFOLDER_INFO_WRITE_UNLOCK (id->info);
		break;
	default:
		break;
	}

	gnome_vfs_uri_unref (uri);
	g_free (filename);
}

static void
integrate_writedir_entry_changed (Folder      *folder, 
				  gchar       *displayname,
				  GnomeVFSURI *changed_uri)
{
	Entry *entry;
	GnomeVFSURI *real_uri;
	const GSList *subs;

	entry = folder_get_entry (folder, displayname);
	if (entry) {
		real_uri = entry_get_real_uri (entry);

		if (gnome_vfs_uri_equal (real_uri, changed_uri)) {
			entry_set_dirty (entry);
			folder_emit_changed (folder, 
					     displayname,
					     GNOME_VFS_MONITOR_EVENT_CHANGED);
		}

		gnome_vfs_uri_unref (real_uri);
	}

	for (subs = folder_list_subfolders (folder); subs; subs = subs->next) {
		Folder *asub = subs->data;
		integrate_writedir_entry_changed (asub, 
						  displayname, 
						  changed_uri);
	}
}

static void 
writedir_monitor_cb (GnomeVFSMonitorHandle    *handle,
		     const gchar              *monitor_uri,
		     const gchar              *info_uri,
		     GnomeVFSMonitorEventType  event_type,
		     gpointer                  user_data)
{
	VFolderInfo *info = user_data;
	GnomeVFSURI *uri;
	gchar *filename, *filename_ts;

	/* Operating on the whole directory, ignore */
	if (!strcmp (monitor_uri, info_uri) ||
	    (!vfolder_check_extension (info_uri, ".desktop") && 
	     !vfolder_check_extension (info_uri, ".directory")))
		return;

	switch (event_type) {
	case GNOME_VFS_MONITOR_EVENT_CHANGED:
		uri = gnome_vfs_uri_new (info_uri);
		filename_ts = gnome_vfs_uri_extract_short_name (uri);
		filename = vfolder_untimestamp_file_name (filename_ts);

		VFOLDER_INFO_WRITE_LOCK (info);
		integrate_writedir_entry_changed (info->root, filename, uri);
		VFOLDER_INFO_WRITE_UNLOCK (info);

		gnome_vfs_uri_unref (uri);
		g_free (filename_ts);
		g_free (filename);
		break;
	case GNOME_VFS_MONITOR_EVENT_DELETED:
	case GNOME_VFS_MONITOR_EVENT_CREATED:
	default:
		break;
	}
}

static void 
desktopdir_monitor_cb (GnomeVFSMonitorHandle    *handle,
		       const gchar              *monitor_uri,
		       const gchar              *info_uri,
		       GnomeVFSMonitorEventType  event_type,
		       gpointer                  user_data)
{
	VFolderInfo *info = user_data;
	GnomeVFSURI *uri;

	/* Operating on the whole directory, ignore */
	if (!strcmp (monitor_uri, info_uri) ||
	    !vfolder_check_extension (info_uri, ".directory"))
		return;

	switch (event_type) {
	case GNOME_VFS_MONITOR_EVENT_CHANGED:
		uri = gnome_vfs_uri_new (info_uri);

		VFOLDER_INFO_WRITE_LOCK (info);
		integrate_writedir_entry_changed (info->root, 
						  ".directory", 
						  uri);
		VFOLDER_INFO_WRITE_UNLOCK (info);

		gnome_vfs_uri_unref (uri);
		break;
	case GNOME_VFS_MONITOR_EVENT_DELETED:
	case GNOME_VFS_MONITOR_EVENT_CREATED:
	default:
		break;
	}
}


/* 
 * .vfolder-info monitoring
 */
static void
check_monitors_foreach (gpointer key, gpointer val, gpointer user_data)
{
	MonitorHandle *handle = key;
	GSList *children = val;
	GnomeVFSURI *uri, *curi;
	const gchar *path;

	uri = handle->uri;
	path = gnome_vfs_uri_get_path (handle->uri);

	if (handle->type == GNOME_VFS_MONITOR_DIRECTORY) {
		Folder *folder;
		GSList *new_children, *iter, *found;

		folder = vfolder_info_get_folder (handle->info, path);
		if (!folder) {
			gnome_vfs_monitor_callback (
				(GnomeVFSMethodHandle *) handle,
				handle->uri,
				GNOME_VFS_MONITOR_EVENT_DELETED);
			return;
		}

		/* 
		 * FIXME: If someone has an <OnlyUnallocated> folder which also
		 *        has a <Query>, we won't receive change events for
		 *        children matching the query... I think this is corner
		 *        enough to ignore * though.  
		 */
		if (folder->only_unallocated)
			return;

		new_children = folder_list_children (folder);

		for (iter = children; iter; iter = iter->next) {
			gchar *child_name = iter->data;

			/* Look for a child with the same name */
			found = g_slist_find_custom (new_children,
						     child_name,
						     (GCompareFunc) strcmp);
			if (found) {
				g_free (found->data);
				new_children = 
					g_slist_delete_link (new_children, 
							     found);
			} else {
				curi = 
					gnome_vfs_uri_append_file_name (
						handle->uri, 
						child_name);

				gnome_vfs_monitor_callback (
					(GnomeVFSMethodHandle *) handle,
					curi,
				        GNOME_VFS_MONITOR_EVENT_DELETED);

				gnome_vfs_uri_unref (curi);
			}

			g_free (child_name);
		}

		/* Whatever is left is new, send created events */
		for (iter = new_children; iter; iter = iter->next) {
			gchar *child_name = iter->data;

			curi = gnome_vfs_uri_append_file_name (handle->uri, 
							       child_name);

			gnome_vfs_monitor_callback (
				(GnomeVFSMethodHandle *) handle,
				curi,
				GNOME_VFS_MONITOR_EVENT_CREATED);

			gnome_vfs_uri_unref (curi);
			g_free (child_name);
		}

		g_slist_free (new_children);
		g_slist_free (children);
	} 
	else {
		gboolean found;

		found = vfolder_info_get_entry (handle->info, path) ||
			vfolder_info_get_folder (handle->info, path);

		gnome_vfs_monitor_callback (
			(GnomeVFSMethodHandle *) handle,
			handle->uri,
			found ?
			        GNOME_VFS_MONITOR_EVENT_CHANGED :
			        GNOME_VFS_MONITOR_EVENT_DELETED);
	}
}

static gboolean vfolder_info_init (VFolderInfo *info);

static gboolean
filename_monitor_handle (gpointer user_data)
{
	VFolderInfo *info = user_data;
	GHashTable *monitors;
	GSList *iter;

	D (g_print ("*** PROCESSING .vfolder-info!!! ***\n"));
	
	monitors = g_hash_table_new (g_direct_hash, g_direct_equal);

	VFOLDER_INFO_WRITE_LOCK (info);

	/* Don't emit any events while we load */
	info->loading = TRUE;

	/* Compose a hash of all existing monitors and their children */
	for (iter = info->requested_monitors; iter; iter = iter->next) {
		MonitorHandle *mhandle = iter->data;
		GSList *monitored_paths = NULL;
		Folder *folder;

		if (mhandle->type == GNOME_VFS_MONITOR_DIRECTORY) {
			folder = 
				vfolder_info_get_folder (
					info, 
					gnome_vfs_uri_get_path (mhandle->uri));
			if (folder)
				monitored_paths = folder_list_children (folder);
		}

		g_hash_table_insert (monitors, mhandle, monitored_paths);
	}

	vfolder_info_reset (info);
	vfolder_info_init (info);

	/* Start sending events again */
	info->loading = FALSE;

	/* Traverse monitor hash and diff with newly read folder structure */
	g_hash_table_foreach (monitors, check_monitors_foreach, info);

	VFOLDER_INFO_WRITE_UNLOCK (info);

	g_hash_table_destroy (monitors);

	info->filename_reload_tag = 0;
	return FALSE;
}

static void
filename_monitor_cb (GnomeVFSMonitorHandle *handle,
		     const gchar *monitor_uri,
		     const gchar *info_uri,
		     GnomeVFSMonitorEventType event_type,
		     gpointer user_data)
{
	VFolderInfo *info = user_data;

	D (g_print ("*** Filename '%s' monitor %s%s%s called! ***\n",
		    info_uri,
		    event_type == GNOME_VFS_MONITOR_EVENT_CREATED ? "CREATED":"",
		    event_type == GNOME_VFS_MONITOR_EVENT_DELETED ? "DELETED":"",
		    event_type == GNOME_VFS_MONITOR_EVENT_CHANGED ? "CHANGED":""));

	if (info->filename_reload_tag) {
		g_source_remove (info->filename_reload_tag);
		info->filename_reload_tag = 0;
	}

	/* 
	 * Don't process the .vfolder-info for 2 seconds after a delete event or
	 * .5 seconds after a create event.  This allows files to be rewritten
	 * before we start reading it and possibly copying the system default
	 * file over top of it.  
	 */
	switch (event_type) {
	case GNOME_VFS_MONITOR_EVENT_DELETED:
		info->filename_reload_tag = 
			g_timeout_add (2000, filename_monitor_handle, info);
		break;
	case GNOME_VFS_MONITOR_EVENT_CREATED:
		info->filename_reload_tag = 
			g_timeout_add (500, filename_monitor_handle, info);
		break;
	case GNOME_VFS_MONITOR_EVENT_CHANGED:
	default:
		filename_monitor_handle (info);
		break;
	}
}


/* 
 * VFolderInfo Implementation
 */
static VFolderInfo *
vfolder_info_new (const char *scheme)
{
	VFolderInfo *info;

	info = g_new0 (VFolderInfo, 1);
	info->scheme = g_strdup (scheme);

	g_static_rw_lock_init (&info->rw_lock);

	return info;
}

static void
vfolder_info_find_filenames (VFolderInfo *info)
{
	gchar *scheme = info->scheme;
	GnomeVFSURI *file_uri;
	gboolean exists;

	/* 
	 * FIXME: load from gconf 
	 */

	/* 
	 * 1st: Try mandatory system-global file located at
	 * /etc/gnome-vfs-2.0/vfolders/scheme.vfolder-info.  Writability will
	 * depend on permissions of this file.
	 */
	info->filename = g_strconcat (SYSCONFDIR,
				      "/gnome-vfs-2.0/vfolders/",
				      scheme, ".vfolder-info",
				      NULL);
	file_uri = gnome_vfs_uri_new (info->filename);

	exists = gnome_vfs_uri_exists (file_uri);
	gnome_vfs_uri_unref (file_uri);

	if (!exists) {
		/* 
		 * 2nd: Try user-private ~/.gnome2/vfolders/scheme.vfolder-info 
		 */
		g_free (info->filename);
		info->filename = g_strconcat (g_get_home_dir (),
					      "/" DOT_GNOME "/vfolders/",
					      scheme, ".vfolder-info",
					      NULL);
	}

	/* 
	 * Special case for applications-all-users where we want to add any
	 * paths specified in $GNOME2_PATH, for people installing in strange
	 * places.
	 */
	if (!strcmp (scheme, "applications-all-users")) {
		int i;
		const char *path;
		char *dir, **ppath;
		ItemDir *id;
		int weight = 800;

		path = g_getenv ("GNOME2_PATH");
		if (path) {
			ppath = g_strsplit (path, ":", -1);

			for (i = 0; ppath[i] != NULL; i++) {
				dir = g_build_filename (ppath[i], 
							"/share/applications/",
							NULL);
				id = itemdir_new (info, 
						  dir, 
						  ITEM_DIR,
						  weight--);
				g_free (dir);
			}

			g_strfreev (ppath);
		}
	}
}

static gboolean
g_str_case_equal (gconstpointer v1,
		  gconstpointer v2)
{
	const gchar *string1 = v1;
	const gchar *string2 = v2;
  
	return g_ascii_strcasecmp (string1, string2) == 0;
}

/* 31 bit hash function */
static guint
g_str_case_hash (gconstpointer key)
{
	const char *p = key;
	guint h = g_ascii_toupper (*p);
	
	if (h)
		for (p += 1; *p != '\0'; p++)
			h = (h << 5) - h + g_ascii_toupper (*p);

	return h;
}

static gboolean
vfolder_info_init (VFolderInfo *info)
{
	gchar *all_user_scheme;

	info->loading = TRUE;
	info->entries_ht = g_hash_table_new (g_str_case_hash, g_str_case_equal);
	info->root = folder_new (info, "Root", TRUE);

	/* 
	 * Set the extend uri for the root folder to the -all-users version of
	 * the scheme, in case the user doesn't have a private .vfolder-info
	 * file yet.  
	 */
	all_user_scheme = g_strconcat (info->scheme, "-all-users:///", NULL);
	folder_set_extend_uri (info->root, all_user_scheme);
	g_free (all_user_scheme);

	/* 
	 * Set the default writedir, in case there is no .vfolder-info for this
	 * scheme yet.  Otherwise this will be overwritten when we read our
	 * source.
	 */
	info->write_dir = g_strconcat (g_get_home_dir (),
				       "/" DOT_GNOME "/vfolders/",
				       info->scheme,
				       NULL);

	/* Figure out which .vfolder-info to read */
	vfolder_info_find_filenames (info);

	if (g_getenv ("GNOME_VFS_VFOLDER_INFODIR")) {
		gchar *filename = g_strconcat (info->scheme, 
					       ".vfolder-info", 
					       NULL);

		g_free (info->filename);
		info->filename = 
			vfolder_build_uri (
				g_getenv ("GNOME_VFS_VFOLDER_INFODIR"),
				filename,
				NULL);
		g_free (filename);
	}

	if (g_getenv ("GNOME_VFS_VFOLDER_WRITEDIR")) {
		g_free (info->write_dir);
		info->write_dir = 
			vfolder_build_uri (
				g_getenv ("GNOME_VFS_VFOLDER_WRITEDIR"),
				info->scheme,
				NULL);
	}

	info->filename_monitor = 
		vfolder_monitor_file_new (info->filename,
					  filename_monitor_cb,
					  info);

	info->modification_time = time (NULL);
	info->loading = FALSE;
	info->dirty = FALSE;

	/* Read from the user's .vfolder-info if it exists */
	return vfolder_info_read_info (info, NULL, NULL);
}

static void
vfolder_info_destroy (VFolderInfo *info)
{
	if (info == NULL)
		return;

	vfolder_info_reset (info);

	if (info->filename_reload_tag)
		g_source_remove (info->filename_reload_tag);

	g_static_rw_lock_free (&info->rw_lock);

	g_free (info->scheme);

	while (info->requested_monitors) {
		GnomeVFSMethodHandle *monitor = info->requested_monitors->data;
		vfolder_info_cancel_monitor (monitor);
	}

	g_free (info);
}

/* 
 * Call to recursively list folder contents, causing them to allocate entries,
 * so that we get OnlyUnallocated folder counts correctly.
 */
static void
load_folders (Folder *folder)
{
	const GSList *iter;

	for (iter = folder_list_subfolders (folder); iter; iter = iter->next) {
		Folder *folder = iter->data;
		load_folders (folder);
	}
}

static GHashTable *infos = NULL;
G_LOCK_DEFINE_STATIC (vfolder_lock);

VFolderInfo *
vfolder_info_locate (const gchar *scheme)
{
	VFolderInfo *info = NULL;

	G_LOCK (vfolder_lock);

	if (!infos) {
		infos = 
			g_hash_table_new_full (
				g_str_hash, 
				g_str_equal,
				NULL,
				(GDestroyNotify) vfolder_info_destroy);
	}

	info = g_hash_table_lookup (infos, scheme);
	if (info) {
		G_UNLOCK (vfolder_lock);
		return info;
	}
	else {
		info = vfolder_info_new (scheme);
		g_hash_table_insert (infos, info->scheme, info);

		VFOLDER_INFO_WRITE_LOCK (info);
		G_UNLOCK (vfolder_lock);

		if (!vfolder_info_init (info)) {
			D (g_print ("DESTROYING INFO FOR SCHEME: %s\n", 
				    scheme));

			G_LOCK (vfolder_lock);
			g_hash_table_remove (infos, info);
			G_UNLOCK (vfolder_lock);

			return NULL;
		}
			
		if (info->has_unallocated_folder) {
			info->loading = TRUE;
			load_folders (info->root);
			info->loading = FALSE;
		}

		VFOLDER_INFO_WRITE_UNLOCK (info);
		return info;
	}
}

void
vfolder_info_set_dirty (VFolderInfo *info)
{
	if (info->loading)
		return;

	info->dirty = TRUE;
}

static Folder *
get_folder_for_path_list_n (Folder    *parent, 
			    gchar    **paths, 
			    gint       path_index,
			    gboolean   skip_last) 
{
	Folder *child;
	gchar *subname, *subsubname;

	if (!parent || folder_is_hidden (parent))
		return NULL;

	subname = paths [path_index];
	if (!subname)
		return parent;

	subsubname = paths [path_index + 1];
	if (!subsubname && skip_last)
		return parent;

	if (*subname == '\0')
		child = parent;
	else
		child = folder_get_subfolder (parent, subname);

	return get_folder_for_path_list_n (child, 
					   paths, 
					   path_index + 1, 
					   skip_last);
}

static Folder *
get_folder_for_path (Folder *root, const gchar *path, gboolean skip_last) 
{
	gchar **paths;
	Folder *folder;

	paths = g_strsplit (path, "/", -1);
	if (!paths)
		return NULL;

	folder = get_folder_for_path_list_n (root, paths, 0, skip_last);

	g_strfreev (paths);
	
	return folder;
}

Folder *
vfolder_info_get_parent (VFolderInfo *info, const gchar *path)
{
	return get_folder_for_path (info->root, path, TRUE);
}

Folder *
vfolder_info_get_folder (VFolderInfo *info, const gchar *path)
{
	return get_folder_for_path (info->root, path, FALSE);
}

Entry *
vfolder_info_get_entry (VFolderInfo *info, const gchar *path)
{
	Folder *parent;
	gchar *subname;

	parent = vfolder_info_get_parent (info, path);
	if (!parent)
		return NULL;

	subname = strrchr (path, '/');
	if (!subname)
		return NULL;
	else
		subname++;

	return folder_get_entry (parent, subname);
}

const GSList *
vfolder_info_list_all_entries (VFolderInfo *info)
{
	return info->entries;
}

Entry *
vfolder_info_lookup_entry (VFolderInfo *info, const gchar *name)
{
	return g_hash_table_lookup (info->entries_ht, name);
}

void 
vfolder_info_add_entry (VFolderInfo *info, Entry *entry)
{
	info->entries = g_slist_prepend (info->entries, entry);
	g_hash_table_insert (info->entries_ht, 
			     (gchar *) entry_get_displayname (entry),
			     entry);
}

void 
vfolder_info_remove_entry (VFolderInfo *info, Entry *entry)
{
	info->entries = g_slist_remove (info->entries, entry);
	g_hash_table_remove (info->entries_ht, 
			     entry_get_displayname (entry));
}

#ifdef VFOLDER_DEBUG
#define DEBUG_CHANGE_EMIT(_change_uri, _handle_uri)                         \
	g_print ("EMITTING CHANGE: %s for %s, %s%s%s\n",                    \
		 _change_uri,                                               \
		 _handle_uri,                                               \
		 event_type==GNOME_VFS_MONITOR_EVENT_CREATED?"CREATED":"",  \
		 event_type==GNOME_VFS_MONITOR_EVENT_DELETED?"DELETED":"",  \
		 event_type==GNOME_VFS_MONITOR_EVENT_CHANGED?"CHANGED":"")
#else
#define DEBUG_CHANGE_EMIT(_change_uri, _handle_uri)
#endif

void 
vfolder_info_emit_change (VFolderInfo              *info,
			  const char               *path,
			  GnomeVFSMonitorEventType  event_type)
{
	GSList *iter;
	GnomeVFSURI *uri;
	gchar *escpath, *uristr;

	if (info->loading) 
		return;

	escpath = gnome_vfs_escape_path_string (path);
	uristr = g_strconcat (info->scheme, "://", escpath, NULL);
	uri = gnome_vfs_uri_new (uristr);

	for (iter = info->requested_monitors; iter; iter = iter->next) {
		MonitorHandle *handle = iter->data;

		if (gnome_vfs_uri_equal (uri, handle->uri) ||
		    (handle->type == GNOME_VFS_MONITOR_DIRECTORY &&
		     gnome_vfs_uri_is_parent (handle->uri, 
					      uri, 
					      FALSE))) {
			DEBUG_CHANGE_EMIT (uristr, handle->uri->text);

			gnome_vfs_monitor_callback (
				(GnomeVFSMethodHandle *) handle,
				uri,
				event_type);
		}
	}

	gnome_vfs_uri_unref (uri);
	g_free (escpath);
	g_free (uristr);
}

void
vfolder_info_add_monitor (VFolderInfo           *info,
			  GnomeVFSMonitorType    type,
			  GnomeVFSURI           *uri,
			  GnomeVFSMethodHandle **handle)
{
	MonitorHandle *monitor = g_new0 (MonitorHandle, 1);
	monitor->info = info;
	monitor->type = type;

	monitor->uri = uri;
	gnome_vfs_uri_ref (uri);

	info->requested_monitors = g_slist_prepend (info->requested_monitors,
						    monitor);

	D (g_print ("EXTERNALLY WATCHING: %s\n", 
		    gnome_vfs_uri_to_string (uri, 0)));
	
	*handle = (GnomeVFSMethodHandle *) monitor;
}

void 
vfolder_info_cancel_monitor (GnomeVFSMethodHandle  *handle)
{
	MonitorHandle *monitor = (MonitorHandle *) handle;

	monitor->info->requested_monitors = 
		g_slist_remove (monitor->info->requested_monitors, monitor);

	gnome_vfs_uri_unref (monitor->uri);
	g_free (monitor);
}

void
vfolder_info_destroy_all (void)
{
	G_LOCK (vfolder_lock);

	if (infos) {
		g_hash_table_destroy (infos);
		infos = NULL;
	}

	G_UNLOCK (vfolder_lock);
}

void
vfolder_info_dump_entries (VFolderInfo *info, int offset)
{
	g_slist_foreach (info->entries, 
			 (GFunc) entry_dump, 
			 GINT_TO_POINTER (offset));
}
