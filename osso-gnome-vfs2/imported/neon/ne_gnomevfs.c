/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-dns-sd.c - DNS-SD functions

Copyright (C) 2004 Christian Kellner <gicmo@gnome-de.org>

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
   
*/

#include <config.h>

#include "ne_socket.h"
#include "ne_ssl.h"
#include "ne_session.h"
#include "ne_private.h"
#include "ne_request.h"
#include "ne_gnomevfs.h"

#include <errno.h>

#include <libgnomevfs/gnome-vfs-cancellation.h>
#include <libgnomevfs/gnome-vfs-context.h>
#include <libgnomevfs/gnome-vfs-result.h>
#include <libgnomevfs/gnome-vfs-resolve.h>
#include <libgnomevfs/gnome-vfs-inet-connection.h>
#include <libgnomevfs/gnome-vfs-ssl.h>


#define peek_cancellation(_c) _c = gnome_vfs_context_get_cancellation (\
							gnome_vfs_context_peek_current ())

/* ************************************************************************** */
struct ne_socket_s {
	   
	GnomeVFSInetConnection *connection;
	GnomeVFSResult last_error;
	GnomeVFSSocketBuffer *socket_buffer;
	GnomeVFSSocket       *socket;
};

typedef struct ne_socket_s ne_socket_ssl;


struct ne_sock_addr_s {
	GnomeVFSResolveHandle *handle;
	GnomeVFSResult result;
	GnomeVFSAddress *last;
};


typedef struct GnomeVFSAddress ne_inet_addr_s;

/* ************************************************************************** */

GnomeVFSResult
ne_gnomevfs_last_error (ne_request *req)
{
	ne_session *sess;
	
	sess = ne_get_session (req);

	if (sess && sess->socket)
		return sess->socket->last_error;
	
	return GNOME_VFS_OK;
}

/* ************************************************************************** */
#ifndef G_OS_WIN32
#define check_error(__result, __sock) __sock->last_error = __result;   		      	      \
				      switch (__result) {		              	      \
				      case GNOME_VFS_OK: break; 			      \
				      case GNOME_VFS_ERROR_TIMEOUT: return NE_SOCK_TIMEOUT;   \
				      case GNOME_VFS_ERROR_EOF: return NE_SOCK_CLOSED;        \
				      case GNOME_VFS_ERROR_GENERIC:  		      	      \
					      if (errno == EPIPE)			      \
						      return NE_SOCK_CLOSED;		      \
					      else if (errno == ECONNRESET)		      \
						      return NE_SOCK_RESET;		      \
				      default: return NE_SOCK_ERROR; }
#else
#define check_error(__result, __sock) __sock->last_error = __result;   		      	      \
				      switch (__result) {		              	      \
				      case GNOME_VFS_OK: break; 			      \
				      case GNOME_VFS_ERROR_TIMEOUT: return NE_SOCK_TIMEOUT;   \
				      case GNOME_VFS_ERROR_EOF: return NE_SOCK_CLOSED;        \
				      case GNOME_VFS_ERROR_GENERIC: return NE_SOCK_CLOSED;    \
				      default: return NE_SOCK_ERROR; }
#endif

/* ************************************************************************** */
int
ne_sock_init (void)
{
	/* NOOP */
	return 0;
}


void
ne_sock_exit (void)
{
	/* NOOP */
}


/* Resolver API replaced with gnome-vfs ones */
ne_sock_addr *
ne_addr_resolve (const char *hostname, int flags)
{
	ne_sock_addr *res;

	res = g_new0 (ne_sock_addr, 1);

	res->result = gnome_vfs_resolve (hostname, &(res->handle));

	return res;
}

int
ne_addr_result (const ne_sock_addr *addr)
{
	if (addr->result != GNOME_VFS_OK)
		return NE_SOCK_ERROR;

	return 0;
}


const ne_inet_addr *
ne_addr_first (ne_sock_addr *addr)
{
	GnomeVFSAddress *address;

	if (addr->last != NULL) {
		gnome_vfs_address_free (addr->last);
		gnome_vfs_resolve_reset_to_beginning (addr->handle);
	}

	if (! gnome_vfs_resolve_next_address (addr->handle, &address)) {
		return NULL;
	}

	addr->last = address;
	   
	return (ne_inet_addr *) address;
}


