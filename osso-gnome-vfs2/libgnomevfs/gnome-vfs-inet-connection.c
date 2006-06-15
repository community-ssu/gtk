/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-inet-connection.c - Functions for creating and destroying Internet
   connections.

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

   Author: Ettore Perazzoli <ettore@gnu.org> */

#include <config.h>
#include "gnome-vfs-inet-connection.h"
#include "gnome-vfs-private-utils.h"
#include "gnome-vfs-resolve.h"

#include <errno.h>
#include <glib/gmem.h>
#include <glib/gmessages.h>
#include <string.h>
/* Keep <sys/types.h> above the network includes for FreeBSD. */
#include <sys/types.h>

#ifndef G_OS_WIN32
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif

#include <sys/time.h>

struct GnomeVFSInetConnection {

	GnomeVFSAddress *address;	
	guint sock;
	struct timeval *timeout;
};

/**
 * gnome_vfs_inet_connection_create:
 * @connection_return: Pointer to a GnomeVFSInetConnection, which will.
 * contain an allocated GnomeVFSInetConnection object on return.
 * @host_name: String indicating the host to establish an internet connection with.
 * @host_port: The port number to connect to.
 * @cancellation: handle allowing cancellation of the operation.
 *
 * Creates a connection at @connection_return to @host_name using
 * port @port.
 *
 * Return value: #GnomeVFSResult indicating the success of the operation.
 **/
