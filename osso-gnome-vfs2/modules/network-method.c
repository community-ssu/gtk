/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* network-method.c - The

   Copyright (C) 2003 Red Hat, Inc

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

   Authors:
         Alexander Larsson <alexl@redhat.com>
 */

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <gconf/gconf-client.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-directory.h>
#include <libgnomevfs/gnome-vfs-module-shared.h>
#include <libgnomevfs/gnome-vfs-module.h>
#include <libgnomevfs/gnome-vfs-volume-monitor.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-dns-sd.h>
#include <libgnomevfs/gnome-vfs-monitor-private.h>

#include <glib/gi18n-lib.h>

#define PATH_GCONF_GNOME_VFS_SMB "/system/smb"
#define PATH_GCONF_GNOME_VFS_SMB_WORKGROUP "/system/smb/workgroup"
#define PATH_GCONF_GNOME_VFS_DNS_SD "/system/dns_sd"
#define PATH_GCONF_GNOME_VFS_DNS_SD_DISPLAY_LOCAL "/system/dns_sd/display_local"
#define PATH_GCONF_GNOME_VFS_DNS_SD_EXTRA_DOMAINS "/system/dns_sd/extra_domains"
#define DEFAULT_WORKGROUP_NAME "X-GNOME-DEFAULT-WORKGROUP"

typedef struct {
	char *display_name;
	char *icon;
	char *target_uri;
        char *filename;
} NetworkLink;

typedef struct {
	char *prefix;
	GnomeVFSURI *base_uri;
	GnomeVFSMonitorHandle *monitor_handle;
} NetworkRedirect;

typedef struct {
	char *file_name; /* not escaped */
        char *contents;
} NetworkFile;

typedef struct {
	int dummy;
} NetworkMonitor;

typedef enum {
	NETWORK_LOCAL_DISABLED,
	NETWORK_LOCAL_MERGED,
	NETWORK_LOCAL_SEPARATE
} NetworkLocalSetting;

static gboolean have_smb;

/* gconf settings */
static char *current_workgroup;
static NetworkLocalSetting gconf_local_setting;
static char *gconf_extra_domains;


static GList *current_dns_sd_domains = NULL;

static GList *active_links;
static GList *active_redirects;
static GList *active_monitors;
G_LOCK_DEFINE_STATIC (network);

static void refresh_link_lists (void);

typedef struct {
	GnomeVFSHandle *handle;
	char *prefix;
	char *data;
	int len;
	int pos;
} FileHandle;

static FileHandle *
file_handle_new (char *data)
{
	FileHandle *result;
	result = g_new0 (FileHandle, 1);

	if (data) {
		result->data = g_strdup (data);
		result->len = strlen (data);
		result->pos = 0;
	}

	return result;
}

static FileHandle *
file_handle_new_from_handle (GnomeVFSHandle *handle,
			     const char *prefix)
{
	FileHandle *result;
	result = g_new0 (FileHandle, 1);
	
	result->handle = handle;
	result->prefix = g_strdup (prefix);

	return result;
}


static void
file_handle_destroy (FileHandle *handle)
{
	if (handle->handle) {
		gnome_vfs_close (handle->handle);
	}
	g_free (handle->prefix);
	g_free (handle->data);
	g_free (handle);
}

static NetworkLocalSetting
parse_network_local_setting (const char *setting)
{
	if (setting == NULL)
		return NETWORK_LOCAL_DISABLED;
	if (strcmp (setting, "separate") == 0)
		return NETWORK_LOCAL_SEPARATE;
	if (strcmp (setting, "merged") == 0)
		return NETWORK_LOCAL_MERGED;
	return NETWORK_LOCAL_DISABLED;
}

static char *
get_data_for_link (const char *uri, 
		   const char *display_name, 
		   const char *icon)
{
	char *data;

	data = g_strdup_printf ("[Desktop Entry]\n"
				"Encoding=UTF-8\n"
				"Name=%s\n"
				"Type=FSDevice\n"
				"Icon=%s\n"
				"URL=%s\n",
				display_name,
				icon,
				uri);
	return data;
}


/* Called with lock held */
static void
do_link_event (const char *filename,
	       GnomeVFSMonitorEventType event_type)
{
	NetworkMonitor *monitor;
	GnomeVFSURI *uri, *base_uri;
	GList *l;

	/* Need to shortcut this to handle calls
	   before initialization finished */
	if (active_monitors == NULL)
		return;
	
	base_uri = gnome_vfs_uri_new ("network://");
	uri = gnome_vfs_uri_append_file_name (base_uri, filename);
	gnome_vfs_uri_unref (base_uri);
	
	for (l = active_monitors; l != NULL; l = l->next) {
		monitor = l->data;
		
		gnome_vfs_monitor_callback ((GnomeVFSMethodHandle *)monitor,
					    uri, 
					    event_type);
	}
	gnome_vfs_uri_unref (uri);
}

