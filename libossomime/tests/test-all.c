/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* This test disregards any setup of mime types etc in the system and only looks
 * at the test-datadir directory included here.
 */

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>
#include <osso-mime.h>

#define assert_int(a, b) G_STMT_START {				        \
	if (a != b) {						        \
		g_log ("ASSERT INT",					\
		       G_LOG_LEVEL_ERROR,				\
		       "%s:%d: failed: (%s), expected %d, got %d",	\
		       __FILE__,					\
		       __LINE__,					\
		       #a,						\
		       a,						\
		       b);						\
	}								\
	} G_STMT_END			

#define assert_bool(a) G_STMT_START {					\
	if (!a) {   					                \
		g_log ("ASSERT BOOL",					\
		       G_LOG_LEVEL_ERROR,				\
		       "%s:%d: failed: (%s), expected to be TRUE",	\
		       __FILE__,					\
		       __LINE__,					\
		       #a);						\
	}						           	\
	} G_STMT_END			

#define assert_expr(a) G_STMT_START {					\
	if (!(a)) {						        \
		g_log ("ASSERT EXPR",					\
		       G_LOG_LEVEL_ERROR,				\
		       "%s:%d: failed: (%s)",				\
		       __FILE__,					\
		       __LINE__,					\
		       #a);						\
	}								\
	} G_STMT_END			

#define assert_string(a, b) G_STMT_START {				\
	if (!a && b) {						        \
		g_log ("ASSERT SRTING",					\
		       G_LOG_LEVEL_ERROR,				\
		       "%s:%d: failed: expected 'a' to be non-NULL",	\
		       __FILE__,					\
		       __LINE__);					\
	} else if (a && !b) {						\
		g_log ("ASSERT STRING",					\
		       G_LOG_LEVEL_ERROR,				\
		       "%s:%d: failed: expected 'b' to be non-NULL",	\
		       __FILE__,					\
		       __LINE__);					\
	} else if (!a && !b) {						\
		g_log ("ASSERT STRING",					\
		       G_LOG_LEVEL_ERROR,				\
		       "%s:%d: failed: 'a' & 'b' are NULL",		\
		       __FILE__,					\
		       __LINE__);					\
	} else if (strcmp (a, b) != 0) {				\
		g_log ("ASSERT STRING",					\
		       G_LOG_LEVEL_ERROR,				\
		       "%s:%d: failed: (%s), expected '%s', got '%s'",	\
		       __FILE__,					\
		       __LINE__,					\
		       #a,						\
		       a,						\
		       b);						\
	}								\
	} G_STMT_END			

static void
print_header (const gchar *str)
{
	gchar *tmp;
	
	g_print ("\n");
	g_print ("%s\n", str);

	tmp = g_strnfill (strlen (str), '=');
	g_print ("%s\n", tmp);
	g_free (tmp);
}

static void
test_get_mime_types (void)
{
	GList *mimes;

	print_header ("Get mime types for apps and categories");

	g_print ("For non-existent application name\n");
	mimes = osso_mime_application_get_mime_types ("foo/bar");
	assert_int (g_list_length (mimes), 0);
	osso_mime_application_mime_types_list_free (mimes);

	g_print ("For image viewer application, with slash\n");
	mimes = osso_mime_application_get_mime_types ("test/image-viewer.desktop");
	assert_int (g_list_length (mimes), 8);
	osso_mime_application_mime_types_list_free (mimes);

	g_print ("For image viewer application, with dash\n");
	mimes = osso_mime_application_get_mime_types ("test-image-viewer.desktop");
	assert_int (g_list_length (mimes), 8);
	osso_mime_application_mime_types_list_free (mimes);

	g_print ("For media player application\n");
	mimes = osso_mime_application_get_mime_types ("test/mp_ui.desktop");
	assert_int (g_list_length (mimes), 24);
	osso_mime_application_mime_types_list_free (mimes);

	g_print ("For audio category\n");
	mimes = osso_mime_get_mime_types_for_category (OSSO_MIME_CATEGORY_AUDIO);
	assert_int (g_list_length (mimes), 13);
	osso_mime_application_mime_types_list_free (mimes);

	g_print ("For video category\n");
	mimes = osso_mime_get_mime_types_for_category (OSSO_MIME_CATEGORY_VIDEO);
	assert_int (g_list_length (mimes), 7);
	osso_mime_application_mime_types_list_free (mimes);

	g_print ("For invalid category\n");
	mimes = osso_mime_get_mime_types_for_category (23);
	assert_int (g_list_length (mimes), 0);
	osso_mime_application_mime_types_list_free (mimes);
}

