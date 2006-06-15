/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* 
 * THIS METHOD IS OBSOLETE, IT HAS BEEN SUPERSEDED BY THE HTTP NEON METHOD
 * IN http-neon-method.c AND http-proxy.c
 */

/* http-method.c - The HTTP method implementation for the GNOME Virtual File
   System.

   Copyright (C) 1999 Free Software Foundation
   Copyright (C) 2000-2001 Eazel, Inc

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
		 Ettore Perazzoli <ettore@gnu.org> (core HTTP)
		 Ian McKellar <yakk@yakk.net> (WebDAV/PUT)
		 Michael Fleming <mfleming@eazel.com> (Caching, Cleanup)
		 The friendly GNU Wget sources
	*/

/* TODO:
   - Handle redirection.
   - Handle persistent connections.  */

#include <config.h>
#include "http-method.h"

#include "http-authn.h"
#include "http-cache.h"
/* Keep <sys/types.h> above any network includes for FreeBSD. */
#include <sys/types.h>
/* Keep <netinet/in.h> above <arpa/inet.h> for FreeBSD. */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <gconf/gconf-client.h>
#include <libgnomevfs/gnome-vfs-inet-connection.h>
#include <libgnomevfs/gnome-vfs-mime-sniff-buffer.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-module-callback-module-api.h>
#include <libgnomevfs/gnome-vfs-module.h>
#include <libgnomevfs/gnome-vfs-private-utils.h>
#include <libgnomevfs/gnome-vfs-socket-buffer.h>
#include <libgnomevfs/gnome-vfs-socket.h>
#include <libgnomevfs/gnome-vfs-ssl.h>
#include <libgnomevfs/gnome-vfs-standard-callbacks.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h> /* for atoi */
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>

/* This doesn't exist on HP/UX for example */
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#ifdef DEBUG_HTTP_ENABLE
void
http_debug_printf (char *fmt, ...)
{
	va_list args;
	gchar * out;

	g_assert (fmt);

	va_start (args, fmt);

	out = g_strdup_vprintf (fmt, args);

	fprintf (stderr, "HTTP: [0x%08x] [%p] %s\n",
		 (unsigned int) http_util_get_utime (),
		 g_thread_self (), out);

	g_free (out);
	va_end (args);
}
#endif /* DEBUG_HTTP_ENABLE */

/* What do we qualify ourselves as?  */
/* FIXME bugzilla.gnome.org 41160: "gnome-vfs/1.0.0" may not be good. */
#define USER_AGENT_STRING 	"gnome-vfs/" VERSION

/* Custom User-Agent environment variable */
#define CUSTOM_USER_AGENT_VARIABLE "GNOME_VFS_HTTP_USER_AGENT"

/* Standard HTTP[S] port.  */
#define DEFAULT_HTTP_PORT 	80
#define DEFAULT_HTTPS_PORT 	443

/* Standard HTTP proxy port */
#define DEFAULT_HTTP_PROXY_PORT 8080

/* GConf paths and keys */
#define PATH_GCONF_GNOME_VFS "/system/http_proxy"
#define ITEM_GCONF_HTTP_PROXY_PORT "port"
#define ITEM_GCONF_HTTP_PROXY_HOST "host"
#define KEY_GCONF_HTTP_PROXY_PORT (PATH_GCONF_GNOME_VFS "/" ITEM_GCONF_HTTP_PROXY_PORT)
#define KEY_GCONF_HTTP_PROXY_HOST (PATH_GCONF_GNOME_VFS "/" ITEM_GCONF_HTTP_PROXY_HOST)

#define ITEM_GCONF_USE_HTTP_PROXY "use_http_proxy"
#define KEY_GCONF_USE_HTTP_PROXY (PATH_GCONF_GNOME_VFS "/" ITEM_GCONF_USE_HTTP_PROXY)

#define KEY_GCONF_HTTP_AUTH_USER (PATH_GCONF_GNOME_VFS "/" "authentication_user")
#define KEY_GCONF_HTTP_AUTH_PW (PATH_GCONF_GNOME_VFS "/" "authentication_password")
#define KEY_GCONF_HTTP_USE_AUTH (PATH_GCONF_GNOME_VFS "/" "use_authentication")

#define KEY_GCONF_HTTP_PROXY_IGNORE_HOSTS (PATH_GCONF_GNOME_VFS "/" "ignore_hosts")


/* Some status code validation macros.  */
#define HTTP_20X(x)        (((x) >= 200) && ((x) < 300))
#define HTTP_PARTIAL(x)    ((x) == HTTP_STATUS_PARTIAL_CONTENTS)
#define HTTP_REDIRECTED(x) (((x) == HTTP_STATUS_MOVED_PERMANENTLY)	\
			    || ((x) == HTTP_STATUS_MOVED_TEMPORARILY))

/* HTTP/1.1 status codes from RFC2068, provided for reference.  */
/* Successful 2xx.  */
#define HTTP_STATUS_OK			200
#define HTTP_STATUS_CREATED		201
#define HTTP_STATUS_ACCEPTED		202
#define HTTP_STATUS_NON_AUTHORITATIVE	203
#define HTTP_STATUS_NO_CONTENT		204
#define HTTP_STATUS_RESET_CONTENT	205
#define HTTP_STATUS_PARTIAL_CONTENTS	206

/* Redirection 3xx.  */
#define HTTP_STATUS_MULTIPLE_CHOICES	300
#define HTTP_STATUS_MOVED_PERMANENTLY	301
#define HTTP_STATUS_MOVED_TEMPORARILY	302
#define HTTP_STATUS_SEE_OTHER		303
#define HTTP_STATUS_NOT_MODIFIED	304
#define HTTP_STATUS_USE_PROXY		305

/* Client error 4xx.  */
#define HTTP_STATUS_BAD_REQUEST		400
#define HTTP_STATUS_UNAUTHORIZED	401
#define HTTP_STATUS_PAYMENT_REQUIRED	402
#define HTTP_STATUS_FORBIDDEN		403
#define HTTP_STATUS_NOT_FOUND		404
#define HTTP_STATUS_METHOD_NOT_ALLOWED	405
#define HTTP_STATUS_NOT_ACCEPTABLE	406
#define HTTP_STATUS_PROXY_AUTH_REQUIRED 407
#define HTTP_STATUS_REQUEST_TIMEOUT	408
#define HTTP_STATUS_CONFLICT		409
#define HTTP_STATUS_GONE		410
#define HTTP_STATUS_LENGTH_REQUIRED	411
#define HTTP_STATUS_PRECONDITION_FAILED	412
#define HTTP_STATUS_REQENTITY_TOO_LARGE 413
#define HTTP_STATUS_REQURI_TOO_LARGE	414
#define HTTP_STATUS_UNSUPPORTED_MEDIA	415
#define HTTP_STATUS_LOCKED		423

/* Server errors 5xx.  */
#define HTTP_STATUS_INTERNAL		500
#define HTTP_STATUS_NOT_IMPLEMENTED	501
#define HTTP_STATUS_BAD_GATEWAY		502
#define HTTP_STATUS_UNAVAILABLE		503
#define HTTP_STATUS_GATEWAY_TIMEOUT	504
#define HTTP_STATUS_UNSUPPORTED_VERSION 505
#define HTTP_STATUS_INSUFFICIENT_STORAGE 507

/*
 * Static Variables
 */

/* Global variables used by the HTTP proxy config */ 
static GConfClient * gl_client = NULL;
static GMutex *gl_mutex = NULL;		/* This mutex protects preference values
					 * and ensures serialization of authentication
					 * hook callbacks
					 */
static gchar *gl_http_proxy = NULL;
static gchar *gl_http_proxy_auth = NULL;
static GSList *gl_ignore_hosts = NULL;	/* Elements are strings. */
static GSList *gl_ignore_addrs = NULL;	/* Elements are ProxyHostAddrs */

/* Store IP addresses that may represent network or host addresses and may be
 * IPv4 or IPv6. */
typedef enum {
	PROXY_IPv4 = 4,
	PROXY_IPv6 = 6
} ProxyAddrType;

typedef struct {
	ProxyAddrType type;
	struct in_addr addr;
	struct in_addr mask;
#ifdef ENABLE_IPV6
	struct in6_addr addr6;
	struct in6_addr mask6;
#endif
} ProxyHostAddr;

typedef struct {
	GnomeVFSSocketBuffer *socket_buffer;
	char *uri_string;
	GnomeVFSURI *uri;
	/* The list of headers returned with this response, newlines removed */
	GList *response_headers;

	/* File info for this file */
	GnomeVFSFileInfo *file_info;

	/* Bytes read so far.  */
	GnomeVFSFileSize bytes_read;

	/* Bytes to be written... */
	GByteArray *to_be_written;

	/* List of GnomeVFSFileInfo from a directory listing */
	GList *files;

	/* The last HTTP status code returned */
	guint server_status;
} HttpFileHandle;

typedef struct {
	char *username;
	char *password;
	char *keyring;
	char *realm;
	enum AuthnHeaderType type;
} HttpAuthSave;

static GnomeVFSResult resolve_409 		 (GnomeVFSMethod *method,
						  GnomeVFSURI *uri,
						  GnomeVFSContext *context);
static void 	proxy_set_authn			 (const char *username,
						  const char *password);
static void	proxy_unset_authn 		 (void);
static gboolean invoke_callback_send_additional_headers (GnomeVFSURI *uri,
							 GList **list);
static gboolean invoke_callback_headers_received (HttpFileHandle *handle);
static gboolean invoke_callback_basic_authn_fill (HttpFileHandle *handle, 
			     			  enum AuthnHeaderType authn_which);
static gboolean invoke_callback_basic_authn	 (HttpFileHandle *handle, 
			     			  enum AuthnHeaderType authn_which,
			     			  gboolean previous_attempt_failed,
						  HttpAuthSave **auth_save);
static gboolean invoke_callback_save_authn	 (HttpFileHandle *handle, 
			     			  enum AuthnHeaderType authn_which,
						  HttpAuthSave *auth_save);
static gboolean check_authn_retry_request 	 (HttpFileHandle * http_handle,
			   			  enum AuthnHeaderType authn_which,
			   			  const char *prev_authn_header,
						  gboolean first_request,
						  HttpAuthSave **auth_save);
static void parse_ignore_host 			 (gpointer data,
						  gpointer user_data);
#ifdef ENABLE_IPV6
static void ipv6_network_addr 			 (const struct in6_addr *addr,
						  const struct in6_addr *mask,
						  struct in6_addr *res);

/*Check whether the node is IPv6 enabled.*/
static gboolean
have_ipv6 (void)
{
	int s;

	s = socket (AF_INET6, SOCK_STREAM, 0);
	if (s != -1) {
		close (s);
		return TRUE;
	}

	return FALSE;
}
#endif

static GnomeVFSFileInfo *
defaults_file_info_new (void)
{
	GnomeVFSFileInfo *ret;

	/* Fill up the file info structure with default values */
	/* Default to REGULAR unless we find out later via a PROPFIND that it's a collection */

	ret = gnome_vfs_file_info_new();

	ret->type = GNOME_VFS_FILE_TYPE_REGULAR;
	ret->flags = GNOME_VFS_FILE_FLAGS_NONE;

	ret->valid_fields |= 
		GNOME_VFS_FILE_INFO_FIELDS_TYPE
			| GNOME_VFS_FILE_INFO_FIELDS_FLAGS;


	return ret;
}

static HttpFileHandle *
http_file_handle_new (GnomeVFSSocketBuffer *socket_buffer,
		      GnomeVFSURI *uri)
{
	HttpFileHandle *result;

	result = g_new0 (HttpFileHandle, 1);

	result->socket_buffer = socket_buffer;
	result->uri_string = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE );
	result->uri = uri;
	gnome_vfs_uri_ref(result->uri);

	result->file_info = defaults_file_info_new();
	result->file_info->name = gnome_vfs_uri_extract_short_name (uri);

	return result;
}

static void
http_file_handle_destroy (HttpFileHandle *handle)
{
	if (handle == NULL) {
		return;
	}

	gnome_vfs_uri_unref(handle->uri);
	gnome_vfs_file_info_unref (handle->file_info);
	g_free (handle->uri_string);
	if (handle->to_be_written) {
		g_byte_array_free(handle->to_be_written, TRUE);
	}

	g_list_foreach (handle->response_headers, (GFunc) g_free, NULL);
	g_list_free (handle->response_headers);

	g_list_foreach(handle->files, (GFunc)gnome_vfs_file_info_unref, NULL);
	g_list_free(handle->files);

	g_free (handle);
}

/* The following comes from GNU Wget with minor changes by myself.
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.  */
/* Parse the HTTP status line, which is of format:

   HTTP-Version SP Status-Code SP Reason-Phrase

   The function returns the status-code, or -1 if the status line is
   malformed.  The pointer to reason-phrase is returned in RP.  */
static gboolean
parse_status (const char *cline,
	      guint *status_return)
{
	/* (the variables must not be named `major' and `minor', because
	   that breaks compilation with SunOS4 cc.)  */
	guint mjr, mnr;
	guint statcode;
	const guchar *p, *line;

	line = (const guchar *)cline;

	/* The standard format of HTTP-Version is: `HTTP/X.Y', where X is
	   major version, and Y is minor version.  */
	if (strncmp (line, "HTTP/", 5) == 0) {
		line += 5;
		
		/* Calculate major HTTP version.  */
		p = line;
		for (mjr = 0; g_ascii_isdigit (*line); line++)
			mjr = 10 * mjr + (*line - '0');
		if (*line != '.' || p == line)
			return FALSE;
		++line;
		
		/* Calculate minor HTTP version.  */
		p = line;
		for (mnr = 0; g_ascii_isdigit (*line); line++)
			mnr = 10 * mnr + (*line - '0');
		if (*line != ' ' || p == line)
			return -1;
		/* Wget will accept only 1.0 and higher HTTP-versions.  The value of
		   minor version can be safely ignored.  */
		if (mjr < 1)
			return FALSE;
		++line;
	} else if (strncmp (line, "ICY ", 4) == 0) {
		/* FIXME: workaround for broken ShoutCast and IceCast status replies.
		 * They send things like "ICY 200 OK" instead of "HTTP/1.0 200 OK".
		 * Is there a better way to handle this? 
		 */
		mjr = 1;
		mnr = 0;
		line += 4;
	} else {
		return FALSE;
	}
	
	/* Calculate status code.  */
	if (!(g_ascii_isdigit (*line) && g_ascii_isdigit (line[1]) && g_ascii_isdigit (line[2])))
		return -1;
	statcode = 100 * (*line - '0') + 10 * (line[1] - '0') + (line[2] - '0');

	*status_return = statcode;
	return TRUE;
}

