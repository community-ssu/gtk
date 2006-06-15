/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* 
 * vfolder-common.h - Abstract Folder, Entry, Query, and VFolderInfo 
 *                    interfaces.
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

#ifndef VFOLDER_COMMON_H
#define VFOLDER_COMMON_H

#include <glib.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-method.h>

#include "vfolder-util.h"

G_BEGIN_DECLS

/* Turn this on to test the non-FAM monitoring code
 */
#undef VFOLDER_DEBUG_WITHOUT_MONITORING

/* Turn this on to spew debugging info
 */
#undef VFOLDER_DEBUG

#ifdef VFOLDER_DEBUG
#define D(x) x
#else
#define D(x) do {} while (0)
#endif

typedef struct _VFolderInfo VFolderInfo;
typedef struct _Query Query;
typedef struct _Folder Folder;

/* 
 * Entry API
 */
typedef struct {
	gushort         refcnt;
	gushort         allocs;

	/* 
	 * Weight lets us determine if a monitor's file created (or changed) 
	 * event should replace an existing visible entry.  Higher weights 
	 * are more valuable:
	 * 	1000 - WriteDir entries
	 *  	 900 - folder parent entries
	 * 	 800 - GNOME2_PATH entries in order
	 * 	 700 - MergeDir/ItemDir entries in order
	 */
	gushort         weight;  

	VFolderInfo    *info;

	char           *displayname;
	char           *filename;
	GnomeVFSURI    *uri;

	GSList         *keywords;          /* GQuark */
	GSList         *implicit_keywords; /* GQuark */	

	guint           dirty : 1;
	guint           user_private : 1;
} Entry;

Entry        *entry_new                  (VFolderInfo *info,
					  const gchar *filename,
					  const gchar *displayname,
					  gboolean     user_private,
					  gushort      weight);

void          entry_ref                  (Entry *entry);
void          entry_unref                (Entry *entry);

void          entry_alloc                (Entry *entry);
void          entry_dealloc              (Entry *entry);
gboolean      entry_is_allocated         (Entry *entry);

gboolean      entry_make_user_private    (Entry *entry, Folder *folder);
gboolean      entry_is_user_private      (Entry *entry);

gushort       entry_get_weight           (Entry *entry);
void          entry_set_weight           (Entry *entry, gushort weight);

void          entry_set_dirty            (Entry *entry);

void          entry_set_filename         (Entry *entry, const gchar *name);
const gchar  *entry_get_filename         (Entry *entry);

void          entry_set_displayname      (Entry *entry, const gchar *name);
const gchar  *entry_get_displayname      (Entry *entry);

GnomeVFSURI  *entry_get_real_uri         (Entry *entry);

const GSList *entry_get_keywords         (Entry *entry);
void          entry_add_implicit_keyword (Entry *entry, GQuark keyword);

void          entry_quick_read_keys      (Entry        *entry,
					  const gchar  *key1,
					  gchar       **value1,
					  const gchar  *key2,
					  gchar       **value2);

void          entry_dump                 (Entry *entry, int indent);


/* 
 * Folder API
 */
struct _Folder {
	gint               refcnt;

	VFolderInfo       *info;

	char              *name;

	gchar             *extend_uri;
	VFolderMonitor    *extend_monitor;

	Folder            *parent;

	char              *desktop_file;     /* the .directory file */

	Query             *query;

	/* The following is for per file access */
	GHashTable        *excludes;         /* excluded by dirname/fileuri */
	GSList            *includes;         /* included by dirname/fileuri */

	GSList            *subfolders;
	GHashTable        *subfolders_ht;

	GSList            *entries;
	GHashTable        *entries_ht;

	/* Some flags */
	guint              read_only : 1;
	guint              dont_show_if_empty : 1;
	guint              only_unallocated : 1; /* include only unallocated */
	guint              is_link : 1;

	guint              has_user_private_subfolders : 1;
	guint              user_private : 1;

	/* lazily done, will run query only when it needs to */
	guint              dirty : 1;
	guint              loading : 1;
	guint              sorted : 1;
};

typedef struct {
	enum {
		FOLDER = 1,
		DESKTOP_FILE,
		UNKNOWN_URI
	} type;
	
	Folder      *folder;
	Entry       *entry;
	GnomeVFSURI *uri;
} FolderChild;

Folder       *folder_new               (VFolderInfo *info, 
					const gchar *name,
					gboolean     user_private);

void          folder_ref               (Folder *folder);
void          folder_unref             (Folder *folder);

gboolean      folder_make_user_private (Folder *folder);
gboolean      folder_is_user_private   (Folder *folder);

void          folder_set_dirty         (Folder *folder);

void          folder_set_name          (Folder *folder, const gchar *name);
const gchar  *folder_get_name          (Folder *folder);

void          folder_set_query         (Folder *folder, Query *query);
Query        *folder_get_query         (Folder *folder);

void          folder_set_extend_uri    (Folder *folder, const gchar *uri);
const gchar  *folder_get_extend_uri    (Folder *folder);

void          folder_set_desktop_file  (Folder *folder, const gchar *filename);
const gchar  *folder_get_desktop_file  (Folder *folder);

gboolean      folder_get_child         (Folder      *folder, 
					const gchar *name,
					FolderChild *child);