static void
test_get_actions (void)
{
	GSList        *actions;
	OssoURIAction *action;

	print_header ("Testing actions");

	/* Simple test. */
	g_print ("For testapp\n");
	actions = osso_uri_get_actions ("testapp", NULL);
	assert_int (g_slist_length (actions), 1);
	action = actions->data;
	osso_uri_free_actions (actions);

	/* Test with more than 1 action. */
	g_print ("For http\n");
	actions = osso_uri_get_actions_by_uri ("http://www.nokia.com", -1, NULL);
	assert_int (g_slist_length (actions), 3);
	action = actions->data;
	osso_uri_free_actions (actions);

	/* Another one, just proves that you can get the action for a plugin
	 * that adds a scheme, even though the same app handles the service.
	 */
	g_print ("For testplugin\n");
	actions = osso_uri_get_actions ("testplugin", NULL);
	assert_int (g_slist_length (actions), 1);
	action = actions->data;
	osso_uri_free_actions (actions);

	/* Test the fields in the action. */
	g_print ("Test fields in the action\n");
	actions = osso_uri_get_actions ("test-action-fields", NULL);
	assert_int (g_slist_length (actions), 1);
	action = actions->data;
	assert_string (osso_uri_action_get_name (action), "name_of_this_foo_bar_test");
	assert_string (osso_uri_action_get_service (action), "random_app");
	assert_string (osso_uri_action_get_method (action), "method_foo_bar");
	osso_uri_free_actions (actions);
	
	/* Test the fields in the action, service is specifed in the action. */
	g_print ("Test fields in another action\n");
	actions = osso_uri_get_actions ("test-action-fields2", NULL);
	assert_int (g_slist_length (actions), 1);
	action = actions->data;
	assert_string (osso_uri_action_get_name (action), "name_of_this_foo_bar_test2");
	/* Note, that is not possible, not sure if it should be? */
	/*assert_string (osso_uri_action_get_service (action), "another_service");*/
	assert_string (osso_uri_action_get_method (action), "method_foo_bar2");
	assert_string (osso_uri_action_get_translation_domain (action), "foo_bar_domain");
	osso_uri_free_actions (actions);
}

static gboolean
is_default_action (GSList      *actions, 
		   const gchar *action_name, 
		   const gchar *uri)
{
	GSList        *l;
	OssoURIAction *action;

	/* NOTE: uri is only used for the new API calls */

	if (!action_name) {
		return FALSE;
	}

	if (uri) {
		for (l = actions; l; l = l->next) {
			action = l->data;
			if (strcmp (osso_uri_action_get_name (action), action_name) == 0) {
				return osso_uri_is_default_action_by_uri (uri, action, NULL);
			}
		}
	} else {
		for (l = actions; l; l = l->next) {
			action = l->data;
			if (strcmp (osso_uri_action_get_name (action), action_name) == 0) {
				return osso_uri_is_default_action (action, NULL);
			}
		}
	}

	return FALSE;
}

static void
test_system_default_actions (void)
{
	GSList      *actions;
	const gchar *uri_str;

	print_header ("Testing default actions (system)");

	/* Make sure any old local override is removed. */
	unlink (TEST_DATADIR "-local/applications/uri-action-defaults.list");

	/* The browser should be the default for http. */
	g_print ("For http\n");

	uri_str = "http://www.nokia.com";
	actions = osso_uri_get_actions_by_uri (uri_str, -1, NULL);
	assert_int (g_slist_length (actions), 4);

	/* The default. */
	assert_bool (is_default_action (actions, "uri_link_open_link", uri_str));

	/* Existing, non-default. */
	assert_bool (!is_default_action (actions, "uri_link_save_link", uri_str));

	/* Non-existing. */
	assert_bool (!is_default_action (actions, "foo_open", uri_str));
	
	osso_uri_free_actions (actions);

	/* Basic use of the new API to get actions by a URI */
	actions = osso_uri_get_actions_by_uri ("http://www.nokia.com", OSSO_URI_ACTION_NORMAL, NULL);
	assert_int (g_slist_length (actions), 2);
	osso_uri_free_actions (actions);

	/* Getting neutral actions which apply in all conditions where
	 * there is a known mime type. There are 2 here because we
	 * also have the older desktop file which defaults as a
	 * neutral action type. 
	 */
	actions = osso_uri_get_actions_by_uri ("http://www.nokia.com", OSSO_URI_ACTION_NEUTRAL, NULL);
	assert_int (g_slist_length (actions), 2);
	osso_uri_free_actions (actions);

	/* Fallbacks only get returned if the mime type is unknown, so
	 * we test with something known first and then with something
	 * unknown second. 
	 */
	actions = osso_uri_get_actions_by_uri ("http://www.nokia.com", OSSO_URI_ACTION_FALLBACK, NULL);
	assert_int (g_slist_length (actions), 0);
	osso_uri_free_actions (actions);

	actions = osso_uri_get_actions_by_uri ("http://www.imendio.com/sliff.sloff", OSSO_URI_ACTION_FALLBACK, NULL);
	assert_int (g_slist_length (actions), 1);
	osso_uri_free_actions (actions);
}