static GnomeVFSResult
http_status_to_vfs_result (guint status)
{
	if (HTTP_20X (status))
		return GNOME_VFS_OK;

	/* FIXME bugzilla.gnome.org 41163 */
	/* mfleming--I've improved the situation slightly, but more
	 * test cases need to be written to ensure that HTTP (esp DAV) does compatibile
	 * things with the normal file method
	 */

	switch (status) {
	case HTTP_STATUS_PRECONDITION_FAILED:
		/* This mapping is certainly true for MOVE with Overwrite: F, otherwise not so true */
		return GNOME_VFS_ERROR_FILE_EXISTS;
	case HTTP_STATUS_UNAUTHORIZED:
	case HTTP_STATUS_PROXY_AUTH_REQUIRED:
	case HTTP_STATUS_FORBIDDEN:
		/* Note that FORBIDDEN can also be returned on a MOVE in a case which
		 * should be VFS_ERROR_BAD_PARAMETERS
		 */
		return GNOME_VFS_ERROR_ACCESS_DENIED;
	case HTTP_STATUS_NOT_FOUND:
		return GNOME_VFS_ERROR_NOT_FOUND;
	case HTTP_STATUS_METHOD_NOT_ALLOWED:
		/* Note that METHOD_NOT_ALLOWED is also returned in a PROPFIND in a case which
		 * should be FILE_EXISTS.  This is handled in do_make_directory
		 */
	case HTTP_STATUS_BAD_REQUEST:
	case HTTP_STATUS_NOT_IMPLEMENTED:
	case HTTP_STATUS_UNSUPPORTED_VERSION:
		return GNOME_VFS_ERROR_NOT_SUPPORTED;
	case HTTP_STATUS_CONFLICT:
		/* _CONFLICT's usually happen when collection paths don't exist */
		return GNOME_VFS_ERROR_NOT_FOUND;
	case HTTP_STATUS_LOCKED:
		/* Maybe we need a separate GNOME_VFS_ERROR_LOCKED? */
		return GNOME_VFS_ERROR_DIRECTORY_BUSY;
	case HTTP_STATUS_INSUFFICIENT_STORAGE:
		return GNOME_VFS_ERROR_NO_SPACE;
	default:
		return GNOME_VFS_ERROR_GENERIC;
	}
}

/* Header parsing routines.  */

static gboolean
header_value_to_number (const char *header_value,
			gulong *number)
{
	const char *p;
	gulong result;

	p = header_value;

	for (result = 0; g_ascii_isdigit (*p); p++)
		result = 10 * result + (*p - '0');
	if (*p)
		return FALSE;

	*number = result;

	return TRUE;
}

static gboolean
set_content_length (HttpFileHandle *handle,
		    const char *value)
{
	gboolean result;
	gulong size;

	result = header_value_to_number (value, &size);
	if (! result)
		return FALSE;

	DEBUG_HTTP (("Expected size is %lu.", size));
	handle->file_info->size = size;
	handle->file_info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_SIZE;
	return TRUE;
}

static char *
strip_semicolon (const char *value)
{
	char *p;

	p = strchr (value, ';');

	if (p != NULL) {
		return g_strndup (value, p - value);
	}
	else {
		return g_strdup (value);
	}
}

static gboolean
set_content_type (HttpFileHandle *handle,
		  const char *value)
{
	g_free (handle->file_info->mime_type);

	handle->file_info->mime_type = strip_semicolon (value);
	handle->file_info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE;
	
	return TRUE;
}

static gboolean
set_last_modified (HttpFileHandle *handle,
		   const char *value)
{
	time_t time;

	if (! gnome_vfs_atotm (value, &time))
		return FALSE;

	handle->file_info->mtime = time;
	handle->file_info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_MTIME;
	return TRUE;
}

static gboolean
set_access_time (HttpFileHandle *handle,
		 const char *value)
{
	time_t time;

	if (! gnome_vfs_atotm (value, &time))
		return FALSE;

	handle->file_info->atime = time;
	handle->file_info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_ATIME;
	return TRUE;
}

struct _Header {
	const char *name;
	gboolean (* set_func) (HttpFileHandle *handle, const char *value);
};
typedef struct _Header Header;

static Header headers[] = {
	{ "Content-Length", set_content_length },
	{ "Content-Type", set_content_type },
	{ "Last-Modified", set_last_modified },
	{ "Date", set_access_time },
	{ NULL, NULL }
};

static const char *
check_header (const char *header,
	      const char *name)
{
	const char *p, *q;

	for (p = header, q = name; *p != '\0' && *q != '\0'; p++, q++) {
		if (g_ascii_tolower (*p) != g_ascii_tolower (*q))
			break;
	}

	if (*q != '\0' || *p != ':')
		return NULL;

	p++;			/* Skip ':'.  */
	while (*p == ' ' || *p == '\t')
		p++;

	return p;
}

static gboolean
parse_header (HttpFileHandle *handle,
	      const char *header)
{
	guint i;

	for (i = 0; headers[i].name != NULL; i++) {
		const char *value;

		value = check_header (header, headers[i].name);
		if (value != NULL)
			return (* headers[i].set_func) (handle, value);
	}

	/* Simply ignore headers we don't know.  */
	return TRUE;
}

/* Header/status reading.  */

static GnomeVFSResult
get_header (GnomeVFSSocketBuffer *socket_buffer,
	    GString *s,
	    GnomeVFSCancellation *cancellation)
{
	GnomeVFSResult result;
	GnomeVFSFileSize bytes_read;
	guint count;

	ANALYZE_HTTP ("==> +get_header");

	g_string_truncate (s, 0);

	count = 0;
	while (1) {
		char c;

		/* ANALYZE_HTTP ("==> +get_header read"); */
		result = gnome_vfs_socket_buffer_read (socket_buffer, &c, 1,
						       &bytes_read, cancellation);
		/* ANALYZE_HTTP ("==> -get_header read"); */

		if (result != GNOME_VFS_OK) {
			return result;
		}
		if (bytes_read == 0) {
			return GNOME_VFS_ERROR_EOF;
		}

		if (c == '\n') {
			/* Handle continuation lines.  */
			if (count != 0 && (count != 1 || s->str[0] != '\r')) {
				char next;

				result = gnome_vfs_socket_buffer_peekc (
						socket_buffer, &next, cancellation);
				if (result != GNOME_VFS_OK) {
					return result;
				}
				
				if (next == '\t' || next == ' ') {
					if (count > 0
					    && s->str[count - 1] == '\r')
						s->str[count - 1] = '\0';
					continue;
				}
			}

			if (count > 0 && s->str[count - 1] == '\r')
				s->str[count - 1] = '\0';
			break;
		} else {
			g_string_append_c (s, c);
		}

		count++;
	}
	

	ANALYZE_HTTP ("==> -get_header");

	return GNOME_VFS_OK;
}

/* rename this function? */
static GnomeVFSResult
create_handle (GnomeVFSURI *uri,
	       GnomeVFSSocketBuffer *socket_buffer,
	       GnomeVFSContext *context,
	       /* OUT */ HttpFileHandle **p_handle)
{
	GString *header_string;
	GnomeVFSResult result;
	guint server_status;
	GnomeVFSCancellation *cancellation;

	g_return_val_if_fail (p_handle != NULL, GNOME_VFS_ERROR_INTERNAL);

	*p_handle = http_file_handle_new (socket_buffer, uri);

	cancellation = NULL;
	if (context != NULL) {
		cancellation = gnome_vfs_context_get_cancellation(context);
	}
	
	header_string = g_string_new (NULL);

	ANALYZE_HTTP ("==> +create_handle");

	/* This is the status report string, which is the first header.  */
	result = get_header (socket_buffer, header_string, cancellation);
	if (result != GNOME_VFS_OK) {
		goto error;
	}

	if (!parse_status (header_string->str, &server_status)) {
		/* An unparsable status line is fatal */
		result = GNOME_VFS_ERROR_GENERIC;
		goto error;
	}

	(*p_handle)->server_status = server_status;

	ANALYZE_HTTP ("==> +create_handle: fetching headers");

	/* Header fetching loop.  */
	for (;;) {
		result = get_header (socket_buffer, header_string, cancellation);
		if (result != GNOME_VFS_OK) {
			break;
		}

		/* Empty header ends header section.  */
		if (header_string->str[0] == '\0') {
			break;
		}

		(*p_handle)->response_headers = g_list_prepend ((*p_handle)->response_headers, 
							g_strdup (header_string->str));

		/* We don't really care if we successfully parse the
		 * header or not. It might be nice to tell someone we
		 * found a header we can't parse, but it's not clear
		 * who would be interested or how we tell them. In the
		 * past we would return NOT_FOUND if any header could
		 * not be parsed, but that seems wrong.
		 */
		parse_header (*p_handle, header_string->str);
	}

	invoke_callback_headers_received (*p_handle);

	ANALYZE_HTTP ("==> -create_handle: fetching headers");

	if (result != GNOME_VFS_OK) {
		goto error;
	}

	if (! HTTP_20X (server_status) && !HTTP_REDIRECTED(server_status)) {
		result = http_status_to_vfs_result (server_status);
		goto error;
	}

	result = GNOME_VFS_OK;
 error:
	g_string_free (header_string, TRUE);

	ANALYZE_HTTP ("==> -create_handle");
	return result;
}

/*
 * Here's how the gconf gnome-vfs HTTP proxy variables
 * are intended to be used
 *
 * /system/http_proxy/use_http_proxy	
 * 	Type: boolean
 *	If set to TRUE, the client should use an HTTP proxy to connect to all
 *	servers (except those specified in the ignore_hosts key -- see below).
 *	The proxy is specified in other gconf variables below.
 *
 * /system/http_proxy/host
 *	Type: string
 *	The hostname of the HTTP proxy this client should use.  If
 *	use-http-proxy is TRUE, this should be set.  If it is not set, the
 *	application should behave as if use-http-proxy is was set to FALSE.
 *
 * /system/http_proxy/port
 *	Type: int
 *	The port number on the HTTP proxy host that the client should connect to
 *	If use_http_proxy and host are set but this is not set, the application
 *	should use a default port value of 8080
 *
 * /system/http_proxy/authentication-user
 *	Type: string
 *	Username to pass to an authenticating HTTP proxy.
 *
 * /system/http_proxy/authentication_password
 *	Type: string
 *	Password to pass to an authenticating HTTP proxy.
 *  
 * /system/http_proxy/use-authentication
 *	Type: boolean
 * 	TRUE if the client should pass http-proxy-authorization-user and
 *	http-proxy-authorization-password an HTTP proxy
 *
 * /system/http_proxy/ignore_hosts
 * 	Type: list of strings
 * 	A list of hosts (hostnames, wildcard domains, IP addresses, and CIDR
 * 	network addresses) that should be accessed directly.
 */

static void
construct_gl_http_proxy (gboolean use_proxy)
{
	g_free (gl_http_proxy);
	gl_http_proxy = NULL;

	g_slist_foreach (gl_ignore_hosts, (GFunc) g_free, NULL);
	g_slist_free (gl_ignore_hosts);
	gl_ignore_hosts = NULL;
	g_slist_foreach (gl_ignore_addrs, (GFunc) g_free, NULL);
	g_slist_free (gl_ignore_addrs);
	gl_ignore_addrs = NULL;

	if (use_proxy) {
		char *proxy_host;
		int   proxy_port;
		GSList *ignore;

		proxy_host = gconf_client_get_string (gl_client, KEY_GCONF_HTTP_PROXY_HOST, NULL);
		proxy_port = gconf_client_get_int (gl_client, KEY_GCONF_HTTP_PROXY_PORT, NULL);

		if (proxy_host) {
			if (0 != proxy_port && 0xffff >= (unsigned) proxy_port) {
				gl_http_proxy = g_strdup_printf ("%s:%u", proxy_host, (unsigned)proxy_port);
			} else {
				gl_http_proxy = g_strdup_printf ("%s:%u", proxy_host, (unsigned)DEFAULT_HTTP_PROXY_PORT);
			}
			DEBUG_HTTP (("New HTTP proxy: '%s'", gl_http_proxy));
		} else {
			DEBUG_HTTP (("HTTP proxy unset"));
		}
		
		g_free (proxy_host);
		proxy_host = NULL;

		ignore = gconf_client_get_list (gl_client, KEY_GCONF_HTTP_PROXY_IGNORE_HOSTS, GCONF_VALUE_STRING, NULL);
		g_slist_foreach (ignore, (GFunc) parse_ignore_host, NULL);
		g_slist_foreach (ignore, (GFunc) g_free, NULL);
		g_slist_free (ignore);
		ignore = NULL;
	}
}

