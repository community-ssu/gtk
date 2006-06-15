/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-error.c - Error handling for the GNOME Virtual File System.

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

   Author: Ettore Perazzoli <ettore@gnu.org>
*/

#include <config.h>
#include "gnome-vfs-result.h"

#include <glib/gtypes.h>
#include <glib/gi18n-lib.h>
#include <errno.h>
#ifndef G_OS_WIN32
#include <netdb.h>
#endif

#ifndef G_OS_WIN32
/* AIX #defines h_errno */
#ifndef h_errno
extern int h_errno;
#endif
#endif

static char *status_strings[] = {
	/* GNOME_VFS_OK */				N_("No error"),
	/* GNOME_VFS_ERROR_NOT_FOUND */			N_("File not found"),
	/* GNOME_VFS_ERROR_GENERIC */			N_("Generic error"),
	/* GNOME_VFS_ERROR_INTERNAL */			N_("Internal error"),
	/* GNOME_VFS_ERROR_BAD_PARAMETERS */		N_("Invalid parameters"),
	/* GNOME_VFS_ERROR_NOT_SUPPORTED */		N_("Unsupported operation"),
	/* GNOME_VFS_ERROR_IO */			N_("I/O error"),
	/* GNOME_VFS_ERROR_CORRUPTED_DATA */		N_("Data corrupted"),
	/* GNOME_VFS_ERROR_WRONG_FORMAT */		N_("Format not valid"),
	/* GNOME_VFS_ERROR_BAD_FILE */			N_("Bad file handle"),
	/* GNOME_VFS_ERROR_TOO_BIG */			N_("File too big"),
	/* GNOME_VFS_ERROR_NO_SPACE */			N_("No space left on device"),
	/* GNOME_VFS_ERROR_READ_ONLY */			N_("Read-only file system"),
	/* GNOME_VFS_ERROR_INVALID_URI */		N_("Invalid URI"),
	/* GNOME_VFS_ERROR_NOT_OPEN */			N_("File not open"),
	/* GNOME_VFS_ERROR_INVALID_OPEN_MODE */		N_("Open mode not valid"),
	/* GNOME_VFS_ERROR_ACCESS_DENIED */		N_("Access denied"),
	/* GNOME_VFS_ERROR_TOO_MANY_OPEN_FILES */	N_("Too many open files"),
	/* GNOME_VFS_ERROR_EOF */			N_("End of file"),
	/* GNOME_VFS_ERROR_NOT_A_DIRECTORY */		N_("Not a directory"),
	/* GNOME_VFS_ERROR_IN_PROGRESS */		N_("Operation in progress"),
	/* GNOME_VFS_ERROR_INTERRUPTED */		N_("Operation interrupted"),
	/* GNOME_VFS_ERROR_FILE_EXISTS */		N_("File exists"),
	/* GNOME_VFS_ERROR_LOOP */			N_("Looping links encountered"),
	/* GNOME_VFS_ERROR_NOT_PERMITTED */		N_("Operation not permitted"),
	/* GNOME_VFS_ERROR_IS_DIRECTORY */       	N_("Is a directory"),
        /* GNOME_VFS_ERROR_NO_MEMORY */             	N_("Not enough memory"),
	/* GNOME_VFS_ERROR_HOST_NOT_FOUND */		N_("Host not found"),
	/* GNOME_VFS_ERROR_INVALID_HOST_NAME */		N_("Host name not valid"),
	/* GNOME_VFS_ERROR_HOST_HAS_NO_ADDRESS */  	N_("Host has no address"),
	/* GNOME_VFS_ERROR_LOGIN_FAILED */		N_("Login failed"),
	/* GNOME_VFS_ERROR_CANCELLED */			N_("Operation cancelled"),
	/* GNOME_VFS_ERROR_DIRECTORY_BUSY */     	N_("Directory busy"),
	/* GNOME_VFS_ERROR_DIRECTORY_NOT_EMPTY */ 	N_("Directory not empty"),
	/* GNOME_VFS_ERROR_TOO_MANY_LINKS */		N_("Too many links"),
	/* GNOME_VFS_ERROR_READ_ONLY_FILE_SYSTEM */	N_("Read only file system"),
	/* GNOME_VFS_ERROR_NOT_SAME_FILE_SYSTEM */	N_("Not on the same file system"),
	/* GNOME_VFS_ERROR_NAME_TOO_LONG */		N_("Name too long"),
	/* GNOME_VFS_ERROR_SERVICE_NOT_AVAILABLE */     N_("Service not available"),
	/* GNOME_VFS_ERROR_SERVICE_OBSOLETE */          N_("Request obsoletes service's data"),
	/* GNOME_VFS_ERROR_PROTOCOL_ERROR */		N_("Protocol error"),
	/* GNOME_VFS_ERROR_NO_MASTER_BROWSER */		N_("Could not find master browser"),
	/* GNOME_VFS_ERROR_NO_DEFAULT */		N_("No default action associated"),
	/* GNOME_VFS_ERROR_NO_HANDLER */		N_("No handler for URL scheme"),
	/* GNOME_VFS_ERROR_PARSE */			N_("Error parsing command line"),
	/* GNOME_VFS_ERROR_LAUNCH */			N_("Error launching command"),
	/* GNOME_VFS_ERROR_TIMEOUT */			N_("Timeout reached"),
	/* GNOME_VFS_ERROR_NAMESERVER */                N_("Nameserver error"),
 	/* GNOME_VFS_ERROR_LOCKED */			N_("The resource is locked"),
	/* GNOME_VFS_ERROR_DEPRECATED_FUNCTION */       N_("Function call deprecated")
};


