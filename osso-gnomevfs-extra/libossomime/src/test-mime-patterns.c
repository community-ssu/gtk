/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Erik Karlsson <erik.b.karlsson@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation version 2.1 of the
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
#include <string.h>
#include <glib.h>
#include <osso-mime.h>

static gint
test_no_mime_type (void)
{
	GError      *error =NULL;
	const gchar *mimetype;
	GSList      *list;

	mimetype = "doesntexistsmimetype/hgifff";

	list = osso_mime_patterns_get_for_mime_type (mimetype, &error);
	if (!list) {
		if (error != NULL) {
			g_printerr ("[Error] %s\n", error->message);
			g_error_free (error);
			return -1;
		}
		return 0;
	}
	
	g_printerr ("[Error] Found entry: %s\n", (gchar*)list->data);
	
	g_slist_foreach (list, (GFunc) g_free, NULL);
	g_slist_free (list);

	return -1;
}

static gint
test_html (void)
{
	GError      *error = NULL;
	const gchar *mimetype;
	GSList      *list;
	gint         retvalue = 0;
	
	mimetype = "text/html";
	
	list = osso_mime_patterns_get_for_mime_type (mimetype, &error);
	if (list) {
		if ((strcmp ((gchar*) list->data, "*.html") != 0) &&
		    (strcmp ((gchar*) list->data, "*.htm") != 0)) {
			g_printerr ("Didn't contain *.html or *.htm in the list "
				    "for mime type '%s'", mimetype);
			retvalue = -1;
		}	
		
		g_slist_foreach (list, (GFunc) g_free, NULL);
		g_slist_free (list);
	} else {
		retvalue = -1;
	}
	
	return retvalue;
}

static gint
test_image_gif (void)
{
	GError      *error = NULL;
	const gchar *mimetype;
	GSList      *list;

	mimetype = "image/gif";

	list = osso_mime_patterns_get_for_mime_type (mimetype, &error);
	if (!list) {
		if (error != NULL) {
			g_printerr ("[Error] %s\n", error->message);
			g_error_free (error);
		}
		
		g_printerr ("Couldn't find anything for image/gif\n");
		return -1;
	}

	if ((strcmp ((gchar*) list->data, "*.gif") != 0) &&
	    (strcmp ((gchar*) list->data, "*.GIF") != 0)) {
		g_printerr ("Entries didn't contain *.gif or *.GIF (%s)\n", 
			    (gchar*) list->data);
		g_slist_foreach (list, (GFunc) g_free, NULL);
		g_slist_free (list);
		return -1;
	}

	g_slist_foreach (list, (GFunc) g_free, NULL);
	g_slist_free (list);
	
	return 0;
}

int
main (int argc, char** argv)
{
	if (test_image_gif () != 0) {
		g_printerr ("Failed test_image_gif\n");
		return 1;
	}
	
	if (test_no_mime_type () != 0) {
		g_printerr ("Failed test_no_mime_type\n");
		return 1;
	}

        if (test_html () != 0) {
		g_printerr ("Failed test_html\n");
		return 1;
	}

	return 0;
}