static void
parse_ignore_host (gpointer data, gpointer user_data)
{
	gchar *hostname, *input, *netmask;
	gboolean ip_addr = FALSE, has_error = FALSE;
	struct in_addr host;
#ifdef ENABLE_IPV6
	struct in6_addr host6;
#endif
	ProxyHostAddr *elt;
	gint i;

	input = (gchar*) data;
	elt = g_new0 (ProxyHostAddr, 1);
	if ((netmask = strchr (input, '/')) != NULL) {
		hostname = g_strndup (input, netmask - input);
		++netmask;
	}
	else {
		hostname = g_ascii_strdown (input, -1);
	}
	if (inet_pton (AF_INET, hostname, &host) > 0) {
		ip_addr = TRUE;
		elt->type = PROXY_IPv4;
		elt->addr.s_addr = host.s_addr;
		if (netmask) {
			gchar *endptr;
			gint width = strtol (netmask, &endptr, 10);

			if (*endptr != '\0' || width < 0 || width > 32) {
				has_error = TRUE;
			}
			elt->mask.s_addr = htonl (~0 << width);
			elt->addr.s_addr &= elt->mask.s_addr;
		}
		else {
			elt->mask.s_addr = 0xffffffff;
		}
	}
#ifdef ENABLE_IPV6
	else if (have_ipv6 () && inet_pton (AF_INET6, hostname, &host6) > 0) {
		ip_addr = TRUE;
		elt->type = PROXY_IPv6;
		for (i = 0; i < 16; ++i) {
			elt->addr6.s6_addr[i] = host6.s6_addr[i];
		}
		if (netmask) {
			gchar *endptr;
			gint width = strtol (netmask, &endptr, 10);

			if (*endptr != '\0' || width < 0 || width > 128) {
				has_error = TRUE;
			}
			for (i = 0; i < 16; ++i) {
				elt->mask6.s6_addr[i] = 0;
			}
			for (i=0; i < width/8; i++) {
				elt->mask6.s6_addr[i] = 0xff;
			}
			elt->mask6.s6_addr[i] = (0xff << (8 - width % 8)) & 0xff;
			ipv6_network_addr (&elt->addr6, &elt->mask6, &elt->addr6);
		}
		else {
			for (i = 0; i < 16; ++i) {
				elt->mask6.s6_addr[i] = 0xff;
			}
		}
	}
#endif

	if (ip_addr) {
		if (!has_error) {
			gchar *dst = g_new0 (gchar, INET_ADDRSTRLEN);
	
			gl_ignore_addrs = g_slist_append (gl_ignore_addrs, elt);
			DEBUG_HTTP (("Host %s/%s does not go through proxy.",
					hostname,
					inet_ntop(AF_INET, &elt->mask, dst, INET_ADDRSTRLEN)));
			g_free (dst);
		}
		g_free (hostname);
	}
	else {
		/* It is a hostname. */
		gl_ignore_hosts = g_slist_append (gl_ignore_hosts, hostname);
		DEBUG_HTTP (("Host %s does not go through proxy.", hostname));
		g_free (elt);
	}
}

static void
set_proxy_auth (gboolean use_proxy_auth)
{
	char *auth_user;
	char *auth_pw;

	auth_user = gconf_client_get_string (gl_client, KEY_GCONF_HTTP_AUTH_USER, NULL);
	auth_pw = gconf_client_get_string (gl_client, KEY_GCONF_HTTP_AUTH_PW, NULL);

	if (use_proxy_auth) {
		proxy_set_authn (auth_user, auth_pw);
		DEBUG_HTTP (("New HTTP proxy auth user: '%s'", auth_user));
	} else {
		proxy_unset_authn ();
		DEBUG_HTTP (("HTTP proxy auth unset"));
	}

	g_free (auth_user);
	g_free (auth_pw);
}

/**
 * sig_gconf_value_changed 
 * GGconf notify function for when HTTP proxy GConf key has changed.
 */
static void
notify_gconf_value_changed (GConfClient *client,
			    guint        cnxn_id,
			    GConfEntry  *entry,
			    gpointer     data)
{
	const char *key;

	key = gconf_entry_get_key (entry);

	if (strcmp (key, KEY_GCONF_USE_HTTP_PROXY) == 0
	    || strcmp (key, KEY_GCONF_HTTP_PROXY_IGNORE_HOSTS) == 0
	    || strcmp (key, KEY_GCONF_HTTP_PROXY_HOST) == 0
	    || strcmp (key, KEY_GCONF_HTTP_PROXY_PORT) == 0) {
		gboolean use_proxy_value;
		
		g_mutex_lock (gl_mutex);
		
		/* Check and see if we are using the proxy */
		use_proxy_value = gconf_client_get_bool (gl_client, KEY_GCONF_USE_HTTP_PROXY, NULL);
		construct_gl_http_proxy (use_proxy_value);
		
		g_mutex_unlock (gl_mutex);
	} else if (strcmp (key, KEY_GCONF_HTTP_AUTH_USER) == 0
	    || strcmp (key, KEY_GCONF_HTTP_AUTH_PW) == 0
	    || strcmp (key, KEY_GCONF_HTTP_USE_AUTH) == 0) {
		gboolean use_proxy_auth;

		g_mutex_lock (gl_mutex);
		
		use_proxy_auth = gconf_client_get_bool (gl_client, KEY_GCONF_HTTP_USE_AUTH, NULL);
		set_proxy_auth (use_proxy_auth);

		g_mutex_unlock (gl_mutex);
	}
}

/**
 * host_port_from_string
 * splits a <host>:<port> formatted string into its separate components
 */
static gboolean
host_port_from_string (const char *http_proxy,
		       char **p_proxy_host, 
		       guint *p_proxy_port)
{
	char *port_part;
	
	port_part = strchr (http_proxy, ':');
	
	if (port_part && '\0' != ++port_part && p_proxy_port) {
		*p_proxy_port = (guint) strtoul (port_part, NULL, 10);
	} else if (p_proxy_port) {
		*p_proxy_port = DEFAULT_HTTP_PROXY_PORT;
	}
	
	if (p_proxy_host) {
		if ( port_part != http_proxy ) {
			*p_proxy_host = g_strndup (http_proxy, port_part - http_proxy - 1);
		} else {
			return FALSE;
		}
	}

	return TRUE;
}

/* FIXME: should be done using AC_REPLACE_FUNCS */
#ifndef HAVE_INET_PTON
static int
inet_pton(int af, const char *hostname, void *pton)
{
	struct in_addr in;
	if (!inet_aton(hostname, &in))
	    return 0;
	memcpy(pton, &in, sizeof(in));
	return 1;
}
#endif

#ifdef ENABLE_IPV6
static void
ipv6_network_addr (const struct in6_addr *addr, const struct in6_addr *mask, struct in6_addr *res)
{
	gint i;

	for (i = 0; i < 16; ++i) {
		res->s6_addr[i] = addr->s6_addr[i] & mask->s6_addr[i];
	}
}
#endif

static gboolean
proxy_should_for_hostname (const char *hostname)
{
#ifdef ENABLE_IPV6
	struct in6_addr in6, net6;
#endif
	struct in_addr in;
	GSList *elt;
	ProxyHostAddr *addr;


	/* IPv4 address */
	if (inet_pton (AF_INET, hostname, &in) > 0) {
		for (elt = gl_ignore_addrs; elt; elt = g_slist_next (elt)) {
			addr = (ProxyHostAddr*) (elt->data);
			if (addr->type == PROXY_IPv4
			    && (in.s_addr & addr->mask.s_addr) == addr->addr.s_addr) {
				DEBUG_HTTP (("Host %s using direct connection.", hostname)); 
				return FALSE;
			}
		}
	}
#ifdef ENABLE_IPV6
	else if (have_ipv6 () && inet_pton (AF_INET6, hostname, &in6)) {
		for (elt = gl_ignore_addrs; elt; elt = g_slist_next (elt)) {
			addr = (ProxyHostAddr*) (elt->data);
			ipv6_network_addr (&in6, &addr->mask6, &net6);
			if (addr->type == PROXY_IPv6 
			    && IN6_ARE_ADDR_EQUAL (&net6, &addr->addr6)) {
				DEBUG_HTTP (("Host %s using direct connection.", hostname)); 
				return FALSE;
			}
			/* Handle IPv6-wrapped IPv4 addresses. */
			else if (addr->type == PROXY_IPv4
			         && IN6_IS_ADDR_V4MAPPED (&net6)) {
				guint32 v4addr;

				v4addr = net6.s6_addr[12] << 24 | net6.s6_addr[13] << 16 | net6.s6_addr[14] << 8 | net6.s6_addr[15];
				if ((v4addr & addr->mask.s_addr) != addr->addr.s_addr) {
					DEBUG_HTTP (("Host %s using direct connection.", hostname)); 
					return FALSE;
				}
			}
		}
	}
#endif
	/* All hostnames (foo.bar.com) -- independent of IPv4 or IPv6 */

	/* If there are IPv6 addresses in the ignore_hosts list but we do not
	 * have IPv6 available at runtime, then those addresses will also fall
	 * through to here (and harmlessly fail to match). */
	else {
		gchar *hn = g_ascii_strdown (hostname, -1);

		for (elt = gl_ignore_hosts; elt; elt = g_slist_next (elt)) {
			if (*(gchar*) (elt->data) == '*' ) {
				if (g_str_has_suffix (hn,
						(gchar*) (elt->data) + 1)) {
					DEBUG_HTTP (("Host %s using direct connection.", hn));
					g_free (hn);
					return FALSE;
				}
			}
			else if (strcmp (hn, elt->data) == 0) {
				DEBUG_HTTP (("Host %s using direct connection.", hn));
				g_free (hn);
				return FALSE;
			}			
		}
		g_free (hn);
	}

	return TRUE;
}

static char *
proxy_get_authn_header_for_uri_nolock (GnomeVFSURI * uri)
{
	char * ret;

	ret = NULL;

	/* FIXME this needs to be atomic */	
	if (gl_http_proxy_auth != NULL) {
		ret = g_strdup_printf ("Proxy-Authorization: Basic %s\r\n", gl_http_proxy_auth);
	}

	return ret;
}

static char *
proxy_get_authn_header_for_uri (GnomeVFSURI * uri)
{
	char * ret;

	g_mutex_lock (gl_mutex);

	ret = proxy_get_authn_header_for_uri_nolock (uri);

	g_mutex_unlock (gl_mutex);
	
	return ret;
}

/**
 * proxy_for_uri
 * Retrives an appropriate HTTP proxy for a given toplevel uri
 * Currently, only a single HTTP proxy is implemented (there's no way for
 * specifying non-proxy domain names's).  Returns FALSE if the connect should
 * take place directly
 */
static gboolean
proxy_for_uri (
	GnomeVFSToplevelURI * toplevel_uri,
	gchar **p_proxy_host,		/* Callee must free */
	guint *p_proxy_port)		/* Callee must free */
{
	gboolean ret;
	
	ret = proxy_should_for_hostname (toplevel_uri->host_name);

	g_mutex_lock (gl_mutex);

	if (ret && gl_http_proxy != NULL) {
		ret = host_port_from_string (gl_http_proxy, p_proxy_host, p_proxy_port);
	} else {
		p_proxy_host = NULL;
		p_proxy_port = NULL;
		ret = FALSE;
	}

	g_mutex_unlock (gl_mutex);

	return ret;
}

static void
proxy_set_authn (const char *username, const char *password)
{
	char * credentials;

	g_free (gl_http_proxy_auth);
	gl_http_proxy_auth = NULL;

	credentials = g_strdup_printf ("%s:%s", 
			username == NULL ? "" : username, 
			password == NULL ? "" : password);

	gl_http_proxy_auth = http_util_base64 (credentials);

	g_free (credentials);
}

static void
proxy_unset_authn (void)
{
	g_free (gl_http_proxy_auth);
	gl_http_proxy_auth = NULL;
}


static GnomeVFSResult
https_proxy (GnomeVFSSocket **socket_return,
	     gchar *proxy_host,
	     gint proxy_port,
	     gchar *server_host,
	     gint server_port,
	     GnomeVFSCancellation *cancellation)
{
	/* use CONNECT to do https proxying. It goes something like this:
	 * >CONNECT server:port HTTP/1.0
	 * >
	 * <HTTP/1.0 200 Connection-established
	 * <Proxy-agent: Apache/1.3.19 (Unix) Debian/GNU
	 * <
	 * and then we've got an open connection.
	 *
	 * So we sent "CONNECT server:port HTTP/1.0\r\n\r\n"
	 * Check the HTTP status.
	 * Wait for "\r\n\r\n"
	 * Start doing the SSL dance.
	 */

	GnomeVFSResult result;
	GnomeVFSInetConnection *http_connection;
	GnomeVFSSocket *http_socket;
	GnomeVFSSocket *https_socket;
	GnomeVFSSSL *ssl;
	char *buffer;
	GnomeVFSFileSize bytes;
	guint status_code;
	gint fd;

	result = gnome_vfs_inet_connection_create (&http_connection, 
			proxy_host, proxy_port, cancellation);

	if (result != GNOME_VFS_OK) {
		return result;
	}

	fd = gnome_vfs_inet_connection_get_fd (http_connection);

	http_socket = gnome_vfs_inet_connection_to_socket (http_connection);

	buffer = g_strdup_printf ("CONNECT %s:%d HTTP/1.0\r\n\r\n",
			server_host, server_port);
	result = gnome_vfs_socket_write (http_socket, buffer, strlen(buffer),
					 &bytes, cancellation);
	g_free (buffer);

	if (result != GNOME_VFS_OK) {
		gnome_vfs_socket_close (http_socket, cancellation);
		return result;
	}

	buffer = proxy_get_authn_header_for_uri (NULL); /* FIXME need uri */
	if (buffer != NULL) {
		result = gnome_vfs_socket_write (http_socket, buffer, 
						 strlen(buffer), &bytes, cancellation);
		g_free (buffer);
	}

	if (result != GNOME_VFS_OK) {
		gnome_vfs_socket_close (http_socket, cancellation);
		return result;
	}

	bytes = 8192;
	buffer = g_malloc0 (bytes);

	result = gnome_vfs_socket_read (http_socket, buffer, bytes-1, &bytes, cancellation);

	if (result != GNOME_VFS_OK) {
		gnome_vfs_socket_close (http_socket, cancellation);
		g_free (buffer);
		return result;
	}

	if (!parse_status (buffer, &status_code)) {
		gnome_vfs_socket_close (http_socket, cancellation);
		g_free (buffer);
		return GNOME_VFS_ERROR_PROTOCOL_ERROR;
	}

	result = http_status_to_vfs_result (status_code);

	if (result != GNOME_VFS_OK) {
		gnome_vfs_socket_close (http_socket, cancellation);
		g_free (buffer);
		return result;
	}

	/* okay - at this point we've read some stuff from the socket.. */
	/* FIXME: for now we'll assume thats all the headers and nothing but. */

	g_free (buffer);

	result = gnome_vfs_ssl_create_from_fd (&ssl, fd, cancellation);

	if (result != GNOME_VFS_OK) {
		gnome_vfs_socket_close (http_socket, cancellation);
		return result;
	}

	https_socket = gnome_vfs_ssl_to_socket (ssl);

	*socket_return = https_socket;

	return GNOME_VFS_OK;
}