const ne_inet_addr *
ne_addr_next (ne_sock_addr *addr)
{
	GnomeVFSAddress *address;

	if (! gnome_vfs_resolve_next_address (addr->handle, &address)) {
		return NULL;
	}

	if (addr->last != NULL) {
		gnome_vfs_address_free (addr->last);
	}
	   
	addr->last = address;
	return (ne_inet_addr *) address;
	   
}

char *
ne_addr_error (const ne_sock_addr *addr, char *buffer, size_t bufsiz)
{
	const char *error_str;

	error_str = gnome_vfs_result_to_string (addr->result);
	g_strlcpy (buffer, error_str, bufsiz);
	return buffer;
}

void
ne_addr_destroy (ne_sock_addr *addr)
{
	if (addr->last)
		gnome_vfs_address_free (addr->last);

	if (addr->handle)
		gnome_vfs_resolve_free (addr->handle);

	g_free (addr);
}

ne_inet_addr *
ne_iaddr_make (ne_iaddr_type type, const unsigned char *raw)
{
	/* dummy because not used in neon */
	return NULL;
}


int
ne_iaddr_cmp (const ne_inet_addr *i1, const ne_inet_addr *i2)
{
	/* dummy becaused not used in neon */
	return 0;
}


char *
ne_iaddr_print (const ne_inet_addr *ia, char *buffer, size_t bufsiz)
{
	char *string;
	string = gnome_vfs_address_to_string ((GnomeVFSAddress *) ia);
	g_strlcpy (buffer, string, bufsiz);
	g_free (string);
	return buffer;
}

void ne_iaddr_free (ne_inet_addr *addr)
{
	gnome_vfs_address_free ((GnomeVFSAddress *) addr);
}


/* *************************************************************************** */


ne_socket *
ne_sock_create (void)
{
	return g_new0 (ne_socket, 1);
}


int
ne_sock_connect (ne_socket *sock, const ne_inet_addr *addr, 
		 unsigned int port)
{
	GnomeVFSCancellation *cancellation;
	GnomeVFSAddress *address = (GnomeVFSAddress *) addr;
	   
	peek_cancellation (cancellation);
	
	sock->last_error = gnome_vfs_inet_connection_create_from_address (&(sock->connection),
									  address,
									  port,
									  cancellation);
	   
	check_error (sock->last_error, sock);

	if (sock->last_error == GNOME_VFS_ERROR_EOF)
		return NE_SOCK_ERROR;
	  
	sock->socket = gnome_vfs_inet_connection_to_socket (sock->connection);	
	sock->socket_buffer = gnome_vfs_socket_buffer_new (sock->socket);
			 
	return 0;
}


ssize_t
ne_sock_read (ne_socket *sock, char *buffer, size_t count)
{
	GnomeVFSFileSize bytes_read;
	GnomeVFSResult   result;
	GnomeVFSCancellation *cancellation;

	peek_cancellation (cancellation);
	   
	result = gnome_vfs_socket_buffer_read (sock->socket_buffer,
					       buffer,
					       count,
					       &bytes_read,
					       cancellation);
	check_error (result, sock);
	

	return bytes_read;
}


ssize_t
ne_sock_peek (ne_socket *sock, char *buffer, size_t count)
{
	GnomeVFSResult result;
	GnomeVFSCancellation *cancellation;
	   
	peek_cancellation (cancellation);
	result = gnome_vfs_socket_buffer_peekc (sock->socket_buffer,
						buffer,
						cancellation);

	check_error (result, sock);

	return 1;
}


int
ne_sock_block (ne_socket *sock, int n)
{
	/* dummy (not used outside ne_socket.c) */
	return NE_SOCK_ERROR;
}


