/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* gnome-vfs-ssl.c
 *
 * Copyright (C) 2001 Ian McKellar
 * Copyright (C) 2002 Andrew McDonald
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
 */
/*
 * Authors: Ian McKellar <yakk@yakk.net>
 *   My knowledge of SSL programming is due to reading Jeffrey Stedfast's
 *   excellent SSL implementation in Evolution.
 *
 *          Andrew McDonald <andrew@mcdonald.org.uk>
 *   Basic SSL/TLS support using the LGPL'ed GNUTLS Library
 */

#include <config.h>
#include "gnome-vfs-ssl.h"

#include "gnome-vfs-ssl-private.h"
#include "gnome-vfs-private-utils.h"
#include "gnome-vfs-resolve.h"
#include <glib/gmem.h>
#include <string.h>

#ifdef HAVE_OPENSSL
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#elif defined HAVE_GNUTLS
#include <gcrypt.h>
#include <gnutls/gnutls.h>
#endif
#if defined(HAVE_OPENSSL) || defined(HAVE_GNUTLS)
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#ifndef G_OS_WIN32
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif
#endif

#if defined GNUTLS_COMPAT
#define gnutls_certificate_credentials GNUTLS_CERTIFICATE_CREDENTIALS
#define gnutls_session GNUTLS_STATE
#define gnutls_certificate_free_credentials gnutls_certificate_free_sc
#define gnutls_certificate_allocate_credentials gnutls_certificate_allocate_sc
#endif

typedef struct {
#ifdef HAVE_OPENSSL
	int sockfd;
	SSL *ssl;
	struct timeval *timeout;
#elif defined HAVE_GNUTLS
	int sockfd;
	gnutls_session tlsstate;
	gnutls_certificate_client_credentials xcred;
	struct timeval *timeout;
#elif defined HAVE_NSS
	PRFileDesc *sockfd;
#else
	char	dummy;
#endif
} GnomeVFSSSLPrivate;

struct GnomeVFSSSL {
	GnomeVFSSSLPrivate *private;
};

#ifdef HAVE_GNUTLS
static int gcry_gthread_mutex_init (void **priv)			      
{									      
	GMutex* lock = g_mutex_new();
	if (!lock)								      
		return ENOMEM;							      
	*priv = lock;							      
	return 0;								      
}									      

static int gcry_gthread_mutex_destroy (void **lock)			      
{ 
	g_mutex_free (*lock);
	return 0; 
}

static int gcry_gthread_mutex_lock (void **lock)			      
{ 
	g_mutex_lock(*lock);
	return 0;
}				      

static int gcry_gthread_mutex_unlock (void **lock)			      
{ 
	g_mutex_unlock(*lock);
	return 0;
}				      

static struct gcry_thread_cbs gcry_threads_gthread =			      
{
	GCRY_THREAD_OPTION_USER, NULL,					      
	gcry_gthread_mutex_init, gcry_gthread_mutex_destroy,			      
	gcry_gthread_mutex_lock, gcry_gthread_mutex_unlock 
};

#endif

void 
_gnome_vfs_ssl_init (void) {
#ifdef HAVE_OPENSSL
	SSL_library_init ();
#elif defined HAVE_GNUTLS
	gcry_control (GCRYCTL_SET_THREAD_CBS, &gcry_threads_gthread);
	gnutls_global_init();
#endif
}

/**
 * gnome_vfs_ssl_enabled:
 *
 * Checks whether GnomeVFS was compiled with SSL support.
 *
 * Return value: %TRUE if GnomeVFS was compiled with SSL support,
 * otherwise %FALSE.
 **/
gboolean
gnome_vfs_ssl_enabled (void)
{
#if defined HAVE_OPENSSL || defined HAVE_GNUTLS
	return TRUE;
#else
	return FALSE;
#endif
}


#ifdef HAVE_GNUTLS
/*
  From the gnutls documentation:

  To tell you whether a file descriptor should be selected for either reading 
  or writing, gnutls_record_get_direction() returns 0 if the interrupted 
  function was trying to read data, and 1 if it was trying to write data.

*/

