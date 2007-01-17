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
#include <stdlib.h>
#include <string.h>
#include <libgnomevfs/gnome-vfs.h>
#include <hildon-mime.h>

static gboolean   use_default = FALSE;
static gchar     *get_actions = NULL;
static gchar     *get_actions_by_uri = NULL;
static gchar     *action_type = NULL;
static gchar     *is_default = NULL;
static gchar     *get_default = NULL;
static gchar     *get_default_by_uri = NULL;
static gchar     *set_default = NULL;
static gchar     *set_default_by_uri = NULL;
static gchar     *set_default_to_nothing = NULL;
static gchar     *get_scheme = NULL;
static gchar    **open_uris = NULL;

static GOptionEntry entries[] = 
{
	{ "get-actions", 0, 
	  0, G_OPTION_ARG_STRING, 
	  &get_actions, 
	  "Get a list of actions for a scheme like \"http\" [DEPRECATED]",
	  NULL },
	{ "get-default", 0, 
	  0, G_OPTION_ARG_STRING, 
	  &get_default, 
	  "Get the default action for a scheme like \"http\" [DEPRECATED]", 
	  NULL },
	{ "set-default", 0, 
	  0, G_OPTION_ARG_STRING, 
	  &set_default, 
	  "Set the default action for a scheme like \"http\" (used WITH --get-default) [DEPRECATED]",
	  NULL },
	{ "use-default", 'd', 
	  0, G_OPTION_ARG_NONE, 
	  &use_default, 
	  "Use the no action when opening a URI, this means we do NOT get the default action first.", 
	  NULL },
	{ "get-actions-by-uri", 'a', 
	  0, G_OPTION_ARG_STRING, 
	  &get_actions_by_uri, 
	  "Get a list of actions by uri (can be used with --action-type)",
	  NULL },
	{ "action-type", 't', 
	  0, G_OPTION_ARG_STRING, 
	  &action_type, 
	  "The action type (\"Normal\", \"Neutral\" or \"Fallback\"), used with --get-actions-by-uri",
	  NULL },
	{ "is-default", 'i', 
	  0, G_OPTION_ARG_STRING, 
	  &is_default, 
	  "Return TRUE or FALSE for an action name (this is used WITH --get-actions)", 
	  NULL },
	{ "get-default-by-uri", 'g', 
	  0, G_OPTION_ARG_STRING, 
	  &get_default_by_uri, 
	  "Get the default action by uri", 
	  NULL },
	{ "set-default-by-uri", 's', 
	  0, G_OPTION_ARG_STRING, 
	  &set_default_by_uri, 
	  "Set the default action by uri (used WITH --get-default-by-uri)",
	  NULL },
	{ "set-default-to-nothing", 'n', 
	  0, G_OPTION_ARG_STRING, 
	  &set_default_to_nothing, 
	  "Set the default action for a scheme like \"http\" to nothing, this unsets it",
	  NULL },
	{ "get-scheme", 'e', 
	  0, G_OPTION_ARG_STRING, 
	  &get_scheme, 
	  "Get the scheme from a URI like \"http://www.google.com\"",
	  NULL },
	{ "open-uri", 'u', 
	  0, G_OPTION_ARG_STRING_ARRAY, 
	  &open_uris, 
	  "Open one or more URIs (for example, \"-u http://... -u callto://...\", etc)",
	  NULL },
	{ NULL }
};

