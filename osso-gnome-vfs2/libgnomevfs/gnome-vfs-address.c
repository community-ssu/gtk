/* gnome-vfs-address.c - Address functions

   Copyright (C) 2004 Christian Kellner

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

#include <libgnomevfs/gnome-vfs-address.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#ifndef G_OS_WIN32
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

struct _GnomeVFSAddress {

	   struct sockaddr *sa;
};

#define SA(__s)				((struct sockaddr *) __s)

#define SIN_LEN				sizeof (struct sockaddr_in)
#define SIN(__s)				((struct sockaddr_in *) __s)

#ifdef ENABLE_IPV6

# define SIN6_LEN				sizeof (struct sockaddr_in6)
# define SIN6(__s)				((struct sockaddr_in6 *) __s)
# define VALID_AF(__sa)			(__sa->sa_family == AF_INET  || __sa->sa_family == AF_INET6)
# define SA_SIZE(__sa)			(__sa->sa_family == AF_INET ? SIN_LEN : \
   											         SIN6_LEN)
# define AF_SIZE(__af)			(__af == AF_INET6 ? SIN6_LEN : SIN_LEN)
# define MAX_ADDRSTRLEN			INET6_ADDRSTRLEN

#else /* ENABLE_IPV6 */

# define VALID_AF(__sa)			(__sa->sa_family == AF_INET)
# define AF_SIZE(__af)			SIN_LEN
# define SA_SIZE(_sa)			SIN_LEN
# define MAX_ADDRSTRLEN			INET_ADDRSTRLEN

#endif


/* Register GnomeVFSAddress in the glib type system */
GType 
gnome_vfs_address_get_type (void) {
	static GType addr_type = 0;

	if (addr_type == 0) {
		addr_type = g_boxed_type_register_static ("GnomeVFSAddress",
				(GBoxedCopyFunc) gnome_vfs_address_dup,
				(GBoxedFreeFunc) gnome_vfs_address_free);
	}

	return addr_type;
}

												 

/**
 * gnome_vfs_address_new_from_string:
 * @address: A string representation of the address.
 * 
 * Creates a new #GnomeVFSAddress from the given string or %NULL
 * if @address isn't a valid.
 * 
 * Return value: The new #GnomeVFSAddress.
 *
 * Since: 2.8
 **/
GnomeVFSAddress *
gnome_vfs_address_new_from_string (const char *address)
{
	struct sockaddr_in sin;
#ifdef G_OS_WIN32
	int address_length;
#endif

#if (defined (G_OS_WIN32) || defined (HAVE_INET_PTON)) && defined (ENABLE_IPV6)
	struct sockaddr_in6 sin6;
#endif

	sin.sin_family = AF_INET;

#ifdef G_OS_WIN32
	address_length = SIN_LEN;
	if (WSAStringToAddress ((LPTSTR) address, AF_INET, NULL,
				(LPSOCKADDR) &sin.sin_addr,
				&address_length) == 0)
		return gnome_vfs_address_new_from_sockaddr (SA (&sin), SIN_LEN);
#  ifdef ENABLE_IPV6
	address_length = SIN6_LEN;
	if (WSAStringToAddress ((LPTSTR) address, AF_INET6, NULL,
				(LPSOCKADDR) &sin6.sin6_addr,
				&address_length) == 0) {
		sin6.sin6_family = AF_INET6;
		return gnome_vfs_address_new_from_sockaddr (SA (&sin6), SIN6_LEN);
	}
#  endif  /* ENABLE_IPV6 */

#elif defined (HAVE_INET_PTON)
	if (inet_pton (AF_INET, address, &sin.sin_addr) > 0)
		return gnome_vfs_address_new_from_sockaddr (SA (&sin), SIN_LEN);
#  ifdef ENABLE_IPV6		
	if (inet_pton (AF_INET6, address, &sin6.sin6_addr) > 0) {
		sin6.sin6_family = AF_INET6;
		return gnome_vfs_address_new_from_sockaddr (SA (&sin6), SIN6_LEN);
	}
#  endif /* ENABLE_IPV6 */

#elif defined (HAVE_INET_ATON)
	if (inet_aton (address, &sin.sin_addr) > 0)
		return gnome_vfs_address_new_from_sockaddr (SA (&sin), SIN_LEN);
#else
	if ((sin.sin_addr.s_addr = inet_addr (address)) != INADDR_NONE)
		return gnome_vfs_address_new_from_sockaddr (SA (&sin), SIN_LEN);
#endif	

	return NULL;
}

