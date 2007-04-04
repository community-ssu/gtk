/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * This is file is part of libhildonmime
 *
 * Copyright (C) 2004-2006 Nokia Corporation.
 *
 * Contact: Erik Karlsson <erik.b.karlsson@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; version 2.1 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <libgnomevfs/gnome-vfs.h>
#include <hildon-mime.h>

int
main (int argc, char **argv)
{
	gchar              *name;
	HildonMimeCategory  category;
	const gchar        *str;

	gnome_vfs_init ();

	if (argc < 2) {
		g_printerr ("Usage: %s <mime type>\n", argv[0]);
		return 1;
	}

	name = argv[1];
	category = hildon_mime_get_category_for_mime_type (name);

	str = hildon_mime_get_category_name (category);
	if (!str) {
		g_print ("No category found\n");
	} else {
		g_print ("The category is '%s' (%d)\n", str, category);
	}

	return 0;
}

