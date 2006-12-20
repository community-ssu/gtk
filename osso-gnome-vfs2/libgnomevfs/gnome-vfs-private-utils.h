/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-private-utils.h - Private utility functions for the GNOME Virtual
   File System.

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

   Author: Ettore Perazzoli <ettore@comm2000.it> */

#ifndef GNOME_VFS_PRIVATE_UTILS_H
#define GNOME_VFS_PRIVATE_UTILS_H

/* You should not use calls in here outside GnomeVFS. The APIs in here may
 * break even when the GnomeVFS APIs are otherwise frozen.
 */

#include <libgnomevfs/gnome-vfs-cancellation.h>
#include <libgnomevfs/gnome-vfs-handle.h>
#include <libgnomevfs/gnome-vfs-uri.h>

#ifdef G_OS_WIN32
#include <io.h>

#undef GNOME_VFS_DATADIR
extern char *_gnome_vfs_datadir;
#define GNOME_VFS_DATADIR _gnome_vfs_datadir

#undef GNOME_VFS_LIBDIR
extern char *_gnome_vfs_libdir;
#define GNOME_VFS_LIBDIR _gnome_vfs_libdir

#undef GNOME_VFS_PREFIX
extern char *_gnome_vfs_prefix;
#define GNOME_VFS_PREFIX _gnome_vfs_prefix

#undef GNOME_VFS_LOCALEDIR
extern char *_gnome_vfs_localedir;
#define GNOME_VFS_LOCALEDIR _gnome_vfs_localedir

#undef GNOME_VFS_SYSCONFDIR
extern char *_gnome_vfs_sysconfdir;
#define GNOME_VFS_SYSCONFDIR _gnome_vfs_sysconfdir

#endif

G_BEGIN_DECLS

gboolean	 _gnome_vfs_have_ipv6			 (void);

gchar   	*_gnome_vfs_canonicalize_pathname         (char *path);
GnomeVFSResult   gnome_vfs_remove_optional_escapes 	 (char *escaped_uri);

GnomeVFSResult	gnome_vfs_create_temp 	(const gchar *prefix,
					 gchar **name_return,
					 GnomeVFSHandle **handle_return);
gboolean	gnome_vfs_atotm		(const gchar *time_string,
					 time_t *value_return);

GnomeVFSURI    *gnome_vfs_uri_new_private (const gchar *text_uri, 
					   gboolean allow_unknown_method,
					   gboolean allow_unsafe_method,
					   gboolean allow_translate);


gboolean	_gnome_vfs_istr_has_prefix (const char *haystack,
					   const char *needle);
gboolean	_gnome_vfs_istr_has_suffix (const char *haystack,
					   const char *needle);

GnomeVFSResult _gnome_vfs_uri_resolve_all_symlinks_uri (GnomeVFSURI *uri,
							GnomeVFSURI **result_uri);
GnomeVFSResult  _gnome_vfs_uri_resolve_all_symlinks (const char *text_uri,
						     char **resolved_text_uri);
char *          gnome_vfs_resolve_symlink          (const char *path,
						    const char *symlink);


gboolean  _gnome_vfs_uri_is_in_subdir (GnomeVFSURI *uri, GnomeVFSURI *dir);


GnomeVFSResult _gnome_vfs_url_show_using_handler_with_env (const char   *url,
                                                           char        **envp);
gboolean       _gnome_vfs_use_handler_for_scheme          (const char   *scheme);

gboolean       _gnome_vfs_prepend_terminal_to_vector      (int          *argc,
							   char       ***argv);

gboolean       _gnome_vfs_socket_set_blocking             (int       sock_fd,
		                                           gboolean  blocking);

int	       _gnome_vfs_pipe				  (int       *fds);
gboolean       _gnome_vfs_pipe_set_blocking               (int        pipe_fd,
							   gboolean   blocking);

#ifdef G_OS_WIN32
void	       _gnome_vfs_map_winsock_error_to_errno	  (void);
const char    *_gnome_vfs_winsock_strerror		  (int		 error);

#define _GNOME_VFS_SOCKET_IS_ERROR(s) ((s) == SOCKET_ERROR)
#define _GNOME_VFS_SOCKET_CLOSE(s) closesocket(s)
#define _GNOME_VFS_SOCKET_READ(s,b,n) recv(s,b,n,0)
#define _GNOME_VFS_SOCKET_WRITE(s,b,n) send(s,b,n,0)

#else

#define _GNOME_VFS_SOCKET_IS_ERROR(s) ((s) < 0)
#define _GNOME_VFS_SOCKET_CLOSE(s) close(s)
#define _GNOME_VFS_SOCKET_READ(s,b,n) read(s,b,n)
#define _GNOME_VFS_SOCKET_WRITE(s,b,n) write(s,b,n)

#endif

G_END_DECLS

#endif /* _GNOME_VFS_PRIVATE_UTILS_H */