/**
 * gnome_vfs_address_new_from_sockaddr:
 * @sa: A pointer to a sockaddr.
 * @len: The size of @sa.
 *
 * Creates a new #GnomeVFSAddress from @sa.
 *
 * Return value: The new #GnomeVFSAddress 
 * or %NULL if @sa was invalid or the address family isn't supported.
 *
 * Since: 2.8
 **/
GnomeVFSAddress *
gnome_vfs_address_new_from_sockaddr (struct sockaddr *sa,
							  int              len)
{
	   GnomeVFSAddress *addr;

	   g_return_val_if_fail (sa != NULL, NULL);
	   g_return_val_if_fail (VALID_AF (sa), NULL);
	   g_return_val_if_fail (len == AF_SIZE (sa->sa_family), NULL);
	   
	   addr = g_new0 (GnomeVFSAddress, 1);
	   addr->sa = g_memdup (sa, len);

	   return addr;
}

/**
 * gnome_vfs_address_new_from_ipv4:
 * @ipv4_address: A IPv4 Address in network byte order
 *
 * Creates a new #GnomeVFSAddress from @ipv4_address.
 *
 * Note that this function should be avoided because newly written
 * code should be protocol independent.
 * 
 * Return value: A new #GnomeVFSAdress.
 *
 * Since: 2.8
 **/
GnomeVFSAddress *
gnome_vfs_address_new_from_ipv4 (guint32 ipv4_address)
{
	   GnomeVFSAddress *addr;
	   struct sockaddr_in *sin;
	   
	   sin = g_new0 (struct sockaddr_in, 1);
	   sin->sin_addr.s_addr = ipv4_address;
	   sin->sin_family = AF_INET;

	   addr = gnome_vfs_address_new_from_sockaddr (SA (sin), SIN_LEN);

	   return addr;
}

/**
 * gnome_vfs_address_get_family_type:
 * @address: A pointer to a #GnomeVFSAddress
 *
 * Use this function to retrive the address family of @address.
 * 
 * Return value: The address family of @address.
 *
 * Since: 2.8
 **/
int
gnome_vfs_address_get_family_type (GnomeVFSAddress *address)
{
	   g_return_val_if_fail (address != NULL, -1);

	   return address->sa->sa_family;
}


/**
 * gnome_vfs_address_to_string:
 * @address: A pointer to a #GnomeVFSAddress
 * 
 * Translate @address to a printable string.
 * 
 * Returns: A newly alloced string representation of @address which
 * the caller must free.
 *
 * Since: 2.8
 **/
char *
gnome_vfs_address_to_string (GnomeVFSAddress *address)
{
#ifdef G_OS_WIN32
	   char text_addr[100];
	   DWORD text_length = sizeof (text_addr);

	   if (WSAAddressToString (address->sa, SA_SIZE (address->sa),
				   NULL, text_addr, &text_length) == 0)
		   return g_strdup (text_addr);

	   return NULL;
#else
	   const char *text_addr;
#ifdef HAVE_INET_NTOP
	   char buf[MAX_ADDRSTRLEN];
#endif	
	   g_return_val_if_fail (address != NULL, NULL);
	   
	   text_addr = NULL;

	   switch (address->sa->sa_family) {
#if defined (ENABLE_IPV6) && defined (HAVE_INET_NTOP)	
	   case AF_INET6:
			 
			 text_addr = inet_ntop (AF_INET6,
							    &SIN6 (address->sa)->sin6_addr,
							    buf,
							    sizeof (buf));
			 break;
#endif
	   case AF_INET:
#if HAVE_INET_NTOP
			 text_addr = inet_ntop (AF_INET,
							    &SIN (address->sa)->sin_addr,
							    buf,
							    sizeof (buf));
#else
			 text_addr = inet_ntoa (SIN (address->sa)->sin_addr);
#endif  /* HAVE_INET_NTOP */
			 break;
	   }
	   
	  
	   return text_addr != NULL ? g_strdup (text_addr) : NULL;
#endif
}