static void
test_local_default_actions (void)
{
	GSList        *actions, *l;
	OssoURIAction *action;
	OssoURIAction *default_action;
	const gchar   *uri_str;
	gboolean       success;

	/* FIXME: Implement. */
	
	print_header ("Testing default actions (local)");

	/* Make sure any old local override is removed. */
	unlink (TEST_DATADIR "-local/applications/uri-action-defaults.list");

	/* The browser should be the default for http. */
	g_print ("For http\n");
	actions = osso_uri_get_actions_by_uri ("http://www.imendio.com", -1, NULL);
	assert_int (g_slist_length (actions), 4);

	/* The default. */
	assert_bool (is_default_action (actions, "uri_link_open_link", NULL));

	/* Override the default. */
	action = NULL;
	for (l = actions; l; l = l->next) {
		action = l->data;

		if (!osso_uri_is_default_action (action, NULL)) {
			break;
		}

		action = NULL;
	}

	/* Note: If this fails, there's a bug in the test or the test data. */
	if (!action) {
		g_error ("Found no non-default action, test bug.");
	}
	
	if (!osso_uri_set_default_action ("http", action, NULL)) {
		g_error ("Couldn't set default...\n");
	}

	assert_bool (!is_default_action (actions, "http", NULL));
	assert_bool (is_default_action (actions, osso_uri_action_get_name (action), NULL));

	/* Reset the default. */
	if (!osso_uri_set_default_action ("http", NULL, NULL)) {
		g_error ("Couldn't set default...\n");
	}

	/* We're back to the system default. */
	assert_bool (is_default_action (actions, "uri_link_open_link", NULL));
	
	osso_uri_free_actions (actions);

	/*
	 * Test getting and setting default actions 
	 */

	/* Set to nothing */
	success = osso_uri_set_default_action_by_uri ("http://www.nokia.com", NULL, NULL);
	assert_bool (success);

	/* Test it is unset */
	default_action = osso_uri_get_default_action_by_uri ("http://www.nokia.com", NULL);
	assert_expr (default_action == NULL);

	/* Test setting a NORMAL action */
	uri_str = "http://www.google.co.uk/intl/en_uk/images/logo.gif";
	actions = osso_uri_get_actions_by_uri (uri_str, -1, NULL);

	for (l = actions; l; l = l->next) {
		action = l->data;

		if (strcmp (osso_uri_action_get_name (action), "addr_ap_address_book") == 0) {
			break;
		} 

		action = NULL;
	}

	assert_expr (action != NULL);

	osso_uri_action_ref (action);
	osso_uri_free_actions (actions);

	success = osso_uri_set_default_action_by_uri (uri_str, action, NULL);
	assert_bool (success);

	actions = osso_uri_get_actions_by_uri (uri_str, -1, NULL);
	assert_bool (is_default_action (actions, osso_uri_action_get_name (action), uri_str));
	osso_uri_action_unref (action);

	/* Test setting a NEUTRAL action */ 
	actions = osso_uri_get_actions_by_uri (uri_str, OSSO_URI_ACTION_NEUTRAL, NULL);

	for (l = actions; l; l = l->next) {
		action = l->data;

		if (strcmp (osso_uri_action_get_name (action), "uri_link_save_link") == 0) {
			break;
		} 

		action = NULL;
	}

	assert_expr (action != NULL);

	osso_uri_action_ref (action);
	osso_uri_free_actions (actions);
	
	success = osso_uri_set_default_action_by_uri (uri_str, action, NULL);
	assert_bool (success);

	actions = osso_uri_get_actions_by_uri (uri_str, -1, NULL);
	assert_bool (is_default_action (actions, osso_uri_action_get_name (action), uri_str));
	osso_uri_action_unref (action);

	/* Test setting a FALLBACK action */ 
	uri_str = "http://www.imendio.com/sliff.sloff";
	actions = osso_uri_get_actions_by_uri (uri_str, OSSO_URI_ACTION_FALLBACK, NULL);
	assert_int (g_slist_length (actions), 1);

	action = actions->data;
	assert_string (osso_uri_action_get_name (action), "uri_link_open_link_fallback");

	osso_uri_action_ref (action);
	osso_uri_free_actions (actions);
	
	success = osso_uri_set_default_action_by_uri (uri_str, action, NULL);
	assert_bool (success);

	actions = osso_uri_get_actions_by_uri (uri_str, -1, NULL);
	assert_bool (is_default_action (actions, osso_uri_action_get_name (action), uri_str));
	osso_uri_action_unref (action);

	/* Clean up for new set of tests */
	unlink (TEST_DATADIR "-local/applications/uri-action-defaults.list");

	/* Test setting a new NORMAL action with the old API */ 
	uri_str = "http://www.google.co.uk/intl/en_uk/images/logo.gif";
	actions = osso_uri_get_actions_by_uri (uri_str, -1, NULL);

	for (l = actions; l; l = l->next) {
		action = l->data;

		if (strcmp (osso_uri_action_get_name (action), "addr_ap_address_book") == 0) {
			break;
		} 

		action = NULL;
	}

	assert_expr (action != NULL);

	osso_uri_action_ref (action);
	osso_uri_free_actions (actions);

	success = osso_uri_set_default_action ("http", action, NULL);
	assert_bool (success);

	actions = osso_uri_get_actions_by_uri (uri_str, -1, NULL);
	assert_bool (is_default_action (actions, osso_uri_action_get_name (action), uri_str));
	assert_bool (is_default_action (actions, osso_uri_action_get_name (action), NULL));
	osso_uri_action_unref (action);
	
	/* Test setting a new NEUTRAL action with the old API */ 
	actions = osso_uri_get_actions_by_uri (uri_str, OSSO_URI_ACTION_NEUTRAL, NULL);

	for (l = actions; l; l = l->next) {
		action = l->data;

		if (strcmp (osso_uri_action_get_name (action), "uri_link_save_link") == 0) {
			break;
		} 

		action = NULL;
	}

	assert_expr (action != NULL);

	osso_uri_action_ref (action);
	osso_uri_free_actions (actions);
	
	success = osso_uri_set_default_action ("http", action, NULL);
	assert_bool (success);

	actions = osso_uri_get_actions_by_uri (uri_str, -1, NULL);
	assert_bool (is_default_action (actions, osso_uri_action_get_name (action), uri_str));
	assert_bool (is_default_action (actions, osso_uri_action_get_name (action), NULL));
	osso_uri_action_unref (action);

	/* Test setting a new FALLBACK action with the old API */ 
	uri_str = "http://www.imendio.com/sliff.sloff";
	actions = osso_uri_get_actions_by_uri (uri_str, OSSO_URI_ACTION_FALLBACK, NULL);
	assert_int (g_slist_length (actions), 1);

	action = actions->data;
	assert_string (osso_uri_action_get_name (action), "uri_link_open_link_fallback");

	osso_uri_action_ref (action);
	osso_uri_free_actions (actions);
	
	success = osso_uri_set_default_action ("http", action, NULL);
	assert_bool (success);

	actions = osso_uri_get_actions_by_uri (uri_str, -1, NULL);
	assert_bool (is_default_action (actions, osso_uri_action_get_name (action), uri_str));
	assert_bool (is_default_action (actions, osso_uri_action_get_name (action), NULL));
	osso_uri_action_unref (action);

	/* Clean up. */
	unlink (TEST_DATADIR "-local/applications/uri-action-defaults.list");
}

