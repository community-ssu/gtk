/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004-2005 Nokia Corporation.
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
#include <expat.h>

#include "ovu-cap-parser.h"

#define d(x)

struct _OvuCaps {
	GList *memory_entries;

	/* FIXME: services and inbox data. */
};


typedef enum {
	PARSER_STATE_INVALID,

	PARSER_STATE_START,
	PARSER_STATE_CAPABILITY,

	PARSER_STATE_GENERAL,

	PARSER_STATE_MEMORY,
	PARSER_STATE_MEMORY_TYPE,
	PARSER_STATE_MEMORY_LOCATION,
	PARSER_STATE_MEMORY_FREE,
	PARSER_STATE_MEMORY_USED,
	PARSER_STATE_MEMORY_SHARED,
	PARSER_STATE_MEMORY_FILESIZE,
	PARSER_STATE_MEMORY_FOLDERSIZE,
	PARSER_STATE_MEMORY_FILELEN,
	PARSER_STATE_MEMORY_FOLDERLEN,
	PARSER_STATE_MEMORY_CASE,
	PARSER_STATE_MEMORY_EXT,

	PARSER_STATE_INBOX,
	PARSER_STATE_SERVICE,
	
	PARSER_STATE_SKIP
} ParserState;


typedef struct {
	GList             *state;

	GList             *memory_entries;

	gchar             *memory_type;
	GnomeVFSFileSize   memory_free;
	GnomeVFSFileSize   memory_used;
	gboolean           memory_case_sensitive;

	GError           **error;
} ParserData;

static void    cap_parser_start_node_cb   (void                 *user_data, 
					   const char           *node_name,
					   const char          **attr);

static void    cap_parser_end_node_cb     (void                 *user_data,
					   const char           *node_name);
static void    cap_parser_text_cb         (void                 *user_data,
					   const XML_Char       *s,
                                           int                   len);
static XML_Parser
cap_parser_create_parser                  (ParserData           *data);


static void
cap_parser_push_state (ParserData *data, ParserState state)
{
	data->state = g_list_prepend (data->state,
				      GINT_TO_POINTER (state));
}      

static ParserState
cap_parser_pop_state (ParserData *data)
{
	ParserState state;
	
	if (!data->state) {
		return PARSER_STATE_INVALID;
	}
	
	state = GPOINTER_TO_INT (data->state->data);
	data->state = g_list_delete_link (data->state, data->state);

	return state;
}      

static ParserState
cap_parser_peek_state (ParserData *data)
{
	if (!data->state) {
		return PARSER_STATE_START;
	}
	
	return GPOINTER_TO_INT (data->state->data);
}

static const char *
cap_parser_get_attribute_value (const char  *name, const char **attr)
{
	gint i = 0;
  
	while (attr[i]) {
		if (strcmp (name, attr[i]) == 0) {
			return attr[i + 1];
		}
		i += 2;
	}
	
	return "";
}