int
ne_sock_fullwrite (ne_socket *sock, const char *data, size_t count)
{
	GnomeVFSResult result;
	GnomeVFSFileSize bytes_written;
	GnomeVFSCancellation *cancellation;
	char *pos;

	peek_cancellation (cancellation);
	   
	pos = (char *) data;
	   
	do {
		result = gnome_vfs_socket_write (sock->socket,
						 pos,
						 count,
						 &bytes_written,
						 cancellation);
		count -= bytes_written;
		pos += bytes_written;
			 
	} while (result == GNOME_VFS_OK && count > 0);

	check_error (result, sock);
	   
	return 0;
}


ssize_t
ne_sock_readline (ne_socket *sock, char *buffer, size_t len)
{
	GnomeVFSFileSize bytes_read, total_bytes;
	GnomeVFSResult res;
	GnomeVFSCancellation *cancellation;
	gboolean got_boundary;
	char *pos;

	peek_cancellation (cancellation);
	   
	pos = buffer;
	total_bytes = 0;
	bytes_read = 0;
	   
	do {
			 
		res = gnome_vfs_socket_buffer_read_until (sock->socket_buffer,
							  pos,
							  len,
							  "\n",
							  1,
							  &bytes_read,
							  &got_boundary,
							  cancellation);
		   
		total_bytes += bytes_read;
		len -= bytes_read;
		pos += bytes_read;
			 
	} while (res == GNOME_VFS_OK && !got_boundary && len > 0);

	check_error (res, sock);

	return got_boundary ? total_bytes : NE_SOCK_ERROR;
}

ssize_t
ne_sock_fullread (ne_socket *sock, char *buffer, size_t len)
{
	GnomeVFSFileSize bytes_read, total_bytes;
	GnomeVFSResult res;
	GnomeVFSCancellation *cancellation;
	char *pos;

	peek_cancellation (cancellation);
	   
	pos = buffer;
	total_bytes = 0;
	   
	do {
			 
		res = gnome_vfs_socket_buffer_read (sock->socket_buffer,
						    pos,
						    len,
						    &bytes_read,
						    cancellation);
			 
		total_bytes += bytes_read;
		len -= bytes_read;
		pos += bytes_read;
			 
	} while (res == GNOME_VFS_OK && len > 0);

	check_error (res, sock);

	return total_bytes;
}


int
ne_sock_accept (ne_socket *sock, int fd)
{
	/* dummy (not used outside ne_socket.c) */
	return NE_SOCK_ERROR;
}


int
ne_sock_fd (const ne_socket *sock)
{

	return gnome_vfs_inet_connection_get_fd (sock->connection);
}


int
ne_sock_close (ne_socket *sock)
{
	GnomeVFSCancellation *cancellation;

	peek_cancellation (cancellation);

	if (sock->socket_buffer) {
		gnome_vfs_socket_buffer_flush (sock->socket_buffer,
					       cancellation);
		   
		gnome_vfs_socket_buffer_destroy (sock->socket_buffer,
						 TRUE,
						 cancellation);
	}


	g_free (sock);

	return 0;
}


const char *
ne_sock_error (const ne_socket *sock)
{
	return gnome_vfs_result_to_string (sock->last_error);
}


void
ne_sock_read_timeout (ne_socket *sock, int timeout)
{
	GTimeVal tv;
	GnomeVFSCancellation *cancellation;
	   
	peek_cancellation (cancellation);

	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	   
	gnome_vfs_socket_set_timeout (sock->socket,
				      &tv,
				      cancellation);	   
}


int
ne_service_lookup (const char *name)
{
	/* dummy (not used outside ne_socket.c) */
	return 0;
}


void ne_sock_switch_ssl (ne_socket *sock, void *ssl)
{
	/* dummy (not used outside ne_socket.c) */   
}


int
ne_sock_connect_ssl (ne_socket *sock, ne_ssl_context *ctx,
		     void *userdata)
{
	/* Enable SSL with an already-negotiated SSL socket. */
	return 0;
}


/* ************************************************************************** */