static GnomeVFSResult
connect_to_uri (
	GnomeVFSToplevelURI *toplevel_uri, 
	/* OUT */ GnomeVFSSocketBuffer **p_socket_buffer,
	/* OUT */ gboolean * p_proxy_connect)
{
	guint host_port;
	char *proxy_host;
	guint proxy_port;
	GnomeVFSResult result;
	GnomeVFSCancellation * cancellation;
	GnomeVFSInetConnection *connection;
	GnomeVFSSSL *ssl;
	GnomeVFSSocket *socket;
	gboolean https = FALSE;

	cancellation = gnome_vfs_context_get_cancellation (
				gnome_vfs_context_peek_current ());

	g_return_val_if_fail (p_socket_buffer != NULL, GNOME_VFS_ERROR_INTERNAL);
	g_return_val_if_fail (p_proxy_connect != NULL, GNOME_VFS_ERROR_INTERNAL);
	g_return_val_if_fail (toplevel_uri != NULL, GNOME_VFS_ERROR_INTERNAL);

	if (!g_ascii_strcasecmp (gnome_vfs_uri_get_scheme (&toplevel_uri->uri), 
				"https") ||
	    !g_ascii_strcasecmp (gnome_vfs_uri_get_scheme (&toplevel_uri->uri), 
				"davs")) {
		if (!gnome_vfs_ssl_enabled ()) {
			return GNOME_VFS_ERROR_NOT_SUPPORTED;
		}
		https = TRUE;
	}

	if (toplevel_uri->host_port == 0) {
		if (https) {
			host_port = DEFAULT_HTTPS_PORT;
		} else {
			host_port = DEFAULT_HTTP_PORT;
		}
	} else {
		host_port = toplevel_uri->host_port;
	}

	ANALYZE_HTTP ("==> +Making connection");

	if (toplevel_uri->host_name == NULL) {
		result = GNOME_VFS_ERROR_INVALID_URI;
		goto error;
	}

	if (proxy_for_uri (toplevel_uri, &proxy_host, &proxy_port)) {
		if (https) {
			*p_proxy_connect = FALSE;

			result = https_proxy (&socket, proxy_host, proxy_port,
					toplevel_uri->host_name, host_port, cancellation);

			g_free (proxy_host);
			proxy_host = NULL;

			if (result != GNOME_VFS_OK) {
				return result;
			}

		} else {
			*p_proxy_connect = TRUE;

			result = gnome_vfs_inet_connection_create (&connection,
							proxy_host,
							proxy_port, 
							cancellation);
			if (result != GNOME_VFS_OK) {
				return result;
			}
			socket = gnome_vfs_inet_connection_to_socket 
								(connection);

			g_free (proxy_host);
			proxy_host = NULL;
		}
	} else {
		*p_proxy_connect = FALSE;

		if (https) {
			result = gnome_vfs_ssl_create (&ssl, 
					toplevel_uri->host_name, host_port, cancellation);

			if (result != GNOME_VFS_OK) {
				return result;
			}
			socket = gnome_vfs_ssl_to_socket (ssl);
		} else {
			result = gnome_vfs_inet_connection_create (&connection,
						   toplevel_uri->host_name,
						   host_port,
						   cancellation);
			if (result != GNOME_VFS_OK) {
				return result;
			}
			socket = gnome_vfs_inet_connection_to_socket 
								(connection);
		}
	}

	*p_socket_buffer = gnome_vfs_socket_buffer_new (socket);

	if (*p_socket_buffer == NULL) {
		gnome_vfs_socket_close (socket, cancellation);
		return GNOME_VFS_ERROR_INTERNAL;
	}

	ANALYZE_HTTP ("==> -Making connection");

error:
	return result;
}

static GString *
build_request (const char * method, GnomeVFSToplevelURI * toplevel_uri, gboolean proxy_connect, gboolean force_slash_at_end)
{
	gchar *uri_string = NULL;
	GString *request;
	GnomeVFSURI *uri;
	gchar *user_agent;

	uri = (GnomeVFSURI *)toplevel_uri;

	if (proxy_connect) {
		uri_string = gnome_vfs_uri_to_string (uri,
						      GNOME_VFS_URI_HIDE_USER_NAME
						      | GNOME_VFS_URI_HIDE_PASSWORD);

	} else {
		uri_string = gnome_vfs_uri_to_string (uri,
						      GNOME_VFS_URI_HIDE_USER_NAME
						      | GNOME_VFS_URI_HIDE_PASSWORD
						      | GNOME_VFS_URI_HIDE_HOST_NAME
						      | GNOME_VFS_URI_HIDE_HOST_PORT
						      | GNOME_VFS_URI_HIDE_TOPLEVEL_METHOD);
	}

	if (force_slash_at_end && uri_string[strlen (uri_string) - 1] != '/') {
		char *tmp;

		tmp = uri_string;
		uri_string = g_strconcat (tmp, "/", NULL);
		g_free (tmp);
	}
	
	/* Request line.  */
	request = g_string_new ("");

	g_string_append_printf (request, "%s %s%s HTTP/1.0\r\n", method, uri_string,
				gnome_vfs_uri_get_path (uri)[0] == '\0' ? "/" : "" );

	DEBUG_HTTP (("-->Making request '%s %s'", method, uri_string));
	
	g_free (uri_string);
	uri_string = NULL;

	/* `Host:' header.  */
	if(toplevel_uri->host_port && toplevel_uri->host_port != 0) {
		g_string_append_printf (request, "Host: %s:%d\r\n",
					toplevel_uri->host_name, toplevel_uri->host_port);
	} else {
		g_string_append_printf (request, "Host: %s\r\n",
					toplevel_uri->host_name);
	}

	/* `Accept:' header.  */
	g_string_append (request, "Accept: */*\r\n");

	/* `User-Agent:' header.  */
	user_agent = getenv (CUSTOM_USER_AGENT_VARIABLE);

	if(user_agent == NULL) {
		user_agent = USER_AGENT_STRING;
	}

	g_string_append_printf (request, "User-Agent: %s\r\n", user_agent);

	return request;
}

static GnomeVFSResult
xmit_request (GnomeVFSSocketBuffer *socket_buffer, 
	      GString *request, 
	      GByteArray *data,
	      GnomeVFSCancellation *cancellation)
{
	GnomeVFSResult result;
	GnomeVFSFileSize bytes_written;

	ANALYZE_HTTP ("==> Writing request and header");

	/* Transmit the request headers.  */
	result = gnome_vfs_socket_buffer_write (socket_buffer, request->str, 
						request->len, &bytes_written,
						cancellation);

	if (result != GNOME_VFS_OK) {
		goto error;
	}

	/* Transmit the body */
	if(data && data->data) {
		ANALYZE_HTTP ("==> Writing data");
		
		result = gnome_vfs_socket_buffer_write (socket_buffer, 
							data->data, data->len, &bytes_written,
							cancellation);
	}

	if (result != GNOME_VFS_OK) {
		goto error;
	}

	result = gnome_vfs_socket_buffer_flush (socket_buffer, cancellation);	

error:
	return result;
}

static void
http_auth_save_free (HttpAuthSave *auth_save)
{
	g_free (auth_save->username);
	g_free (auth_save->password);
	g_free (auth_save->keyring);
	g_free (auth_save->realm);
	g_free (auth_save);
}

static GnomeVFSResult
make_request (HttpFileHandle **handle_return,
	      GnomeVFSURI *uri,
	      const gchar *method,
	      GByteArray *data,
	      gchar *extra_headers,
	      GnomeVFSContext *context,
	      gboolean force_slash_at_end)
{
	GnomeVFSSocketBuffer *socket_buffer;
	GnomeVFSResult result;
	GnomeVFSToplevelURI *toplevel_uri;
	GString *request;
	gboolean proxy_connect;
	char *authn_header_request;
	char *authn_header_proxy;
	gboolean first_auth;
	HttpAuthSave *auth_save;
	GnomeVFSCancellation *cancellation;
	
	g_return_val_if_fail (handle_return != NULL, GNOME_VFS_ERROR_INTERNAL);
 	*handle_return = NULL;

	ANALYZE_HTTP ("==> +make_request");

	cancellation = NULL;
	if (context != NULL) {
		cancellation = gnome_vfs_context_get_cancellation(context);
	}
	
	request 		= NULL;
	proxy_connect 		= FALSE;
	authn_header_request	= NULL;
	authn_header_proxy	= NULL;
	
	toplevel_uri = (GnomeVFSToplevelURI *) uri;

	first_auth = TRUE;
	auth_save = NULL;
	for (;;) {
		GList *list;

		g_free (authn_header_request);
		g_free (authn_header_proxy);

		socket_buffer = NULL;
		result = connect_to_uri (toplevel_uri, &socket_buffer, 
				&proxy_connect);
		
		if (result != GNOME_VFS_OK) {
			break;
		}

		request = build_request (method, toplevel_uri, proxy_connect,
					 force_slash_at_end);

		authn_header_request = http_authn_get_header_for_uri (uri);

		if (authn_header_request != NULL) {
			g_string_append (request, authn_header_request);
		}

		if (proxy_connect) {
			authn_header_proxy = proxy_get_authn_header_for_uri (uri);

			if (authn_header_proxy != NULL) {
				g_string_append (request, authn_header_proxy);
			}
		}
		
		/* `Content-Length' header.  */
		if (data != NULL) {
			g_string_append_printf (request, "Content-Length: %d\r\n", data->len);
		}
		
		/* Extra headers. */
		if (extra_headers != NULL) {
			g_string_append (request, extra_headers);
		}

		/* Extra headers from user */
		list = NULL;

		if (invoke_callback_send_additional_headers (uri, &list)) {
			GList *i;

			for (i = list; i; i = i->next) {
				g_string_append (request, i->data);
				g_free (i->data);
				i->data = NULL;
			}

			g_list_free (list);
		}

		/* Empty line ends header section.  */
		g_string_append (request, "\r\n");

		result = xmit_request (socket_buffer, request, data, cancellation);
		g_string_free (request, TRUE);
		request = NULL;

		if (result != GNOME_VFS_OK) {
			break;
		}

		/* Read the headers and create our internal HTTP file handle.  */
		result = create_handle (uri, socket_buffer, context, handle_return);

		if (result == GNOME_VFS_OK) {
			socket_buffer = NULL;
			break;
		}
		if ((*handle_return)->server_status == HTTP_STATUS_UNAUTHORIZED) {
			if (auth_save != NULL) {
				http_auth_save_free (auth_save);
				auth_save = NULL;
			}
			if (! check_authn_retry_request (*handle_return, AuthnHeader_WWW, authn_header_request, first_auth, &auth_save)) {
				break;
			}
		} else if ((*handle_return)->server_status == HTTP_STATUS_PROXY_AUTH_REQUIRED) {
			if (auth_save != NULL) {
				http_auth_save_free (auth_save);
				auth_save = NULL;
			}
			if (! check_authn_retry_request (*handle_return, AuthnHeader_Proxy, authn_header_proxy, first_auth, &auth_save)) {
				break;
			}
		} else {
			break;
		}
		first_auth = FALSE;
		http_file_handle_destroy (*handle_return);
		*handle_return = NULL;
	}

	if (auth_save != NULL) {
		invoke_callback_save_authn (*handle_return,
					    auth_save->type,
					    auth_save);
		
		http_auth_save_free (auth_save);
		auth_save = NULL;
	}
	
	g_free (authn_header_request);
	g_free (authn_header_proxy);

	if (result != GNOME_VFS_OK && *handle_return != NULL) {
		http_file_handle_destroy (*handle_return);
		*handle_return = NULL;
	}

 	if (request != NULL) {
		g_string_free (request, TRUE);
	}
	
	if (socket_buffer != NULL) {
		gnome_vfs_socket_buffer_destroy (socket_buffer, TRUE, cancellation);
	}
	
	ANALYZE_HTTP ("==> -make_request");
	return result;
}

static void
http_handle_close (HttpFileHandle *handle, 
		   GnomeVFSContext *context)
{
	GnomeVFSCancellation *cancellation;
	
	ANALYZE_HTTP ("==> +http_handle_close");
	
	cancellation = NULL;
	if (context != NULL) {
		cancellation = gnome_vfs_context_get_cancellation(context);
	}
	
	if (handle != NULL) {
		if (handle->socket_buffer) {
			gnome_vfs_socket_buffer_flush (handle->socket_buffer, cancellation);
			gnome_vfs_socket_buffer_destroy (handle->socket_buffer,
							 TRUE, cancellation);
			handle->socket_buffer = NULL;
		}

		http_file_handle_destroy (handle);
	}
	
	ANALYZE_HTTP ("==> -http_handle_close");
}

static GnomeVFSResult
do_open (GnomeVFSMethod *method,
	 GnomeVFSMethodHandle **method_handle,
	 GnomeVFSURI *uri,
	 GnomeVFSOpenMode mode,
	 GnomeVFSContext *context)
{
	HttpFileHandle *handle;
	GnomeVFSResult result = GNOME_VFS_OK;
	
	g_return_val_if_fail (uri->parent == NULL, GNOME_VFS_ERROR_INVALID_URI);
	g_return_val_if_fail (!(mode & GNOME_VFS_OPEN_READ && 
				mode & GNOME_VFS_OPEN_WRITE),
			      GNOME_VFS_ERROR_INVALID_OPEN_MODE);

	ANALYZE_HTTP ("==> +do_open");
	DEBUG_HTTP (("+Open URI: '%s' mode:'%c'", gnome_vfs_uri_to_string(uri, 0), 
		     mode & GNOME_VFS_OPEN_READ ? 'R' : 'W'));
	
	if (mode & GNOME_VFS_OPEN_READ) {
		result = make_request (&handle, uri, "GET", NULL, NULL,
				       context, FALSE);
	} else {
		handle = http_file_handle_new(NULL, uri); /* shrug */
	}
	if (result == GNOME_VFS_OK) {
		*method_handle = (GnomeVFSMethodHandle *) handle;
	} else {
		*method_handle = NULL;
	}

	DEBUG_HTTP (("-Open (%d) handle:0x%08x", result, (unsigned int)handle));
	ANALYZE_HTTP ("==> -do_open");
	
	return result;
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
	/* try to write a zero length file - this appears to be the 
	 * only reliable way of testing if a put will succeed. 
	 * Xythos can apparently tell us if we have write permission by
	 * playing with LOCK, but mod_dav cannot. */
	HttpFileHandle *handle;
	GnomeVFSResult result;
	GByteArray *bytes;
	
	ANALYZE_HTTP ("==> +do_create");
	DEBUG_HTTP (("+Create URI: '%s'", gnome_vfs_uri_get_path (uri)));

	http_cache_invalidate_uri_parent (uri);

	/* Don't ignore exclusive; it should check first whether
	   the file exists, since the http protocol default is to 
	   overwrite by default */
	/* FIXME we've stopped using HEAD -- we should use GET instead  */
	/* FIXME we should check the cache here */
	if (exclusive) {
		
		ANALYZE_HTTP ("==> Checking to see if file exists");
		
		result = make_request (&handle, uri, "HEAD", NULL, NULL,
				       context, FALSE);
		http_handle_close (handle, context);
		
		if (result != GNOME_VFS_OK &&
		    result != GNOME_VFS_ERROR_NOT_FOUND) {
			return result;
		}
		if (result == GNOME_VFS_OK) {
			return GNOME_VFS_ERROR_FILE_EXISTS;
		}
	}
	
	ANALYZE_HTTP ("==> Creating initial file");
	bytes  = g_byte_array_new();
      	result = make_request (&handle, uri, "PUT", bytes, NULL, context, FALSE);
	http_handle_close(handle, context);
	g_byte_array_free (bytes, TRUE);
		
	if (result != GNOME_VFS_OK) {
		/* the PUT failed */
		
		/* FIXME bugzilla.gnome.org 45131
		 * If you PUT a file with an invalid name to Xythos, it 
		 * returns a 403 Forbidden, which is different from the behaviour
		 * in MKCOL or MOVE.  Unfortunately, it is not possible to discern whether 
		 * that 403 Forbidden is being returned because of invalid characters in the name
		 * or because of permissions problems
		 */  

		if (result == GNOME_VFS_ERROR_NOT_FOUND) {
			result = resolve_409 (method, uri, context);
		}

		return result;
	}

	/* FIXME bugzilla.gnome.org 41159: do we need to do something more intelligent here? */
	result = do_open (method, method_handle, uri, GNOME_VFS_OPEN_WRITE, context);

	DEBUG_HTTP (("-Create (%d) handle:0x%08x", result, (unsigned int)handle));
	ANALYZE_HTTP ("==> -do_create");

	return result;
}

