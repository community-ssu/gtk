/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * bonobo-storage-fs.c: Sample file-system based Storage implementation
 *
 * This is just a sample file-system based Storage implementation.
 * it is only used for debugging purposes
 *
 * Authors:
 *   Miguel de Icaza (miguel@gnu.org)
 *   Michael Meeks   (michael@ximian.com)
 *
 * Copyright 2001, Ximian, Inc
 */

#include <config.h>
#include "bonobo-storage-fs.h"

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <bonobo/bonobo-storage.h>

#include "bonobo-stream-fs.h"

static char *
concat_dir_and_file (const char *dir, const char *file)
{
	g_return_val_if_fail (dir != NULL, NULL);
	g_return_val_if_fail (file != NULL, NULL);

        /* If the directory name doesn't have a / on the end, we need
	   to add one so we get a proper path to the file */
	if (dir[0] != '\0' && !G_IS_DIR_SEPARATOR (dir [strlen(dir) - 1]))
		return g_strconcat (dir, G_DIR_SEPARATOR_S, file, NULL);
	else
		return g_strconcat (dir, file, NULL);
}

static BonoboObjectClass *bonobo_storage_fs_parent_class;

static void
bonobo_storage_fs_finalize (GObject *object)
{
	BonoboStorageFS *storage_fs = BONOBO_STORAGE_FS (object);

	g_free (storage_fs->path);
	storage_fs->path = NULL;

	G_OBJECT_CLASS (bonobo_storage_fs_parent_class)->finalize (object);
}

static Bonobo_StorageInfo*
fs_get_info (PortableServer_Servant storage,
	     const CORBA_char *path,
	     const Bonobo_StorageInfoFields mask,
	     CORBA_Environment *ev)
{
	BonoboStorageFS *storage_fs = BONOBO_STORAGE_FS (
		bonobo_object (storage));
	Bonobo_StorageInfo *si;
	struct stat st;
	char *full = NULL;
	gboolean dangling = FALSE;

	if (mask & ~(Bonobo_FIELD_CONTENT_TYPE | Bonobo_FIELD_SIZE |
		     Bonobo_FIELD_TYPE)) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_NotSupported, NULL);
		return CORBA_OBJECT_NIL;
	}

	full = concat_dir_and_file (storage_fs->path, path);
	if (g_stat (full, &st) == -1) {
		if (g_lstat (full, &st) == -1)
			goto get_info_except;
		else
			dangling = TRUE;
	}

	si = Bonobo_StorageInfo__alloc ();
	
	si->size = st.st_size;
	si->name = CORBA_string_dup (path);

	if (S_ISDIR (st.st_mode)) {
		si->type = Bonobo_STORAGE_TYPE_DIRECTORY;
		si->content_type = CORBA_string_dup ("x-directory/normal");
	} else {
		si->type = Bonobo_STORAGE_TYPE_REGULAR;
		if (dangling)
			si->content_type =
				CORBA_string_dup ("x-symlink/dangling");
		else
			si->content_type = 
				CORBA_string_dup (
					gnome_vfs_mime_type_from_name (full));
	}

	g_free (full);

	return si;

 get_info_except:

	if (full)
		g_free (full);
	
	if (errno == EACCES) 
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_NoPermission, 
				     NULL);
	else if (errno == ENOENT) 
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_NotFound, 
				     NULL);
	else 
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_IOError, NULL);
	
	return CORBA_OBJECT_NIL;
}

static void
fs_set_info (PortableServer_Servant         storage,
	     const CORBA_char              *path,
	     const Bonobo_StorageInfo      *info,
	     const Bonobo_StorageInfoFields mask,
	     CORBA_Environment             *ev)
{
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
			     ex_Bonobo_Storage_NotSupported, 
			     NULL);
}

static Bonobo_Stream
fs_open_stream (PortableServer_Servant  storage, 
		const CORBA_char       *path, 
		Bonobo_Storage_OpenMode mode, 
		CORBA_Environment      *ev)
{
	BonoboStorageFS *storage_fs = BONOBO_STORAGE_FS (
		bonobo_object (storage));
	BonoboObject *stream;
	char *full;

	full = concat_dir_and_file (storage_fs->path, path);
	stream = BONOBO_OBJECT (
		bonobo_stream_fs_open (full, mode, 0644, ev));
	g_free (full);

	return CORBA_Object_duplicate (
		BONOBO_OBJREF (stream), ev);
}