#ifdef NE_HAVE_SSL
int
ne__negotiate_ssl (ne_request *req)
{
	GnomeVFSSSL *ssl;
	GnomeVFSCancellation *cancellation;
	ne_session *sess;
	ne_socket  *sock;
	int fd;

	sess = ne_get_session (req);
	sock = sess->socket;
	
	if (!gnome_vfs_ssl_enabled ()) {
		sock->last_error = GNOME_VFS_ERROR_NOT_SUPPORTED;
		return NE_SOCK_ERROR;
	}
	
	peek_cancellation (cancellation);

	
	fd = gnome_vfs_inet_connection_get_fd (sock->connection);

	sock->last_error = gnome_vfs_ssl_create_from_fd (&ssl,
							 fd,
							 cancellation);

	if (sock->last_error != GNOME_VFS_OK) {
		return NE_SOCK_ERROR;
	}

	gnome_vfs_socket_free (sock->socket);
	sock->socket = gnome_vfs_ssl_to_socket (ssl);

	gnome_vfs_socket_buffer_flush (sock->socket_buffer,
				       cancellation);
	
	sock->last_error = gnome_vfs_socket_buffer_destroy (sock->socket_buffer,
							    FALSE,
							    cancellation);

	gnome_vfs_inet_connection_free (sock->connection, cancellation);
	
	sock->socket_buffer = gnome_vfs_socket_buffer_new (sock->socket);

	return 0;
}
#endif

/* *************************************************************************** */

ne_ssl_context *
ne_ssl_context_create (int mode)
{
	return NULL;
}

void
ne_ssl_context_destroy (ne_ssl_context *ctx)
{

}

/* *************************************************************************** */
char *ne_ssl_readable_dname(const ne_ssl_dname *dn)
{
	return NULL;
}

ne_ssl_certificate *ne_ssl_cert_read(const char *filename)
{
	return NULL;
}

int ne_ssl_cert_cmp(const ne_ssl_certificate *c1, const ne_ssl_certificate *c2)
{
	return 1;
}

const ne_ssl_certificate *ne_ssl_cert_signedby(const ne_ssl_certificate *cert)
{ 
	return NULL;
}

const ne_ssl_dname *ne_ssl_cert_issuer(const ne_ssl_certificate *cert)
{
	return NULL;
}

const ne_ssl_dname *ne_ssl_cert_subject(const ne_ssl_certificate *cert)
{
	return NULL;
}

void ne_ssl_cert_free(ne_ssl_certificate *cert) {}

ne_ssl_client_cert *ne_ssl_clicert_read(const char *filename)
{
	return NULL;
}

const ne_ssl_certificate *ne_ssl_clicert_owner(const ne_ssl_client_cert *ccert)
{
	return NULL;
}

int ne_ssl_clicert_encrypted(const ne_ssl_client_cert *ccert)
{
	return -1;
}

int ne_ssl_clicert_decrypt(ne_ssl_client_cert *ccert, const char *password)
{
	return -1;
}

void ne_ssl_clicert_free(ne_ssl_client_cert *ccert) {}

void ne_ssl_trust_default_ca(ne_session *sess) {}



void ne_ssl_ctx_trustcert(ne_ssl_context *ctx, const ne_ssl_certificate *cert)
{}


int ne_ssl_cert_digest(const ne_ssl_certificate *cert, char digest[60])
{
	return -1;
}

void ne_ssl_cert_validity(const ne_ssl_certificate *cert, char *from, char *until)
{}

const char *ne_ssl_cert_identity(const ne_ssl_certificate *cert)
{
	return NULL;
}


const char *ne_ssl_clicert_name(const ne_ssl_client_cert *ccert)
{
	return NULL;
}

int ne_ssl_dname_cmp(const ne_ssl_dname *dn1, const ne_ssl_dname *dn2)
{
	return -1;
}

int ne_ssl_cert_write(const ne_ssl_certificate *cert, const char *filename)
{
	return -1;
}

char *ne_ssl_cert_export(const ne_ssl_certificate *cert)
{
	return NULL;
}

ne_ssl_certificate *ne_ssl_cert_import(const char *data)
{
	return NULL;
}

void ne_ssl_set_clicert(ne_session *sess, const ne_ssl_client_cert *cc) 
{}

void ne_ssl_context_trustcert(ne_ssl_context *ctx, const ne_ssl_certificate *cert)
{
}
