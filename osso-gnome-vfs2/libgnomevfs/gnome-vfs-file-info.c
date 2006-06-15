/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-file-info.c - Handling of file information for the GNOME
   Virtual File System.

   Copyright (C) 1999 Free Software Foundation

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Ettore Perazzoli <ettore@comm2000.it>
*/

#include <config.h>
#include "gnome-vfs-file-info.h"

#include <glib/gmessages.h>
#include <glib/gstrfuncs.h>
#include <glib/gthread.h>
#include <string.h>

/* Mutex for making GnomeVFSFileInfo ref's/unref's atomic */
/* Note that an atomic increment function (such as is present in NSPR) is preferable */
/* FIXME: This mutex is probably not needed and might be causing performance issues */
static GStaticMutex file_info_ref_lock = G_STATIC_MUTEX_INIT;

/* Register GnomeVFSFileInfo in the type system */
GType 
gnome_vfs_file_info_get_type (void) {
	static GType fi_type = 0;

	if (fi_type == 0) {
		fi_type = g_boxed_type_register_static ( "GnomeVFSFileInfo",
				(GBoxedCopyFunc) gnome_vfs_file_info_dup,
				(GBoxedFreeFunc) gnome_vfs_file_info_unref);
	}

	return fi_type;
}


/**
 * gnome_vfs_file_info_new
 * 
 * Allocate and initialize a new file information struct.
 * 
 * Returns: A pointer to the new file information struct.
 **/
GnomeVFSFileInfo *
gnome_vfs_file_info_new (void)
{
	GnomeVFSFileInfo *new;

	new = g_new0 (GnomeVFSFileInfo, 1);

	/* `g_new0()' is enough to initialize everything (we just want
           all the members to be set to zero).  */

	new->refcount = 1;
	
	return new;
}


/**
 * gnome_vfs_file_info_ref
 * @info: Pointer to a file information struct
 * 
 * Increment reference count
 **/
void
gnome_vfs_file_info_ref (GnomeVFSFileInfo *info)
{
	g_return_if_fail (info != NULL);
	g_return_if_fail (info->refcount > 0);

	g_static_mutex_lock (&file_info_ref_lock);
	info->refcount += 1;
	g_static_mutex_unlock (&file_info_ref_lock);
	
}

/**
 * gnome_vfs_file_info_unref
 * @info: Pointer to a file information struct
 * 
 * Destroy @info
 **/
void
gnome_vfs_file_info_unref (GnomeVFSFileInfo *info)
{
	g_return_if_fail (info != NULL);
	g_return_if_fail (info->refcount > 0);

	g_static_mutex_lock (&file_info_ref_lock);
	info->refcount -= 1;
	g_static_mutex_unlock (&file_info_ref_lock);

	if (info->refcount == 0) {
		gnome_vfs_file_info_clear (info);
		g_free (info);
	}
}


/**
 * gnome_vfs_file_info_clear
 * @info: Pointer to a file information struct
 * 
 * Clear @info so that it's ready to accept new data. This is
 * supposed to be used when @info already contains meaningful information which
 * we want to replace.
 **/
void
gnome_vfs_file_info_clear (GnomeVFSFileInfo *info)
{
	guint old_refcount;
	
	g_return_if_fail (info != NULL);

	g_free (info->name);
	g_free (info->symlink_name);
	g_free (info->mime_type);

	/* Ensure the ref count is maintained correctly */
	g_static_mutex_lock (&file_info_ref_lock);

	old_refcount = info->refcount;
	memset (info, 0, sizeof (*info));
	info->refcount = old_refcount;

	g_static_mutex_unlock (&file_info_ref_lock);
}


/**
 * gnome_vfs_file_info_get_mime_type
 * @info: A pointer to a file information struct
 * 
 * Retrieve MIME type from @info. There is no need to free the return
 * value.
 * 
 * Returns: A pointer to a string representing the MIME type.
 **/
const gchar *
gnome_vfs_file_info_get_mime_type (GnomeVFSFileInfo *info)
{
	g_return_val_if_fail (info != NULL, NULL);

	return info->mime_type;
}