int
main (int argc, char **argv)
{
	GOptionContext *context;
	GError         *error = NULL;
	
	context = g_option_context_new ("- test the hildon-uri API.");
	g_option_context_add_main_entries (context, entries, NULL);
	g_option_context_parse (context, &argc, &argv, NULL);
	g_option_context_free (context);

	if ((!use_default && !get_actions && !get_actions_by_uri &&
	     !get_default && !get_default_by_uri && 
	     !set_default && !set_default_by_uri && !set_default_to_nothing && 
	     !get_scheme && !open_uris) ||
	    (set_default && !get_default) || 
	    (is_default && !get_actions)) {
 		g_printerr ("Usage: %s --help\n", argv[0]); 
		return EXIT_FAILURE;
	}
	
	gnome_vfs_init ();

	if (set_default_to_nothing) {
		hildon_uri_set_default_action (set_default_to_nothing, NULL, &error);
		if (error != NULL) {
			g_printerr ("Could not set default to nothing for scheme:'%s', error:%d->'%s'\n", 
				    set_default_to_nothing, error->code, error->message);
			g_clear_error (&error); 
			return EXIT_FAILURE;
		}

		g_print ("Default for scheme:'%s' unset.\n", set_default_to_nothing);
	}

	if (get_default && set_default) {
		HildonURIAction *default_action;
		
		default_action = hildon_uri_get_default_action (get_default, &error);
		if (error != NULL) {
			g_printerr ("Could not get default action for scheme:'%s', error:%d->'%s'\n", 
				    get_default, error->code, error->message);
			g_clear_error (&error); 
			return EXIT_FAILURE;
		} 

		hildon_uri_set_default_action (set_default, default_action, &error);
		if (error != NULL) {
			g_printerr ("Could not set default action for scheme:'%s', error:%d->'%s'\n", 
				    set_default, error->code, error->message);
			g_clear_error (&error); 
			return EXIT_FAILURE;
		}

		if (default_action) {
			hildon_uri_action_unref (default_action);
		}

		g_print ("Default for:'%s' set for '%s' too.\n", 
			 get_default, set_default);
	}

	if (get_actions && is_default) {
		GSList *actions;
		GSList *l;

		actions = hildon_uri_get_actions (get_actions, &error);
		if (error != NULL) {
			g_printerr ("Could not get actions for scheme:'%s', error:%d->'%s'\n", 
				    get_actions, error->code, error->message);
			g_clear_error (&error); 
			return EXIT_FAILURE;
		}

		if (actions) {
			HildonURIAction *action;
			const gchar   *name = NULL;
			gboolean       found = FALSE;

			for (l = actions; l && !found; l = l->next) {
				action = l->data;
				name = hildon_uri_action_get_name (action);

				if (name && strcmp (name, is_default) == 0) {
					found = TRUE;
				}
			}

			if (found) {
				gboolean is_default_action;

				is_default_action = hildon_uri_is_default_action (action, NULL);

				g_print ("Action:'%s' %s the default action\n", 
					 is_default, is_default_action ? "is" : "is NOT");
			} else {
				g_print ("Action:'%s' was not found\n", is_default);
			}
			
			hildon_uri_free_actions (actions);
		} else {
			g_print ("No actions for scheme:'%s'\n", get_actions);
		}
	}

	if (get_default_by_uri && set_default_by_uri) {
		HildonURIAction *default_action;
		
		default_action = hildon_uri_get_default_action_by_uri (get_default_by_uri, &error);
		if (error != NULL) {
			g_printerr ("Could not get default action by uri:'%s', error:%d->'%s'\n", 
				    get_default_by_uri, error->code, error->message);
			g_clear_error (&error); 
			return EXIT_FAILURE;
		} 

		hildon_uri_set_default_action_by_uri (set_default_by_uri, default_action, &error);
		if (error != NULL) {
			g_printerr ("Could not set default action by uri:'%s', error:%d->'%s'\n", 
				    set_default_by_uri, error->code, error->message);
			g_clear_error (&error); 
			return EXIT_FAILURE;
		}

		if (default_action) {
			hildon_uri_action_unref (default_action);
		}

		g_print ("Default for:'%s' set for '%s' too.\n", 
			 get_default_by_uri, set_default_by_uri);
	}

	if (get_actions_by_uri && is_default) {
		GSList *actions;
		GSList *l;

		actions = hildon_uri_get_actions_by_uri (get_actions_by_uri, -1, &error);
		if (error != NULL) {
			g_printerr ("Could not get actions by uri:'%s', error:%d->'%s'\n", 
				    get_actions_by_uri, error->code, error->message);
			g_clear_error (&error); 
			return EXIT_FAILURE;
		}

		if (actions) {
			HildonURIAction *action;
			const gchar   *name = NULL;
			gboolean       found = FALSE;

			for (l = actions; l && !found; l = l->next) {
				action = l->data;
				name = hildon_uri_action_get_name (action);

				if (name && strcmp (name, is_default) == 0) {
					found = TRUE;
				}
			}

			if (found) {
				gboolean is_default_action;

				is_default_action = hildon_uri_is_default_action (action, NULL);

				g_print ("Action:'%s' %s the default action\n", 
					 is_default, is_default_action ? "is" : "is NOT");
			} else {
				g_print ("Action:'%s' was not found\n", is_default);
			}
			
			hildon_uri_free_actions (actions);
		} else {
			g_print ("No actions for uri:'%s'\n", get_actions_by_uri);
		}
	}

	if (get_default) {
		HildonURIAction *default_action;

		default_action = hildon_uri_get_default_action (get_default, &error);
		if (error != NULL) {
			g_printerr ("Could not get default action for scheme:'%s', error:%d->'%s'\n", 
				    get_default, error->code, error->message);
			g_clear_error (&error); 
			return EXIT_FAILURE;
		} 

		if (default_action) {
			g_print ("Default action for scheme:'%s' is '%s'\n", 
				 get_default, hildon_uri_action_get_name (default_action));
			hildon_uri_action_unref (default_action);
		} else {
			g_print ("No default action for scheme:'%s'\n", 
				 get_default);
		}
	}

	if (get_default_by_uri) {
		HildonURIAction *default_action;

		default_action = hildon_uri_get_default_action_by_uri (get_default_by_uri, &error);
		if (error != NULL) {
			g_printerr ("Could not get default action for uri:'%s', error:%d->'%s'\n", 
				    get_default_by_uri, error->code, error->message);
			g_clear_error (&error); 
			return EXIT_FAILURE;
		} 

		if (default_action) {
			g_print ("Default action for uri:'%s' is '%s'\n", 
				 get_default_by_uri, hildon_uri_action_get_name (default_action));
			hildon_uri_action_unref (default_action);
		} else {
			g_print ("No default action for uri:'%s'\n", 
				 get_default_by_uri);
		}
	}

	if (get_actions) {
		GSList *actions = NULL;
		GSList *l;

		actions = hildon_uri_get_actions (get_actions, &error);

		if (error != NULL) {
			g_printerr ("Could not get actions for scheme:'%s', error:%d->'%s'\n", 
				    get_actions, error->code, error->message);
			g_clear_error (&error); 
			return EXIT_FAILURE;
		}

		if (actions) {
			g_print ("Actions for scheme:'%s' are:\n", get_actions);
			
			for (l = actions; l; l = l->next) {
				HildonURIAction *action;
				
				action = l->data;
				
				g_print ("\t%s\n", hildon_uri_action_get_name (action));
			}
			
			hildon_uri_free_actions (actions);
		} else {
			g_print ("No actions for scheme:'%s'\n", get_actions);
		}
	}

	if (get_actions_by_uri) {
		GSList *actions = NULL;
		GSList *l;

		if (!action_type) {
			actions = hildon_uri_get_actions_by_uri (get_actions_by_uri, -1, &error);
		} else {
			HildonURIActionType type;
			
			type = HILDON_URI_ACTION_NORMAL;
			
			if (action_type) {
				if (g_ascii_strcasecmp (action_type, "Neutral") == 0) {
					type = HILDON_URI_ACTION_NEUTRAL;
				} else if (g_ascii_strcasecmp (action_type, "Fallback") == 0) {
					type = HILDON_URI_ACTION_FALLBACK;
				}
			}

			actions = hildon_uri_get_actions_by_uri (get_actions_by_uri, type, &error);
		}

		if (error != NULL) {
			g_printerr ("Could not get actions for uri:'%s', error:%d->'%s'\n", 
				    get_actions_by_uri, error->code, error->message);
			g_clear_error (&error); 
			return EXIT_FAILURE;
		}

		if (actions) {
			g_print ("Actions for uri:'%s' are:\n", get_actions_by_uri);
			
			for (l = actions; l; l = l->next) {
				HildonURIAction *action;
				
				action = l->data;
				
				g_print ("\t%s\n", hildon_uri_action_get_name (action));
			}
			
			hildon_uri_free_actions (actions);
		} else {
			g_print ("No actions for uri:'%s'\n", get_actions_by_uri);
		}
	}

	if (get_scheme) {
		gchar *scheme;

		scheme = hildon_uri_get_scheme_from_uri (get_scheme, &error);
		if (error != NULL) {
			g_printerr ("Could not get scheme from uri:'%s', error:%d->'%s'\n", 
				    get_scheme, error->code, error->message);
			g_clear_error (&error); 
			return EXIT_FAILURE;
		}

		if (!scheme) {
			g_printerr ("Could not get scheme from uri:'%s'\n", 
				    get_scheme);
			return EXIT_FAILURE;
		}

		g_print ("Scheme for URI:'%s' is '%s'\n", 
			 get_scheme, scheme);
		g_free (scheme);
	}

	if (open_uris) {
		const gchar *uri;
		gint         i = 0;

		if (use_default) {
			g_print ("Using NO action to open ALL these URIs...\n");
		}
		
		while ((uri = open_uris[i++]) != NULL) {
		       HildonURIAction *default_action = NULL;
		       gboolean       success;

		       if (!use_default) {
			       gchar *scheme;

			       scheme = hildon_uri_get_scheme_from_uri (uri, NULL);
			       default_action = hildon_uri_get_default_action (scheme, NULL);
			       
			       if (default_action) {
				       g_print ("Using default action:'%s' to open URI:'%s'...\n", 
						hildon_uri_action_get_name (default_action), uri);
			       } else {
				       g_print ("Using no action to open URI:'%s'...\n", 
						uri);
			       }
			       
			       g_free (scheme);
		       }

		       success = hildon_uri_open (uri, default_action, &error);

		       if (default_action) {
			       hildon_uri_action_unref (default_action);
		       }
		
		       if (error != NULL) {
			       g_printerr ("Could not open URI:'%s', error:%d->'%s'\n", 
					   uri, error->code, error->message);
			       
			       g_clear_error (&error); 
			       return EXIT_FAILURE;
		       }

		       if (!success) {
			       g_printerr ("Could not open URI:'%s', no error either?\n", 
					   uri);
		       }
			
		       g_print ("Opened URI:'%s' successfully\n\n", uri);
		}
	}

	return EXIT_SUCCESS;
}