/**
 * gnome_vfs_address_get_ipv4:
 * @address: A #GnomeVFSAddress
 * 
 * 
 * Returns: The associated IPv4 address in network byte order.
 *
 * Note that you should avoid using this function because newly written
 * code should be protocol independent.
 *
 * Since: 2.8
 **/
guint32
gnome_vfs_address_get_ipv4 (GnomeVFSAddress *address)
{
	   g_return_val_if_fail (address != NULL, 0);
	   g_return_val_if_fail (address->sa != NULL, 0);

	   if (address->sa->sa_family != AF_INET)
			 return 0;

	   return (guint32) SIN (address->sa)->sin_addr.s_addr;
}


/**
 * gnome_vfs_address_get_sockaddr:
 * @address: A #GnomeVFSAddress
 * @port: A valid port in host byte order to set in the returned sockaddr
 * structure.
 * @len: A pointer to an int which will contain the length of the
 * return sockaddr structure.
 *
 * This function tanslates @address into a equivalent
 * sockaddr structure. The port specified at @port will
 * be set in the structure and @len will be set to the length
 * of the structure.
 * 
 * 
 * Return value: A newly allocated sockaddr structure the caller must free
 * or %NULL if @address did not point to a valid #GnomeVFSAddress.
 **/
struct sockaddr *
gnome_vfs_address_get_sockaddr (GnomeVFSAddress *address,
						  guint16          port,
						  int             *len)
{
	   struct sockaddr *sa;

	   g_return_val_if_fail (address != NULL, NULL);
	   
	   sa = g_memdup (address->sa, SA_SIZE (address->sa));

	   switch (address->sa->sa_family) {
#ifdef ENABLE_IPV6
	   case AF_INET6:
			 SIN6 (sa)->sin6_port = g_htons (port);
			 
			 if (len != NULL)
				    *len = SIN6_LEN;
			 break;
#endif
	   case AF_INET:
			 SIN (sa)->sin_port = g_htons (port);

			 if (len != NULL)
				    *len = SIN_LEN;
			 break;

	   }

	   return sa;
}

/**
 * gnome_vfs_address_dup:
 * @address: A #GnomeVFSAddress.
 * 
 * Duplicates @adderss.
 * 
 * Return value: Duplicated @address or %NULL if @address was not valid.
 *
 * Since: 2.8
 **/
GnomeVFSAddress *
gnome_vfs_address_dup (GnomeVFSAddress *address)
{
	   GnomeVFSAddress *addr;

	   g_return_val_if_fail (address != NULL, NULL);
	   g_return_val_if_fail (VALID_AF (address->sa), NULL);

	   addr = g_new0 (GnomeVFSAddress, 1);
	   addr->sa = g_memdup (address->sa, SA_SIZE (address->sa));

	   return addr;
}

/**
 * gnome_vfs_address_free:
 * @address: A #GnomeVFSAddress.
 *
 * Frees the memory allocated for @address.
 *
 * Since: 2.8
 **/
void
gnome_vfs_address_free (GnomeVFSAddress *address)
{
	   g_return_if_fail (address != NULL);

	   if (address->sa)
			 g_free (address->sa);

	   g_free (address);
}


