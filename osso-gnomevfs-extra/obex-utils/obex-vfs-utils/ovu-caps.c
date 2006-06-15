/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Nokia Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; version 2 of the
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
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>

#include "ovu-caps.h"
#include "ovu-cap-parser.h"
#include "ovu-cap-client.h"

#define d(x)

struct _OvuCaps {
	GList *memory_entries;

	/* Add "Services" and "Inbox" data here later. */
};

struct _OvuCapsMemory {
        gchar            *type;
        GnomeVFSFileSize  used;
        GnomeVFSFileSize  free;
        gboolean          case_sensitive;
};


OvuCapsMemory *
ovu_caps_memory_new (const gchar      *type,
		     GnomeVFSFileSize  free,
		     GnomeVFSFileSize  used,
		     gboolean          case_sensitive)
{
	OvuCapsMemory *memory;
	
	memory = g_new0 (OvuCapsMemory, 1);

	memory->type = g_strdup (type);
	memory->free = free;
	memory->used = used;
	memory->case_sensitive = case_sensitive;

	return memory;
}

void
ovu_caps_memory_free (OvuCapsMemory *memory)
{
	g_free (memory->type);
	g_free (memory);
}

gboolean
ovu_caps_memory_equal (OvuCapsMemory *m1, OvuCapsMemory *m2)
{
	if (strcmp (m1->type, m2->type) != 0) {
		d(g_print ("type mismatch: %s %s\n",
			   m1->type, m2->type));
		return FALSE;
	}

	if (m1->free != m2->free) {
		d(g_print ("free mismatch: %d %d\n",
			   (int) m1->free, (int) m2->free));
		return FALSE;
	}

	if (m1->used != m2->used) {
		d(g_print ("used mismatch: %d %d\n",
			   (int) m1->used, (int) m2->used));
		return FALSE;
	}

	if (m1->case_sensitive != m2->case_sensitive) {
		d(g_print ("case mismatch: %d %d\n",
			   m1->case_sensitive,
			   m2->case_sensitive));
		return FALSE;
	}

	return TRUE;
}

OvuCaps *
ovu_caps_get_from_uri (const gchar *uri, GError **error)
{
	OvuCaps *caps;

	caps = ovu_cap_client_get_caps (uri, error);
	if (!caps) {
		return NULL;
	}

	return caps;
}

void
ovu_caps_free (OvuCaps *caps)
{
	g_list_foreach (caps->memory_entries,
			(GFunc) ovu_caps_memory_free, NULL);
	
	g_list_free (caps->memory_entries);

	g_free (caps);
}

GList *
ovu_caps_get_memory_entries (OvuCaps *caps)
{
	g_return_val_if_fail (caps != NULL, NULL);

	return caps->memory_entries;
}

const gchar *
ovu_caps_memory_get_type (OvuCapsMemory *memory)
{
	return memory->type;
}

GnomeVFSFileSize
ovu_caps_memory_get_used (OvuCapsMemory *memory)
{
	return memory->used;
}

GnomeVFSFileSize
ovu_caps_memory_get_free (OvuCapsMemory *memory)
{
	return memory->free;
}

gboolean
ovu_caps_memory_get_case_sensitive (OvuCapsMemory *memory)
{
	return memory->case_sensitive;
}