static GnomeVFSResult
do_close (GnomeVFSMethod *method,
	  GnomeVFSMethodHandle *method_handle,
	  GnomeVFSContext *context)
{
	HttpFileHandle *old_handle;
	HttpFileHandle *new_handle;
	GnomeVFSResult result;
	
	ANALYZE_HTTP ("==> +do_close");
	DEBUG_HTTP (("+Close handle:0x%08x", (unsigned int)method_handle));

	old_handle = (HttpFileHandle *) method_handle;
	
	/* if the handle was opened in write mode then:
	 * 1) there won't be a connection open, and
	 * 2) there will be data to_be_written...
	 */
	if (old_handle->to_be_written != NULL) {
		GnomeVFSURI *uri = old_handle->uri;
		GByteArray *bytes = old_handle->to_be_written;
		GnomeVFSMimeSniffBuffer *sniff_buffer;
		char *extraheader = NULL;
		const char *mime_type = NULL;
		
		sniff_buffer = 
			gnome_vfs_mime_sniff_buffer_new_from_existing_data (bytes->data, 
									    bytes->len);

		if (sniff_buffer != NULL) {
			mime_type = 
				gnome_vfs_get_mime_type_for_buffer (
						sniff_buffer);
			if (mime_type != NULL) {
				extraheader = g_strdup_printf(
						"Content-type: %s\r\n", 
						mime_type);
			}
			gnome_vfs_mime_sniff_buffer_free (sniff_buffer);

		}

		http_cache_invalidate_uri (uri);

		ANALYZE_HTTP ("==> doing PUT");
		result = make_request (&new_handle, uri, "PUT", bytes, 
				       extraheader, context, FALSE);
		g_free (extraheader);
		http_handle_close (new_handle, context);
	} else {
		result = GNOME_VFS_OK;
	}
	
	http_handle_close (old_handle, context);
	
	DEBUG_HTTP (("-Close (%d)", result));
	ANALYZE_HTTP ("==> -do_close");
	
	return result;
}
	
static GnomeVFSResult
do_write (GnomeVFSMethod *method,
	  GnomeVFSMethodHandle *method_handle,
	  gconstpointer buffer,
	  GnomeVFSFileSize num_bytes,
	  GnomeVFSFileSize *bytes_read,
	  GnomeVFSContext *context)
{
	HttpFileHandle *handle;

	DEBUG_HTTP (("+Write handle:0x%08x", (unsigned int)method_handle));

	handle = (HttpFileHandle *) method_handle;

	if(handle->to_be_written == NULL) {
		handle->to_be_written = g_byte_array_new();
	}
	handle->to_be_written = g_byte_array_append(handle->to_be_written, buffer, num_bytes);
	*bytes_read = num_bytes;
	
	DEBUG_HTTP (("-Write (0)"));
	
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
	HttpFileHandle *handle;
	GnomeVFSResult result;
	GnomeVFSCancellation *cancellation;

	ANALYZE_HTTP ("==> +do_read");
	DEBUG_HTTP (("+Read handle=0x%08x", (unsigned int) method_handle));

	cancellation = NULL;
	if (context != NULL) {
		cancellation = gnome_vfs_context_get_cancellation(context);
	}

	
	handle = (HttpFileHandle *) method_handle;

	if (handle->file_info->flags & GNOME_VFS_FILE_INFO_FIELDS_SIZE) {
		GnomeVFSFileSize max_bytes;

		max_bytes = handle->file_info->size - handle->bytes_read;
		num_bytes = MIN (max_bytes, num_bytes);
	}

	result = gnome_vfs_socket_buffer_read (handle->socket_buffer, buffer, 
					       num_bytes, bytes_read, cancellation);
	
	if (*bytes_read == 0) {
		return GNOME_VFS_ERROR_EOF;
	}				       

	handle->bytes_read += *bytes_read;

	DEBUG_HTTP (("-Read (%d)", result));
	ANALYZE_HTTP ("==> -do_read");

	return result;
}


/* Directory handling - WebDAV servers only */
static void
process_resourcetype_node (GnomeVFSFileInfo *file_info, xmlNodePtr node)
{
	xmlNodePtr it;
	
	file_info->valid_fields |= 
		GNOME_VFS_FILE_INFO_FIELDS_TYPE;
	file_info->type = GNOME_VFS_FILE_TYPE_REGULAR;
	
	for (it = node->xmlChildrenNode; it != NULL; it = it->next) {
		if ((!xmlIsBlankNode (it))
		    && (it->name != NULL)
		    && (strcmp (it->name, "collection") == 0)) {
			file_info->type = GNOME_VFS_FILE_TYPE_DIRECTORY;
		}
	}
}

static void
process_propfind_propstat (xmlNodePtr node, 
			   GnomeVFSFileInfo *file_info)
{
	xmlNodePtr l;
	gboolean treat_as_directory;

	treat_as_directory = FALSE;

	while (node != NULL) {
		if (strcmp ((char *)node->name, "prop") != 0) {
			/* node name != "prop" - prop is all we care about */
			node = node->next;
			continue;
		}
		/* properties of the file */
		l = node->xmlChildrenNode;
		while (l != NULL) {
			char *node_content_xml = xmlNodeGetContent(l);
			if (node_content_xml) {
				if (strcmp ((char *)l->name, "getcontenttype") == 0) {

					file_info->valid_fields |= 
						GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE;
					
					if (!file_info->mime_type) {
						file_info->mime_type = strip_semicolon (node_content_xml);
					}
				} else if (strcmp ((char *)l->name, "getcontentlength") == 0){
					file_info->valid_fields |= 
						GNOME_VFS_FILE_INFO_FIELDS_SIZE;
					file_info->size = atoi(node_content_xml);
				} else if (strcmp((char *)l->name, "getlastmodified") == 0) {
					if (gnome_vfs_atotm (node_content_xml, &(file_info->mtime))) {
						file_info->ctime = file_info->mtime;
						file_info->valid_fields |= 
							GNOME_VFS_FILE_INFO_FIELDS_MTIME 
							| GNOME_VFS_FILE_INFO_FIELDS_CTIME;
					}
				} 
				/* Unfortunately, we don't have a mapping for "creationdate" */

				xmlFree (node_content_xml);
				node_content_xml = NULL;
			}
			if (strcmp ((char *)l->name, "resourcetype") == 0) {
				process_resourcetype_node (file_info, l);
			}
			l = l->next;
		}
		node = node->next;
	}
	
	/* If this is a DAV collection, do we tell nautilus to treat it
	 * as a directory or as a web page?
	 */
	if (file_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_TYPE
	    && file_info->type == GNOME_VFS_FILE_TYPE_DIRECTORY) {
		g_free (file_info->mime_type);
		if (treat_as_directory) {
			file_info->mime_type = g_strdup ("x-directory/webdav-prefer-directory");
			file_info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE;
		} else {
			file_info->mime_type = g_strdup ("x-directory/webdav");
			file_info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE;
		}
	}
	
	
	if ((file_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE) == 0) {
		file_info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE;
		file_info->mime_type = g_strdup (gnome_vfs_mime_type_from_name_or_default (file_info->name, "text/plain"));
	}

	if ((file_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_TYPE) == 0) {
		/* Is this a reasonable assumption ? */
		file_info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_TYPE;
		file_info->type = GNOME_VFS_FILE_TYPE_REGULAR;
	}
}

/* a strcmp that doesn't barf on NULLs */
static gint
null_handling_strcmp (const char *a, const char *b) 
{
	if ((a == NULL) != (b == NULL)) {
		return 1;
	}
	
	if (a == NULL && b == NULL) {
		return 0;
	}
	
	return strcmp (a, b);
}

#if 0
static char *
unescape_unreserved_chars (const char *in_string)
{
	/* RFC 2396 section 2.2 */
	static const char * reserved_chars = "%;/?:@&=+$,";

	char *ret, *write_char;
	const char * read_char;

	if (in_string == NULL) {
		return NULL;
	}
	
	ret = g_new (char, strlen (in_string) + 1);

	for (read_char = in_string, write_char = ret ; *read_char != '\0' ; read_char++) {
		if (read_char[0] == '%' 
		    && g_ascii_isxdigit (read_char[1]) 
		    && g_ascii_isxdigit (read_char[2])) {
			char unescaped;
			
			unescaped = (g_ascii_xdigit_value (read_char[1]) << 4) | g_ascii_xdigit_value (read_char[2]);
			
			if (strchr (reserved_chars, unescaped)) {
				*write_char++ = *read_char++;
				*write_char++ = *read_char++;
				*write_char++ = *read_char; /*The last ++ is done in the for statement */
			} else {
				*write_char++ = unescaped;
				read_char += 2; /*The last ++ is done in the for statement */ 
			}
		} else {
			*write_char++ = *read_char;
		}
	}
	*write_char++ = '\0';       	
	
	return ret;
}
#endif /* 0 */

static xmlNodePtr
find_child_node_named (xmlNodePtr node, 
		       const char *child_node_name)
{
	xmlNodePtr child;

	child = node->xmlChildrenNode;

	for (child = node->xmlChildrenNode; child != NULL; child = child->next) {
		if (0 == strcmp (child->name, child_node_name)) {
			return child;
		}
	}

	return NULL;
}

/* Look for a <status> tag in the children of node, and returns
 * the corresponding (HTTP) error code
 */
static gboolean
get_status_node (xmlNodePtr node, guint *status_code)
{
	xmlNodePtr status_node;
	char *status_string;
	gboolean ret;

	status_node = find_child_node_named (node, "status");

	if (status_node != NULL) {
		status_string =	xmlNodeGetContent (status_node);
		ret = parse_status (status_string, status_code);
		xmlFree (status_string);
	} else {
		ret = FALSE;
	}
	
	return ret;
}

static GnomeVFSFileInfo *
process_propfind_response(xmlNodePtr n,
			  GnomeVFSURI *base_uri)
{
	GnomeVFSFileInfo *file_info = defaults_file_info_new ();
	GnomeVFSURI *second_base = gnome_vfs_uri_append_path (base_uri, "/");
	guint status_code;
	
	file_info->valid_fields = GNOME_VFS_FILE_INFO_FIELDS_NONE;
	
	while (n != NULL) {
		if (strcmp ((char *)n->name, "href") == 0) {
			char *nodecontent = xmlNodeGetContent (n);
			GnomeVFSResult rv;
			
			rv = gnome_vfs_remove_optional_escapes (nodecontent);
			
			if (nodecontent != NULL && *nodecontent != '\0' && rv == GNOME_VFS_OK) {
				gint len;
				GnomeVFSURI *uri = gnome_vfs_uri_new (nodecontent);
				
				if (uri != NULL) {
					if ((0 == null_handling_strcmp (base_uri->text, uri->text)) ||
					    (0 == null_handling_strcmp (second_base->text, uri->text))) {
						file_info->name = NULL; /* this file is the . directory */
					} else {
						if (file_info->name != NULL) {
							/* Don't leak if a (potentially malicious)
							 * server returns several href in its answer
							 */
							g_free (file_info->name);
						}
						file_info->name = gnome_vfs_uri_extract_short_name (uri);
						if (file_info->name != NULL) {
							len = strlen (file_info->name) -1;
							if (file_info->name[len] == '/') {
								/* trim trailing `/` - it confuses stuff */
								file_info->name[len] = '\0';
							}
						} else {
							g_warning ("Invalid filename in PROPFIND '%s'; silently skipping", nodecontent);
						}
					}
					gnome_vfs_uri_unref (uri);
				} else {
					g_warning ("Can't make URI from href in PROPFIND '%s'; silently skipping", nodecontent);
				}
			} else {
				g_warning ("got href without contents in PROPFIND response");
			}

			xmlFree (nodecontent);
		} else if (strcmp ((char *)n->name, "propstat") == 0) {
			if (get_status_node (n, &status_code) && status_code == 200) {
				process_propfind_propstat (n->xmlChildrenNode, file_info);
			}
		}
		n = n->next;
	}

	gnome_vfs_uri_unref (second_base);

	return file_info;
}


static gint /* GCompareFunc */
http_glist_find_header (gconstpointer a, gconstpointer b)
{
	if ( NULL != a && NULL != b) {
		return g_ascii_strncasecmp ( (const char *)a, (const char *)b, strlen((const char *)b));
	} else {
		return -1;
	}
}

