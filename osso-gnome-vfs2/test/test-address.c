/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* test-address.c - Test program for the GNOME Virtual File System.

   Copyright (C) 2005 Christian Kellner

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

   Author: Christian Kellner <gicmo@gnome.org>
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#undef G_DISABLE_ASSERT
#undef G_LOG_DOMAIN

#include <stdio.h>
#include <libgnomevfs/gnome-vfs-init.h>
#include <libgnomevfs/gnome-vfs-address.h>

int
main (int argc, char **argv)
{
	GnomeVFSAddress *v4_a;
	GnomeVFSAddress *v4_b;
	guint32          v4_numeric;
	gboolean         equal;
#ifdef ENABLE_IPV6
	GnomeVFSAddress *v6_a;
	GnomeVFSAddress *v6_b;
#endif
	fprintf (stderr, "Testing GnomeVFSAddress\n");
		
	gnome_vfs_init ();

	/* v4 test cases */
	
	v4_a = gnome_vfs_address_new_from_string ("127.3.2.1");

	/* generate a numeric 127.0.0.1 */
	v4_numeric = g_htonl (2130706433U);
	
	v4_b = gnome_vfs_address_new_from_ipv4 (v4_numeric);
	
	g_assert (v4_a && v4_b);
	g_assert (gnome_vfs_address_get_family_type (v4_a) == AF_INET);
	g_assert (gnome_vfs_address_get_family_type (v4_b) == AF_INET);

	g_assert (gnome_vfs_address_get_ipv4 (v4_b) == v4_numeric);
	
	/* compare the whole address for now */
	
	equal = gnome_vfs_address_match (v4_a, v4_b, 32);
	g_assert (equal == FALSE);
	
	equal = gnome_vfs_address_match (v4_a, v4_b, 24);
	g_assert (equal == FALSE);
	
	equal = gnome_vfs_address_match (v4_a, v4_b, 16);
	g_assert (equal == FALSE);
	
	equal = gnome_vfs_address_match (v4_a, v4_b, 8);
	g_assert (equal == TRUE);

	equal = gnome_vfs_address_match (v4_a, v4_b, 0);
	g_assert (equal == FALSE);

	gnome_vfs_address_free (v4_a);
	v4_a = gnome_vfs_address_new_from_string ("127.0.0.1");

	equal = gnome_vfs_address_match (v4_a, v4_b, 32);
	g_assert (equal == TRUE);

	gnome_vfs_address_free (v4_b);
	v4_b = gnome_vfs_address_dup (v4_a);

	equal = gnome_vfs_address_equal (v4_a, v4_b);
	g_assert (equal == TRUE);
	
	equal = gnome_vfs_address_match (v4_a, v4_b, 32);
	g_assert (equal == TRUE);

#ifdef INADDR_LOOPBACK
	v4_numeric = gnome_vfs_address_get_ipv4 (v4_a);
	g_assert (INADDR_LOOPBACK == g_ntohl (v4_numeric));
#endif
	
#ifdef ENABLE_IPV6
	/* Mapped v4 test cases */

	v6_a = gnome_vfs_address_new_from_string ("::ffff:127.0.0.1");

	g_assert (v6_a);
	g_assert (gnome_vfs_address_get_family_type (v6_a) == AF_INET6);

	/* v4 mapped in v6 test (v4_a still is 127.0.0.1) */
	equal = gnome_vfs_address_match (v4_a, v6_a, 32);
	g_assert (equal == TRUE);

	gnome_vfs_address_free (v6_a);
	
	v6_a = gnome_vfs_address_new_from_string ("fe80::dead:babe");
	v6_b = gnome_vfs_address_new_from_string ("fe80::dead:beef");

	g_assert (v6_a && v6_b);
	g_assert (gnome_vfs_address_get_family_type (v6_a) == AF_INET6);
	g_assert (gnome_vfs_address_get_family_type (v6_b) == AF_INET6);

	equal = gnome_vfs_address_match (v6_a, v6_b, 128);
	g_assert (equal == FALSE);

	/* fe80::dead:bx* */
	equal = gnome_vfs_address_match (v6_a, v6_b, 120);
	g_assert (equal == FALSE);

	/* fe80::dead:b* */
	/* both address are equal from this mask on */
	equal = gnome_vfs_address_match (v6_a, v6_b, 116);
	g_assert (equal == TRUE);

	/* fe80::dead:* */
	equal = gnome_vfs_address_match (v6_a, v6_b, 112);
	g_assert (equal == TRUE);

	/* fe80::* */
	equal = gnome_vfs_address_match (v6_a, v6_b, 64);
	g_assert (equal == TRUE);
	
	/* _dup test for v6 */	
	gnome_vfs_address_free (v6_b);
	v6_b = gnome_vfs_address_dup (v6_a);
	
	equal = gnome_vfs_address_equal (v6_a, v6_b);
	g_assert (equal == TRUE);

	gnome_vfs_address_free (v6_a);
	gnome_vfs_address_free (v6_b);
	
#endif 	
	gnome_vfs_address_free (v4_a);
	gnome_vfs_address_free (v4_b);

	
	return 0;
}