#ifndef SSL_ERROR_WANT_READ
#define SSL_ERROR_WANT_READ 0
#endif

#ifndef SSL_ERROR_WANT_WRITE
#define SSL_ERROR_WANT_WRITE 1
#endif

#endif

#if defined(HAVE_OPENSSL) || defined(HAVE_GNUTLS)

static GnomeVFSResult
handle_ssl_read_write (int fd, int error, struct timeval *timeout,
		       GnomeVFSCancellation *cancellation)
{
	fd_set   read_fds;
	fd_set   write_fds;
	int res;
	int max_fd;
	int cancel_fd;
	struct timeval tout;

	cancel_fd = -1;
	
 retry:
	FD_ZERO (&read_fds);
	FD_ZERO (&write_fds);
	max_fd = fd;
	
	if (cancellation != NULL) {
		cancel_fd = gnome_vfs_cancellation_get_fd (cancellation);
		FD_SET (cancel_fd, &read_fds);
		max_fd = MAX (max_fd, cancel_fd);
	}
	
	if (error == SSL_ERROR_WANT_READ) {
		FD_SET (fd, &read_fds);
	}
	if (error == SSL_ERROR_WANT_WRITE) {
		FD_SET (fd, &write_fds);
	}
	
	if (timeout != NULL) {
		tout.tv_sec  = timeout->tv_sec;
		tout.tv_usec = timeout->tv_usec;
	}

	res = select (max_fd + 1, &read_fds, &write_fds, NULL, 
		      timeout ? &tout : NULL);
	
	if (res == -1 && errno == EINTR) {
		goto retry;
	}
	
	if (res == 0) {
		return GNOME_VFS_ERROR_TIMEOUT;
	} else if (res > 0) {
		if (cancel_fd != -1 && FD_ISSET (cancel_fd, &read_fds)) {
			return GNOME_VFS_ERROR_CANCELLED;
		}

		return GNOME_VFS_OK;
	}
	return GNOME_VFS_ERROR_INTERNAL;
}

#endif

/**
 * gnome_vfs_ssl_create:
 * @handle_return: pointer to a GnmoeVFSSSL struct, which will
 * contain an allocated GnomeVFSSSL object on return.
 * @host: string indicating the host to establish an SSL connection with.
 * @port: the port number to connect to.
 * @cancellation: handle allowing cancellation of the operation.
 *
 * Creates an SSL socket connection at @handle_return to @host using
 * port @port.
 *
 * Return value: GnomeVFSResult indicating the success of the operation
 **/