static char *
redirect_parse_response_header (GList *response_headers)
{
	char *header;
	const char *uri;
	GList *node;
	
	node = g_list_find_custom (response_headers, (gpointer)"Location:", http_glist_find_header);
	for (; node != NULL 
	     ; node = g_list_find_custom (g_list_next (node), (gpointer)"Location:", http_glist_find_header)) {

		header = (char *)node->data;

		/* skip through the header name */
		uri = strchr (header, (unsigned char)':');

		if (uri == NULL) {
			continue;
		}

		uri++;

		/* skip to the uri */
		for (; *uri != '\0' 
			&& (*uri == ' ' || *uri == '\t') 
		     ; uri++);

		return g_strdup (uri);
	}
	return NULL;
}


static GnomeVFSResult
make_propfind_request (HttpFileHandle **handle_return,
		       GnomeVFSURI *uri,
		       gint depth,
		       GnomeVFSContext *context,
		       gboolean force_slash_at_end)
{
	GnomeVFSResult result = GNOME_VFS_OK;
	GnomeVFSFileSize bytes_read, num_bytes=(64*1024);
	char *buffer = g_malloc(num_bytes);
	xmlParserCtxtPtr parserContext;
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;
	char *extraheaders = g_strdup_printf("Depth: %d\r\n", depth);
	gboolean found_root_node_props;
	char *redirect_to;
	int redirect_depth;
	GnomeVFSURI *redirect_uri;

	GByteArray *request = g_byte_array_new();
	char *request_str = "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
		"<D:propfind xmlns:D=\"DAV:\" >"
		"<D:prop>"
                "<D:creationdate/>"
                "<D:getcontentlength/>"
                "<D:getcontenttype/>"
                "<D:getlastmodified/>"
                "<D:resourcetype/>"
		"</D:prop>"
		"</D:propfind>";

	ANALYZE_HTTP ("==> +make_propfind_request");

	request = g_byte_array_append(request, request_str, 
			strlen(request_str));

	parserContext = xmlCreatePushParserCtxt(NULL, NULL, "", 0, "PROPFIND");

	if (depth > 0) {
		http_cache_invalidate_uri_and_children (uri);
	}

	redirect_depth = 0;
	redirect_uri = NULL;
 redirect:
	result = make_request (handle_return, uri, "PROPFIND", request, 
			       extraheaders, context,
			       (redirect_depth == 0) ? force_slash_at_end : FALSE);

	/* Handle PROPFIND redirects. We often get this because we missed a / at the end
	   of the uri for a folder */
	if (result == GNOME_VFS_OK &&
	    ((*handle_return)->server_status == 301 ||
	     (*handle_return)->server_status == 302)) {
		redirect_to = redirect_parse_response_header ((*handle_return)->response_headers);
		redirect_depth++;
		if (redirect_to != NULL && redirect_depth < 7) {
			if (redirect_uri != NULL) {
				gnome_vfs_uri_unref (redirect_uri);
			}
			redirect_uri = gnome_vfs_uri_new (redirect_to);

			uri = redirect_uri;
			http_handle_close (*handle_return, context);
			*handle_return = NULL;
			g_free (redirect_to);
			goto redirect;
		}
		g_free (redirect_to);
		result = GNOME_VFS_ERROR_TOO_MANY_LINKS;
	}
	
	/* FIXME bugzilla.gnome.org 43834: It looks like some http
	 * servers (eg, www.yahoo.com) treat PROPFIND as a GET and
	 * return a 200 OK. Others may return access denied errors or
	 * redirects or any other legal response. This case probably
	 * needs to be made more robust.
	 */
	if (result == GNOME_VFS_OK && (*handle_return)->server_status != 207) { /* Multi-Status */
		DEBUG_HTTP (("HTTP server returned an invalid PROPFIND response: %d", (*handle_return)->server_status));
		result = GNOME_VFS_ERROR_NOT_SUPPORTED;
	}
	/* Some servers (download.microsoft.com) will just close
	 * the connection (EOF) without returning any HTTP status.
	 */
	if (result == GNOME_VFS_ERROR_EOF) {
		DEBUG_HTTP (("HTTP server returned an empty PROPFIND response"));
		result = GNOME_VFS_ERROR_NOT_SUPPORTED;
	}
	
	if (result == GNOME_VFS_OK) {
		do {
			result = do_read (NULL, (GnomeVFSMethodHandle *) *handle_return,
					  buffer, num_bytes, &bytes_read, context);
			
			if (result != GNOME_VFS_OK ) {
				break;
			}
			xmlParseChunk (parserContext, buffer, bytes_read, 0);
			buffer[bytes_read]=0;
		} while (bytes_read > 0);
	}

	if (result == GNOME_VFS_ERROR_EOF) {
		result = GNOME_VFS_OK;
	}
	
	if (result != GNOME_VFS_OK) {
		goto cleanup;
	}
	
	xmlParseChunk (parserContext, "", 0, 1);

	doc = parserContext->myDoc;
	if (doc == NULL) {
		result = GNOME_VFS_ERROR_GENERIC;
		goto cleanup;
	}

#if 0
	/* Enable this block of code if you want to dump the xml answer
	 * sent by the server 
	 */
	{
		xmlChar *query_result;
		int result_len;
		xmlDocDumpMemory (doc, &query_result, &result_len);
		g_print ("propfind query result: %s\n", query_result);
		g_free (query_result);
	}	
#endif

	cur = doc->xmlRootNode;
	if (strcmp ((char *)cur->name, "multistatus") != 0) {
		DEBUG_HTTP (("Couldn't find <multistatus>.\n"));
		result = GNOME_VFS_ERROR_GENERIC;
		goto cleanup;
	}
	cur = cur->xmlChildrenNode;
	
	found_root_node_props = FALSE;
	while (cur != NULL) {
		if (strcmp ((char *)cur->name, "response") == 0) {
			GnomeVFSFileInfo *file_info;
			guint status;
			
			/* Some webdav servers (eg resin) put the HTTP status
			 * code for PROPFIND request in the xml answer instead
			 * of directly sending a 404 
			 */
			if (get_status_node (cur, &status) && !HTTP_20X(status)) {
				result = http_status_to_vfs_result (status);
				goto cleanup;
			}
			
			file_info = 
				process_propfind_response (cur->xmlChildrenNode, uri);
			
			if (file_info->name != NULL) { 
				(*handle_return)->files = g_list_append ((*handle_return)->files, file_info);
			} else {
				/* This response refers to the root node */
				/* Abandon the old information that came from create_handle*/
				
				file_info->name = (*handle_return)->file_info->name;
				(*handle_return)->file_info->name = NULL;
				gnome_vfs_file_info_unref ((*handle_return)->file_info);
				(*handle_return)->file_info = file_info;
				found_root_node_props = TRUE;
			}
			
		} else {
			DEBUG_HTTP(("expecting <response> got <%s>\n", cur->name));
		}
		cur = cur->next;
	}
	
	if (!found_root_node_props) {
		DEBUG_HTTP (("Failed to find root request node properties during propfind"));
		result = GNOME_VFS_ERROR_GENERIC;
		goto cleanup;
	}

	/*
	 * RFC 2518
	 * Section 8.1, final line
	 * "The results of this method [PROPFIND] SHOULD NOT be cached"
	 * Well, at least its not "MUST NOT"
	 */
	
	if (depth == 0) {
		http_cache_add_uri (uri, (*handle_return)->file_info, TRUE);
	} else {
		http_cache_add_uri_and_children (uri, (*handle_return)->file_info, (*handle_return)->files);
	}

cleanup:
	if (redirect_uri != NULL) {
		gnome_vfs_uri_unref (redirect_uri);
	}
	g_free(buffer);
	g_free(extraheaders);
	g_byte_array_free (request, TRUE);
	xmlFreeParserCtxt(parserContext);
	
	if (result != GNOME_VFS_OK) {
		http_handle_close (*handle_return, context);
		*handle_return = NULL;
	}
	
	ANALYZE_HTTP ("==> -make_propfind_request");
	
	return result;
}

static GnomeVFSResult
do_open_directory(GnomeVFSMethod *method,
		  GnomeVFSMethodHandle **method_handle,
		  GnomeVFSURI *uri,
		  GnomeVFSFileInfoOptions options,
		  GnomeVFSContext *context) 
{
	/* TODO move to using the gnome_vfs_file_info_list family of functions */
	GnomeVFSResult result;
	HttpFileHandle *handle = NULL;
	GnomeVFSFileInfo * file_info_cached;
	GList *child_file_info_cached_list = NULL;

	ANALYZE_HTTP ("==> +do_open_directory");
	DEBUG_HTTP (("+Open_Directory options: %d URI: '%s'", options, gnome_vfs_uri_to_string (uri, 0)));

	/* Check the cache--is this even a directory?  
	 * (Nautilus, in particular, seems to like to make this call on non directories
	 */

	file_info_cached = http_cache_check_uri (uri);

	if (file_info_cached) {
		if (GNOME_VFS_FILE_TYPE_DIRECTORY != file_info_cached->type) {
			ANALYZE_HTTP ("==> Cache Hit (Negative)");	
			gnome_vfs_file_info_unref (file_info_cached);
			result = GNOME_VFS_ERROR_NOT_A_DIRECTORY;
			goto error;
		}
		gnome_vfs_file_info_unref (file_info_cached);
		file_info_cached = NULL;
	}

	
	/* The check for directory contents is more stringent */
	file_info_cached = http_cache_check_directory_uri (uri, &child_file_info_cached_list);

	if (file_info_cached) {
		handle = http_file_handle_new (NULL, uri);
		gnome_vfs_file_info_unref (handle->file_info);
		handle->file_info = file_info_cached;
		handle->files = child_file_info_cached_list;
		result = GNOME_VFS_OK;
	} else {
		/* Make sure we have a slash on the end of the uri for the
		   folder PROPFIND request, see bugzilla #92908.
		   This is not needed since we handle redirects, but it means
		   we send less PROPFIND requests. */
		result = make_propfind_request(&handle, uri, 1, context, TRUE);

		/* Work around the case where there was a non-directory existing
		   with that name and we got a 301 redirect, due to the slash
		   added above */
		if (result == GNOME_VFS_ERROR_NOT_SUPPORTED) {
			result = GNOME_VFS_ERROR_NOT_A_DIRECTORY;
		}
		if (result == GNOME_VFS_OK
		    && handle->file_info->type != GNOME_VFS_FILE_TYPE_DIRECTORY) {
			result = GNOME_VFS_ERROR_NOT_A_DIRECTORY;
			http_handle_close (handle, context);
			handle = NULL;
		}
	}
	
	*method_handle = (GnomeVFSMethodHandle *)handle;

error:
	DEBUG_HTTP (("-Open_Directory (%d) handle:0x%08x", result, (unsigned int)handle));
	ANALYZE_HTTP ("==> -do_open_directory");
	
	return result;
}

static GnomeVFSResult
do_close_directory (GnomeVFSMethod *method,
		    GnomeVFSMethodHandle *method_handle,
		    GnomeVFSContext *context) 
{
	HttpFileHandle *handle;
	
	DEBUG_HTTP (("+Close_Directory"));
	
	handle = (HttpFileHandle *) method_handle;
	
	http_handle_close(handle, context);

	DEBUG_HTTP (("-Close_Directory (0) handle:0x%08x", (unsigned int) method_handle));

	return GNOME_VFS_OK;
}
       
static GnomeVFSResult
do_read_directory (GnomeVFSMethod *method,
       GnomeVFSMethodHandle *method_handle,
       GnomeVFSFileInfo *file_info,
       GnomeVFSContext *context)
{
	HttpFileHandle *handle;
	GnomeVFSResult result;

	DEBUG_HTTP (("+Read_Directory handle:0x%08x", (unsigned int) method_handle));

	handle = (HttpFileHandle *) method_handle;
	
	if (handle->files && g_list_length (handle->files)) {
		GnomeVFSFileInfo *original_info = g_list_nth_data (handle->files, 0);
		gboolean found_entry = FALSE;
		
		/* mfleming -- Why is this check here?  Does anyone set original_info->name to NULL? */
		if (original_info->name != NULL && original_info->name[0]) {
			gnome_vfs_file_info_copy (file_info, original_info); 
			found_entry = TRUE;
		}
		
		/* remove our GnomeVFSFileInfo from the list */
		handle->files = g_list_remove (handle->files, original_info);
		gnome_vfs_file_info_unref (original_info);
	
		/* mfleming -- Is this necessary? */
		if (found_entry) {
			result = GNOME_VFS_OK;
		} else {
			result = do_read_directory (method, method_handle, file_info, context);
		}
	} else {
		result = GNOME_VFS_ERROR_EOF;
	}

	DEBUG_HTTP (("-Read_Directory (%d)", result));
	return result;
}
 