/**
 * gnome_vfs_file_info_copy
 * @dest: Pointer to a struct to copy @src's information into
 * @src: Pointer to the information to be copied into @dest
 * 
 * Copy information from @src into @dest.
 **/
void
gnome_vfs_file_info_copy (GnomeVFSFileInfo *dest,
			  const GnomeVFSFileInfo *src)
{
	guint old_refcount;

	g_return_if_fail (dest != NULL);
	g_return_if_fail (src != NULL);

	/* The primary purpose of this lock is to guarentee that the
	 * refcount is correctly maintained, not to make the copy
	 * atomic.  If you want to make the copy atomic, you probably
	 * want serialize access differently (or perhaps you shouldn't
	 * use copy)
	 */
	g_static_mutex_lock (&file_info_ref_lock);

	old_refcount = dest->refcount;

	/* Copy basic information all at once; we will fix pointers later.  */

	memcpy (dest, src, sizeof (*src));

	/* Duplicate dynamically allocated strings.  */

	dest->name = g_strdup (src->name);
	dest->symlink_name = g_strdup (src->symlink_name);
	dest->mime_type = g_strdup (src->mime_type);

	dest->refcount = old_refcount;

	g_static_mutex_unlock (&file_info_ref_lock);

}

/**
 * gnome_vfs_file_info_dup:
 * @orig: Pointer to a file information structure to duplicate
 * 
 * Duplicates @orig and returns it.
 * 
 * Returns: a new file information struct that duplicates the information in @orig.
 **/
GnomeVFSFileInfo *
gnome_vfs_file_info_dup (const GnomeVFSFileInfo *orig)
{
	GnomeVFSFileInfo * ret;

	g_return_val_if_fail (orig != NULL, NULL);

	ret = gnome_vfs_file_info_new();

	gnome_vfs_file_info_copy (ret, orig);

	return ret;
}


static gboolean
mime_matches (char *a, char *b)
{
	if (a == NULL && b == NULL) {
		return TRUE;
	} else if ((a != NULL && b == NULL) ||
		   (a == NULL && b != NULL)) {
		return FALSE;
	} else {
		g_assert (a != NULL && b != NULL);
		return g_ascii_strcasecmp (a, b) == 0;
	}
}

static gboolean
symlink_name_matches (char *a, char *b)
{
	if (a == NULL && b == NULL) {
		return TRUE;
	} else if ((a != NULL && b == NULL) ||
		   (a == NULL && b != NULL)) {
		return FALSE;
	} else {
		g_assert (a != NULL && b != NULL);
		return strcmp (a, b) == 0;
	}
}

/**
 * gnome_vfs_file_info_matches
 * @a: first GnomeVFSFileInfo struct to compare
 * @b: second GnomeVFSFileInfo struct to compare
 *
 * Compare the two file info structs, return TRUE if they match exactly
 * the same file data.
 *
 * Returns: TRUE if the two GnomeVFSFileInfos match, otherwise return FALSE.
 **/
gboolean
gnome_vfs_file_info_matches (const GnomeVFSFileInfo *a,
			     const GnomeVFSFileInfo *b)
{
	g_return_val_if_fail (a != NULL, FALSE);
	g_return_val_if_fail (b != NULL, FALSE);
	g_return_val_if_fail (a->name != NULL, FALSE);
	g_return_val_if_fail (b->name != NULL, FALSE);

	/* This block of code assumes that the GnomeVfsFileInfo 
	   was correctly allocated with g_new0 which means that each pair
	   of fields are either invalid and equal to NULL or are valid 
	   If both GnomeVfsFileInfos have only invalid fields, the 
	   function returns TRUE. That is, it says the infos match which is,
	   in a sense, true :)
	*/

	if (a->type != b->type
	    || a->size != b->size
	    || a->block_count != b->block_count
	    || a->atime != b->atime
	    || a->mtime != b->mtime
	    || a->ctime != b->ctime
	    || a->flags != b->flags
	    || a->permissions != b->permissions
	    || a->device != b->device
	    || a->inode != b->inode
	    || a->link_count != b->link_count
	    || a->uid != b->uid
	    || a->gid != b->gid
	    || strcmp (a->name, b->name) != 0
	    || !mime_matches (a->mime_type, b->mime_type)
	    || !symlink_name_matches (a->symlink_name, b->symlink_name)) {
		return FALSE;
	} else {
		return TRUE;
	}
}