/* Call with lock held */
static void
remove_link (const char *filename)
{
	GList *l;
	NetworkLink *link, *found;

	found = NULL;
	for (l = active_links; l != NULL; l = l->next) {
		link = l->data;

		if (strcmp (link->filename, filename) == 0) {
			found = link;
			active_links = g_list_remove_link (active_links, l);

			do_link_event (found->filename,
				       GNOME_VFS_MONITOR_EVENT_DELETED);
			
			break;
		}
	}

	if (found) {
		g_free (found->filename);
		g_free (found->target_uri);
		g_free (found->display_name);
		g_free (found->icon);
		g_free (found);
	}
}

/* Call with lock held */
static void
add_link (const char *filename,
	  const char *target_uri,
	  const char *display_name,
	  const char *icon)
{
	NetworkLink *link;

	link = g_new0 (NetworkLink, 1);
	link->filename = g_strdup (filename);
	link->target_uri = g_strdup (target_uri);
	link->display_name = g_strdup (display_name);
	link->icon = g_strdup (icon);

	active_links = g_list_prepend (active_links, link);
	do_link_event (filename,
		       GNOME_VFS_MONITOR_EVENT_CREATED);
}

/* Call with lock held */
static void
add_dns_sd_domain (const char *domain)
{
	char *filename, *link_uri;
	
	filename = g_strconcat ("dnssdlink-",
				domain, NULL);
	
	link_uri = g_strdup_printf ("dns-sd://%s/",
				    domain);
	add_link (filename,
		  link_uri,
		  domain,
		  "gnome-fs-network");
	g_free (filename);
	g_free (link_uri);
}

/* Call with lock held */
static void
remove_dns_sd_domain (const char *domain)
{
	char *filename;
	
	filename = g_strconcat ("dnssdlink-",
				domain, NULL);
	remove_link (filename);
	g_free (filename);
}

/* Call with lock held */
static void
add_dns_sd_domains (const char *domain_list)
{
	char **domains;
	int i;

	if (domain_list == NULL)
		return;
	
	domains = g_strsplit (domain_list, ",", 0);

	for (i = 0; domains[i] != NULL; i++) {
		add_dns_sd_domain (domains[i]);
	}
	
	g_strfreev (domains);
}

/* Call with lock held */
static void
remove_dns_sd_domains (const char *domain_list)
{
	char **domains;
	int i;

	if (domain_list == NULL)
		return;
	
	domains = g_strsplit (domain_list, ",", 0);

	for (i = 0; domains[i] != NULL; i++) {
		remove_dns_sd_domain (domains[i]);
	}
	
	g_strfreev (domains);
}


static void
add_redirect (const char *prefix,
	      const char *base_uri)
{
	NetworkRedirect *redirect;

	redirect = g_new0 (NetworkRedirect, 1);
	redirect->prefix = g_strdup (prefix);
	redirect->base_uri = gnome_vfs_uri_new (base_uri);

	G_LOCK (network);
	active_redirects = g_list_prepend (active_redirects, redirect);
	G_UNLOCK (network);
}


/* Call with lock held */
static NetworkLink *
find_network_link (const char *filename)
{
	GList *l;
	NetworkLink *link;

	for (l = active_links; l != NULL; l = l->next) {
		link = l->data;
		if (strcmp (filename, link->filename) == 0)
			return link;
	}
	return NULL;
}

static char *
network_link_create_data (NetworkLink *link)
{
	return get_data_for_link (link->target_uri, 
				  link->display_name, 
				  link->icon);
}

/* Call with lock held */
static NetworkRedirect *
find_network_redirect (const char *filename)
{
	GList *l;
	NetworkRedirect *redirect;

	for (l = active_redirects; l != NULL; l = l->next) {
		redirect = l->data;
		if (g_str_has_prefix (filename, redirect->prefix))
			return redirect;
	}
	return NULL;
}

static GnomeVFSURI *
network_redirect_get_uri (NetworkRedirect *redirect, const char *filename)
{
	g_assert (g_str_has_prefix (filename, redirect->prefix));
	
	return gnome_vfs_uri_append_file_name (redirect->base_uri,
					       filename + strlen (redirect->prefix));
}

static void
network_monitor_callback (GnomeVFSMonitorHandle *handle,
			  const gchar *monitor_uri,
			  const gchar *info_uri,
			  GnomeVFSMonitorEventType event_type,
			  gpointer user_data)
{
	NetworkMonitor *monitor;
	NetworkRedirect *redirect;
	GList *l;
	GnomeVFSURI *uri, *base_uri;
	char *short_name;
	char *new_name;

	redirect = user_data;

	uri = gnome_vfs_uri_new (info_uri);
	short_name = gnome_vfs_uri_extract_short_name (uri);
	gnome_vfs_uri_unref (uri);

	new_name = g_strconcat (redirect->prefix, short_name, NULL);
	base_uri = gnome_vfs_uri_new ("network://");
	uri = gnome_vfs_uri_append_file_name (base_uri, new_name);
	gnome_vfs_uri_unref (base_uri);
	
	G_LOCK (network);
	for (l = active_monitors; l != NULL; l = l->next) {
		monitor = l->data;

		gnome_vfs_monitor_callback ((GnomeVFSMethodHandle *)monitor,
					    uri, 
					    event_type);
	}
	G_UNLOCK (network);
	gnome_vfs_uri_unref (uri);
}