GSList       *folder_list_children     (Folder      *folder);

Entry        *folder_get_entry         (Folder *folder, const gchar *filename);
const GSList *folder_list_entries      (Folder *folder);
void          folder_remove_entry      (Folder *folder, Entry *entry);
void          folder_add_entry         (Folder *folder, Entry *entry);

void          folder_add_include       (Folder *folder, const gchar *file);
void          folder_remove_include    (Folder *folder, const gchar *file);

void          folder_add_exclude       (Folder *folder, const gchar *file);
void          folder_remove_exclude    (Folder *folder, const gchar *file);

Folder       *folder_get_subfolder     (Folder *folder, const gchar *name);
const GSList *folder_list_subfolders   (Folder *folder);
void          folder_remove_subfolder  (Folder *folder, Folder *sub);
void          folder_add_subfolder     (Folder *folder, Folder *sub);

gboolean      folder_is_hidden         (Folder *folder);

void          folder_dump_tree         (Folder *folder, int indent);

void          folder_emit_changed      (Folder                   *folder,
					const gchar              *child,
					GnomeVFSMonitorEventType  event_type);


/* 
 * Query API
 */
struct _Query {
	enum {
		QUERY_OR,
		QUERY_AND,
		QUERY_PARENT,
		QUERY_KEYWORD,
		QUERY_FILENAME
	} type;
	union {
		GSList   *queries;
		GQuark    keyword;
		gchar    *filename;
	} val;
	guint not : 1;
};

Query    *query_new (int type);

void      query_free (Query *query);

gboolean  query_try_match (Query  *query,
			   Folder *folder,
			   Entry  *efile);

/* 
 * VFolderInfo API
 */
/* NOTE: Exposed only so do_monitor_cancel can lock the VFolderInfo */
typedef struct {
	GnomeVFSURI         *uri;
	GnomeVFSMonitorType  type;
	VFolderInfo         *info;
} MonitorHandle;

struct _VFolderInfo {
	GStaticRWLock   rw_lock;

	char           *scheme;

	char           *filename;
	VFolderMonitor *filename_monitor;
	guint           filename_reload_tag;

	/* dir where user changes to items are stored */
	char           *write_dir; 
	VFolderMonitor *write_dir_monitor;

	/* deprecated */
	char           *desktop_dir; /* directory with .directorys */
	VFolderMonitor *desktop_dir_monitor;

	/* Consider item dirs and mergedirs writeable?? */
	/* Monitoring on mergedirs?? */
	GSList         *item_dirs; 	    /* CONTAINS: ItemDir */

	GSList         *entries; 	    /* CONTAINS: Entry */
	GHashTable     *entries_ht; 	    /* KEY: Entry->name, VALUE: Entry */

	/* The root folder */
	Folder         *root;

	/* some flags */
	guint           read_only : 1;
	guint           dirty : 1;
	guint           loading : 1;
	guint           has_unallocated_folder : 1;

	GSList         *requested_monitors; /* CONTAINS: MonitorHandle */

	/* ctime for folders */
	time_t          modification_time;
};

#define VFOLDER_INFO_READ_LOCK(vi) \
	g_static_rw_lock_reader_lock (&(vi->rw_lock))
#define VFOLDER_INFO_READ_UNLOCK(vi) \
	g_static_rw_lock_reader_unlock (&(vi->rw_lock))

#define VFOLDER_INFO_WRITE_LOCK(vi) \
	g_static_rw_lock_writer_lock (&(vi->rw_lock))
/* Writes out .vfolder-info file if there are changes */
#define VFOLDER_INFO_WRITE_UNLOCK(vi)                   \
	vfolder_info_write_user (vi);                   \
	g_static_rw_lock_writer_unlock (&(vi->rw_lock))

VFolderInfo  *vfolder_info_locate           (const gchar *scheme);

void          vfolder_info_set_dirty        (VFolderInfo *info);
void          vfolder_info_write_user       (VFolderInfo *info);

Folder       *vfolder_info_get_folder       (VFolderInfo *info, 
					     const gchar *path);
Folder       *vfolder_info_get_parent       (VFolderInfo *info, 
					     const gchar *path);
Entry        *vfolder_info_get_entry        (VFolderInfo *info, 
					     const gchar *path);

const GSList *vfolder_info_list_all_entries (VFolderInfo *info);
Entry        *vfolder_info_lookup_entry     (VFolderInfo *info, 
					     const gchar *name);
void          vfolder_info_add_entry        (VFolderInfo *info, Entry *entry);
void          vfolder_info_remove_entry     (VFolderInfo *info, Entry *entry);

void          vfolder_info_emit_change      (VFolderInfo              *info,
					     const char               *path,
					     GnomeVFSMonitorEventType  event_type);

void          vfolder_info_add_monitor      (VFolderInfo           *info,
					     GnomeVFSMonitorType    type,
					     GnomeVFSURI           *path,
					     GnomeVFSMethodHandle **handle);
void          vfolder_info_cancel_monitor   (GnomeVFSMethodHandle  *handle);

void          vfolder_info_destroy_all      (void);

void          vfolder_info_dump_entries     (VFolderInfo *info, int offset);

G_END_DECLS

#endif /* VFOLDER_COMMON_H */
