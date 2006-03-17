/*
 * This is a VERY incomplete and even somewhat incorrect implementation of a
 * reader/parser for fdo's menu-spec compliant menus. Quite Maemo specific.
 *
 */

#include "libhildonmenu.h"

#include <libosso.h>
#include <osso-log.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <dirent.h>
#include <sys/stat.h>
#include <libxml/xmlreader.h>
#include <string.h>
#include <errno.h>

/* For XML */
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#define DESKTOP_FILE_SUFFIX ".desktop"
#define DESKTOP_ENTRY_GROUP "Desktop Entry"

/* This comes from Task Navigator */
#define EXTRAS_MENU_STRING "tana_fi_extras"

typedef struct {
	gchar *name;
	gchar *icon;
	gchar *exec;
	gchar *service;
	gchar *desktop_id;
	gboolean allocated;
} desktop_entry_t;

/* Iterator to extras */
static GtkTreeIter *extras_iter = NULL;

/* Function for getting a list of all available files */
GList *get_desktop_files(gchar *directory, GList *desktop_files);

/* Function for getting the item from the list */
desktop_entry_t *get_desktop_item( GList *desktop_files,
		const gchar *desktop_id );

/* Recursive function for reading the menu and adding to treemodel */
static void read_menu_conf(const char *filename,
		GtkTreeStore *menu_tree, xmlDocPtr doc,
		xmlNodePtr root_element, GtkTreeIter *iterator,
		GList *desktop_files);

/* Function for writing the menu to a file */
gboolean write_menu_conf( GtkTreeModel *model, gint menu_type );

static void destroy_desktop_item(gpointer data, gpointer null)
{
    desktop_entry_t * de = (desktop_entry_t *)data;

    g_free(de->name);
    g_free(de->icon);
    g_free(de->exec);
    g_free(de->service);
    g_free(de->desktop_id);

    g_free(de);
}

GdkPixbuf *get_icon(const char *icon_name, int icon_size)
{
	GtkIconTheme *icon_theme;
	GdkPixbuf *pixbuf = NULL;
	GError *error = NULL;


	if (icon_name) {
		icon_theme = gtk_icon_theme_get_default();

		pixbuf = gtk_icon_theme_load_icon(icon_theme,
				icon_name,
				icon_size,
				GTK_ICON_LOOKUP_NO_SVG, &error);

		if (error) {
			ULOG_ERR("Error loading icon '%s': %s\n", icon_name,
					error->message);
			g_error_free(error);
			error = NULL;
		}
	} else {
		ULOG_ERR("Error loading icon: no icon name\n");
	}

	return pixbuf;
} /* static GdkPixbuf *get_icon() */