/**
 * gnome_vfs_result_from_errno_code
 * @errno_code: integer of the same type as the system "errno"
 *
 * Converts a system errno value to a GnomeVFSResult.
 *
 * Return value: a GnomeVFSResult equivalent to @errno_code
 **/
GnomeVFSResult
gnome_vfs_result_from_errno_code (int errno_code)
{
	/* Please keep these in alphabetical order.  */
	switch (errno_code) {
	case E2BIG:        return GNOME_VFS_ERROR_TOO_BIG;
	case EACCES:	   return GNOME_VFS_ERROR_ACCESS_DENIED;
	case EBUSY:	   return GNOME_VFS_ERROR_DIRECTORY_BUSY;
	case EBADF:	   return GNOME_VFS_ERROR_BAD_FILE;
#ifdef ECONNREFUSED
	case ECONNREFUSED: return GNOME_VFS_ERROR_SERVICE_NOT_AVAILABLE;
#endif
	case EEXIST:	   return GNOME_VFS_ERROR_FILE_EXISTS;
	case EFAULT:	   return GNOME_VFS_ERROR_INTERNAL;
	case EFBIG:	   return GNOME_VFS_ERROR_TOO_BIG;
	case EINTR:	   return GNOME_VFS_ERROR_INTERRUPTED;
	case EINVAL:	   return GNOME_VFS_ERROR_BAD_PARAMETERS;
	case EIO:	   return GNOME_VFS_ERROR_IO;
	case EISDIR:	   return GNOME_VFS_ERROR_IS_DIRECTORY;
#ifdef ELOOP
	case ELOOP:	   return GNOME_VFS_ERROR_LOOP;
#endif
	case EMFILE:	   return GNOME_VFS_ERROR_TOO_MANY_OPEN_FILES;
	case EMLINK:	   return GNOME_VFS_ERROR_TOO_MANY_LINKS;
#ifdef ENETUNREACH
	case ENETUNREACH:  return GNOME_VFS_ERROR_SERVICE_NOT_AVAILABLE;
#endif
	case ENFILE:	   return GNOME_VFS_ERROR_TOO_MANY_OPEN_FILES;
#if ENOTEMPTY != EEXIST
	case ENOTEMPTY:    return GNOME_VFS_ERROR_DIRECTORY_NOT_EMPTY;
#endif
	case ENOENT:	   return GNOME_VFS_ERROR_NOT_FOUND;
	case ENOMEM:	   return GNOME_VFS_ERROR_NO_MEMORY;
	case ENOSPC:	   return GNOME_VFS_ERROR_NO_SPACE;
	case ENOTDIR:	   return GNOME_VFS_ERROR_NOT_A_DIRECTORY;
	case EPERM:	   return GNOME_VFS_ERROR_NOT_PERMITTED;
	case EROFS:	   return GNOME_VFS_ERROR_READ_ONLY_FILE_SYSTEM;
#ifdef ETIMEDOUT
	case ETIMEDOUT:    return GNOME_VFS_ERROR_TIMEOUT;
#endif
	case EXDEV:	   return GNOME_VFS_ERROR_NOT_SAME_FILE_SYSTEM;
	
		/* FIXME bugzilla.eazel.com 1191: To be completed.  */
	default:	return GNOME_VFS_ERROR_GENERIC;
	}
}

/**
 * gnome_vfs_result_from_errno:
 * 
 * Converts the system errno to a GnomeVFSResult.
 *
 * Return value: a GnomeVFSResult equivalent to the current system errno
 **/
GnomeVFSResult
gnome_vfs_result_from_errno (void)
{
       return gnome_vfs_result_from_errno_code(errno);
}
 

#ifndef G_OS_WIN32
/**
 * gnome_vfs_result_from_h_errno:
 * 
 * Converts the system "h_errno" to a GnomeVFSResult (h_errno represents errors
 * accessing and finding internet hosts)
 *
 * Return value: a GnomeVFSResult equivalent to the current system "h_errno"
 **/
GnomeVFSResult
gnome_vfs_result_from_h_errno (void)
{
	return gnome_vfs_result_from_h_errno_val (h_errno);
}

/**
 * gnome_vfs_result_from_h_errno_val:
 * @h_errno_code: A integer containing representing the same error codes
 * as the system h_errno.
 * 
 * Converts the error code of @h_errno into a #GnomeVFSResult.
 * 
 * Return Value: The #GnomeVFSResult equivalent to the h_errno error code.
 **/
GnomeVFSResult
gnome_vfs_result_from_h_errno_val (int h_errno_code)
{
	switch (h_errno_code) {
	case HOST_NOT_FOUND:	return GNOME_VFS_ERROR_HOST_NOT_FOUND;
	case NO_ADDRESS:	return GNOME_VFS_ERROR_HOST_HAS_NO_ADDRESS;
	case TRY_AGAIN:		return GNOME_VFS_ERROR_NAMESERVER;
	case NO_RECOVERY:	return GNOME_VFS_ERROR_NAMESERVER;
	default:
		return GNOME_VFS_ERROR_GENERIC;
	}
}
#endif

/**
 * gnome_vfs_result_to_string:
 * @result: the result to convert to a string
 *
 * Returns a string representation of @result, useful for debugging
 * purposes, but probably not appropriate for passing to the user.
 *
 * Return value: a string representing @result
 **/
const char *
gnome_vfs_result_to_string (GnomeVFSResult result)
{
	if ((guint) result >= (guint) (sizeof (status_strings)
				      / sizeof (*status_strings)))
		return _("Unknown error");

	return _(status_strings[(guint) result]);
}