/* Call with lock held */
static void
network_monitor_add (NetworkMonitor *monitor)
{
	GnomeVFSResult res;
	GnomeVFSMonitorHandle *handle;
	NetworkRedirect *redirect;
	char *uri;
	GList *l;

	if (active_monitors == NULL) {
		for (l = active_redirects; l != NULL; l = l->next) {
			redirect = l->data;

			uri = gnome_vfs_uri_to_string (redirect->base_uri, 0);
			res = gnome_vfs_monitor_add (&handle,
						     uri,
						     GNOME_VFS_MONITOR_DIRECTORY,
						     network_monitor_callback,
						     redirect);
			g_free (uri);
			if (res == GNOME_VFS_OK) {
				redirect->monitor_handle = handle;
			}
		}
	}

	active_monitors = g_list_prepend (active_monitors, monitor);
}

static void
network_monitor_remove (NetworkMonitor *monitor)
{
	NetworkRedirect *redirect;
	GList *l;

	if (active_monitors == NULL) {
		return;
	}

	active_monitors = g_list_remove (active_monitors, monitor);
	
	if (active_monitors == NULL) {
		/* This was the last monitor */
		
		for (l = active_redirects; l != NULL; l = l->next) {
			redirect = l->data;

			if (redirect->monitor_handle) {
				gnome_vfs_monitor_cancel (redirect->monitor_handle);
				redirect->monitor_handle = NULL;
			}
		}
	}

}