GList *get_desktop_files(gchar *directory, GList *desktop_files)
{
	DIR *dir_handle = NULL;
	struct dirent *dir_entry = NULL;
	struct stat buf;

	gchar *full_path;

	/* No directory given.. */
	if (directory == NULL) {
		/* ..so we start from the "default" dir */
		full_path = DEFAULT_APPS_DIR;
	} else {
		full_path = directory;
	}

	if ((dir_handle = opendir(full_path)) == NULL) {
		ULOG_ERR("Error reading file '%s'\n", full_path );
		return NULL;
	}

	ULOG_DEBUG( "get_desktop_files: reading dir '%s'", full_path );

	gchar *current_path = NULL;

	GKeyFile *key_file = NULL;
	GError *error = NULL;
	desktop_entry_t *item = NULL;


	while ((dir_entry = readdir(dir_handle)) != NULL) {

		if (strcmp(dir_entry->d_name, ".") == 0
				|| strcmp(dir_entry->d_name, "..") == 0) {
			continue;
		}

		current_path = g_build_filename(full_path,
				dir_entry->d_name, NULL);

		if (stat(current_path, &buf) < 0) {
			ULOG_ERR("%s: %s\n",
					current_path, strerror(errno));

		} else if (S_ISDIR(buf.st_mode)) {
			/* Recurse */
			ULOG_DEBUG( "get_desktop_files: recursing into '%s'",
					current_path );

			desktop_files = get_desktop_files(g_strdup(current_path),
					desktop_files);

		} else if (S_ISREG(buf.st_mode) &&
				g_str_has_suffix(current_path,
					DESKTOP_FILE_SUFFIX)) {

			key_file = g_key_file_new();

			/* Let's read the interesting stuff */
			if ( g_key_file_load_from_file( key_file, current_path,
					G_KEY_FILE_NONE, &error ) == FALSE ) {

				ULOG_ERR("Error reading '%s': %s\n",
						current_path, error->message);
				
				g_error_free(error);
				error = NULL;
                g_free(current_path);
				
				continue;
			}
						

			gchar *type = g_key_file_get_string(
					key_file,
					DESKTOP_ENTRY_GROUP,
					DESKTOP_ENTRY_TYPE_FIELD,
					NULL);

			/* We're only interested in apps */
			if ( !type || strcmp(type, "Application") != 0 ) {
				g_key_file_free(key_file);
                g_free(current_path);
                g_free(type);
				continue;
			}

            g_free(type);

			item = (desktop_entry_t *)
				g_malloc0(sizeof(desktop_entry_t));

			item->icon = g_key_file_get_string(
					key_file,
					DESKTOP_ENTRY_GROUP,
					DESKTOP_ENTRY_ICON_FIELD,
					NULL);

			item->name = g_key_file_get_string(
					key_file,
					DESKTOP_ENTRY_GROUP,
					DESKTOP_ENTRY_NAME_FIELD,
					NULL);

			item->exec = g_key_file_get_string(
					key_file,
					DESKTOP_ENTRY_GROUP,
					DESKTOP_ENTRY_EXEC_FIELD,
					NULL);

			item->service = g_key_file_get_string(
					key_file,
					DESKTOP_ENTRY_GROUP,
					DESKTOP_ENTRY_SERVICE_FIELD,
					NULL);

			item->desktop_id = g_strdup( dir_entry->d_name );

			item->allocated = FALSE;


			ULOG_DEBUG( "get_desktop_files: appending .desktop = %s",
					dir_entry->d_name);

			desktop_files = g_list_append(desktop_files, item);

			g_key_file_free(key_file);

		}

		if (current_path) {
			g_free(current_path);
		}

	}

	closedir(dir_handle);
	g_free(dir_entry);

	if (directory) {
		g_free(directory);
	}

	return desktop_files;
}


desktop_entry_t *get_desktop_item( GList *desktop_files,
		const gchar *desktop_id )
{
	desktop_entry_t *item = NULL;
	
	GList *loop = g_list_first(desktop_files);

	while ( loop != NULL ) {

		item = loop->data;

		if ( strcmp( item->desktop_id, desktop_id ) == 0 ) {
			break;
		}

		item = NULL;
		loop = loop->next;
	}

	return item;
} /* GtkTreeIter *get_desktop_item() */