static Bonobo_Storage
fs_open_storage (PortableServer_Servant  storage,
		 const CORBA_char       *path, 
		 Bonobo_Storage_OpenMode mode,
		 CORBA_Environment      *ev)
{
	BonoboStorageFS *storage_fs = BONOBO_STORAGE_FS (
		bonobo_object (storage));
	BonoboObject *new_storage;
	char *full;

	full = concat_dir_and_file (storage_fs->path, path);
	new_storage = BONOBO_OBJECT (
		bonobo_storage_fs_open (full, mode, 0644, ev));
	g_free (full);

	return CORBA_Object_duplicate (
		BONOBO_OBJREF (new_storage), ev);
}

static void
fs_copy_to (PortableServer_Servant storage,
	    Bonobo_Storage         dest,
	    CORBA_Environment     *ev)
{
	BonoboStorageFS *storage_fs = BONOBO_STORAGE_FS (
		bonobo_object (storage));

	bonobo_storage_copy_to (
		BONOBO_OBJREF (storage_fs), dest, ev);
}

static void
fs_rename (PortableServer_Servant storage,
	   const CORBA_char      *path, 
	   const CORBA_char      *new_path,
	   CORBA_Environment     *ev)
{
	BonoboStorageFS *storage_fs = BONOBO_STORAGE_FS (
		bonobo_object (storage));
	char *full_old, *full_new;

	full_old = concat_dir_and_file (storage_fs->path, path);
	full_new = concat_dir_and_file (storage_fs->path, new_path);

	if (g_rename (full_old, full_new) == -1) {

		if ((errno == EACCES) || (errno == EPERM) || (errno == EROFS)) 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_NoPermission, 
					     NULL);
		else if (errno == ENOENT) 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_NotFound, 
					     NULL);
		else if ((errno == EEXIST) || (errno == ENOTEMPTY)) 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_NameExists, 
					     NULL);
		else 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_IOError, 
					     NULL);
	}

	g_free (full_old);
	g_free (full_new);
}

static void
fs_commit (PortableServer_Servant storage,
	   CORBA_Environment     *ev)
{
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_Bonobo_Stream_NotSupported, NULL);
}

static void
fs_revert (PortableServer_Servant storage,
	   CORBA_Environment     *ev)
{
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_Bonobo_Stream_NotSupported, NULL);
}

static Bonobo_Storage_DirectoryList *
fs_list_contents (PortableServer_Servant   storage,
		  const CORBA_char        *path, 
		  Bonobo_StorageInfoFields mask,
		  CORBA_Environment       *ev)
{
	BonoboStorageFS *storage_fs = BONOBO_STORAGE_FS (
		bonobo_object (storage));
	Bonobo_Storage_DirectoryList *list = NULL;
	Bonobo_StorageInfo *buf;
	struct stat st;
	GDir *dir = NULL;
	const gchar *entry;
	gint i, max, v, num_entries = 0;
	gchar *full = NULL;
	gchar *full_dir = NULL;

	if (mask & ~(Bonobo_FIELD_CONTENT_TYPE | Bonobo_FIELD_SIZE |
		     Bonobo_FIELD_TYPE)) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_NotSupported, NULL);
		return CORBA_OBJECT_NIL;
	}
         
	full_dir = concat_dir_and_file (storage_fs->path, path);
	if (!(dir = g_dir_open (full_dir, 0, NULL)))
	                {
			g_free (full_dir);
			goto list_contents_except;
			}
	
	for (max = 0; g_dir_read_name (dir); max++)
		/* Do nothing */;

	g_dir_rewind (dir);

	buf = CORBA_sequence_Bonobo_StorageInfo_allocbuf (max);
	list = Bonobo_Storage_DirectoryList__alloc();
	list->_buffer = buf;
	CORBA_sequence_set_release (list, TRUE); 
	
	for (i = 0; (entry = g_dir_read_name (dir)) && (i < max); i++) {
		
		if ((entry[0] == '.' && entry[1] == '\0') ||
		    (entry[0] == '.' && entry[1] == '.' 
		     && entry[2] == '\0')) {
			i--;
			continue; /* Ignore . and .. */
		}

		buf [i].name = CORBA_string_dup (entry);
		buf [i].size = 0;
		buf [i].content_type = NULL;

		full = concat_dir_and_file (full_dir, entry);
		v = g_stat (full, &st);

		if (v == -1) {
			/*
			 * The stat failed -- two common cases are where
			 * the file was removed between the call to readdir
			 * and the iteration, and where the file is a dangling
			 * symlink.
			 */
			if (errno == ENOENT
#ifdef ELOOP
					    || errno == ELOOP
#endif
							      ) {
				v = g_lstat (full, &st);
				if (v == 0) {
					/* FIXME - x-symlink/dangling is odd */
					buf [i].size = st.st_size;
					buf [i].type = Bonobo_STORAGE_TYPE_REGULAR;
					buf [i].content_type =
						CORBA_string_dup ("x-symlink/dangling");
					g_free (full);
					num_entries++;
					continue;
				}
			}

			/* Unless it's something grave, just skip the file */
			if (errno != ENOMEM && errno != EFAULT && errno != ENOTDIR) {
				i--;
				g_free (full);
				continue;
			}

			goto list_contents_except;
		}

		buf [i].size = st.st_size;
	
		if (S_ISDIR (st.st_mode)) { 
			buf [i].type = Bonobo_STORAGE_TYPE_DIRECTORY;
			buf [i].content_type = 
				CORBA_string_dup ("x-directory/normal");
		} else { 
			buf [i].type = Bonobo_STORAGE_TYPE_REGULAR;
			buf [i].content_type = 
				CORBA_string_dup (
					gnome_vfs_mime_type_from_name (full));
		}

		g_free (full);

		num_entries++;
	}

	list->_length = num_entries; 

	g_dir_close (dir);
	g_free (full_dir);
	
	return list; 

 list_contents_except:

	if (dir)
		g_dir_close (dir);

	if (list) 
		CORBA_free (list);

	if (full)
		g_free (full);
	
	if (errno == ENOENT) 
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_NotFound, 
				     NULL);
	else if (errno == ENOTDIR) 
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_NotStorage, 
				     NULL);
	else 
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_IOError, NULL);
	
	return CORBA_OBJECT_NIL;
}