int
main (int argc, char **argv)
{
	gchar    *tmp;
	GError   *error = NULL;
	gboolean  ret;

	/* Use our custom data here. */
	g_setenv ("XDG_DATA_DIRS", TEST_DATADIR, TRUE);
	g_setenv ("XDG_DATA_HOME", TEST_DATADIR "-local", TRUE);

	tmp = g_strdup_printf ("../libossomime/osso-update-category-database -v %s",
			       TEST_DATADIR "/mime");
	ret = g_spawn_command_line_sync (tmp, NULL, NULL, NULL, &error);
	g_free (tmp);
	if (!ret) {
		g_printerr ("Couldn't launch ../libossomime/osso-update-category-database: %s\n",
			    error->message);
		g_clear_error (&error);
		return 1;
	}
	
	tmp = g_strdup_printf ("update-mime-database -v %s",
			       TEST_DATADIR "/mime");
	ret = g_spawn_command_line_sync (tmp, NULL, NULL, NULL, &error);
	g_free (tmp);
	if (!ret) {
		g_printerr ("Couldn't launch update-mime-database: %s\n",
			    error->message);
		g_clear_error (&error);
		return 1;
	}
	
	tmp = g_strdup_printf ("update-desktop-database -v %s",
			       TEST_DATADIR "/applications");
	ret = g_spawn_command_line_sync (tmp, NULL, NULL, NULL, &error);
	g_free (tmp);
	if (!ret) {
		g_printerr ("Couldn't launch update-desktop-database: %s\n",
			    error->message);
		g_clear_error (&error);
		return 1;
	}

	if (!gnome_vfs_init()) {
		g_error ("Could not initialise GnomeVFS");
		return 1;
	}

	if (0) {	
		test_get_mime_types ();
		test_get_actions ();
	}

	test_system_default_actions ();
	test_local_default_actions ();

	gnome_vfs_shutdown ();

	return 0;
}