static void read_menu_conf(const char *filename, GtkTreeStore *menu_tree,
		xmlDocPtr doc, xmlNodePtr root_element, GtkTreeIter *iterator,
		GList *desktop_files)
{
	gint level = 0;

	/* Make sure we have a valid iterator */
	if (!gtk_tree_store_iter_is_valid(menu_tree, iterator)) {
		gtk_tree_store_append(menu_tree, iterator, NULL);
	}

	if (gtk_tree_store_iter_is_valid(menu_tree, iterator)) {
		level = gtk_tree_store_iter_depth(menu_tree, iterator);
	}

	/* Always add the "Favourites" */
	if (level == 0) {
		gtk_tree_store_set(menu_tree,
				iterator,
				TREE_MODEL_NAME,
				g_strdup( FAVOURITES_NAME ),
				TREE_MODEL_ICON,
				get_icon(ICON_FAVOURITES, ICON_SIZE),
				TREE_MODEL_EXEC,
				"",
				TREE_MODEL_SERVICE,
				"",
				TREE_MODEL_DESKTOP_ID,
				"",
				-1);
	}

	if (doc == NULL) {
		doc = xmlReadFile(filename, NULL, 0);

		if (doc == NULL) {
			ULOG_ERR( "read_menu_conf: unable to read '%s'", filename );
			goto check_unallocated;
		}
	}

	if (root_element == NULL) {
		root_element = xmlDocGetRootElement(doc);

		if (root_element == NULL) {
			ULOG_DEBUG( "read_menu_conf: failed to get root element." );
			xmlFreeDoc(doc);
			goto check_unallocated;
		}
	}

	if (xmlStrcmp(root_element->name, (const xmlChar *) "Menu") != 0) {
		/* Not a menu */
		ULOG_DEBUG( "read_menu_conf: not a menu.");
		goto check_unallocated;
	}


	xmlNodePtr current_element = NULL;
	xmlChar *key;
		
	GtkTreeIter child_iterator;
	desktop_entry_t *item;

	GdkPixbuf *icon;

	/* Loop through the elements and add to parent */
	for (current_element = root_element->xmlChildrenNode;
			current_element != NULL;
			current_element = current_element->next) {

		if (strcmp(current_element->name, "Menu") == 0) {
			
			/* Submenu */
			ULOG_DEBUG( "read_menu_conf: "
					"level %i: appending submenu.", level );

			gtk_tree_store_append(menu_tree, &child_iterator, iterator);

			read_menu_conf(filename, menu_tree,
					doc, current_element, &child_iterator, desktop_files);
		} else if (strcmp(current_element->name, "Name") == 0) {

			if (level == 0) {
				/* H4rd c0d3d for top level */
				continue;
			} else {
				key = xmlNodeListGetString(doc,
						current_element->xmlChildrenNode,
						1);
			}

			ULOG_DEBUG( "read_menu_conf: "
					"level %i: Name = '%s'", level, key);

			gtk_tree_store_set(menu_tree, iterator,
					TREE_MODEL_NAME,
					g_strdup( key ),
					TREE_MODEL_ICON,
					get_icon(ICON_FOLDER, ICON_SIZE),
					TREE_MODEL_EXEC,
					"",
					TREE_MODEL_SERVICE,
					"",
					TREE_MODEL_DESKTOP_ID,
					"",
					-1);

			if ( strcmp( key, EXTRAS_MENU_STRING ) == 0 ) {
				extras_iter = gtk_tree_iter_copy( iterator );
			}

			xmlFree(key);

		} else if (strcmp(current_element->name, "MergeFile") == 0) {

			/* FIXME: skip this for now */
			continue;

			key = xmlNodeListGetString(doc,
					current_element->xmlChildrenNode, 1);

			/* Relative path. prepend current dir */
			if (g_str_has_prefix(key, "/") == FALSE) {
				key = g_build_path(G_DIR_SEPARATOR_S,
						g_path_get_dirname(filename), key, NULL);
			}

			/* Recursion */
			read_menu_conf(key, menu_tree, NULL, NULL, iterator, desktop_files);

			xmlFree(key);

		} else if (strcmp(current_element->name, "Include") == 0) {

			xmlNodePtr child_element = NULL;
			GtkTreeIter child_iter;

			for (child_element = current_element->xmlChildrenNode;
					child_element != NULL;
					child_element = child_element->next) {

				if (strcmp(child_element->name, "Filename") == 0) {
					
					/* Get the children */
					key = xmlNodeListGetString(doc,
							child_element->xmlChildrenNode,
							1);

					if ( !key || strlen( key ) == 0) {
						xmlFree( key );
						continue;
					}

					/* Get the contents */
					item = get_desktop_item(desktop_files, key);

					if ( !item ) {
						xmlFree( key );
						continue;
					}

					ULOG_DEBUG( "read_menu_conf: level %i: "
							"appending .desktop", level );

					gtk_tree_store_append(menu_tree,
							&child_iter, iterator);

					/* Check that we have the app icon.. */
					icon = get_icon( item->icon, ICON_SIZE );
					
					if ( !icon ) {
						/* .. or use the default */
						icon = get_icon( ICON_DEFAULT_APP,
								ICON_SIZE );
					}
					
					gtk_tree_store_set(menu_tree,
							&child_iter,
							TREE_MODEL_NAME,
							item->name,
							TREE_MODEL_ICON,
							icon,
							TREE_MODEL_EXEC,
							item->exec,
							TREE_MODEL_SERVICE,
							item->service,
							TREE_MODEL_DESKTOP_ID,
							item->desktop_id,
							-1);

					/* Mark the item allocated */
					item->allocated = TRUE;
					
					xmlFree(key);
				}
			}

		} else if (strcmp(current_element->name, "Separator") == 0) {
			ULOG_DEBUG( "read_menu_conf: level %i: "
					"appending separator.", level );

			GtkTreeIter child_iter;
			
			gtk_tree_store_append(menu_tree,
					&child_iter, iterator);

			gtk_tree_store_set(menu_tree,
					&child_iter,
					TREE_MODEL_NAME,
					SEPARATOR_STRING,
					TREE_MODEL_ICON,
					NULL,
					TREE_MODEL_EXEC,
					SEPARATOR_STRING,
					TREE_MODEL_SERVICE,
					SEPARATOR_STRING,
					TREE_MODEL_DESKTOP_ID,
					SEPARATOR_STRING,
					-1);
		}

	}

	check_unallocated:

	if ( level == 0 ) {

		/* Create the extras menu if it does not exist */
		if ( !extras_iter || !gtk_tree_store_iter_is_valid(
					menu_tree, extras_iter ) ) {

			ULOG_DEBUG( "Menu '%s' does not exist.", EXTRAS_MENU_STRING );

			GtkTreeIter iter;

			gtk_tree_model_get_iter_first(
					GTK_TREE_MODEL( menu_tree ), &iter );

			extras_iter = g_malloc0( sizeof( GtkTreeIter ) );

			gtk_tree_store_append( menu_tree, extras_iter, &iter );

			
			
			if ( !gtk_tree_store_iter_is_valid(
						menu_tree, extras_iter ) ) {
				ULOG_ERR( "read_menu_conf: failed to create "
						"'%s' -menu.", EXTRAS_MENU_STRING );
				return;
			}

			gtk_tree_store_set(menu_tree,
					extras_iter,
					TREE_MODEL_NAME,
					EXTRAS_MENU_STRING,
					TREE_MODEL_ICON,
					get_icon(ICON_FOLDER, ICON_SIZE),
					TREE_MODEL_EXEC,
					"",
					TREE_MODEL_SERVICE,
					"",
					TREE_MODEL_DESKTOP_ID,
					"",
					-1 );

			ULOG_DEBUG( "Menu '%s' created.", EXTRAS_MENU_STRING );
		} else {
			ULOG_DEBUG( "Menu '%s' exists.", EXTRAS_MENU_STRING );
		}

		GList *loop = g_list_first( desktop_files );
		desktop_entry_t *item;

		while ( loop ) {

			item = loop->data;
			if ( item->allocated == FALSE ) {
				ULOG_DEBUG( "read_menu_conf: "
						"unallocated item: '%s'",
						item->desktop_id );

				GtkTreeIter item_iter;

				gtk_tree_store_append( menu_tree,
						&item_iter, extras_iter );

				gtk_tree_store_set(menu_tree,
						&item_iter,
						TREE_MODEL_NAME,
						item->name,
						TREE_MODEL_ICON,
						get_icon( item->icon, ICON_SIZE ),
						TREE_MODEL_EXEC,
						item->exec,
						TREE_MODEL_SERVICE,
						item->service,
						TREE_MODEL_DESKTOP_ID,
						item->desktop_id,
						-1 );
				
				item->allocated = TRUE;
			}

			loop = loop->next;
		}

		ULOG_DEBUG( "read_menu_conf: DONE!" );
	}

	return;
}