static void
cap_parser_start_node_cb (void        *user_data, 
			  const char  *node_name,
			  const char **attr)
{
	ParserData  *data;
	ParserState  state;
	const gchar *version;
	
	data = (ParserData *) user_data;
	
	state = cap_parser_peek_state (data);
	
	switch (state) {
	case PARSER_STATE_START:
		if (strcmp (node_name, "Capability") != 0) {
			g_set_error (data->error,
				     G_MARKUP_ERROR,
				     G_MARKUP_ERROR_INVALID_CONTENT,
				     "Outermost element must be a <Capability>, not <%s>",
				     node_name);
			return;
		}

		version = cap_parser_get_attribute_value ("version", attr);
		if (strcmp (version, "1.0") != 0) {
			g_warning ("Version expected is '1.0', not '%s'\n", version);
		}

		cap_parser_push_state (data, PARSER_STATE_CAPABILITY);
		break;

	case PARSER_STATE_CAPABILITY:
		if (strcmp (node_name, "General") == 0) {
			cap_parser_push_state (data, PARSER_STATE_GENERAL);
		}
		else if (strcmp (node_name, "Inbox") == 0) {
			cap_parser_push_state (data, PARSER_STATE_INBOX);
		}
		else if (strcmp (node_name, "Service") == 0) {
			cap_parser_push_state (data, PARSER_STATE_SERVICE);
		} else {
			g_set_error (data->error,
				     G_MARKUP_ERROR,
				     G_MARKUP_ERROR_INVALID_CONTENT,
				     "Don't expect node '%s' as child of 'Cap'",
				     node_name);
			return;
		}
		break;
		
	case PARSER_STATE_GENERAL:
		if (strcmp (node_name, "Memory") == 0) {
			cap_parser_push_state (data, PARSER_STATE_MEMORY);
		}
		else if (strcmp (node_name, "Manufacturer") == 0 ||
			 strcmp (node_name, "Model") == 0 ||
			 strcmp (node_name, "SN") == 0 ||
			 strcmp (node_name, "OEM") == 0 ||
			 strcmp (node_name, "SW") == 0 ||
			 strcmp (node_name, "FW") == 0 ||
			 strcmp (node_name, "HW") == 0 ||
			 strcmp (node_name, "Language") == 0 ||
			 strcmp (node_name, "Ext") == 0) {

			/* Skip these for now. */
			cap_parser_push_state (data, PARSER_STATE_SKIP);
		} else {
			g_set_error (data->error,
				     G_MARKUP_ERROR,
				     G_MARKUP_ERROR_INVALID_CONTENT,
				     "Don't expect node '%s' as child of 'General'",
				     node_name);
			return;
		}
		
		break;

	case PARSER_STATE_MEMORY:
		if (strcmp (node_name, "MemType") == 0) {
			cap_parser_push_state (data, PARSER_STATE_MEMORY_TYPE);
		}
		else if (strcmp (node_name, "Location") == 0) {
			cap_parser_push_state (data, PARSER_STATE_MEMORY_LOCATION);
		}
		else if (strcmp (node_name, "Free") == 0) {
			cap_parser_push_state (data, PARSER_STATE_MEMORY_FREE);
		}
		else if (strcmp (node_name, "Used") == 0) {
			cap_parser_push_state (data, PARSER_STATE_MEMORY_USED);
		}
		else if (strcmp (node_name, "Shared") == 0) {
			cap_parser_push_state (data, PARSER_STATE_MEMORY_SHARED);
		}
		else if (strcmp (node_name, "FileSize") == 0) {
			cap_parser_push_state (data, PARSER_STATE_MEMORY_FILESIZE);
		}
		else if (strcmp (node_name, "FolderSize") == 0) {
			cap_parser_push_state (data, PARSER_STATE_MEMORY_FOLDERSIZE);
		}
		else if (strcmp (node_name, "FileNLen") == 0) {
			cap_parser_push_state (data, PARSER_STATE_MEMORY_FILELEN);
		}
		else if (strcmp (node_name, "FolderNLen") == 0) {
			cap_parser_push_state (data, PARSER_STATE_MEMORY_FOLDERLEN);
		}
		else if (strcmp (node_name, "CaseSenN") == 0) {
			cap_parser_push_state (data, PARSER_STATE_MEMORY_CASE);
			data->memory_case_sensitive = TRUE;
		}
		else if (strcmp (node_name, "Ext") == 0) {
			cap_parser_push_state (data, PARSER_STATE_MEMORY_EXT);
		} else {
			g_set_error (data->error,
				     G_MARKUP_ERROR,
				     G_MARKUP_ERROR_INVALID_CONTENT,
				     "Don't expect node '%s' as child of 'Memory'",
				     node_name);
			return;
		}
		break;
		
	case PARSER_STATE_INBOX:
	case PARSER_STATE_SERVICE:
		/* Skip these for now. */
		cap_parser_push_state (data, PARSER_STATE_SKIP);
		break;
		
	case PARSER_STATE_SKIP:
		cap_parser_push_state (data, PARSER_STATE_SKIP);
		break;

	default:
		g_warning ("Node not handled: '%s'\n", node_name);
		cap_parser_push_state (data, PARSER_STATE_SKIP);
		break;
	}
}