/**
 * gnome_vfs_file_info_list_ref:
 * @list: list of GnomeVFSFileInfo elements
 *
 * Increments the reference count of the items in @list by one.
 *
 * Return value: @list
 **/
GList *
gnome_vfs_file_info_list_ref (GList *list)
{
	g_list_foreach (list, (GFunc) gnome_vfs_file_info_ref, NULL);
	return list;
}

/**
 * gnome_vfs_file_info_list_unref:
 * @list: list of GnomeVFSFileInfo elements
 *
 * Decrements the reference count of the items in @list by one.
 * Note that the list is *not freed* even if each member of the list
 * is freed.
 *
 * Return value: @list
 **/
GList *
gnome_vfs_file_info_list_unref (GList *list)
{
	g_list_foreach (list, (GFunc) gnome_vfs_file_info_unref, NULL);
	return list;
}

/**
 * gnome_vfs_file_info_list_copy:
 * @list: list of GnomeVFSFileInfo elements
 *
 * Creates a duplicate of @list, and references each member of
 * that list.
 *
 * Return value: a newly referenced duplicate of @list
 **/
GList *
gnome_vfs_file_info_list_copy (GList *list)
{
	return g_list_copy (gnome_vfs_file_info_list_ref (list));
}

/**
 * gnome_vfs_file_info_list_free:
 * @list: list of GnomeVFSFileInfo elements
 *
 * Decrements the reference count of each member of @list by one,
 * and frees the list itself.
 **/
void
gnome_vfs_file_info_list_free (GList *list)
{
	g_list_free (gnome_vfs_file_info_list_unref (list));
}


/* Register GnomeVfsGetFileInfoResult into the GType system  */
GType
gnome_vfs_get_file_info_result_get_type (void)
{
	static GType our_type = 0;

	if (our_type == 0) {
	    our_type = g_boxed_type_register_static ("GnomeVfsGetFileInfoResult",
	        (GBoxedCopyFunc) gnome_vfs_get_file_info_result_dup,
	        (GBoxedFreeFunc) gnome_vfs_get_file_info_result_free);
	}
	    
	return our_type;
}
/**
 * gnome_vfs_get_file_info_result_dup:
 * @result: A #GnomeVFSGetFileInfoResult.
 * 
 * Duplicate @result.
 *
 * Note: The internal uri and file_ info objects are not duplicated
 * but their refcount is incremented by 1.
 *
 * Return value: A duplicated version of @result.
 *
 * Since: 2.12
 */
GnomeVFSGetFileInfoResult*
gnome_vfs_get_file_info_result_dup (GnomeVFSGetFileInfoResult *result)
{
	GnomeVFSGetFileInfoResult* copy;
	
	g_return_val_if_fail (result != NULL, NULL);

	copy = g_new0 (GnomeVFSGetFileInfoResult, 1);

	/* "Copy" and ref the objects: */
	copy->uri = result->uri;
	gnome_vfs_uri_ref (copy->uri);

	copy->result = result->result;

	copy->file_info = result->file_info;
	gnome_vfs_file_info_ref (copy->file_info);

	return copy;
}

/**
 * gnome_vfs_get_file_info_result_free:
 * @result: A #GnomeVFSGetFileInfoResult.
 *
 * Unrefs the internal uri and file_info objects and frees the
 * memory  allocated for @result.
 * 
 * Since: 2.12
 **/
void
gnome_vfs_get_file_info_result_free (GnomeVFSGetFileInfoResult *result)
{
	g_return_if_fail (result != NULL);
 
	gnome_vfs_uri_unref (result->uri);
	result->uri = NULL;

	gnome_vfs_file_info_unref (result->file_info);
	result->file_info = NULL;

	g_free (result);
}