GtkTreeModel *get_menu_contents(void)
{
	GtkTreeStore *contents;
	GList *desktop_files = NULL;

	GtkTreeIter content_iter;

	gchar *user_home_dir;
	gchar *user_menu_conf_file;

	desktop_files = get_desktop_files(g_strdup(DEFAULT_APPS_DIR), desktop_files);

	contents = gtk_tree_store_new(
			TREE_MODEL_COLUMNS,
			G_TYPE_STRING,      /* Name */
			GDK_TYPE_PIXBUF,   /* Icon */
			G_TYPE_STRING,	   /* Exec */
			G_TYPE_STRING,	   /* Service  */
			G_TYPE_STRING	   /* Desktop ID */
			);
		
    gtk_tree_store_append(contents, &content_iter, NULL);

	/* Get $HOME */
	user_home_dir = getenv( "HOME" );

	/* Build the user menu filename */
	user_menu_conf_file = g_build_filename( user_home_dir, USER_MENU_FILE, NULL );
	
	/* If the user has her own menu file.. */
	if ( g_file_test( user_menu_conf_file, G_FILE_TEST_EXISTS ) ) {
		ULOG_DEBUG( "get_menu_contents: using user menu file = '%s'\n",
				user_menu_conf_file );

		/* .. read it */
		read_menu_conf(user_menu_conf_file, contents,
				NULL, NULL, &content_iter, desktop_files);
	} else {
		/* Use the system-wide menu file */
		read_menu_conf(SYSTEMWIDE_MENU_FILE, contents,
				NULL, NULL, &content_iter, desktop_files);
	}

	/* Cleanup */
	g_free( user_menu_conf_file );
	g_list_foreach( desktop_files, (GFunc) destroy_desktop_item, NULL );
	g_list_free( desktop_files );

	return GTK_TREE_MODEL(contents);
} /* static GtkTreeModel *get_menu_contents() */