static GnomeVFSResult
do_open (GnomeVFSMethod *method,
	 GnomeVFSMethodHandle **method_handle,
	 GnomeVFSURI *uri,
	 GnomeVFSOpenMode mode,
	 GnomeVFSContext *context)
{
	FileHandle *file_handle;
	NetworkLink *link;
	NetworkRedirect *redirect;
	char *name, *data;
	GnomeVFSResult res;
	GnomeVFSHandle *redirect_handle;
	GnomeVFSURI *redirect_uri;
	char *redirect_prefix;

	_GNOME_VFS_METHOD_PARAM_CHECK (method_handle != NULL);
	_GNOME_VFS_METHOD_PARAM_CHECK (uri != NULL);

	if (mode & GNOME_VFS_OPEN_WRITE) {
		return GNOME_VFS_ERROR_NOT_PERMITTED;
	}

	if (strcmp (uri->text, "/") == 0) {
		return GNOME_VFS_ERROR_NOT_PERMITTED;
	}

	name = gnome_vfs_uri_extract_short_name (uri);
	
	G_LOCK (network);

	redirect_uri = NULL;
	redirect_prefix = NULL;
	file_handle = NULL;
	link = find_network_link (name);
	if (link != NULL) {
		data = network_link_create_data (link);
		file_handle = file_handle_new (data);
		g_free (data);
	} else {
		redirect = find_network_redirect (name);
		if (redirect != NULL) {
			redirect_uri = network_redirect_get_uri (redirect, name);
			redirect_prefix = g_strdup (redirect->prefix);
		} 
	}
	g_free (name);
	
	G_UNLOCK (network);

	if (redirect_uri != NULL) {
		res = gnome_vfs_open_uri (&redirect_handle,
					  redirect_uri,
					  mode);
		gnome_vfs_uri_unref (redirect_uri);
		if (res != GNOME_VFS_OK) {
			return res;
		}
		
		file_handle =
			file_handle_new_from_handle (redirect_handle,
						     redirect_prefix);
	}
	g_free (redirect_prefix);
	
	if (file_handle == NULL) {
		return GNOME_VFS_ERROR_NOT_FOUND;
	}

		
	*method_handle = (GnomeVFSMethodHandle *) file_handle;

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_create (GnomeVFSMethod *method,
	   GnomeVFSMethodHandle **method_handle,
	   GnomeVFSURI *uri,
	   GnomeVFSOpenMode mode,
	   gboolean exclusive,
	   guint perm,
	   GnomeVFSContext *context)
{
	return GNOME_VFS_ERROR_NOT_PERMITTED;
}

static GnomeVFSResult
do_close (GnomeVFSMethod *method,
	  GnomeVFSMethodHandle *method_handle,
	  GnomeVFSContext *context)
{
	FileHandle *file_handle;

	g_return_val_if_fail (method_handle != NULL, GNOME_VFS_ERROR_INTERNAL);

	file_handle = (FileHandle *) method_handle;

	file_handle_destroy (file_handle);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_read (GnomeVFSMethod *method,
	 GnomeVFSMethodHandle *method_handle,
	 gpointer buffer,
	 GnomeVFSFileSize num_bytes,
	 GnomeVFSFileSize *bytes_read,
	 GnomeVFSContext *context)
{
	FileHandle *file_handle;
	int read_len;

	g_return_val_if_fail (method_handle != NULL, GNOME_VFS_ERROR_INTERNAL);

	file_handle = (FileHandle *) method_handle;

	if (file_handle->handle) {
		return gnome_vfs_read (file_handle->handle,
				       buffer,
				       num_bytes,
				       bytes_read);
	} else {
		*bytes_read = 0;
		
		if (file_handle->pos >= file_handle->len) {
			return GNOME_VFS_ERROR_EOF;
		}
		
		read_len = MIN (num_bytes, file_handle->len - file_handle->pos);
		
		memcpy (buffer, file_handle->data + file_handle->pos, read_len);
		*bytes_read = read_len;
		file_handle->pos += read_len;
	
		return GNOME_VFS_OK;
	}
}

static GnomeVFSResult
do_write (GnomeVFSMethod *method,
	  GnomeVFSMethodHandle *method_handle,
	  gconstpointer buffer,
	  GnomeVFSFileSize num_bytes,
	  GnomeVFSFileSize *bytes_written,
	  GnomeVFSContext *context)
{
	return GNOME_VFS_ERROR_NOT_PERMITTED;
}


static GnomeVFSResult
do_seek (GnomeVFSMethod *method,
	 GnomeVFSMethodHandle *method_handle,
	 GnomeVFSSeekPosition whence,
	 GnomeVFSFileOffset offset,
	 GnomeVFSContext *context)
{
	FileHandle *file_handle;

	file_handle = (FileHandle *) method_handle;

	if (file_handle->handle) {
		return gnome_vfs_seek (file_handle->handle,
				       whence, offset);
	} else {
		switch (whence) {
		case GNOME_VFS_SEEK_START:
			file_handle->pos = offset;
			break;
		case GNOME_VFS_SEEK_CURRENT:
			file_handle->pos += offset;
			break;
		case GNOME_VFS_SEEK_END:
			file_handle->pos = file_handle->len + offset;
			break;
		}
		
		if (file_handle->pos < 0) {
			file_handle->pos = 0;
		}
	
		if (file_handle->pos > file_handle->len) {
			file_handle->pos = file_handle->len;
		}
	
		return GNOME_VFS_OK;
	}
}

static GnomeVFSResult
do_tell (GnomeVFSMethod *method,
	 GnomeVFSMethodHandle *method_handle,
	 GnomeVFSFileSize *offset_return)
{
	FileHandle *file_handle;

	file_handle = (FileHandle *) method_handle;

	if (file_handle->handle) {
		return gnome_vfs_tell (file_handle->handle,
				       offset_return);
	} else {
		*offset_return = file_handle->pos;
		return GNOME_VFS_OK;
	}
}


static GnomeVFSResult
do_truncate_handle (GnomeVFSMethod *method,
		    GnomeVFSMethodHandle *method_handle,
		    GnomeVFSFileSize where,
		    GnomeVFSContext *context)
{
	return GNOME_VFS_ERROR_NOT_PERMITTED;
}

static GnomeVFSResult
do_truncate (GnomeVFSMethod *method,
	     GnomeVFSURI *uri,
	     GnomeVFSFileSize where,
	     GnomeVFSContext *context)
{
	return GNOME_VFS_ERROR_NOT_PERMITTED;
}

typedef struct {
	GnomeVFSDirectoryHandle *handle;
	char *prefix;
} DirectoryHandleRedirect;


typedef struct {
	GnomeVFSFileInfoOptions options;
	GList *filenames;
	GList *handles;
} DirectoryHandle;

static DirectoryHandle *
directory_handle_new (GnomeVFSFileInfoOptions options)
{
	DirectoryHandle *result;

	result = g_new0 (DirectoryHandle, 1);
	result->options = options;
	result->filenames = NULL;

	return result;
}

static void
free_directory_handle_redirect (DirectoryHandleRedirect *redir)
{
	gnome_vfs_directory_close (redir->handle);
	g_free (redir->prefix);
	g_free (redir);
}

static void
directory_handle_destroy (DirectoryHandle *dir_handle)
{
	g_list_foreach (dir_handle->handles, (GFunc)free_directory_handle_redirect, NULL);
	g_list_foreach (dir_handle->filenames, (GFunc)g_free, NULL);
	g_list_free (dir_handle->filenames);
	g_free (dir_handle);
}

static void
directory_handle_add_filename (DirectoryHandle *dir_handle, char *filename)
{
	dir_handle->filenames = g_list_prepend (dir_handle->filenames, g_strdup (filename));
} 

static GnomeVFSResult
do_open_directory (GnomeVFSMethod *method,
		   GnomeVFSMethodHandle **method_handle,
		   GnomeVFSURI *uri,
		   GnomeVFSFileInfoOptions options,
		   GnomeVFSContext *context)
{
	DirectoryHandle *dir_handle;
	NetworkLink *link;
	NetworkRedirect *redirect;
	GList *uris, *l, *p, *prefixes;
	GnomeVFSResult res;
	GnomeVFSURI *redirect_uri;
	DirectoryHandleRedirect *redirect_dir_handle;
	
	refresh_link_lists ();
	
	dir_handle = directory_handle_new (options);

	G_LOCK (network);

	for (l = active_links; l != NULL; l = l->next) {
		link = l->data;
		
		directory_handle_add_filename (dir_handle, link->filename);
	}

	uris = NULL;
	prefixes = NULL;
	for (l = active_redirects; l != NULL; l = l->next) {
		redirect = l->data;

		uris = g_list_prepend (uris, gnome_vfs_uri_dup (redirect->base_uri));
		prefixes = g_list_prepend (prefixes, strdup (redirect->prefix));
	}

	G_UNLOCK (network);

	for (l = uris, p = prefixes; l != NULL; l = l->next, p = p->next) {
		GnomeVFSDirectoryHandle *handle;
		redirect_uri = l->data;

		res = gnome_vfs_directory_open_from_uri (&handle, redirect_uri, options);
		if (res == GNOME_VFS_OK) {
			redirect_dir_handle = g_new0 (DirectoryHandleRedirect, 1);
			redirect_dir_handle->handle = handle;
			redirect_dir_handle->prefix = g_strdup (p->data);
				
			dir_handle->handles =
				g_list_prepend (dir_handle->handles,
						redirect_dir_handle);
		}
		gnome_vfs_uri_unref (redirect_uri);
		g_free (p->data);
	}
	g_list_free (uris);
	g_list_free (prefixes);
		
	*method_handle = (GnomeVFSMethodHandle *) dir_handle;
	
	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_close_directory (GnomeVFSMethod *method,
		    GnomeVFSMethodHandle *method_handle,
		    GnomeVFSContext *context)
{
	DirectoryHandle *dir_handle;

	dir_handle = (DirectoryHandle *) method_handle;

	directory_handle_destroy (dir_handle);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_read_directory (GnomeVFSMethod *method,
		   GnomeVFSMethodHandle *method_handle,
		   GnomeVFSFileInfo *file_info,
		   GnomeVFSContext *context)
{
	DirectoryHandle *handle;
	DirectoryHandleRedirect *redirect_handle;
	GnomeVFSResult res;
	GList *entry;
	char *prefixed_name;

	handle = (DirectoryHandle *) method_handle;

	if (handle->filenames != NULL) {
		entry = handle->filenames;
		handle->filenames = g_list_remove_link (handle->filenames, entry);
		
		file_info->valid_fields = GNOME_VFS_FILE_INFO_FIELDS_NONE;
		file_info->name = g_strdup (entry->data);
		g_free (entry->data);
		g_list_free_1 (entry);
		
		file_info->mime_type = g_strdup ("application/x-desktop");
		file_info->type = GNOME_VFS_FILE_TYPE_REGULAR;
		file_info->valid_fields |=
			GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE |
			GNOME_VFS_FILE_INFO_FIELDS_TYPE;
		
		file_info->permissions =
			GNOME_VFS_PERM_USER_READ |
			GNOME_VFS_PERM_OTHER_READ |
			GNOME_VFS_PERM_GROUP_READ;
		file_info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS;
		
		return GNOME_VFS_OK;
	}

	while (handle->handles != NULL) {
		redirect_handle = handle->handles->data;
		res = gnome_vfs_directory_read_next (redirect_handle->handle,
						     file_info);

		if (res == GNOME_VFS_OK) {
			prefixed_name = g_strconcat (redirect_handle->prefix,
						     file_info->name, NULL);
			g_free (file_info->name);
			file_info->name = prefixed_name;
		}

		if (res != GNOME_VFS_OK) {
			free_directory_handle_redirect (redirect_handle);
			handle->handles = g_list_remove_link (handle->handles,
							      handle->handles);
			continue;
		}
		return res;
	}
	
		
	return GNOME_VFS_ERROR_EOF;
}

static GnomeVFSResult
do_get_file_info (GnomeVFSMethod *method,
		  GnomeVFSURI *uri,
		  GnomeVFSFileInfo *file_info,
		  GnomeVFSFileInfoOptions options,
		  GnomeVFSContext *context)
{
	NetworkLink *link;
	NetworkRedirect *redirect;
	char *name;

	file_info->valid_fields = GNOME_VFS_FILE_INFO_FIELDS_NONE;
	
	if (strcmp (uri->text, "/") == 0) {
		file_info->name = g_strdup ("/");
		
		file_info->mime_type = g_strdup ("x-directory/normal");
		file_info->type = GNOME_VFS_FILE_TYPE_DIRECTORY;
		file_info->valid_fields |=
			GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE |
			GNOME_VFS_FILE_INFO_FIELDS_TYPE;

		file_info->permissions =
			GNOME_VFS_PERM_USER_READ |
			GNOME_VFS_PERM_OTHER_READ |
			GNOME_VFS_PERM_GROUP_READ;
		file_info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS;
		
		return GNOME_VFS_OK;
	}

	name = gnome_vfs_uri_extract_short_name (uri);
	link = find_network_link (name);
	if (link != NULL) {
		g_free (name);
		file_info->name = gnome_vfs_uri_extract_short_name (uri);
		
		file_info->mime_type = g_strdup ("application/x-desktop");
		file_info->type = GNOME_VFS_FILE_TYPE_REGULAR;
		file_info->valid_fields |=
			GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE |
			GNOME_VFS_FILE_INFO_FIELDS_TYPE;
		
		file_info->permissions =
			GNOME_VFS_PERM_USER_READ |
			GNOME_VFS_PERM_OTHER_READ |
			GNOME_VFS_PERM_GROUP_READ;
		file_info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS;
		
		return GNOME_VFS_OK;
	}

	redirect = find_network_redirect (name);
	if (redirect != NULL) {
		GnomeVFSURI *redirect_uri;
		GnomeVFSResult res;
		char *prefixed_name;
		
		redirect_uri = network_redirect_get_uri (redirect, name);
		res = gnome_vfs_get_file_info_uri (redirect_uri,
						   file_info,
						   options);
		g_free (name);

		if (res == GNOME_VFS_OK) {
			prefixed_name = g_strconcat (redirect->prefix,
						     file_info->name, NULL);
			g_free (file_info->name);
			file_info->name = prefixed_name;
		}
		
		gnome_vfs_uri_unref (redirect_uri);
		return res;
		
	}
	g_free (name);
	return GNOME_VFS_ERROR_NOT_FOUND;
}

static GnomeVFSResult
do_get_file_info_from_handle (GnomeVFSMethod *method,
			      GnomeVFSMethodHandle *method_handle,
			      GnomeVFSFileInfo *file_info,
			      GnomeVFSFileInfoOptions options,
			      GnomeVFSContext *context)
{
	FileHandle *file_handle;
	GnomeVFSResult res;
	char *prefixed_name;

	file_handle = (FileHandle *) method_handle;

	if (file_handle->handle) {
		res = gnome_vfs_get_file_info_from_handle (file_handle->handle,
							   file_info, options);
		if (res == GNOME_VFS_OK) {
			prefixed_name = g_strconcat (file_handle->prefix,
						     file_info->name, NULL);
			g_free (file_info->name);
			file_info->name = prefixed_name;
		}
		return res;
	} else {
		file_info->valid_fields = GNOME_VFS_FILE_INFO_FIELDS_NONE;
		
		file_info->mime_type = g_strdup ("application/x-desktop");
		file_info->size = file_handle->len;
		file_info->type = GNOME_VFS_FILE_TYPE_REGULAR;
		file_info->valid_fields |=
			GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE |
			GNOME_VFS_FILE_INFO_FIELDS_SIZE |
			GNOME_VFS_FILE_INFO_FIELDS_TYPE;
		
		return GNOME_VFS_OK;
	}
}

static gboolean
do_is_local (GnomeVFSMethod *method,
	     const GnomeVFSURI *uri)
{
	return TRUE;
}


static GnomeVFSResult
do_make_directory (GnomeVFSMethod *method,
		   GnomeVFSURI *uri,
		   guint perm,
		   GnomeVFSContext *context)
{
	return GNOME_VFS_ERROR_NOT_PERMITTED;
}

static GnomeVFSResult
do_remove_directory (GnomeVFSMethod *method,
		     GnomeVFSURI *uri,
		     GnomeVFSContext *context)
{
	return GNOME_VFS_ERROR_NOT_PERMITTED;
}


static GnomeVFSResult
do_find_directory (GnomeVFSMethod *method,
		   GnomeVFSURI *near_uri,
		   GnomeVFSFindDirectoryKind kind,
		   GnomeVFSURI **result_uri,
		   gboolean create_if_needed,
		   gboolean find_if_needed,
		   guint permissions,
		   GnomeVFSContext *context)
{
	return GNOME_VFS_ERROR_NOT_SUPPORTED;
}

static GnomeVFSResult
do_move (GnomeVFSMethod *method,
	 GnomeVFSURI *old_uri,
	 GnomeVFSURI *new_uri,
	 gboolean force_replace,
	 GnomeVFSContext *context)
{
	return GNOME_VFS_ERROR_NOT_PERMITTED;
}

static GnomeVFSResult
do_unlink (GnomeVFSMethod *method,
	   GnomeVFSURI *uri,
	   GnomeVFSContext *context)
{
	return GNOME_VFS_ERROR_NOT_PERMITTED;
}

static GnomeVFSResult
do_create_symbolic_link (GnomeVFSMethod *method,
			 GnomeVFSURI *uri,
			 const char *target_reference,
			 GnomeVFSContext *context)
{
	return GNOME_VFS_ERROR_NOT_PERMITTED;
}

/* When checking whether two locations are on the same file system, we are
   doing this to determine whether we can recursively move or do other
   sorts of transfers.  When a symbolic link is the "source", its
   location is the location of the link file, because we want to
   know about transferring the link, whereas for symbolic links that
   are "targets", we use the location of the object being pointed to,
   because that is where we will be moving/copying to. */
static GnomeVFSResult
do_check_same_fs (GnomeVFSMethod *method,
		  GnomeVFSURI *source_uri,
		  GnomeVFSURI *target_uri,
		  gboolean *same_fs_return,
		  GnomeVFSContext *context)
{
	return TRUE;
}

static GnomeVFSResult
do_set_file_info (GnomeVFSMethod *method,
		  GnomeVFSURI *uri,
		  const GnomeVFSFileInfo *info,
		  GnomeVFSSetFileInfoMask mask,
		  GnomeVFSContext *context)
{
	return GNOME_VFS_ERROR_NOT_PERMITTED;
}


static GnomeVFSResult
do_file_control (GnomeVFSMethod *method,
		 GnomeVFSMethodHandle *method_handle,
		 const char *operation,
		 gpointer operation_data,
		 GnomeVFSContext *context)
{
	return GNOME_VFS_ERROR_NOT_SUPPORTED;
}


static GnomeVFSResult
do_monitor_add (GnomeVFSMethod *method,
		GnomeVFSMethodHandle **method_handle_return,
		GnomeVFSURI *uri,
		GnomeVFSMonitorType monitor_type)
{
	if (monitor_type == GNOME_VFS_MONITOR_DIRECTORY &&
	    (strcmp (uri->text, "") == 0 ||
	     strcmp (uri->text, "/") == 0)) {
		NetworkMonitor *monitor;
		
		monitor = g_new0 (NetworkMonitor, 1);

		G_LOCK (network);

		network_monitor_add (monitor);
		
		G_UNLOCK (network);
		
		
		*method_handle_return = (GnomeVFSMethodHandle *)monitor;

		return GNOME_VFS_OK;
	}
	
	return GNOME_VFS_ERROR_NOT_SUPPORTED;
}

static GnomeVFSResult
do_monitor_cancel (GnomeVFSMethod *method,
		   GnomeVFSMethodHandle *method_handle)
{
	NetworkMonitor *monitor;

	monitor = (NetworkMonitor *)method_handle;

	G_LOCK (network);
	network_monitor_remove (monitor);
	G_UNLOCK (network);
	g_free (monitor);
	
	return GNOME_VFS_OK;
}


static GnomeVFSMethod method = {
	sizeof (GnomeVFSMethod),
	do_open,
	do_create,
	do_close,
	do_read,
	do_write,
	do_seek,
	do_tell,
	do_truncate_handle,
	do_open_directory,
	do_close_directory,
	do_read_directory,
	do_get_file_info,
	do_get_file_info_from_handle,
	do_is_local,
	do_make_directory,
	do_remove_directory,
	do_move,
	do_unlink,
	do_check_same_fs,
	do_set_file_info,
	do_truncate,
	do_find_directory,
	do_create_symbolic_link,
	do_monitor_add,
	do_monitor_cancel,
	do_file_control
};

static void
notify_gconf_extra_domains_changed (GConfClient *client,
				    guint        cnxn_id,
				    GConfEntry  *entry,
				    gpointer     data)
{
	G_LOCK (network);


	remove_dns_sd_domains (gconf_extra_domains);
	g_free (gconf_extra_domains);
	
	gconf_extra_domains = gconf_client_get_string (client, PATH_GCONF_GNOME_VFS_DNS_SD_EXTRA_DOMAINS, NULL);
	add_dns_sd_domains (gconf_extra_domains);

	G_UNLOCK (network);
}

static void
notify_gconf_value_changed (GConfClient *client,
			    guint        cnxn_id,
			    GConfEntry  *entry,
			    gpointer     data)
{
	G_LOCK (network);

	g_free (current_workgroup);

	current_workgroup = gconf_client_get_string (client, PATH_GCONF_GNOME_VFS_SMB_WORKGROUP, NULL);
	if (current_workgroup == NULL || !current_workgroup[0]) {
		g_free (current_workgroup);
		current_workgroup = g_strdup (DEFAULT_WORKGROUP_NAME);
	}

	G_UNLOCK (network);
}

static void
diff_sorted_lists (GList *list1, GList *list2, GCompareFunc compare,
		   GList **added, GList **removed)
{
	int order;
	
	*added = *removed = NULL;

	while (list1 != NULL &&
	       list2 != NULL) {
		order = (*compare) (list1->data, list2->data);
		if (order < 0) {
			*removed = g_list_prepend (*removed, list1->data);
			list1 = list1->next;
		} else if (order > 0) {
			*added = g_list_prepend (*added, list2->data);
			list2 = list2->next;
		} else { /* same item */
			list1 = list1->next;
			list2 = list2->next;
		}
	}

	while (list1 != NULL) {
		*removed = g_list_prepend (*removed, list1->data);
		list1 = list1->next;
	}
	while (list2 != NULL) {
		*added = g_list_prepend (*added, list2->data);
		list2 = list2->next;
	}
}

static void
refresh_link_lists (void)
{
	char hostname[256];
	GList *added, *removed, *node;
	char *domain, *dot;
	GList *domains, *l;
	GnomeVFSResult res;
	
	domain = NULL;
	if (gethostname (hostname, sizeof(hostname)) == 0) {
		dot = strchr (hostname, '.');
		if (dot != NULL &&
		    dot[0] != 0 &&
		    dot[1] != 0) {
			domain = dot + 1;
		}
	}

	domains = NULL;
	if (domain != NULL)
		res = gnome_vfs_dns_sd_list_browse_domains_sync (domain,
								 2000,
								 &domains);
	else
		res = GNOME_VFS_OK;
	
	if (res == GNOME_VFS_OK) {
		
		G_LOCK (network);
		
		diff_sorted_lists (current_dns_sd_domains, domains,
				   (GCompareFunc) strcmp,
				   &added, &removed);

		for (l = removed; l != NULL; l = l->next) {
			domain = l->data;

			remove_dns_sd_domain (domain);
			node = g_list_find_custom (current_dns_sd_domains,
						   domain,
						   (GCompareFunc)strcmp);
			if (node) {
				g_free (node->data);
				current_dns_sd_domains =
					g_list_delete_link (current_dns_sd_domains,
							    node);
			}
		}
		
		for (l = added; l != NULL; l = l->next) {
			domain = l->data;

			add_dns_sd_domain (domain);
			current_dns_sd_domains = g_list_prepend (current_dns_sd_domains,
								 g_strdup (domain));
		}
		
		if (added != NULL)
			current_dns_sd_domains = g_list_sort (current_dns_sd_domains, (GCompareFunc) strcmp);

		g_list_free (added);
		g_list_free (removed);
		
		g_list_foreach (domains, (GFunc)g_free, domains);
		g_list_free (domains);

		G_UNLOCK (network);
	}
}



GnomeVFSMethod *
vfs_module_init (const char *method_name, const char *args)
{
	GConfClient *gconf_client;
	GnomeVFSURI *uri;
	char *workgroup_uri, *workgroup_escaped, *setting;
	
	gconf_client = gconf_client_get_default ();

	gconf_client_add_dir (gconf_client, PATH_GCONF_GNOME_VFS_SMB, GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	gconf_client_add_dir (gconf_client, PATH_GCONF_GNOME_VFS_DNS_SD, GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);

	current_workgroup = gconf_client_get_string (gconf_client, PATH_GCONF_GNOME_VFS_SMB_WORKGROUP, NULL);
	if (current_workgroup == NULL || !current_workgroup[0]) {
		g_free (current_workgroup);
		current_workgroup = g_strdup (DEFAULT_WORKGROUP_NAME);
	}

	setting = gconf_client_get_string (gconf_client, PATH_GCONF_GNOME_VFS_DNS_SD_DISPLAY_LOCAL, NULL);
	gconf_local_setting = parse_network_local_setting (setting);
	g_free (setting);

	/* TODO: catch changes to gconf */
	switch (gconf_local_setting) {
	case NETWORK_LOCAL_MERGED:
		add_redirect ("dnssd-local-",
			      "dns-sd://local/");
		break;
	case NETWORK_LOCAL_SEPARATE:
		add_dns_sd_domain ("local");
		break;
	default:
		break;
	}
	
	gconf_extra_domains = gconf_client_get_string (gconf_client, PATH_GCONF_GNOME_VFS_DNS_SD_EXTRA_DOMAINS, NULL);
	add_dns_sd_domains (gconf_extra_domains);
	gconf_client_notify_add (gconf_client, PATH_GCONF_GNOME_VFS_DNS_SD_EXTRA_DOMAINS, notify_gconf_extra_domains_changed, NULL, NULL, NULL);

	
	gconf_client_notify_add (gconf_client, PATH_GCONF_GNOME_VFS_SMB_WORKGROUP, notify_gconf_value_changed, NULL, NULL, NULL);

	g_object_unref (gconf_client);

	uri = gnome_vfs_uri_new ("smb://");
	have_smb = uri != NULL;
	if (uri != NULL) {
		gnome_vfs_uri_unref (uri);
	}

	if (have_smb) {
		if (current_workgroup != NULL) {
			workgroup_escaped = gnome_vfs_escape_string (current_workgroup);
			workgroup_uri = g_strdup_printf ("smb://%s/", workgroup_escaped);
			add_redirect ("smb-workgroup-",
				      workgroup_uri);
			g_free (workgroup_uri);
			g_free (workgroup_escaped);
		}

		add_link ("smblink-root",
			  "smb://",
			  _("Windows Network"),
			  "gnome-fs-network");
	}

	
	return &method;
}

void
vfs_module_shutdown (GnomeVFSMethod *method)
{
}