static GnomeVFSResult
do_get_file_info (GnomeVFSMethod *method,
		  GnomeVFSURI *uri,
		  GnomeVFSFileInfo *file_info,
		  GnomeVFSFileInfoOptions options,
		  GnomeVFSContext *context)
{
	HttpFileHandle *handle;
	GnomeVFSResult result;
	GnomeVFSFileInfo * file_info_cached;

	ANALYZE_HTTP ("==> +do_get_file_info");
	DEBUG_HTTP (("+Get_File_Info options: %d URI: '%s'", options, gnome_vfs_uri_to_string( uri, 0)));

	file_info_cached = http_cache_check_uri (uri);

	if (file_info_cached != NULL) {
		gnome_vfs_file_info_copy (file_info, file_info_cached);
		gnome_vfs_file_info_unref (file_info_cached);
		ANALYZE_HTTP ("==> Cache Hit");	
		result = GNOME_VFS_OK;
	} else {
		/*
		 * Start off by making a PROPFIND request.  Fall back to a HEAD if it fails
		 */
		
		result = make_propfind_request (&handle, uri, 0, context, FALSE);
		
		/* Note that theoretically we could not bother with this request if we get a 404 back,
		 * but since some servers seem to return wierd things on PROPFIND (mostly 200 OK's...)
		 * I'm not going to count on the PROPFIND response....
		 */ 
		if (result == GNOME_VFS_OK) {
			gnome_vfs_file_info_copy (file_info, handle->file_info);
			http_handle_close (handle, context);
			handle = NULL;
		} else {
			g_assert (handle == NULL); /* Make sure we're not leaking some old one */
			
			/* Lame buggy servers (eg: www.mozilla.org,
			 * www.corel.com)) return an HTTP error for a
			 * HEAD where a GET would succeed. In these
			 * cases lets try to do a GET.
			 */
			if (result != GNOME_VFS_OK) {
				g_assert (handle == NULL); /* Make sure we're not leaking some old one */

				ANALYZE_HTTP ("==> do_get_file_info: do GET ");

				result = make_request (&handle, uri, "GET", NULL, NULL, context, FALSE);
				if (result == GNOME_VFS_OK) {
					gnome_vfs_file_info_copy (file_info, handle->file_info);
					http_cache_add_uri (uri, handle->file_info, FALSE);
					http_handle_close (handle, context);
					handle = NULL;
				}

				/* If we get a redirect, we should be
				 * basing the MIME type on the type of
				 * the page we'll be redirected
				 * too. Maybe we even want to take the
				 * "follow_links" setting into account.
				 */
				/* FIXME: For now we treat all
				 * redirects as if they lead to a
				 * text/html. That works pretty well,
				 * but it's not correct.
				 */
				if (handle != NULL && HTTP_REDIRECTED (handle->server_status)) {
					g_free (file_info->mime_type);
					file_info->mime_type = g_strdup ("text/html");
				}
			}
			
			if (result == GNOME_VFS_ERROR_NOT_FOUND) { /* 404 not found */
				/* FIXME bugzilla.gnome.org 43835: mfleming: Is this code really appropriate?
				 * In any case, it doesn't seem to be appropriate for a DAV-enabled
				 * server, since they don't seem to send 301's when you PROPFIND collections
				 * without a trailing '/'.
				 */
				if (uri->text != NULL && *uri->text != '\0' 
				    && uri->text[strlen(uri->text)-1] != '/') {
					GnomeVFSURI *tmpuri = gnome_vfs_uri_append_path (uri, "/");
					
					result = do_get_file_info (method, tmpuri, file_info, options, context);
					gnome_vfs_uri_unref (tmpuri);
				}
			}
		}
	}
	
	DEBUG_HTTP (("-Get_File_Info (%d)", result));
	ANALYZE_HTTP ("==> -do_get_file_info");
	
	return result;
}

static GnomeVFSResult
do_get_file_info_from_handle (GnomeVFSMethod *method,
			      GnomeVFSMethodHandle *method_handle,
			      GnomeVFSFileInfo *file_info,
			      GnomeVFSFileInfoOptions options,
			      GnomeVFSContext *context)
{
	HttpFileHandle *handle;
	
	DEBUG_HTTP (("+Get_File_Info_From_Handle"));
	
	handle = (HttpFileHandle *) method_handle;
	
	gnome_vfs_file_info_copy (file_info, handle->file_info);
	
	DEBUG_HTTP (("-Get_File_Info_From_Handle"));
	
	return GNOME_VFS_OK;
}

static gboolean
do_is_local (GnomeVFSMethod *method,
	     const GnomeVFSURI *uri)
{
	DEBUG_HTTP (("+Is_Local"));
	return FALSE;
}

static GnomeVFSResult 
do_make_directory (GnomeVFSMethod *method, 
		   GnomeVFSURI *uri,
                   guint perm, 
		   GnomeVFSContext *context) 
{
	/* MKCOL /path HTTP/1.0 */

	HttpFileHandle *handle;
	GnomeVFSResult result;

	DEBUG_HTTP (("+Make_Directory URI: '%s'", gnome_vfs_uri_to_string (uri, 0)));
	ANALYZE_HTTP ("==> +do_make_directory");

	/*
	 * MKCOL returns a 405 if you try to MKCOL on something that
	 * already exists.  Of course, we don't know whether that means that 
	 * the server doesn't support DAV or the collection already exists.
	 * So we do a PROPFIND first to find out
	 */
	/* FIXME check cache here */
	result = make_propfind_request(&handle, uri, 0, context, FALSE);

	if (result == GNOME_VFS_OK) {
		result = GNOME_VFS_ERROR_FILE_EXISTS;
	} else {
		/* Make sure we're not leaking an old one */
		g_assert (handle == NULL);
		
		if (result == GNOME_VFS_ERROR_NOT_FOUND) {
			http_cache_invalidate_uri_parent (uri);
			result = make_request (&handle, uri, "MKCOL", NULL, NULL, context, FALSE);
		}
	}
	http_handle_close (handle, context);
	
	if (result == GNOME_VFS_ERROR_NOT_FOUND) {
		result = resolve_409 (method, uri, context);
	}

	ANALYZE_HTTP ("==> -do_make_directory");
	DEBUG_HTTP (("-Make_Directory (%d)", result));

	return result;
}

static GnomeVFSResult 
do_remove_directory(GnomeVFSMethod *method, 
		    GnomeVFSURI *uri, 
		    GnomeVFSContext *context) 
{
	/* DELETE /path HTTP/1.0 */
	HttpFileHandle *handle;
	GnomeVFSResult result;

	ANALYZE_HTTP ("==> +do_remove_directory");
	DEBUG_HTTP (("+Remove_Directory URI: '%s'", gnome_vfs_uri_to_string (uri, 0)));

	http_cache_invalidate_uri_parent (uri);

	/* FIXME this should return GNOME_VFS_ERROR_DIRECTORY_NOT_EMPTY if the
	 * directory is not empty
	 */
	result = make_request (&handle, uri, "DELETE", NULL, NULL,
			       context, FALSE);
	http_handle_close (handle, context);
	
	DEBUG_HTTP (("-Remove_Directory (%d)", result));
	ANALYZE_HTTP ("==> -do_remove_directory");
	
	return result;
}

static gboolean 
is_same_fs (const GnomeVFSURI *a, 
	    const GnomeVFSURI *b)
{
	return null_handling_strcmp (gnome_vfs_uri_get_scheme (a), gnome_vfs_uri_get_scheme (b)) == 0
		&& null_handling_strcmp (gnome_vfs_uri_get_host_name (a), gnome_vfs_uri_get_host_name (b)) == 0
	  	&& null_handling_strcmp (gnome_vfs_uri_get_user_name (a), gnome_vfs_uri_get_user_name (b)) == 0
	  	&& null_handling_strcmp (gnome_vfs_uri_get_password (a), gnome_vfs_uri_get_password (b)) == 0
		&& (gnome_vfs_uri_get_host_port (a) == gnome_vfs_uri_get_host_port (b));
}

static GnomeVFSResult
do_move (GnomeVFSMethod *method,
	 GnomeVFSURI *old_uri,
	 GnomeVFSURI *new_uri,
	 gboolean force_replace,
	 GnomeVFSContext *context)
{

	/*
	 * MOVE /path1 HTTP/1.0
	 * Destination: /path2
	 * Overwrite: (T|F)
	 */

	HttpFileHandle *handle;
	GnomeVFSResult result;

	char *destpath, *destheader;

	ANALYZE_HTTP ("==> +do_move");
	DEBUG_HTTP (("+Move URI: '%s' Dest: '%s'", 
		gnome_vfs_uri_to_string (old_uri, 0), 
		gnome_vfs_uri_to_string (new_uri, 0)));

	if (!is_same_fs (old_uri, new_uri)) {
		return GNOME_VFS_ERROR_NOT_SAME_FILE_SYSTEM;
	}	
	
	destpath = gnome_vfs_uri_to_string (new_uri, GNOME_VFS_URI_HIDE_USER_NAME|GNOME_VFS_URI_HIDE_PASSWORD);
	destheader = g_strdup_printf ("Destination: %s\r\nOverwrite: %c\r\n", destpath, force_replace ? 'T' : 'F' );

	result = make_request (&handle, old_uri, "MOVE", NULL, destheader, context, FALSE);
	http_handle_close (handle, context);
	g_free (destheader);
	handle = NULL;

	if (result == GNOME_VFS_ERROR_NOT_FOUND) {
		result = resolve_409 (method, new_uri, context);
	}

	http_cache_invalidate_uri_parent (old_uri);
	http_cache_invalidate_uri_parent (new_uri);

	DEBUG_HTTP (("-Move (%d)", result));
	ANALYZE_HTTP ("==> -do_move");

	return result;
}


static GnomeVFSResult 
do_unlink(GnomeVFSMethod *method,
	GnomeVFSURI *uri,
	  GnomeVFSContext *context)
{
	GnomeVFSResult result;

	/* FIXME need to make sure this fails on directories */
	ANALYZE_HTTP ("==> +do_unlink");
	DEBUG_HTTP (("+Unlink URI: '%s'", gnome_vfs_uri_to_string (uri, 0)));
	result = do_remove_directory (method, uri, context);
	DEBUG_HTTP (("-Unlink (%d)", result));
	ANALYZE_HTTP ("==> -do_unlink");
	
	return result;
}