static void
fs_erase (PortableServer_Servant storage,
	  const CORBA_char      *path,
	  CORBA_Environment     *ev)
{
	BonoboStorageFS *storage_fs = BONOBO_STORAGE_FS (
		bonobo_object (storage));
	char *full;

	full = concat_dir_and_file (storage_fs->path, path);

	if (g_remove (full) == -1) {

		if (errno == ENOENT) 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_NotFound, 
					     NULL);
		else if (errno == ENOTEMPTY || errno == EEXIST)
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_NotEmpty, 
					     NULL);
		else if (errno == EACCES || errno == EPERM)
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_NoPermission, 
					     NULL);
		else 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_IOError, NULL);
	}

	g_free (full);
}

static void
bonobo_storage_fs_class_init (BonoboStorageFSClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	POA_Bonobo_Storage__epv *epv = &klass->epv;
	
	bonobo_storage_fs_parent_class = 
		g_type_class_peek_parent (klass);

	epv->getInfo       = fs_get_info;
	epv->setInfo       = fs_set_info;
	epv->openStream    = fs_open_stream;
	epv->openStorage   = fs_open_storage;
	epv->copyTo        = fs_copy_to;
	epv->rename        = fs_rename;
	epv->commit        = fs_commit;
	epv->revert        = fs_revert;
	epv->listContents  = fs_list_contents;
	epv->erase         = fs_erase;
	
	object_class->finalize = bonobo_storage_fs_finalize;
}


static void 
bonobo_storage_fs_init (GObject *object)
{
	/* nothing to do */
}

BONOBO_TYPE_FUNC_FULL (BonoboStorageFS,
		       Bonobo_Storage,
		       bonobo_object_get_type (),
		       bonobo_storage_fs);

/** 
 * bonobo_storage_fs_open:
 * @path: path to existing directory that represents the storage
 * @flags: open flags.
 * @mode: mode used if @flags containst Bonobo_Storage_CREATE for the storage.
 *
 * Returns a BonoboStorage object that represents the storage at @path
 */
BonoboStorageFS *
bonobo_storage_fs_open (const char *path, gint flags,
			gint mode, CORBA_Environment *ev)
{
	BonoboStorageFS *storage_fs;
	struct stat st;
	
	g_return_val_if_fail (path != NULL, NULL);
	g_return_val_if_fail (ev != NULL, NULL);

	/* Most storages are files */
	mode = mode | 0111;

	if ((flags & Bonobo_Storage_CREATE) &&
	    (g_mkdir (path, mode) == -1) && (errno != EEXIST)) {

		if (errno == EACCES) 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_NoPermission, 
					     NULL);
		else 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_IOError, NULL);
		return NULL;
	}

	if (g_stat (path, &st) == -1) {
		
		if (errno == ENOENT) 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_NotFound, 
					     NULL);
		else 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_IOError,
					     NULL);
		return NULL;
	}

	if (!S_ISDIR (st.st_mode)) { 
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_NotStorage, NULL);
		return NULL;
	}

	storage_fs = g_object_new (bonobo_storage_fs_get_type (), NULL);
	storage_fs->path = g_strdup (path);

	return storage_fs;
}
