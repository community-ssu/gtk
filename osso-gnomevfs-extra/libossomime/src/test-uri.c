#include <config.h>
#include <stdlib.h>
#include <libgnomevfs/gnome-vfs.h>
#include <osso-mime.h>

static gboolean   use_default = FALSE;
static gchar     *get_actions = NULL;
static gchar     *get_default = NULL;
static gchar     *set_default = NULL;
static gchar     *set_default_to_nothing = NULL;
static gchar     *get_scheme = NULL;
static gchar    **open_uris = NULL;

static GOptionEntry entries[] = 
{
	{ "use-default", 'd', 
	  0, G_OPTION_ARG_NONE, 
	  &use_default, 
	  "Use the no action when opening a URI, this means we do NOT get the default action first.", 
	  NULL },
	{ "get-actions", 'a', 
	  0, G_OPTION_ARG_STRING, 
	  &get_actions, 
	  "Get a list of actions for a scheme like \"http\"",
	  NULL },
	{ "get-default", 'g', 
	  0, G_OPTION_ARG_STRING, 
	  &get_default, 
	  "Get the default action for a scheme like \"http\"", 
	  NULL },
	{ "set-default", 's', 
	  0, G_OPTION_ARG_STRING, 
	  &set_default, 
	  "Set the default action for a scheme like \"http\" (this is used WITH --get-default)",
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
	
	context = g_option_context_new ("- test the osso-uri API.");
	g_option_context_add_main_entries (context, entries, NULL);
	g_option_context_parse (context, &argc, &argv, NULL);
	
	g_option_context_free (context);

	if ((!use_default && !get_actions && !get_default && 
	     !set_default && !set_default_to_nothing && 
	     !get_scheme && !open_uris) ||
	    (set_default && !get_default)) {
		g_printerr ("Usage: %s --help\n", argv[0]);
		return EXIT_FAILURE;
	}

	gnome_vfs_init ();

	if (set_default_to_nothing) {
		osso_uri_set_default_action (set_default_to_nothing, NULL, &error);
		if (error != NULL) {
			g_printerr ("Could not set default to nothing for scheme:'%s', error:%d->'%s'\n", 
				    set_default_to_nothing, error->code, error->message);
			
			g_clear_error (&error); 
			return EXIT_FAILURE;
		}

		g_print ("Default for scheme:'%s' unset.\n", set_default_to_nothing);
	}

	if (get_default && set_default) {
		OssoURIAction *default_action;
		
		default_action = osso_uri_get_default_action (get_default, &error);
		if (error != NULL) {
			g_printerr ("Could not get default action for scheme:'%s', error:%d->'%s'\n", 
				    get_default, error->code, error->message);
			
			g_clear_error (&error); 
			return EXIT_FAILURE;
		} 

		osso_uri_set_default_action (set_default, default_action, &error);
		if (error != NULL) {
			g_printerr ("Could not set default action for scheme:'%s', error:%d->'%s'\n", 
				    set_default, error->code, error->message);
			
			g_clear_error (&error); 
			return EXIT_FAILURE;
		}

		if (default_action) {
			osso_uri_action_unref (default_action);
		}

		g_print ("Default for:'%s' set for '%s' too.\n", 
			 get_default, set_default);
	}

	if (get_default) {
		OssoURIAction *default_action;

		default_action = osso_uri_get_default_action (get_default, &error);
		if (error != NULL) {
			g_printerr ("Could not get default action for scheme:'%s', error:%d->'%s'\n", 
				    get_default, error->code, error->message);
			
			g_clear_error (&error); 
			return EXIT_FAILURE;
		} 

		if (default_action) {
			g_print ("Default action for scheme:'%s' is '%s'\n", 
				 get_default, osso_uri_action_get_name (default_action));
			osso_uri_action_unref (default_action);
		} else {
			g_print ("No default action for scheme:'%s'\n", 
				 get_default);
		}
	}

	if (get_actions) {
		GSList *actions;
		GSList *l;

		actions = osso_uri_get_actions (get_actions, &error);
		if (error != NULL) {
			g_printerr ("Could not get actions for scheme:'%s', error:%d->'%s'\n", 
				    get_actions, error->code, error->message);
			
			g_clear_error (&error); 
			return EXIT_FAILURE;
		}

		if (actions) {
			g_print ("Actions for scheme:'%s' are:\n", get_actions);
			
			for (l = actions; l; l = l->next) {
				OssoURIAction *action;
				
				action = l->data;
				
				g_print ("\t%s\n", osso_uri_action_get_name (action));
			}
			
			osso_uri_free_actions (actions);
		} else {
			g_print ("No actions for scheme:'%s'\n", get_actions);
		}
	}

	if (get_scheme) {
		gchar *scheme;

		scheme = osso_uri_get_scheme_from_uri (get_scheme, &error);
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
		       OssoURIAction *default_action = NULL;
		       gboolean       success;

		       if (!use_default) {
			       gchar *scheme;

			       scheme = osso_uri_get_scheme_from_uri (uri, NULL);
			       default_action = osso_uri_get_default_action (scheme, NULL);
			       
			       if (default_action) {
				       g_print ("Using default action:'%s' to open URI:'%s'...\n", 
						osso_uri_action_get_name (default_action), uri);
			       } else {
				       g_print ("Using no action to open URI:'%s'...\n", 
						uri);
			       }
			       
			       g_free (scheme);
		       }

		       success = osso_uri_open (uri, default_action, &error);

		       if (default_action) {
			       osso_uri_action_unref (default_action);
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

