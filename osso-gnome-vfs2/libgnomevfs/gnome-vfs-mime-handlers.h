/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* gnome-vfs-mime-handlers.h - Mime type handlers for the GNOME Virtual
   File System.

   Copyright (C) 2000 Eazel, Inc.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Maciej Stachowiak <mjs@eazel.com> */

#ifndef GNOME_VFS_MIME_HANDLERS_H
#define GNOME_VFS_MIME_HANDLERS_H

#define DISABLE_ORBIT 1

#include <libgnomevfs/gnome-vfs-result.h>
#ifndef DISABLE_ORBIT
#include <bonobo-activation/bonobo-activation-server-info.h>
#endif
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <libgnomevfs/gnome-vfs-uri.h>

G_BEGIN_DECLS

typedef enum {
	GNOME_VFS_MIME_APPLICATION_ARGUMENT_TYPE_URIS,
	GNOME_VFS_MIME_APPLICATION_ARGUMENT_TYPE_PATHS,
	GNOME_VFS_MIME_APPLICATION_ARGUMENT_TYPE_URIS_FOR_NON_FILES
} GnomeVFSMimeApplicationArgumentType;

typedef struct _GnomeVFSMimeApplicationPrivate GnomeVFSMimeApplicationPrivate;

typedef struct {
	char *id; /* PRIVATE */
	char *name;
#ifndef GNOME_VFS_DISABLE_DEPRECATED
	char *command;
	gboolean can_open_multiple_files;
	GnomeVFSMimeApplicationArgumentType expects_uris;
	GList *supported_uri_schemes;
#else
	char *_command;
	gboolean _can_open_multiple_files;
	GnomeVFSMimeApplicationArgumentType _expects_uris;
	GList *_supported_uri_schemes;
#endif
	gboolean requires_terminal;

	/* Padded to avoid future breaks in ABI compatibility */
	void *reserved1;

	GnomeVFSMimeApplicationPrivate *priv;
} GnomeVFSMimeApplication;

/* Database */

GnomeVFSMimeApplication *gnome_vfs_mime_get_default_application                   (const char              *mime_type);
GnomeVFSMimeApplication *gnome_vfs_mime_get_default_application_for_uri           (const char		   *uri,
									           const char              *mime_type);
GList *                  gnome_vfs_mime_get_all_applications                      (const char              *mime_type);
GList *			 gnome_vfs_mime_get_all_applications_for_uri	          (const char              *uri,
									           const char              *mime_type);
#ifndef DISABLE_ORBIT
Bonobo_ServerInfo *      gnome_vfs_mime_get_default_component                     (const char              *mime_type);
#else
gpointer                 gnome_vfs_mime_get_default_component                     (const char              *mime_type);
#endif
GList *                  gnome_vfs_mime_get_all_components                        (const char              *mime_type);

/* MIME types */

const char 	        *gnome_vfs_mime_get_description   		          (const char              *mime_type);
gboolean 	         gnome_vfs_mime_can_be_executable   		          (const char              *mime_type);

/* Application */

GnomeVFSMimeApplication *gnome_vfs_mime_application_new_from_desktop_id           (const char              *id);
GnomeVFSResult           gnome_vfs_mime_application_launch                        (GnomeVFSMimeApplication *app,
									           GList                   *uris);
GnomeVFSResult           gnome_vfs_mime_application_launch_with_env	          (GnomeVFSMimeApplication *app,
									           GList                   *uris,
									           char                   **envp);
const char		*gnome_vfs_mime_application_get_desktop_id	          (GnomeVFSMimeApplication *app);
const char		*gnome_vfs_mime_application_get_desktop_file_path         (GnomeVFSMimeApplication *app);
const char		*gnome_vfs_mime_application_get_name                      (GnomeVFSMimeApplication *app);
const char              *gnome_vfs_mime_application_get_generic_name              (GnomeVFSMimeApplication *app);
const char              *gnome_vfs_mime_application_get_icon                      (GnomeVFSMimeApplication *app);
const char		*gnome_vfs_mime_application_get_exec		          (GnomeVFSMimeApplication *app);
const char		*gnome_vfs_mime_application_get_binary_name	          (GnomeVFSMimeApplication *app);
gboolean		 gnome_vfs_mime_application_supports_uris	          (GnomeVFSMimeApplication *app);
gboolean		 gnome_vfs_mime_application_requires_terminal             (GnomeVFSMimeApplication *app);
gboolean		 gnome_vfs_mime_application_supports_startup_notification (GnomeVFSMimeApplication *app);
const char		*gnome_vfs_mime_application_get_startup_wm_class          (GnomeVFSMimeApplication *app);
GnomeVFSMimeApplication *gnome_vfs_mime_application_copy                          (GnomeVFSMimeApplication *app);
gboolean		 gnome_vfs_mime_application_equal		          (GnomeVFSMimeApplication *app_a,
									           GnomeVFSMimeApplication *app_b);
void                     gnome_vfs_mime_application_free                          (GnomeVFSMimeApplication *app);

/* Lists */

void                     gnome_vfs_mime_application_list_free                     (GList                   *list);
void                     gnome_vfs_mime_component_list_free                       (GList                   *list);

G_END_DECLS

#ifndef GNOME_VFS_DISABLE_DEPRECATED
#include <libgnomevfs/gnome-vfs-mime-deprecated.h>
#endif

#endif /* GNOME_VFS_MIME_HANDLERS_H */