/* NOTE!
 *
 * This function is NOT recursive and only supports depth specified
 * in the TN UI spec. Also, separators are not supported in submenus.
 * 
 * This works just fine for our current needs, but is teh sux. */
gboolean set_menu_contents( GtkTreeModel *model )
{
	g_assert( GTK_IS_TREE_MODEL( model ) );

	gboolean return_value = FALSE;

	gboolean include_open = FALSE;	

	GtkTreeIter root_iter;
	GtkTreeIter iter;
	GtkTreeIter child_iter;

	gchar *root_name = NULL;
	gchar *name = NULL;
	gchar *desktop_id = NULL;

	gchar *child_name = NULL;
	gchar *child_desktop_id = NULL;


	gtk_tree_model_get_iter_first( model, &root_iter );

	if ( !gtk_tree_store_iter_is_valid(
				GTK_TREE_STORE( model ), &root_iter ) ) {
		ULOG_ERR( "set_menu_contents: failed to get root iterator." );
		return FALSE;

	} else if ( !gtk_tree_model_iter_children( model, &iter, &root_iter ) ) {
		ULOG_ERR( "set_menu_contents: the root node has no children." );
		return FALSE;
	}

	/* Initialize libxml */
	LIBXML_TEST_VERSION

	gchar *user_home_dir = getenv( "HOME" );
	gchar *user_menu_conf_file = g_build_filename(
			user_home_dir, USER_MENU_FILE, NULL );

	/* Make sure we have the directory for user's menu file */
	gchar *user_menu_dir = g_path_get_dirname( user_menu_conf_file );
	
	if ( g_mkdir_with_parents( user_menu_dir, 0755  ) < 0 ) {
		ULOG_ERR( "set_menu_contents: "
				"failed to create directory '%s'.",
				user_menu_dir );

		g_free( user_menu_dir );
		return FALSE;
	}
			
	xmlTextWriterPtr writer;
	
	/* Always write to the user-specific file */
	writer = xmlNewTextWriterFilename( user_menu_conf_file, 0);

	if (writer == NULL) {
		ULOG_ERR( "set_menu_contents: failed to create xml writer." );
		return FALSE;
	}

	/* Start the document */
	int ret = xmlTextWriterStartDocument(writer, NULL, "UTF-8", NULL);

	if ( ret < 0 ) {
		ULOG_ERR( "set_menu_contents: failed to start the document writing." );
		return FALSE;
	}

	/* Start the top -level menu */
	ret = xmlTextWriterStartElement(writer, BAD_CAST "Menu");

	if ( ret < 0 ) {
		ULOG_ERR( "set_menu_contents: failed to start top level menu." );
		return FALSE;
	}

	/* Get the name */
	gtk_tree_model_get( model, &root_iter, 
			TREE_MODEL_NAME,
			&root_name,
			-1 );

	/* The name is not really needed for top-level, but let's add it anyway */
	ret = xmlTextWriterWriteElement( writer, "Name", root_name );


	
	/* Loop! */
	do {
		gtk_tree_model_get( model, &iter, 
				TREE_MODEL_NAME,
				&name,
				TREE_MODEL_DESKTOP_ID,
				&desktop_id,
				-1 );

	
		if ( desktop_id && strcmp( desktop_id, SEPARATOR_STRING ) == 0 ) {
			/* </Include> */
			if ( include_open == TRUE ) {
				ret = xmlTextWriterEndElement( writer );

				include_open = FALSE;
			}
			
			/* <Separator/> */
			ret = xmlTextWriterWriteElement( writer, "Separator", NULL );
			ret = xmlTextWriterEndElement( writer );
			
		} else if ( !desktop_id || strlen( desktop_id ) == 0 ) {
			/* </Include> */
			if ( include_open == TRUE ) {
				ret = xmlTextWriterEndElement(writer );
				include_open = FALSE;
			}
	
			/* <Menu> */
			ret = xmlTextWriterStartElement( writer, BAD_CAST "Menu" );
			
			/* <Name> ... </Name> */
			ret = xmlTextWriterWriteElement( writer, "Name", name );
			 
			/* .. and has children.. */
			if ( gtk_tree_model_iter_children( model,
						&child_iter, &iter ) ) {
				
				/* <Include> */
				ret = xmlTextWriterStartElement(writer,
						BAD_CAST "Include");
				
				/* ..loop them! */
				do {
					gtk_tree_model_get( model, &child_iter, 
							TREE_MODEL_NAME,
							&child_name,
							TREE_MODEL_DESKTOP_ID,
							&child_desktop_id,
							-1 );

					if ( child_desktop_id &&
							strlen( child_desktop_id ) > 0 ) {
						
						/* <Filename> ... </Filename> */
						ret = xmlTextWriterWriteElement( writer,
								"Filename",
								child_desktop_id );
					}
					
					if ( child_name ) {
						g_free( child_name );
					}

					if ( child_desktop_id ) {
						g_free( child_desktop_id );
					}

				} while ( gtk_tree_model_iter_next(
							model, &child_iter ) );

				/* </Include> */
				ret = xmlTextWriterEndElement( writer );
			}

			/* </Menu> */
			ret = xmlTextWriterEndElement( writer );

		} else {
			if ( include_open == FALSE ) {
				ret = xmlTextWriterStartElement(writer,
						BAD_CAST "Include");

				include_open = TRUE;
			}

			/* <Filename> ... </Filename> */
			ret = xmlTextWriterWriteElement( writer,
					"Filename", desktop_id );
		}

		if ( name ) {
			g_free( name );
		}

		if ( desktop_id ) {
			g_free( desktop_id );
		}
	
	} while ( gtk_tree_model_iter_next( model, &iter ) );
	

	/* </Menu> */
	ret = xmlTextWriterEndElement( writer );

	/* End the document */
	ret = xmlTextWriterEndDocument( writer );

	if ( ret < 0 ) {
		ULOG_ERR( "set_menu_contents: failed to write menu conf." );
	} else {
		ULOG_DEBUG( "set_menu_contents: wrote '%s'", user_menu_conf_file );
		return_value = TRUE;
	}

	/* Cleanup */
	xmlFreeTextWriter( writer );
	xmlCleanupParser();
	
	if ( user_home_dir ) {
		g_free( user_home_dir );
	}

	if ( user_menu_conf_file ) {
		g_free( user_menu_conf_file );
	}

	if ( user_menu_dir ) {
		g_free( user_menu_dir );
	}
	
	if ( root_name ) {
		g_free( root_name );
	}

	/*
	if ( name ) {
		g_free( name );
	}

	if ( desktop_id ) {
		g_free( desktop_id );
	}

	if ( child_name ) {
		g_free( child_name );
	}

	if ( child_desktop_id ) {
		g_free( child_desktop_id );
	}
	*/

	return return_value;
}