static GnomeVFSResult 
do_check_same_fs (GnomeVFSMethod *method,
		  GnomeVFSURI *a,
		  GnomeVFSURI *b,
		  gboolean *same_fs_return,
		  GnomeVFSContext *context)
{
	*same_fs_return = is_same_fs (a, b);
	
	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_set_file_info (GnomeVFSMethod *method,
		  GnomeVFSURI *uri,
		  const GnomeVFSFileInfo *info,
		  GnomeVFSSetFileInfoMask mask,
		  GnomeVFSContext *context)
{
	GnomeVFSURI *parent_uri, *new_uri;
	GnomeVFSResult result;
	
	/* FIXME: For now, we only support changing the name. */
	if ((mask & ~(GNOME_VFS_SET_FILE_INFO_NAME)) != 0) {
		return GNOME_VFS_ERROR_NOT_SUPPORTED;
	}
	
	/* FIXME bugzillagnome.org 40645: Make sure this returns an
	 * error for incoming names with "/" characters in them,
	 * instead of moving the file.
	 */
	
	/* Share code with do_move. */
	parent_uri = gnome_vfs_uri_get_parent (uri);
	if (parent_uri == NULL) {
		return GNOME_VFS_ERROR_NOT_FOUND;
	}
	new_uri = gnome_vfs_uri_append_file_name (parent_uri, info->name);
	gnome_vfs_uri_unref (parent_uri);
	result = do_move (method, uri, new_uri, FALSE, context);
	gnome_vfs_uri_unref (new_uri);
	return result;
}

static GnomeVFSMethod method = {
	sizeof (GnomeVFSMethod),
	do_open,
	do_create,
	do_close,
	do_read,
	do_write,
	NULL, /* seek */
	NULL, /* tell */
	NULL, /* truncate_handle */
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
	NULL, /* truncate */
	NULL, /* find_directory */
	NULL  /* create_symbolic_link */
};

GnomeVFSMethod *
vfs_module_init (const char *method_name, 
		 const char *args)
{
	GError *gconf_error = NULL;
	gboolean use_proxy;
	gboolean use_proxy_auth;

	LIBXML_TEST_VERSION
		
	gl_client = gconf_client_get_default ();

	gl_mutex = g_mutex_new ();
	
	gconf_client_add_dir (gl_client, PATH_GCONF_GNOME_VFS, GCONF_CLIENT_PRELOAD_ONELEVEL, &gconf_error);
	if (gconf_error) {
		DEBUG_HTTP (("GConf error during client_add_dir '%s'", gconf_error->message));
		g_error_free (gconf_error);
		gconf_error = NULL;
	}

	gconf_client_notify_add (gl_client, PATH_GCONF_GNOME_VFS, notify_gconf_value_changed, NULL, NULL, &gconf_error);
	if (gconf_error) {
		DEBUG_HTTP (("GConf error during notify_error '%s'", gconf_error->message));
		g_error_free (gconf_error);
		gconf_error = NULL;
	}

	/* Load the http proxy setting */	
	use_proxy = gconf_client_get_bool (gl_client, KEY_GCONF_USE_HTTP_PROXY, &gconf_error);

	if (gconf_error != NULL) {
		DEBUG_HTTP (("GConf error during client_get_bool '%s'", gconf_error->message));
		g_error_free (gconf_error);
		gconf_error = NULL;
	} else {
		construct_gl_http_proxy (use_proxy);
	}

	use_proxy_auth = gconf_client_get_bool (gl_client, KEY_GCONF_HTTP_USE_AUTH, &gconf_error);

	if (gconf_error != NULL) {
		DEBUG_HTTP (("GConf error during client_get_bool '%s'", gconf_error->message));
		g_error_free (gconf_error);
		gconf_error = NULL;
	} else {
		set_proxy_auth (use_proxy_auth);
	}

	http_authn_init ();
	http_cache_init ();

	return &method;
}

void
vfs_module_shutdown (GnomeVFSMethod *method)
{
	g_object_unref (G_OBJECT (gl_client));

	http_authn_shutdown ();
	
	http_cache_shutdown();

	g_mutex_free (gl_mutex);

	gl_client = NULL;
}

/* A "409 Conflict" currently maps to GNOME_VFS_ERROR_NOT_FOUND because it can be returned
 * when the parent collection/directory does not exist.  Unfortunately, Xythos also returns
 * this code when the destination filename of a PUT, MKCOL, or MOVE contains illegal characters, 
 * eg "my*file:name".
 * 
 * The only way to resolve this is to ask...
 */

static GnomeVFSResult
resolve_409 (GnomeVFSMethod *method, GnomeVFSURI *uri, GnomeVFSContext *context)
{
	GnomeVFSFileInfo *file_info;
	GnomeVFSURI *parent_dest_uri;
	GnomeVFSResult result;


	ANALYZE_HTTP ("==> +resolving 409");

	file_info = gnome_vfs_file_info_new ();
	parent_dest_uri = gnome_vfs_uri_get_parent (uri);

	if (parent_dest_uri != NULL) {
		result = do_get_file_info (method,
					   parent_dest_uri,
					   file_info,
					   GNOME_VFS_FILE_INFO_DEFAULT,
					   context);

		gnome_vfs_file_info_unref (file_info);
		file_info = NULL;
		
		gnome_vfs_uri_unref (parent_dest_uri);
		parent_dest_uri = NULL;
	} else {
		result = GNOME_VFS_ERROR_NOT_FOUND;
	}
	
	if (result == GNOME_VFS_OK) {
		/* The destination filename contains characters that are not allowed
		 * by the server.  This is a bummer mapping, but EINVAL is what
		 * the Linux FAT filesystems return on bad filenames, so at least
		 * its not without precedent...
		 */ 
		result = GNOME_VFS_ERROR_BAD_PARAMETERS;
	} else {
		/* The destination's parent path does not exist */
		result = GNOME_VFS_ERROR_NOT_FOUND;
	}

	ANALYZE_HTTP ("==> -resolving 409");

	return result;
}

static gboolean
invoke_callback_headers_received (HttpFileHandle *handle)
{
	GnomeVFSModuleCallbackReceivedHeadersIn in_args;
	GnomeVFSModuleCallbackReceivedHeadersOut out_args;
	gboolean ret = FALSE;

	memset (&in_args, 0, sizeof (in_args));
	memset (&out_args, 0, sizeof (out_args));

	in_args.uri = handle->uri;
	in_args.headers = handle->response_headers;

	ret = gnome_vfs_module_callback_invoke (GNOME_VFS_MODULE_CALLBACK_HTTP_RECEIVED_HEADERS,
						&in_args, sizeof (in_args),
						&out_args, sizeof (out_args));

	return ret;
}

static gboolean
invoke_callback_send_additional_headers (GnomeVFSURI  *uri,
					 GList       **headers)
{
	GnomeVFSModuleCallbackAdditionalHeadersIn in_args;
	GnomeVFSModuleCallbackAdditionalHeadersOut out_args;
	gboolean ret = FALSE;

	memset (&in_args, 0, sizeof (in_args));
	memset (&out_args, 0, sizeof (out_args));

	in_args.uri = uri;

	ret = gnome_vfs_module_callback_invoke (GNOME_VFS_MODULE_CALLBACK_HTTP_SEND_ADDITIONAL_HEADERS,
						&in_args, sizeof (in_args),
						&out_args, sizeof (out_args));

	if (ret) {
		*headers = out_args.headers;
		return TRUE;
	}

	if (out_args.headers) {
		g_list_foreach (out_args.headers, (GFunc)g_free, NULL);
		g_list_free (out_args.headers);
	}

	*headers = NULL;

	return FALSE;
}

static gboolean
invoke_callback_basic_authn_fill (HttpFileHandle *handle, 
				  enum AuthnHeaderType authn_which)
{
	GnomeVFSModuleCallbackFillAuthenticationIn in_args;
	GnomeVFSModuleCallbackFillAuthenticationOut out_args;
	gboolean ret;
	char *username;
	

	ret = FALSE;
	
	memset (&in_args, 0, sizeof (in_args));
	memset (&out_args, 0, sizeof (out_args));

	in_args.uri = gnome_vfs_uri_to_string (handle->uri, GNOME_VFS_URI_HIDE_NONE);
	in_args.server = (char *)gnome_vfs_uri_get_host_name (handle->uri);
	in_args.port = gnome_vfs_uri_get_host_port (handle->uri);
	username = (char *)gnome_vfs_uri_get_user_name (handle->uri);
	if (username != NULL && username[0] != 0) {
		in_args.username = username;
	}
	in_args.protocol = "http";
	in_args.authtype = authn_which == AuthnHeader_WWW ? "basic" : "proxy";

	ret = http_authn_parse_response_header_basic (authn_which, handle->response_headers, &in_args.object);
		
	if (!ret) {
		goto error;
	}

	DEBUG_HTTP (("Invoking %s fill authentication callback for uri %s",
		authn_which == AuthnHeader_WWW ? "basic" : "proxy", in_args.uri));

	ret = gnome_vfs_module_callback_invoke (GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION,
						&in_args, sizeof (in_args), 
						&out_args, sizeof (out_args)); 

	if (!ret) {
		DEBUG_HTTP (("No callback registered"));
		goto error;
	}
	
	ret = out_args.valid;

	if (!ret) {
		DEBUG_HTTP (("No username provided by callback"));
		goto error;
	}

	DEBUG_HTTP (("Back from authentication callback, adding credentials"));

	if (authn_which == AuthnHeader_WWW) {
		http_authn_session_add_credentials (handle->uri, out_args.username, out_args.password);
	} else /* if (authn_which == AuthnHeader_Proxy) */ {
		proxy_set_authn (out_args.username, out_args.password);
	}
error:
	g_free (in_args.uri);
	g_free (in_args.object);
	g_free (out_args.username);
	g_free (out_args.domain);
	g_free (out_args.password);

	return ret;
}


static gboolean
invoke_callback_basic_authn (HttpFileHandle *handle, 
			     enum AuthnHeaderType authn_which,
			     gboolean previous_attempt_failed,
			     HttpAuthSave **auth_save_out)
{
	GnomeVFSModuleCallbackFullAuthenticationIn in_args;
	GnomeVFSModuleCallbackFullAuthenticationOut out_args;
	gboolean ret;
	char *username;
	HttpAuthSave *auth_save;

	ret = FALSE;

	*auth_save_out = NULL;
	memset (&in_args, 0, sizeof (in_args));
	memset (&out_args, 0, sizeof (out_args));

	in_args.uri = gnome_vfs_uri_to_string (handle->uri, GNOME_VFS_URI_HIDE_NONE);
	in_args.server = (char *)gnome_vfs_uri_get_host_name (handle->uri);
	in_args.port = gnome_vfs_uri_get_host_port (handle->uri);
	username = (char *)gnome_vfs_uri_get_user_name (handle->uri);
	if (username != NULL && username[0] != 0) {
		in_args.username = username;
	}
	in_args.protocol = "http";
	in_args.authtype = authn_which == AuthnHeader_WWW ? "basic" : "proxy";

	in_args.flags = GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_PASSWORD | GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_SAVING_SUPPORTED;
	if (in_args.username == NULL) {
		in_args.flags |= GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_USERNAME;
	}	
	if (previous_attempt_failed) {
		in_args.flags |= GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_PREVIOUS_ATTEMPT_FAILED;
	}
	
	in_args.default_user = in_args.username;
	
	ret = http_authn_parse_response_header_basic (authn_which, handle->response_headers, &in_args.object);

	if (!ret) {
		goto error;
	}

	DEBUG_HTTP (("Invoking %s authentication callback for uri %s",
		authn_which == AuthnHeader_WWW ? "basic" : "proxy", in_args.uri));

	ret = gnome_vfs_module_callback_invoke (GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION,
						&in_args, sizeof (in_args), 
						&out_args, sizeof (out_args)); 

	if (!ret) {
		DEBUG_HTTP (("No callback registered"));
		goto error;
	}

	ret = !out_args.abort_auth;

	if (!ret) {
		DEBUG_HTTP (("No username provided by callback"));
		goto error;
	}

	DEBUG_HTTP (("Back from authentication callback, adding credentials"));

	username = (char *)gnome_vfs_uri_get_user_name (handle->uri);
	if (username == NULL || username[0] == 0) {
		username = out_args.username;
	}

	if (out_args.save_password) {
		auth_save = g_new (HttpAuthSave, 1);
		*auth_save_out = auth_save;
		auth_save->username = g_strdup (username);
		auth_save->password = g_strdup (out_args.password);
		auth_save->keyring = g_strdup (out_args.keyring);
		auth_save->realm = g_strdup (in_args.object);
		auth_save->type = authn_which;
	}
	
	if (authn_which == AuthnHeader_WWW) {
		http_authn_session_add_credentials (handle->uri, username, out_args.password);
	} else /* if (authn_which == AuthnHeader_Proxy) */ {
		proxy_set_authn (username, out_args.password);
	}
error:
	g_free (in_args.uri);
	g_free (in_args.object);
	g_free (out_args.username);
	g_free (out_args.domain);
	g_free (out_args.password);
	g_free (out_args.keyring);

	return ret;
}

static gboolean
invoke_callback_save_authn (HttpFileHandle *handle, 
			    enum AuthnHeaderType authn_which,
			    HttpAuthSave *auth_save)
{
	GnomeVFSModuleCallbackSaveAuthenticationIn in_args;
	GnomeVFSModuleCallbackSaveAuthenticationOut out_args;
	gboolean ret;

	ret = FALSE;

	memset (&in_args, 0, sizeof (in_args));
	memset (&out_args, 0, sizeof (out_args));

	in_args.keyring = auth_save->keyring;

	in_args.uri = gnome_vfs_uri_to_string (handle->uri, GNOME_VFS_URI_HIDE_NONE);
	in_args.server = (char *)gnome_vfs_uri_get_host_name (handle->uri);
	in_args.port = gnome_vfs_uri_get_host_port (handle->uri);
	in_args.username = auth_save->username;
	in_args.password = auth_save->password;
	in_args.protocol = "http";
	in_args.authtype = authn_which == AuthnHeader_WWW ? "basic" : "proxy";
	in_args.object = auth_save->realm;

	DEBUG_HTTP (("Invoking %s authentication save callback for uri %s",
		authn_which == AuthnHeader_WWW ? "basic" : "proxy", in_args.uri));

	ret = gnome_vfs_module_callback_invoke (GNOME_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION,
						&in_args, sizeof (in_args), 
						&out_args, sizeof (out_args)); 

	if (!ret) {
		DEBUG_HTTP (("No callback registered"));
		goto error;
	}

error:
	g_free (in_args.uri);

	return ret;
}



static int
strcmp_allow_nulls (const char *s1, const char *s2)
{
	return strcmp (s1 == NULL ? "" : s1, s2 == NULL ? "" : s2);
}


/* Returns TRUE if the given URL has changed authentication credentials
 * from the last request (eg, another thread updated the authn information) 
 * or if the application provided new credentials via a callback
 *
 * prev_authn_header is NULL if the previous request contained no authn information.
 */

gboolean
check_authn_retry_request (HttpFileHandle * http_handle,
			   enum AuthnHeaderType authn_which,
			   const char *prev_authn_header,
			   gboolean first_request,
			   HttpAuthSave **auth_save)
{
	gboolean ret;
	char *current_authn_header;

	*auth_save = NULL;
	current_authn_header = NULL;
	
	g_mutex_lock (gl_mutex);

	if (authn_which == AuthnHeader_WWW) {
		current_authn_header = http_authn_get_header_for_uri (http_handle->uri);
	} else if (authn_which == AuthnHeader_Proxy) {
		current_authn_header = proxy_get_authn_header_for_uri_nolock (http_handle->uri);
	} else {
		g_assert_not_reached ();
	}

	ret = FALSE;
	if (0 == strcmp_allow_nulls (current_authn_header, prev_authn_header)) {
		if (first_request) {
			ret = invoke_callback_basic_authn_fill (http_handle, authn_which);
		}
		if (!ret) {
			/* TODO: shouldn't say previous_attempt_failed if the fill callback failed */
			ret = invoke_callback_basic_authn (http_handle, authn_which, prev_authn_header == NULL, auth_save);
		}
	} else {
		ret = TRUE;
	}

	g_mutex_unlock (gl_mutex);

	g_free (current_authn_header);

	return ret;
} 


utime_t
http_util_get_utime (void)
{
    struct timeval tmp;
    gettimeofday (&tmp, NULL);
    return (utime_t)tmp.tv_usec + ((gint64)tmp.tv_sec) * 1000000LL;
}


/* BASE64 code ported from neon (http://www.webdav.org/neon) */
static const gchar b64_alphabet[65] = {
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/=" };

gchar *
http_util_base64 (const gchar *text)
{
	/* The tricky thing about this is doing the padding at the end,
	 * doing the bit manipulation requires a bit of concentration only */
	gchar *buffer, *point;
	gint inlen, outlen;

	/* Use 'buffer' to store the output. Work out how big it should be...
	 * This must be a multiple of 4 bytes 
	 */

	inlen = strlen (text);
	outlen = (inlen*4)/3;
	if ((inlen % 3) > 0) { /* got to pad */
		outlen += 4 - (inlen % 3);
	}

	buffer = g_malloc (outlen + 1); /* +1 for the \0 */

	/* now do the main stage of conversion, 3 bytes at a time,
	 * leave the trailing bytes (if there are any) for later
	 */

	for (point=buffer; inlen>=3; inlen-=3, text+=3) {
		*(point++) = b64_alphabet[ (*text)>>2 ];
		*(point++) = b64_alphabet[ ((*text)<<4 & 0x30) | (*(text+1))>>4 ];
		*(point++) = b64_alphabet[ ((*(text+1))<<2 & 0x3c) | (*(text+2))>>6 ];
		*(point++) = b64_alphabet[ (*(text+2)) & 0x3f ];
	}

	/* Now deal with the trailing bytes */
	if (inlen) {
		/* We always have one trailing byte */
		*(point++) = b64_alphabet[ (*text)>>2 ];
		*(point++) = b64_alphabet[ ( ((*text)<<4 & 0x30) |
									 (inlen==2?(*(text+1))>>4:0) ) ];
		*(point++) = (inlen==1?'=':b64_alphabet[ (*(text+1))<<2 & 0x3c ] );
		*(point++) = '=';
	}

	/* Null-terminate */
	*point = '\0';

	return buffer;
}

static gboolean at_least_one_test_failed = FALSE;

static void
test_failed (const char *format, ...)
{
	va_list arguments;
	char *message;

	va_start (arguments, format);
	message = g_strdup_vprintf (format, arguments);
	va_end (arguments);

	g_message ("test failed: %s", message);
	at_least_one_test_failed = TRUE;
}

#define VERIFY_BOOLEAN_RESULT(function, expected) \
	G_STMT_START {											\
		gboolean result = function; 								\
		if (! ((result && expected) || (!result && !expected))) {				\
			test_failed ("%s: returned '%d' expected '%d'", #function, (int)result, (int)expected);	\
		}											\
	} G_STMT_END


static gboolean
http_self_test (void)
{
	g_message ("self-test: http\n");

	VERIFY_BOOLEAN_RESULT (proxy_should_for_hostname ("localhost"), FALSE);
	VERIFY_BOOLEAN_RESULT (proxy_should_for_hostname ("LocalHost"), FALSE);
	VERIFY_BOOLEAN_RESULT (proxy_should_for_hostname ("127.0.0.1"), FALSE);
	VERIFY_BOOLEAN_RESULT (proxy_should_for_hostname ("127.127.0.1"), FALSE);
	VERIFY_BOOLEAN_RESULT (proxy_should_for_hostname ("www.yahoo.com"), TRUE);

	return !at_least_one_test_failed;
}

gboolean vfs_module_self_test (void);

gboolean
vfs_module_self_test (void)
{
	gboolean ret;

	ret = TRUE;

	ret = http_authn_self_test () && ret;

	ret = http_self_test () && ret;

	return ret;
}

