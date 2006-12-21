/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2006 Nokia Corporation.
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

/* Contact: Andrey Kochanov <andrey.kochanov@nokia.com> */

#ifndef OSSO_MIME_GLOBS_H_
#define OSSO_MIME_GLOBS_H_

G_BEGIN_DECLS

#define OSSO_MIME_PATTERNS_ERROR osso_mime_patterns_error_quark()

typedef enum { 
	OSSO_MIME_PATTERNS_ERROR_INTERNAL
	/* Could add more errors in the future */
} OssoMimePatternsError;

/**
 * Returns a list of file "glob" patterns that are registered for certain mime
 * type.  For example by passing mime type "text/html" the list would contain a
 * list of strings "*.html" and "*.htm".
 * 
 * @param mime_type mime type as a string. For example "image/gif"
 * @param error error to store possible error information 
 */
GSList *osso_mime_patterns_get_for_mime_type (const gchar  *mime_type,
					      GError      **error);
GQuark  osso_mime_patterns_error_quark       (void);


G_END_DECLS

#endif /* OSSO_MIME_GLOBS_H_ */