GnomeVFSResult
gnome_vfs_inet_connection_create (GnomeVFSInetConnection **connection_return,
				  const gchar             *host_name,
				  guint                    host_port,
				  GnomeVFSCancellation    *cancellation)
{
	GnomeVFSInetConnection *new;
	GnomeVFSResolveHandle *rh;
	GnomeVFSAddress *address;
	GnomeVFSResult res;
	gint sock, len, ret;
	struct sockaddr *saddr;

	g_return_val_if_fail (connection_return != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (host_name != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (host_port != 0, GNOME_VFS_ERROR_BAD_PARAMETERS);

	res = gnome_vfs_resolve (host_name, &rh);

	if (res != GNOME_VFS_OK)
		return res;

	sock = -1;
	
	while (gnome_vfs_resolve_next_address (rh, &address)) {
		sock = socket (gnome_vfs_address_get_family_type (address),
			       SOCK_STREAM, 0);
#ifdef G_OS_WIN32
		if (sock == SOCKET_ERROR) {
			_gnome_vfs_map_winsock_error_to_errno ();
			sock = -1;
		}
#endif
		if (sock > -1) {
			saddr = gnome_vfs_address_get_sockaddr (address,
								host_port,
								&len);
			ret = connect (sock, saddr, len);
			g_free (saddr);
			
			if (ret == 0)
				break;

			_GNOME_VFS_SOCKET_CLOSE (sock);
			sock = -1;
		}

		gnome_vfs_address_free (address);
		
	}

	gnome_vfs_resolve_free (rh);
	
	if (sock < 0)
		return gnome_vfs_result_from_errno ();
	
	new = g_new0 (GnomeVFSInetConnection, 1);
	new->address = address;
	new->sock = sock;

	_gnome_vfs_socket_set_blocking (new->sock, FALSE);

	*connection_return = new;
	return GNOME_VFS_OK;
}

/**
 * gnome_vfs_inet_connection_create_from_address:
 * @connection_return: Pointer to a GnomeVFSInetConnection, which will.
 * contain An allocated GnomeVFSInetConnection object on return.
 * @address: A valid #GnomeVFSAddress.
 * @host_port: The port number to connect to.
 * @cancellation: Handle allowing cancellation of the operation.
 *
 * Creates a connection at @connection_return to @address using
 * port @port.
 *
 * Return value: #GnomeVFSResult indicating the success of the operation.
 *
 * Since: 2.8
 **/
GnomeVFSResult
gnome_vfs_inet_connection_create_from_address (GnomeVFSInetConnection **connection_return,
					       GnomeVFSAddress         *address,
					       guint                    host_port,
					       GnomeVFSCancellation    *cancellation)
{
	GnomeVFSInetConnection *new;
	gint sock, len, ret;
	struct sockaddr *saddr;
	
	sock = socket (gnome_vfs_address_get_family_type (address),
		       SOCK_STREAM, 0);
#ifdef G_OS_WIN32
	if (sock == SOCKET_ERROR) {
		_gnome_vfs_map_winsock_error_to_errno ();
		sock = -1;
	}
#endif
	if (sock < 0)
		return gnome_vfs_result_from_errno ();
	
	saddr = gnome_vfs_address_get_sockaddr (address,
						host_port,
						&len);
	
	ret = connect (sock, saddr, len);
	g_free (saddr);

	if (ret < 0) {
		_GNOME_VFS_SOCKET_CLOSE (sock);
		return gnome_vfs_result_from_errno ();
	}

		
	new = g_new0 (GnomeVFSInetConnection, 1);
	new->address = gnome_vfs_address_dup (address);
	new->sock = sock;

	_gnome_vfs_socket_set_blocking (new->sock, FALSE);

	*connection_return = new;
	return GNOME_VFS_OK;
}


/**
 * gnome_vfs_inet_connection_destroy:
 * @connection: Connection to destroy.
 * @cancellation: Handle for cancelling the operation.
 *
 * Closes/Destroys @connection.
 **/
void
gnome_vfs_inet_connection_destroy (GnomeVFSInetConnection *connection,
				   GnomeVFSCancellation   *cancellation)
{
	g_return_if_fail (connection != NULL);

	_GNOME_VFS_SOCKET_CLOSE (connection->sock);
	
	gnome_vfs_inet_connection_free (connection, cancellation);
}


/**
 * gnome_vfs_inet_connection_free:
 * @connection: Connection to free.
 * @cancellation: Handle for cancelling the operation.
 *
 * Frees @connection without closing the socket.
 **/
void
gnome_vfs_inet_connection_free (GnomeVFSInetConnection *connection,
				GnomeVFSCancellation *cancellation)
{
	g_return_if_fail (connection != NULL);
	
	if (connection->timeout)
		g_free (connection->timeout);

	if (connection->address)
		gnome_vfs_address_free (connection->address);

	g_free (connection);
	
}

/**
 * gnome_vfs_inet_connection_close:
 * @connection: connection to close
 * @cancellation: handle allowing cancellation of the operation
 *
 * Closes @connection, freeing all used resources.
 **/
static void
gnome_vfs_inet_connection_close (GnomeVFSInetConnection *connection,
				 GnomeVFSCancellation *cancellation)
{
	gnome_vfs_inet_connection_destroy (connection, cancellation);
}

/**
 * gnome_vfs_inet_connection_get_fd:
 * @connection: Connection to get the file descriptor from
 *
 * Retrieve the UNIX file descriptor corresponding to @connection.
 *
 * Return value: file descriptor
 **/
gint 
gnome_vfs_inet_connection_get_fd (GnomeVFSInetConnection *connection)
{
	g_return_val_if_fail (connection != NULL, -1);
	return connection->sock;
}

/**
 * gnome_vfs_inet_connection_get_ip:
 * @connection: Connection to get the ip from.
 *
 * Retrieve the ip address of the other side of a connected @connection.
 *
 * Return value: String version of the ip.
 *
 * Since: 2.8
 **/
char *
gnome_vfs_inet_connection_get_ip (GnomeVFSInetConnection *connection)
{
	return gnome_vfs_address_to_string (connection->address);
}

/**
 * gnome_vfs_inet_connection_get_address:
 * @connection: Connection to get the address from. 
 *
 * Retrieve the address of the other side of a connected @connection.
 * 
 * Return Value: A #GnomeVFSAddress containing the address.
 *
 * Since 2.8
 **/
GnomeVFSAddress *
gnome_vfs_inet_connection_get_address (GnomeVFSInetConnection *connection)
{
	return gnome_vfs_address_dup (connection->address);
}

/* SocketImpl for InetConnections */

/**
 * gnome_vfs_inet_connection_read:
 * @connection: Connection to read data from.
 * @buffer: Allocated buffer of at least @bytes bytes to be read into.
 * @bytes: Number of bytes to read from @socket into @buffer.
 * @bytes_read: Pointer to a GnomeVFSFileSize, will contain
 * the number of bytes actually read from the socket on return.
 * @cancellation: Handle allowing cancellation of the operation.
 *
 * Read @bytes bytes of data from @connection into @buffer.
 *
 * Return value: GnomeVFSResult indicating the success of the operation
 **/
static GnomeVFSResult 
gnome_vfs_inet_connection_read (GnomeVFSInetConnection *connection,
		                gpointer buffer,
		                GnomeVFSFileSize bytes,
		                GnomeVFSFileSize *bytes_read,
				GnomeVFSCancellation *cancellation)
{
	gint     read_val;
	fd_set   read_fds;
	int max_fd, cancel_fd;
	struct timeval timeout;

	cancel_fd = -1;
	
read_loop:
	read_val = _GNOME_VFS_SOCKET_READ (connection->sock, buffer, bytes);
#ifdef G_OS_WIN32
	if (read_val == SOCKET_ERROR) {
		_gnome_vfs_map_winsock_error_to_errno ();
		read_val = -1;
	}
#endif
	if (read_val == -1 && errno == EAGAIN) {

		FD_ZERO (&read_fds);
		FD_SET (connection->sock, &read_fds);
		max_fd = connection->sock;
	
		if (cancellation != NULL) {
			cancel_fd = gnome_vfs_cancellation_get_fd (cancellation);
			FD_SET (cancel_fd, &read_fds);
			max_fd = MAX (max_fd, cancel_fd);
		}
		
		/* select modifies the timeval struct so set it every loop */
		if (connection->timeout != NULL) {
			timeout.tv_sec = connection->timeout->tv_sec;
			timeout.tv_usec = connection->timeout->tv_usec;
		}
		
		read_val = select (max_fd + 1, &read_fds, NULL, NULL,
				   connection->timeout ? &timeout : NULL);
#ifdef G_OS_WIN32
		if (read_val == SOCKET_ERROR) {
			_gnome_vfs_map_winsock_error_to_errno ();
			read_val = -1;
		}
#endif
		if (read_val == 0) {
			return GNOME_VFS_ERROR_TIMEOUT;
		} else if (read_val != -1) { 	
			
			if (cancel_fd != -1 && FD_ISSET (cancel_fd, &read_fds)) {
				return GNOME_VFS_ERROR_CANCELLED;
			}
			
			if (FD_ISSET (connection->sock, &read_fds)) {
				goto read_loop;
			}

		}
	} 
	
	if (read_val == -1) {
		*bytes_read = 0;

		if (gnome_vfs_cancellation_check (cancellation)) {
			return GNOME_VFS_ERROR_CANCELLED;
		} 
		
		if (errno == EINTR) {
			goto read_loop;
		} else {
			return gnome_vfs_result_from_errno ();
		}

	} else {
		*bytes_read = read_val;
	}

	return *bytes_read == 0 ? GNOME_VFS_ERROR_EOF : GNOME_VFS_OK;
}

/**
 * gnome_vfs_inet_connection_write:
 * @connection: Connection to write data to.
 * @buffer: Data to write to the connection.
 * @bytes: Number of bytes from @buffer to write to @socket.
 * @bytes_written: Pointer to a GnomeVFSFileSize, will contain
 * the number of bytes actually written to the connection on return.
 * @cancellation: Handle allowing cancellation of the operation.
 *
 * Write @bytes bytes of data from @buffer to @connection.
 *
 * Return value: GnomeVFSResult indicating the success of the operation
 **/
static GnomeVFSResult 
gnome_vfs_inet_connection_write (GnomeVFSInetConnection *connection,
			         gconstpointer buffer,
			         GnomeVFSFileSize bytes,
			         GnomeVFSFileSize *bytes_written,
				 GnomeVFSCancellation *cancellation)
{
	gint    write_val;
	fd_set  write_fds, *read_fds, pipe_fds;
	int max_fd, cancel_fd;
	struct timeval timeout;


	cancel_fd = -1;
	read_fds  = NULL;
	
write_loop:	
	write_val = _GNOME_VFS_SOCKET_WRITE (connection->sock, buffer, bytes);
#ifdef G_OS_WIN32
	if (write_val == SOCKET_ERROR) {
		_gnome_vfs_map_winsock_error_to_errno ();
		write_val = -1;
	}
#endif
	if (write_val == -1 && errno == EAGAIN) {
			
         	FD_ZERO (&write_fds);
		FD_SET (connection->sock, &write_fds);
		max_fd = connection->sock;

		if (cancellation != NULL) {
			cancel_fd = gnome_vfs_cancellation_get_fd (cancellation);
			read_fds = &pipe_fds;
			FD_ZERO (read_fds);
			FD_SET (cancel_fd, read_fds);
			max_fd = MAX (max_fd, cancel_fd);
		}

		/* select modifies the timeval struct so set it every loop */
		if (connection->timeout != NULL) {
			timeout.tv_sec = connection->timeout->tv_sec;
			timeout.tv_usec = connection->timeout->tv_usec;
		}

		
		write_val = select (max_fd + 1, read_fds, &write_fds, NULL,
					connection->timeout ? &timeout : NULL);
#ifdef G_OS_WIN32
		if (write_val == SOCKET_ERROR) {
			_gnome_vfs_map_winsock_error_to_errno ();
			write_val = -1;
		}
#endif
		if (write_val == 0) {
			return GNOME_VFS_ERROR_TIMEOUT;
		} else if (write_val != -1) {

			if (cancel_fd != -1 && FD_ISSET (cancel_fd, read_fds)) {
				return GNOME_VFS_ERROR_CANCELLED;
			}
			
			if (FD_ISSET (connection->sock, &write_fds)) {
				goto write_loop;
			}

		}
	}

	if (write_val == -1) {
		*bytes_written = 0;

	        if (gnome_vfs_cancellation_check (cancellation)) {
			return GNOME_VFS_ERROR_CANCELLED;
		}

		if (errno == EINTR) {
			goto write_loop;
		} else {
			return gnome_vfs_result_from_errno ();
		}

	} else {
		*bytes_written = write_val;
		return GNOME_VFS_OK;
	}
}

/**
 * gnome_vfs_inet_connection_set_timeout:
 * @connection: Connection to set the timeout of.
 * @timeout: The timeout to set.
 * @cancellation: Optional cancellation object.
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


static GnomeVFSResult
gnome_vfs_inet_connection_set_timeout (GnomeVFSInetConnection *connection,
				       GTimeVal *timeout,
				       GnomeVFSCancellation *cancellation)
{
	if (timeout == NULL) {
		if (connection->timeout != NULL) {
			g_free (connection->timeout);
			connection->timeout = NULL;
		}
	} else {
		if (connection->timeout == NULL)
			connection->timeout = g_new0 (struct timeval, 1);

		connection->timeout->tv_sec  = timeout->tv_sec;
		connection->timeout->tv_usec = timeout->tv_usec;
	}
	
	return GNOME_VFS_OK;
}

static GnomeVFSSocketImpl inet_connection_socket_impl = {
	(GnomeVFSSocketReadFunc)gnome_vfs_inet_connection_read,
	(GnomeVFSSocketWriteFunc)gnome_vfs_inet_connection_write,
	(GnomeVFSSocketCloseFunc)gnome_vfs_inet_connection_close,
	(GnomeVFSSocketSetTimeoutFunc)gnome_vfs_inet_connection_set_timeout
};

/**
 * gnome_vfs_inet_connection_to_socket:
 * @connection: Connection to convert to wrapper in a GnomeVFSSocket.
 *
 * Wrapper @connection inside a standard GnomeVFSSocket for convenience.
 *
 * Return value: a newly created GnomeVFSSocket around @connection.
 **/
GnomeVFSSocket *
gnome_vfs_inet_connection_to_socket (GnomeVFSInetConnection *connection)
{
	return gnome_vfs_socket_new (&inet_connection_socket_impl, connection);
}

/**
 * gnome_vfs_inet_connection_to_socket_buffer:
 * @connection: Connection to convert to wrapper in a GnomeVFSSocketBuffer.
 *
 * Wrapper @connection inside a standard GnomeVFSSocketBuffer for convenience.
 *
 * Return value: a newly created GnomeVFSSocketBuffer around @connection.
 **/
GnomeVFSSocketBuffer *
gnome_vfs_inet_connection_to_socket_buffer (GnomeVFSInetConnection *connection)
{
	GnomeVFSSocket *socket;
	socket = gnome_vfs_inet_connection_to_socket (connection);
	return gnome_vfs_socket_buffer_new (socket);
}