GnomeVFSResult
gnome_vfs_ssl_create (GnomeVFSSSL **handle_return, 
		      const char   *host, 
		      unsigned int  port,
		      GnomeVFSCancellation *cancellation)
{
/* FIXME: add *some* kind of cert verification! */
#if defined(HAVE_OPENSSL) || defined(HAVE_GNUTLS)
	GnomeVFSResolveHandle *rh;
	GnomeVFSAddress *address;
	GnomeVFSResult res;
	gint sock, len, ret;
	struct sockaddr *saddr;

	g_return_val_if_fail (handle_return != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (host != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (port != 0, GNOME_VFS_ERROR_BAD_PARAMETERS);

	res = gnome_vfs_resolve (host, &rh);

	if (res != GNOME_VFS_OK)
		return res;

	sock = -1;
	
	while (gnome_vfs_resolve_next_address (rh, &address)) {
		sock = socket (gnome_vfs_address_get_family_type (address),
			       SOCK_STREAM, 0);

		if (sock > -1) {
			saddr = gnome_vfs_address_get_sockaddr (address,
								port,
								&len);
			ret = connect (sock, saddr, len);
			g_free (saddr);
			
			if (ret == 0)
				break;

			close (sock);
			sock = -1;
		}

		gnome_vfs_address_free (address);
		
	}

	gnome_vfs_resolve_free (rh);
	
	if (sock < 0)
		return gnome_vfs_result_from_errno ();

	_gnome_vfs_socket_set_blocking (sock, FALSE);
	
	gnome_vfs_address_free (address);
	
	return gnome_vfs_ssl_create_from_fd (handle_return, sock, cancellation);
#else
	return GNOME_VFS_ERROR_NOT_SUPPORTED;
#endif
}

#ifdef HAVE_GNUTLS
static const int protocol_priority[] = {GNUTLS_TLS1, GNUTLS_SSL3, 0};
static const int cipher_priority[] = 
	{GNUTLS_CIPHER_RIJNDAEL_128_CBC, GNUTLS_CIPHER_3DES_CBC,
	 GNUTLS_CIPHER_RIJNDAEL_256_CBC, GNUTLS_CIPHER_ARCFOUR, 0};
static const int comp_priority[] =
	{GNUTLS_COMP_ZLIB, GNUTLS_COMP_NULL, 0};
static const int kx_priority[] =
	{GNUTLS_KX_DHE_RSA, GNUTLS_KX_RSA, GNUTLS_KX_DHE_DSS, 0};
static const int mac_priority[] =
	{GNUTLS_MAC_SHA, GNUTLS_MAC_MD5, 0};

#endif


/**
 * gnome_vfs_ssl_create_from_fd:
 * @handle_return: pointer to a GnmoeVFSSSL struct, which will
 * contain an allocated GnomeVFSSSL object on return.
 * @fd: file descriptior to try and establish an SSL connection over.
 * @cancellation: handle allowing cancellation of the operation.

 *
 * Try to establish an SSL connection over the file descriptor @fd.
 *
 * Return value: a GnomeVFSResult indicating the success of the operation
 **/
GnomeVFSResult
gnome_vfs_ssl_create_from_fd (GnomeVFSSSL **handle_return, 
		              gint fd,
			      GnomeVFSCancellation *cancellation)
{
#ifdef HAVE_OPENSSL
	GnomeVFSSSL *ssl;
	SSL_CTX *ssl_ctx = NULL;
	int ret;
	int error;
	GnomeVFSResult res;

	ssl = g_new0 (GnomeVFSSSL, 1);
	ssl->private = g_new0 (GnomeVFSSSLPrivate, 1);
	ssl->private->sockfd = fd;

        /* SSLv23_client_method will negotiate with SSL v2, v3, or TLS v1 */
        ssl_ctx = SSL_CTX_new (SSLv23_client_method ());

	if (ssl_ctx == NULL) {
		return GNOME_VFS_ERROR_INTERNAL;
	}

        /* FIXME: SSL_CTX_set_verify (ssl_ctx, SSL_VERIFY_PEER, &ssl_verify);*/
        ssl->private->ssl = SSL_new (ssl_ctx);

	if (ssl->private->ssl == NULL) {
		return GNOME_VFS_ERROR_IO;
	}

        SSL_set_fd (ssl->private->ssl, fd);

 retry:
	ret = SSL_connect (ssl->private->ssl);
	if (ret != 1) {
		error = SSL_get_error (ssl->private->ssl, ret);
		res = GNOME_VFS_ERROR_IO;
		if (error == SSL_ERROR_WANT_READ ||
		    error == SSL_ERROR_WANT_WRITE) {
			res = handle_ssl_read_write (fd, error, NULL, cancellation);
			if (res == GNOME_VFS_OK) {
				goto retry;
			} 
		} else if (error == SSL_ERROR_SYSCALL && ret != 0) {
			res = gnome_vfs_result_from_errno ();
		}

	retry_shutdown:
                ret = SSL_shutdown (ssl->private->ssl);
		if (ret != 1) {
			error = SSL_get_error (ssl->private->ssl, ret);
			if (error == SSL_ERROR_WANT_READ ||
			    error == SSL_ERROR_WANT_WRITE) {
				/* No fancy select stuff here, just busy loop */
				goto retry_shutdown;
			}
		}

                if (ssl->private->ssl->ctx)
                        SSL_CTX_free (ssl->private->ssl->ctx);

                SSL_free (ssl->private->ssl);
		g_free (ssl->private);
		g_free (ssl);
		return res;
	}

	*handle_return = ssl;

	return GNOME_VFS_OK;

#elif defined HAVE_GNUTLS
	GnomeVFSResult res;
	GnomeVFSSSL *ssl;
	int err;

	ssl = g_new0 (GnomeVFSSSL, 1);
	ssl->private = g_new0 (GnomeVFSSSLPrivate, 1);
	ssl->private->sockfd = fd;

	err = gnutls_certificate_allocate_credentials (&ssl->private->xcred);
	if (err < 0) {
		g_free (ssl->private);
		g_free (ssl);
		return GNOME_VFS_ERROR_INTERNAL;
	}

	gnutls_init (&ssl->private->tlsstate, GNUTLS_CLIENT);

	/* set socket */
	gnutls_transport_set_ptr (ssl->private->tlsstate, 
				  GINT_TO_POINTER (fd));

	gnutls_protocol_set_priority (ssl->private->tlsstate, protocol_priority);
	gnutls_cipher_set_priority (ssl->private->tlsstate, cipher_priority);
	gnutls_compression_set_priority (ssl->private->tlsstate, comp_priority);
	gnutls_kx_set_priority (ssl->private->tlsstate, kx_priority);
	gnutls_mac_set_priority (ssl->private->tlsstate, mac_priority);

	gnutls_cred_set (ssl->private->tlsstate, GNUTLS_CRD_CERTIFICATE,
			 ssl->private->xcred);

 retry:
	res = GNOME_VFS_ERROR_IO;
	err = gnutls_handshake (ssl->private->tlsstate);

	if (err == GNUTLS_E_INTERRUPTED || err == GNUTLS_E_AGAIN) {

		res = handle_ssl_read_write (ssl->private->sockfd,
					     gnutls_record_get_direction (ssl->private->tlsstate),
					     ssl->private->timeout,
					     cancellation);
		if (res == GNOME_VFS_OK)
			goto retry;
	}

	if (err < 0) {
		gnutls_certificate_free_credentials (ssl->private->xcred);
		gnutls_deinit (ssl->private->tlsstate);
		g_free (ssl->private);
		g_free (ssl);
		return res;
	}

	*handle_return = ssl;

	return GNOME_VFS_OK;
#else
	return GNOME_VFS_ERROR_NOT_SUPPORTED;
#endif
}

/**
 * gnome_vfs_ssl_read:
 * @ssl: SSL socket to read data from
 * @buffer: allocated buffer of at least @bytes bytes to be read into.
 * @bytes: number of bytes to read from @ssl into @buffer.
 * @bytes_read: pointer to a GnomeVFSFileSize, will contain
 * the number of bytes actually read from the socket on return.
 * @cancellation: handle allowing cancellation of the operation.
 *
 * Read @bytes bytes of data from the SSL socket @ssl into @buffer.
 *
 * Return value: GnomeVFSResult indicating the success of the operation
 **/
GnomeVFSResult 
gnome_vfs_ssl_read (GnomeVFSSSL *ssl,
		    gpointer buffer,
		    GnomeVFSFileSize bytes,
		    GnomeVFSFileSize *bytes_read,
		    GnomeVFSCancellation *cancellation)
{
#ifdef HAVE_OPENSSL
	GnomeVFSResult res;
	int ret;
	int error;
	
	if (bytes == 0) {
		*bytes_read = 0;
		return GNOME_VFS_OK;
	}

 retry:
	ret = SSL_read (ssl->private->ssl, buffer, bytes);
	if (ret <= 0) {
		res = GNOME_VFS_ERROR_IO;
		error = SSL_get_error (ssl->private->ssl, ret);
		if (error == SSL_ERROR_WANT_READ ||
		    error == SSL_ERROR_WANT_WRITE) {
			res = handle_ssl_read_write (SSL_get_fd (ssl->private->ssl),
						     error,
						     ssl->private->timeout,
						     cancellation);
			if (res == GNOME_VFS_OK) {
				goto retry;
			}
		} else if (error == SSL_ERROR_SYSCALL) {
			if (ret == 0) {
				res = GNOME_VFS_ERROR_EOF;
			} else {
				res = gnome_vfs_result_from_errno ();
			}
		} else if (error == SSL_ERROR_ZERO_RETURN) {
			res = GNOME_VFS_ERROR_EOF;
		}
		
		*bytes_read = 0;
		return res;
	}
	*bytes_read = ret;
	
	return GNOME_VFS_OK;
#elif defined HAVE_GNUTLS
	GnomeVFSResult res;
	int ret;

	if (bytes == 0) {
		*bytes_read = 0;
		return GNOME_VFS_OK;
	}

	res = GNOME_VFS_OK;
retry:
	ret = gnutls_record_recv (ssl->private->tlsstate, buffer, bytes);

	if (ret == 0) {
		res = GNOME_VFS_ERROR_EOF;
	} else if (ret < 0) {

		res = GNOME_VFS_ERROR_IO;

		if (ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN) {
			res = handle_ssl_read_write (ssl->private->sockfd,
						     gnutls_record_get_direction (ssl->private->tlsstate),
						     ssl->private->timeout,
						     cancellation);

			if (res == GNOME_VFS_OK) {
				goto retry;
			}
		}

		*bytes_read = 0;
		return res;
	}
	
	*bytes_read = ret;
	return res;
#else
	return GNOME_VFS_ERROR_NOT_SUPPORTED;
#endif
}

/**
 * gnome_vfs_ssl_write:
 * @ssl: SSL socket to write data to
 * @buffer: data to write to the socket
 * @bytes: number of bytes from @buffer to write to @ssl
 * @bytes_written: pointer to a GnomeVFSFileSize, will contain
 * the number of bytes actually written to the socket on return
 * @cancellation: handle allowing cancellation of the operation
 *
 * Write @bytes bytes of data from @buffer to @ssl.
 *
 * Return value: GnomeVFSResult indicating the success of the operation
 **/
GnomeVFSResult 
gnome_vfs_ssl_write (GnomeVFSSSL *ssl,
		     gconstpointer buffer,
		     GnomeVFSFileSize bytes,
		     GnomeVFSFileSize *bytes_written,
		     GnomeVFSCancellation *cancellation)
{
#ifdef HAVE_OPENSSL
	GnomeVFSResult res;
	int ret;
	int error;
	
	if (bytes == 0) {
		*bytes_written = 0;
		return GNOME_VFS_OK;
	}

 retry:
	ret = SSL_write (ssl->private->ssl, buffer, bytes);

	if (ret <= 0) {
		res = GNOME_VFS_ERROR_IO;
		
		error = SSL_get_error (ssl->private->ssl, ret);
		if (error == SSL_ERROR_WANT_READ ||
		    error == SSL_ERROR_WANT_WRITE) {
			res = handle_ssl_read_write (SSL_get_fd (ssl->private->ssl),
						     error,
						     ssl->private->timeout,
						     cancellation);
			if (res == GNOME_VFS_OK) {
				goto retry;
			}
		} else if (error == SSL_ERROR_SYSCALL) {
			res = gnome_vfs_result_from_errno ();
		}
	
		*bytes_written = 0;
		return res;
	}
	*bytes_written = ret;
	return GNOME_VFS_OK;
#elif defined HAVE_GNUTLS
	GnomeVFSResult res;
	int ret;

	if (bytes == 0) {
		*bytes_written = 0;
		return GNOME_VFS_OK;
	}

	res = GNOME_VFS_OK;
retry:
	ret = gnutls_record_send (ssl->private->tlsstate, buffer, bytes);

	if (ret == 0) {
		res = GNOME_VFS_ERROR_EOF;
	} else if (ret < 0) {

		res = GNOME_VFS_ERROR_IO;

		if (ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN) {
			res = handle_ssl_read_write (ssl->private->sockfd,
						     gnutls_record_get_direction (ssl->private->tlsstate),
						     ssl->private->timeout,
						     cancellation);

			if (res == GNOME_VFS_OK) {
				goto retry;
			}
		}

		*bytes_written = 0;
		return res;
	}
	
	*bytes_written = ret;
	return res;
#else
	return GNOME_VFS_ERROR_NOT_SUPPORTED;
#endif
}

/**
 * gnome_vfs_ssl_destroy:
 * @ssl: SSL socket to be closed and destroyed
 * @cancellation: handle allowing cancellation of the operation
 *
 * Free resources used by @ssl and close the connection.
 */
void
gnome_vfs_ssl_destroy (GnomeVFSSSL *ssl,
		       GnomeVFSCancellation *cancellation) 
{
#ifdef HAVE_OPENSSL
	int ret;
	GnomeVFSResult res;	
	int error;

 retry:
	ret = SSL_shutdown (ssl->private->ssl);
	if (ret != 1) {
		error = SSL_get_error (ssl->private->ssl, ret);
		if (error == SSL_ERROR_WANT_READ ||
		    error == SSL_ERROR_WANT_WRITE) {
			res = handle_ssl_read_write (SSL_get_fd (ssl->private->ssl),
						     error,
						     ssl->private->timeout,
						     cancellation);
			if (res == GNOME_VFS_OK) {
				goto retry;
			}
		}
	}
	
	SSL_CTX_free (ssl->private->ssl->ctx);
	SSL_free (ssl->private->ssl);
	close (ssl->private->sockfd);
	if (ssl->private->timeout)
		g_free (ssl->private->timeout);
#elif defined HAVE_GNUTLS
	int ret;
	GnomeVFSResult res;
	
 retry:
	ret = gnutls_bye (ssl->private->tlsstate, GNUTLS_SHUT_RDWR);
	
	if (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED) {
		res = handle_ssl_read_write (ssl->private->sockfd,
					     gnutls_record_get_direction (ssl->private->tlsstate),
					     ssl->private->timeout,
					     cancellation);
		if (res == GNOME_VFS_OK)
			goto retry;
	}

	gnutls_certificate_free_credentials (ssl->private->xcred);
	gnutls_deinit (ssl->private->tlsstate);
	close (ssl->private->sockfd);
#else
#endif
	g_free (ssl->private);
	g_free (ssl);
}

/**
 * gnome_vfs_ssl_set_timeout:
 * @ssl: SSL socket to set the timeout of
 * @timeout: the timeout
 * @cancellation: optional cancellation object
 *
 * Set a timeout of @timeout. If @timeout is NULL following operations
 * will block indefinitely).
 *
 * Note if you set @timeout to 0 (means tv_sec and tv_usec are both 0)
 * every following operation will return immediately. (This can be used
 * for polling.)
 *
 * Return value: GnomeVFSResult indicating the success of the operation
 *
 * Since: 2.8
 **/


GnomeVFSResult
gnome_vfs_ssl_set_timeout (GnomeVFSSSL *ssl,
			   GTimeVal *timeout,
			   GnomeVFSCancellation *cancellation)
{
#if defined(HAVE_OPENSSL) || defined(HAVE_GNUTLS)
	/* reset a timeout */
	if (timeout == NULL) {
		if (ssl->private->timeout != NULL) {
			g_free (ssl->private->timeout);
			ssl->private->timeout = NULL;
		}
	} else {
		if (ssl->private->timeout == NULL)
			ssl->private->timeout = g_new0 (struct timeval, 1);

		ssl->private->timeout->tv_sec  = timeout->tv_sec;
		ssl->private->timeout->tv_usec = timeout->tv_usec;
	}
	
	return GNOME_VFS_OK;
#else
	return GNOME_VFS_ERROR_NOT_SUPPORTED;
#endif
}

static GnomeVFSSocketImpl ssl_socket_impl = {
	(GnomeVFSSocketReadFunc)gnome_vfs_ssl_read,
	(GnomeVFSSocketWriteFunc)gnome_vfs_ssl_write,
	(GnomeVFSSocketCloseFunc)gnome_vfs_ssl_destroy,
	(GnomeVFSSocketSetTimeoutFunc)gnome_vfs_ssl_set_timeout,
};

/**
 * gnome_vfs_ssl_to_socket:
 * @ssl: SSL socket to convert into a standard socket
 *
 * Wrapper an SSL socket inside a standard GnomeVFSSocket.
 *
 * Return value: a newly allocated GnomeVFSSocket corresponding to @ssl.
 **/
GnomeVFSSocket *
gnome_vfs_ssl_to_socket (GnomeVFSSSL *ssl)
{
	return gnome_vfs_socket_new (&ssl_socket_impl, ssl);
}