static void
cap_parser_reset_memory (ParserData *data)
{
	g_free (data->memory_type);
	data->memory_type = NULL;
	data->memory_free = 0;
	data->memory_used = 0;
	data->memory_case_sensitive = FALSE;
}

static void
cap_parser_end_node_cb (void *user_data, const char *node_name)
{
	ParserData    *data;
	ParserState    state;
	OvuCapsMemory *memory;

	data = (ParserData *) user_data;

	state = cap_parser_pop_state (data);

	switch (state) {
	case PARSER_STATE_INVALID:
		return;
		
	case PARSER_STATE_MEMORY:
		memory = ovu_caps_memory_new (data->memory_type,
					      data->memory_free,
					      data->memory_used,
					      data->memory_case_sensitive);
		
		data->memory_entries = g_list_prepend (data->memory_entries, 
						       memory);
		cap_parser_reset_memory (data);
		break;

	case PARSER_STATE_CAPABILITY:
		data->memory_entries = g_list_reverse (data->memory_entries);
		break;

	default:
		break;
	}
}

/* Parse a long, return -1 if input is not strictly valid or null. */
static glong
parse_long (const gchar *str)
{
	gchar *endptr;
	glong  l;

	if (!str) {
		return -1;
	}
	
	l = strtol (str, &endptr, 10);
	if (endptr[0] != '\0') {
		l = -1;
	}			
	
	return l;
}

static void
cap_parser_text_cb (void           *user_data,
		    const XML_Char *s,
		    int             len)
{
	ParserData  *data;
	ParserState  state;
	gchar       *tmp;

	data = (ParserData *) user_data;

	/* text is not null terminated. */
	tmp = g_strndup (s, len);

	state = cap_parser_peek_state (data);

	switch (state) {
	case PARSER_STATE_MEMORY_TYPE:
		data->memory_type = g_strdup (tmp);
		break;
	case PARSER_STATE_MEMORY_FREE:
		data->memory_free = parse_long (tmp);
		break;
	case PARSER_STATE_MEMORY_USED:
		data->memory_used = parse_long (tmp);
		break;

	default:
		break;
	}

	g_free (tmp);
}

static XML_Parser
cap_parser_create_parser (ParserData *data)
{
	XML_Parser parser;

	parser = XML_ParserCreate (NULL);

	XML_SetElementHandler (parser,
			       cap_parser_start_node_cb,
			       cap_parser_end_node_cb);
	
	XML_SetCharacterDataHandler (parser, cap_parser_text_cb);

	XML_SetUserData (parser, data);

	return parser;
}

static void
cap_parser_free (ParserData *data, gboolean free_data)
{
	cap_parser_reset_memory (data);
	
	if (free_data) {
		g_list_foreach (data->memory_entries,
				(GFunc) ovu_caps_memory_free, NULL);
	}
	
	g_free (data);
}

OvuCaps *
ovu_caps_parser_parse (const gchar  *buf,
		       gint          len,
		       GError      **error)
{
	ParserData *data;
	XML_Parser  parser;
	OvuCaps    *caps;

	data = g_new0 (ParserData, 1);

	data->error = error;
	parser = cap_parser_create_parser (data);

	if (XML_Parse (parser, buf, len, TRUE) == 0) {
		caps = NULL;

		if (*error == NULL) {
			g_set_error (error,
				     G_MARKUP_ERROR,
				     G_MARKUP_ERROR_INVALID_CONTENT,
				     "Couldn't parse the incoming data");
		}

		cap_parser_free (data, TRUE);
	} else {
		caps = g_new0 (OvuCaps, 1);
		caps->memory_entries = data->memory_entries;
		
		cap_parser_free (data, FALSE);
	}

	XML_ParserFree (parser);

	return caps;
}

