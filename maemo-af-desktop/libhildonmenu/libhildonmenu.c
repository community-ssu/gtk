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
#define SERVICE_PREFIX "com.nokia."

typedef struct {
	gchar *name;
	gchar *comment;
	gchar *icon;
	gchar *exec;
	gchar *service;
	gchar *desktop_id;
	gboolean allocated;
} desktop_entry_t;

/* Iterator to extras */
static GtkTreeIter *extras_iter = NULL;

/* Function for composing an icon with the given emblem */
static GdkPixbuf *add_emblem_to_icon( GdkPixbuf *icon, GdkPixbuf *emblem );


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

/* Function for creating the XML document to a buffer */
gboolean write_menu_conf( xmlTextWriterPtr writer,
		GtkTreeStore *menu_tree, GtkTreeIter *iterator );

static void destroy_desktop_item(gpointer data, gpointer null)
{
    desktop_entry_t * de = (desktop_entry_t *)data;

    g_free(de->name);
    g_free(de->comment);
    g_free(de->icon);
    g_free(de->exec);
    g_free(de->service);
    g_free(de->desktop_id);

    g_free(de);
}

static GdkPixbuf *add_emblem_to_icon( GdkPixbuf *icon, GdkPixbuf *emblem )
{
	if ( icon == NULL ) {
		ULOG_ERR( "add_emblem_to_icon: icon is NULL." );
		return NULL;
	} else if ( emblem == NULL ) {
		ULOG_ERR( "add_emblem_to_icon: emblem is NULL." );
		return NULL;
	}

	GdkPixbuf *result = gdk_pixbuf_copy( icon );

	if ( result == NULL ) {
		ULOG_ERR( "add_emblem_to_icon: failed to copy icon.");
		return NULL;
	}

	gdk_pixbuf_composite(emblem, result, 0, 0,
			MIN(gdk_pixbuf_get_width(emblem), gdk_pixbuf_get_width(result)),
			MIN(gdk_pixbuf_get_height(emblem), gdk_pixbuf_get_height(result)),
			0, 0, 1, 1, GDK_INTERP_NEAREST, 255);

	return result;
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

/* If we can't find a matching icon size, use the one provided as parameter
 * This is to avoid having two pixbufs in memory when not needed.
 */
GdkPixbuf *get_icon_with_fallback(const char *icon_name,
                                  int icon_size,
                                  GdkPixbuf *fallback)
{
    gint *icon_sizes;
    gint i;
    gboolean found;
	GtkIconTheme *icon_theme;
	GdkPixbuf *pixbuf = NULL;
	GError *error = NULL;


	if (icon_name) {
		icon_theme = gtk_icon_theme_get_default();

        icon_sizes = gtk_icon_theme_get_icon_sizes(icon_theme, icon_name);
        
        i = 0;
        found = FALSE;
        while (icon_sizes[i] != 0)
        {
            /* If found or has scalable icon (-1), use it
             * Well, in theory at least. Maemo-Gtk+ doesn't return -1 at all
             * it seems (scalable is a synonyme for "64" really)
             */ 
            if (icon_sizes[i] == icon_size || icon_sizes[i] == -1)
            {
                found = TRUE;
                break;
            }
            i++;
        }

        g_free (icon_sizes);

        if (!found)
        {
            return fallback;
        }
    
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
} /* static GdkPixbuf *get_icon_with_fallback() */

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

			item->name = g_key_file_get_string(
					key_file,
					DESKTOP_ENTRY_GROUP,
					DESKTOP_ENTRY_NAME_FIELD,
					NULL);

                        /* We don't allow empty names */
                        if ( item->name == NULL) {
                            g_free (item);
                            g_key_file_free(key_file);
                            g_free(current_path);
                            continue;
                        }

 			item->comment = g_key_file_get_locale_string(
 					key_file,
 					DESKTOP_ENTRY_GROUP,
 					DESKTOP_ENTRY_COMMENT_FIELD,
 					NULL,
 					NULL);

			item->icon = g_key_file_get_string(
					key_file,
					DESKTOP_ENTRY_GROUP,
					DESKTOP_ENTRY_ICON_FIELD,
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

            if (item->service && !strchr(item->service, '.')) {
                /* Unqualified domain, add the default com.nokia. */
                gchar * s = g_strconcat (SERVICE_PREFIX, item->service, NULL);
                g_free(item->service);
                item->service = s;
            }

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
	gboolean doc_created = FALSE;

	GdkPixbuf *app_icon                      = NULL;
	GdkPixbuf *thumb_icon                    = NULL;

	GdkPixbuf *favourite_icon                = NULL;
	GdkPixbuf *favourite_open_icon           = NULL;
	GdkPixbuf *favourite_closed_icon         = NULL;
	
	static GdkPixbuf *folder_icon            = NULL;
	static GdkPixbuf *folder_thumb_icon      = NULL;
	static GdkPixbuf *folder_open_icon       = NULL;
	static GdkPixbuf *folder_closed_icon     = NULL;
	
	static GdkPixbuf *emblem_expander_open   = NULL;
	static GdkPixbuf *emblem_expander_closed = NULL;

	/* Get emblems */
	if ( !emblem_expander_open ) {
		emblem_expander_open = get_icon( EMBLEM_EXPANDER_OPEN, ICON_SIZE );
	}

	if ( !emblem_expander_closed ) {
		emblem_expander_closed = get_icon( EMBLEM_EXPANDER_CLOSED, ICON_SIZE );
	}

	/* Get the folder icons */
	if ( !folder_icon ) {
		folder_icon = get_icon(ICON_FOLDER, ICON_SIZE);
		folder_thumb_icon = get_icon_with_fallback(ICON_FOLDER,
							ICON_THUMB_SIZE,
							folder_icon);
	}

	if ( !folder_open_icon ) {
		folder_open_icon = add_emblem_to_icon(
				folder_icon, emblem_expander_open );
	}

	if ( !folder_closed_icon ) {
		folder_closed_icon = add_emblem_to_icon(
				folder_icon, emblem_expander_closed );
	}

	/* Make sure we have a valid iterator */
	if (!gtk_tree_store_iter_is_valid(menu_tree, iterator)) {
		gtk_tree_store_append(menu_tree, iterator, NULL);
	}

	if (gtk_tree_store_iter_is_valid(menu_tree, iterator)) {
		level = gtk_tree_store_iter_depth(menu_tree, iterator);
	}

	/* Always add the "Favourites" */
	if (level == 0) {
		favourite_icon = get_icon(ICON_FAVOURITES, ICON_SIZE);
		favourite_open_icon = add_emblem_to_icon(
				favourite_icon, emblem_expander_open );
		favourite_closed_icon = add_emblem_to_icon(
				favourite_icon, emblem_expander_closed );
		
		gtk_tree_store_set(menu_tree,
				iterator,
				TREE_MODEL_NAME,
				FAVOURITES_NAME,
				TREE_MODEL_ICON,
				favourite_icon,
				TREE_MODEL_EMBLEM_EXPANDER_OPEN,
				favourite_open_icon,
				TREE_MODEL_EMBLEM_EXPANDER_CLOSED,
				favourite_closed_icon,
				TREE_MODEL_EXEC,
				"",
				TREE_MODEL_SERVICE,
				"",
				TREE_MODEL_DESKTOP_ID,
				"",
                TREE_MODEL_COMMENT,
                "",
				-1);
		
		if ( favourite_icon ) {
			g_object_unref( G_OBJECT( favourite_icon ) );
			favourite_icon = NULL;
		}
		
		if ( favourite_open_icon ) {
			g_object_unref( G_OBJECT( favourite_open_icon ) );
			favourite_open_icon = NULL;
		}
		
		if ( favourite_closed_icon ) {
			g_object_unref( G_OBJECT( favourite_closed_icon ) );
			favourite_closed_icon = NULL;
		}

	}

	if (doc == NULL) {
		doc = xmlReadFile(filename, NULL, 0);

		if (doc == NULL) {
			ULOG_ERR( "read_menu_conf: unable to read '%s'", filename );
			goto check_unallocated;
		}
        	doc_created = TRUE;
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
        xmlFreeDoc(doc);
		goto check_unallocated;
	}


	xmlNodePtr current_element = NULL;
	xmlChar *key;
		
	GtkTreeIter child_iterator;
	desktop_entry_t *item;

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
					key,
					TREE_MODEL_ICON,
					folder_icon,
					TREE_MODEL_THUMB_ICON,
					folder_thumb_icon,
					TREE_MODEL_EMBLEM_EXPANDER_OPEN,
					folder_open_icon,
					TREE_MODEL_EMBLEM_EXPANDER_CLOSED,
					folder_closed_icon,
					TREE_MODEL_EXEC,
					"",
					TREE_MODEL_SERVICE,
					"",
					TREE_MODEL_DESKTOP_ID,
					"",
					TREE_MODEL_COMMENT,
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
					app_icon = get_icon( item->icon, ICON_SIZE );
					
					if ( !app_icon ) {
						/* .. or use the default */
						app_icon = get_icon( ICON_DEFAULT_APP,
								ICON_SIZE );
					}

					/* Check if we have an thumb sized icon.. */
					thumb_icon = get_icon_with_fallback( item->icon,
					                                     ICON_THUMB_SIZE,
					                                     app_icon );
					
					gtk_tree_store_set(menu_tree,
							&child_iter,
							TREE_MODEL_NAME,
							item->name,
							TREE_MODEL_ICON,
							app_icon,
							TREE_MODEL_THUMB_ICON,
							thumb_icon,
							TREE_MODEL_EMBLEM_EXPANDER_OPEN,
							NULL,
							TREE_MODEL_EMBLEM_EXPANDER_CLOSED,
							NULL,
							TREE_MODEL_EXEC,
							item->exec,
							TREE_MODEL_SERVICE,
							item->service,
							TREE_MODEL_DESKTOP_ID,
							item->desktop_id,
							TREE_MODEL_COMMENT,
							item->comment,
							-1);

					if ( app_icon ){
						g_object_unref( G_OBJECT( app_icon ) );
						app_icon = NULL;
					}

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
					TREE_MODEL_EMBLEM_EXPANDER_OPEN,
					NULL,
					TREE_MODEL_EMBLEM_EXPANDER_CLOSED,
					NULL,
					TREE_MODEL_THUMB_ICON,
					NULL,
					TREE_MODEL_EXEC,
					SEPARATOR_STRING,
					TREE_MODEL_SERVICE,
					SEPARATOR_STRING,
					TREE_MODEL_DESKTOP_ID,
					SEPARATOR_STRING,
					TREE_MODEL_COMMENT,
					SEPARATOR_STRING,
					-1);
		}

	}
			

	if ( doc_created )
	{
		/* The doc handle was created in this call */
		xmlFreeDoc(doc);
	}

	check_unallocated:

	if ( level == 0 ) {

		GList *loop = g_list_first( desktop_files );
		desktop_entry_t *item;

		/* Create the extras menu if it does not exist */
		if ( !extras_iter || !gtk_tree_store_iter_is_valid(
					menu_tree, extras_iter ) ) {

			ULOG_DEBUG( "Menu '%s' does not exist.",
					EXTRAS_MENU_STRING );

			GtkTreeIter iter;

			gtk_tree_model_get_iter_first(
					GTK_TREE_MODEL( menu_tree ), &iter );

			extras_iter = g_malloc0( sizeof( GtkTreeIter ) );

			GtkTreePath *first_folder;
			GtkTreePath *last_folder;

			find_first_and_last_root_level_folders(
					GTK_TREE_MODEL( menu_tree ),
					&first_folder, &last_folder );

			GtkTreeIter sibling;

			if ( last_folder ) {	
				gtk_tree_model_get_iter(
						GTK_TREE_MODEL( menu_tree ),
						&sibling, last_folder );

				gtk_tree_store_insert_after(
						menu_tree, extras_iter,
						NULL, &sibling );

			} else if ( first_folder ) {
				gtk_tree_model_get_iter(
						GTK_TREE_MODEL( menu_tree ),
						&sibling, first_folder );

				gtk_tree_store_insert_after(
						menu_tree, extras_iter,
						NULL, &sibling );

			} else {
				gtk_tree_store_append( menu_tree,
						extras_iter, &iter );
			}


			if ( !gtk_tree_store_iter_is_valid(
						menu_tree, extras_iter ) ) {
				ULOG_ERR( "read_menu_conf: "
						"failed to create "
						"'%s' -menu.",
						EXTRAS_MENU_STRING );
				return;
			}

			gtk_tree_store_set(menu_tree,
					extras_iter,
					TREE_MODEL_NAME,
					EXTRAS_MENU_STRING,
					TREE_MODEL_ICON,
					folder_icon,
					TREE_MODEL_EMBLEM_EXPANDER_OPEN,
					folder_open_icon,
					TREE_MODEL_EMBLEM_EXPANDER_CLOSED,
					folder_closed_icon,
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


		while ( loop ) {

			item = loop->data;
			
			if ( item->allocated == FALSE ) {

				ULOG_DEBUG( "read_menu_conf: "
						"unallocated item: '%s'",
						item->desktop_id );

				GtkTreeIter item_iter;

				gtk_tree_store_append( menu_tree,
						&item_iter, extras_iter );

				app_icon = get_icon( item->icon, ICON_SIZE );

				gtk_tree_store_set(menu_tree,
						&item_iter,
						TREE_MODEL_NAME,
						item->name,
						TREE_MODEL_ICON,
						app_icon,
						TREE_MODEL_EMBLEM_EXPANDER_OPEN,
						NULL,
						TREE_MODEL_EMBLEM_EXPANDER_CLOSED,
						NULL,
						TREE_MODEL_EXEC,
						item->exec,
						TREE_MODEL_SERVICE,
						item->service,
						TREE_MODEL_DESKTOP_ID,
						item->desktop_id,
						-1 );

				if ( app_icon ) {
					g_object_unref( G_OBJECT ( app_icon ) );
					app_icon = NULL;
				}

				item->allocated = TRUE;
			}

			loop = loop->next;
		}

		/* We don't need our reference to the folder icon pixbuf anymore */
		if ( folder_icon ) {
			g_object_unref( G_OBJECT( folder_icon ) );
			folder_icon = NULL;
		}

		if ( folder_open_icon ) {
			g_object_unref( G_OBJECT( folder_open_icon ) );
			folder_open_icon = NULL;
		}

		if ( folder_closed_icon ) {
			g_object_unref( G_OBJECT( folder_closed_icon ) );
			folder_closed_icon = NULL;
		}

        g_free( extras_iter );
        extras_iter = NULL;

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
			GDK_TYPE_PIXBUF,   /* Thumb Icon */
			GDK_TYPE_PIXBUF,   /* Icon & expander open emblem */
			GDK_TYPE_PIXBUF,   /* Icon & expander closed emblem */
			G_TYPE_STRING,	   /* Exec */
			G_TYPE_STRING,	   /* Service  */
			G_TYPE_STRING,	   /* Desktop ID */
			G_TYPE_STRING	   /* Comment */
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


gboolean write_menu_conf( xmlTextWriterPtr writer,
		GtkTreeStore *menu_tree, GtkTreeIter *iterator )
{
	gboolean return_value = FALSE;
	gboolean include_open = FALSE;

	gchar *name = NULL;
	gchar *desktop_id = NULL;


	/* Make sure we have a valid iterator */
	if (!gtk_tree_store_iter_is_valid( menu_tree, iterator) ) {
		ULOG_ERR(  "write_menu_conf: invalid iterator." );
		return FALSE;
	}


	/* Loop through the model */
	do {
		gtk_tree_model_get( GTK_TREE_MODEL( menu_tree ), iterator, 
				TREE_MODEL_NAME,
				&name,
				TREE_MODEL_DESKTOP_ID,
				&desktop_id,
				-1 );

		if ( !desktop_id || strlen( desktop_id ) == 0 ) {
			/* </Include> */
			if ( include_open == TRUE ) {
				if ( xmlTextWriterEndElement( writer ) < 0 ) {
					ULOG_ERR( "write_menu_conf: "
					          "failed to end Include element." );
					goto cleanup_and_exit;
				} else {
					ULOG_DEBUG( "write_menu_conf: </Include>" );
				}
				include_open = FALSE;
			}
	
			/* <Menu> */
			if ( xmlTextWriterStartElement( writer,
						BAD_CAST "Menu" ) < 0 ) {
				ULOG_ERR( "write_menu_conf: "
				          "failed to start Menu element." );
				goto cleanup_and_exit;
			} else {
				ULOG_DEBUG( "write_menu_conf: <Menu>" );
			}

			/* <Name> ... </Name> */
			if ( xmlTextWriterWriteElement( writer, "Name", name ) < 0 ) {
				ULOG_ERR( "write_menu_conf: "
				          "failed to write Name element." );
				goto cleanup_and_exit;
			} else {
				ULOG_DEBUG( "write_menu_conf: <Name>%s</Name>", name );
			}

			GtkTreeIter child_iterator;

			/* Has children.. */
			if ( gtk_tree_model_iter_children( GTK_TREE_MODEL( menu_tree ),
						&child_iterator, iterator ) ) {

				/* Recurse */
				if ( !write_menu_conf( writer, menu_tree,
							&child_iterator ) ) {
					ULOG_ERR( "write_menu_conf: failed to recurse." );
					goto cleanup_and_exit;
				}
			}

			/* </Menu> */
			if ( xmlTextWriterEndElement( writer ) < 0 ) {
				ULOG_ERR ( "write_menu_conf: "
				           "failed to end Menu element." );
				goto cleanup_and_exit;
			} else {
				ULOG_DEBUG( "write_menu_conf: </Menu>" );
			}

		} else if ( desktop_id && strcmp( desktop_id,
					SEPARATOR_STRING ) == 0 ) {
			/* </Include> */
			if ( include_open == TRUE ) {
				if ( xmlTextWriterEndElement( writer ) < 0 ) {
					ULOG_ERR( "write_menu_conf: "
					          "failed to end Include element." );
					goto cleanup_and_exit;
				} else {
					ULOG_DEBUG( "write_menu_conf: </Include>" );
				}
				include_open = FALSE;
			}

			/* <Separator/> */

			/* This returns -1 for some reason. But this seems to work 
			 * and without this it doesn't work. Oh well.. */
			xmlTextWriterWriteElement( writer, "Separator", NULL );
			xmlTextWriterEndElement( writer );

			ULOG_DEBUG( "write_menu_conf: <Separator/>" );

		} else if ( desktop_id && strlen ( desktop_id ) > 0 ) {
			/* <Include> */
			if ( include_open == FALSE ) {
				if ( xmlTextWriterStartElement( writer,
						BAD_CAST "Include") < 0 ) {
					ULOG_ERR( "write_menu_conf: "
					          "failed to start Include element." );
					goto cleanup_and_exit;
				} else {
					ULOG_DEBUG( "write_menu_conf: <Include>" );
				}

				include_open = TRUE;
			}

			/* <Filename> ... </Filename> */
			if ( xmlTextWriterWriteElement( writer,
					"Filename", desktop_id ) < 0 ) {
				ULOG_ERR( "write_menu_conf: "
				          "failed to write Filename element." );
				goto cleanup_and_exit;
			} else {
				ULOG_DEBUG( "write_menu_conf: "
				            "<Filename>%s</Filename>", desktop_id );
			}

		} else {
			ULOG_ERR( "write_menu_conf: 'what happen?'"
			          " - 'somebody set us up the bomb!'" );
			goto cleanup_and_exit;
		}
		
		if ( name ) {
			g_free( name );
			name = NULL;
		}

		if ( desktop_id ) {
			g_free( desktop_id );
			desktop_id = NULL;
		}

	} while ( gtk_tree_model_iter_next( GTK_TREE_MODEL( menu_tree ), iterator ) );

	/* </Include> */
	if ( include_open == TRUE ) {
		if ( xmlTextWriterEndElement( writer ) < 0 ) {
			ULOG_ERR( "write_menu_conf: "
			          "failed to end Include element." );
			goto cleanup_and_exit;
		} else {
			ULOG_DEBUG( "write_menu_conf: </Include>" );
		}
		include_open = FALSE;
	}

	/* Great! Everything went fine! */
	return_value = TRUE;

	cleanup_and_exit:

	if ( name ) {
		g_free( name );
	}

	if ( desktop_id ) {
		g_free( desktop_id );
	}

	return return_value;
}


void find_first_and_last_root_level_folders( GtkTreeModel *model,
		GtkTreePath **first_folder, GtkTreePath **last_folder )
{
	g_assert( GTK_IS_TREE_MODEL( model ) );

	if ( first_folder == NULL && last_folder == NULL ) {
		ULOG_DEBUG( "find_first_and_last_root_level_folders: "
				"first_folder and last_folder are both "
				"NULL. Doing nothing." );
		return;
	}

	GtkTreeIter top_iter;
	GtkTreeIter child_iter;

	if ( !gtk_tree_model_get_iter_first( model, &top_iter ) ) {
		ULOG_ERR( "find_first_and_last_root_level_folders: model is empty." );
		return;
	}

	if ( !gtk_tree_model_iter_children( model, &child_iter, &top_iter ) ) {
		ULOG_ERR( "find_first_and_last_root_level_folders: tree is empty." );
		return;
	}

	gchar *name = NULL;
	gchar *desktop_id = NULL;

	/* "Reset" the paths */
	if ( first_folder != NULL ) {
		*first_folder = NULL;
	}

	if ( last_folder != NULL ) {
		*last_folder = NULL;
	}

	/* Loop through the children of root level and get the
	 * paths of first and last folders.
	 *
	 * NOTE:
	 *
	 * if user gave NULL for **first_folder or **last_folder then
	 * it will be ignored.
	 * 
	 * If there are no folders *first_folder and *last_folder
	 * will be NULL after the loop finishes.
	 *
	 * if there's only one folder *last_folder will be  NULL 
	 * after the loop finishes. */
	do {
		gtk_tree_model_get( model, &child_iter,
				TREE_MODEL_NAME,
				&name,
				TREE_MODEL_DESKTOP_ID,
				&desktop_id,
				-1 );

		/* If it's a folder.. */
		if ( !desktop_id || strlen( desktop_id ) == 0 ) {
			/* Get the first folder path */
			if ( first_folder != NULL && *first_folder == NULL ) {
				*first_folder = gtk_tree_model_get_path(
						model, &child_iter );
			} else if ( last_folder != NULL ) {
				/* Free up the previous path of last folder */
				if ( *last_folder != NULL ) {
					gtk_tree_path_free( *last_folder );
				}
				/* Update the path of last folder */
				*last_folder = gtk_tree_model_get_path(
						model, &child_iter );
			}
		}
	} while ( gtk_tree_model_iter_next( model, &child_iter ) );
}


gboolean set_separators( GtkTreeModel *model )
{
	g_assert( GTK_IS_TREE_MODEL( model ) );

	GtkTreeIter top_iter;
	GtkTreeIter child_iter;

	if ( !gtk_tree_model_get_iter_first( model, &top_iter ) ) {
		ULOG_ERR( "set_separators: model is empty." );
		return FALSE;
	}

	if ( !gtk_tree_model_iter_children( model, &child_iter, &top_iter ) ) {
		ULOG_ERR( "set_separators: tree is empty." );
		return FALSE;
	}

	gchar *name = NULL;
	gchar *desktop_id = NULL;


	/* Value 0 = not set yet
	 * Value 1 = Application
	 * Value 2 = Folder
	 */
	guint first_item = 0;
	guint last_item  = 0;

	GtkTreeRowReference *first_separator = NULL;
	GtkTreeRowReference *last_separator = NULL;

	GtkTreeRowReference *first_folder = NULL;
	GtkTreeRowReference *last_folder = NULL;

	GtkTreeRowReference *empty_extras_folder = NULL;

	GtkTreePath *path;

	/* Loop through the items on depth 1 (where the separators should be)
	 * and get GtkTreeRowReferences for separators and first and last 
	 * folders and also for Extras folder in case it is empty.
	 *
	 * After the loop we will add the new separators as necessary 
	 * and remove the old ones and the empty Extras folder.
	 *
	 * We could also just move the separators, but that would require 
	 * additional checking for the existence of the separators. If a 
	 * separator is needed and does not exist already we would have to 
	 * create it anyway.
	 */
	do {
		path = gtk_tree_model_get_path(
				model, &child_iter );

		gtk_tree_model_get( model, &child_iter,
				TREE_MODEL_NAME,
				&name,
				TREE_MODEL_DESKTOP_ID,
				&desktop_id,
				-1 );

		if ( strcmp( name, SEPARATOR_STRING ) == 0 &&
				strcmp( desktop_id, SEPARATOR_STRING ) == 0 ) {

			if ( first_separator == NULL ) {
				first_separator = gtk_tree_row_reference_new(
						model, path );
				ULOG_DEBUG( "set_separators: found first separator." );
			} else if ( last_separator == NULL ) {
				last_separator = gtk_tree_row_reference_new(
						model, path );
				ULOG_DEBUG( "set_separators: found last separator." );
			} else {
				ULOG_ERR( "set_separators: more than two separators." );
			}
		} else {
			if ( !desktop_id || strlen( desktop_id ) == 0 ) {
				ULOG_DEBUG( "set_separators: "
						"item is a folder '%s'", name );

				/* Empty Extras -folder is ignored */
				if ( name && strcmp( name,
							EXTRAS_MENU_STRING ) == 0 ) {
					if ( !gtk_tree_model_iter_has_child(
								model, &child_iter ) ) {
						empty_extras_folder =
							gtk_tree_row_reference_new(
								model, path );
						ULOG_DEBUG( "set_separators: "
						            "found empty Extras -folder." );
						continue;
					}
				}

				/* It's a folder */
				if ( first_item == 0 ) {
					first_item = 2;
				}

				/* Get the reference */
				if ( first_folder == NULL ) {
					first_folder = gtk_tree_row_reference_new(
							model, path );
				}

				/* So far the last item is a folder */
				last_item = 2;

				/* Update the reference */
				if ( last_folder != NULL ) {
					gtk_tree_row_reference_free( last_folder );
				}

				last_folder = gtk_tree_row_reference_new(
						model, path );
			} else {
				ULOG_DEBUG( "set_separators: "
						"item is an application '%s'", name );

				/* It's an application */
				if ( first_item == 0 ) {
					first_item = 1;
				}

				/* So far the last item is nn application */
				last_item = 1;
			}
		}
		if ( name ) {
			g_free( name );
			name = NULL;
		}

		if ( desktop_id ) {
			g_free( desktop_id );
			desktop_id = NULL;
		}

		gtk_tree_path_free( path );

	} while ( gtk_tree_model_iter_next( model, &child_iter ) );

	/* Add the top separator if needed */
	if ( first_item == 2 ) {
		ULOG_DEBUG( "set_separators: first item is a folder, no top separator." );
	} else if ( first_item == 1 ) {

		/* Get the iterators */
		GtkTreeIter iter;
		GtkTreeIter sibling;

		GtkTreePath *tmp_path = gtk_tree_row_reference_get_path( first_folder );
		gtk_tree_model_get_iter( model, &sibling, tmp_path );
		gtk_tree_path_free( tmp_path );

		/* Add the separator */
		gtk_tree_store_insert_before( GTK_TREE_STORE( model ),
				&iter, NULL, &sibling );

		gtk_tree_store_set( GTK_TREE_STORE( model ), &iter,
				TREE_MODEL_NAME,
				SEPARATOR_STRING,
				TREE_MODEL_ICON,
				NULL,
				TREE_MODEL_EMBLEM_EXPANDER_OPEN,
				NULL,
				TREE_MODEL_EMBLEM_EXPANDER_CLOSED,
				NULL,
				TREE_MODEL_EXEC,
				SEPARATOR_STRING,
				TREE_MODEL_SERVICE,
				SEPARATOR_STRING,
				TREE_MODEL_DESKTOP_ID,
				SEPARATOR_STRING, -1 );

		ULOG_DEBUG( "set_separators: first item is an app, top separator added." );
	} else {
		ULOG_DEBUG( "set_separators: menu is empty?" );
	}

	/* Add the nottom separator if needed */
	if ( last_item == 2 ) {
		ULOG_DEBUG( "set_separators: last item is a folder, no bottom separator." );
	} else if ( last_item == 1 ) {
		/* Get the iterators */
		GtkTreeIter iter;
		GtkTreeIter sibling;

		GtkTreePath *tmp_path = gtk_tree_row_reference_get_path( last_folder );
		gtk_tree_model_get_iter( model, &sibling, tmp_path );
		gtk_tree_path_free( tmp_path );

		/* Add the separator */
		gtk_tree_store_insert_after( GTK_TREE_STORE( model ),
				&iter, NULL, &sibling );

		gtk_tree_store_set( GTK_TREE_STORE( model ), &iter,
				TREE_MODEL_NAME,
				SEPARATOR_STRING,
				TREE_MODEL_ICON,
				NULL,
				TREE_MODEL_EMBLEM_EXPANDER_OPEN,
				NULL,
				TREE_MODEL_EMBLEM_EXPANDER_CLOSED,
				NULL,
				TREE_MODEL_EXEC,
				SEPARATOR_STRING,
				TREE_MODEL_SERVICE,
				SEPARATOR_STRING,
				TREE_MODEL_DESKTOP_ID,
				SEPARATOR_STRING, -1 );

		ULOG_DEBUG( "set_separators: last item is an app, bottom separator added." );
	} else {
		ULOG_DEBUG( "set_separators: menu is empty?" );
	}

	/* Remove the original separators */
	if ( first_separator != NULL ) {
		GtkTreePath *tmp_path =
			gtk_tree_row_reference_get_path( first_separator );
		GtkTreeIter tmp_iter;
		gtk_tree_model_get_iter( model, &tmp_iter, tmp_path );
		gtk_tree_store_remove( GTK_TREE_STORE( model ), &tmp_iter );
		gtk_tree_path_free( tmp_path );
	}

	if ( last_separator != NULL ) {
		GtkTreePath *tmp_path =
			gtk_tree_row_reference_get_path( last_separator );
		GtkTreeIter tmp_iter;
		gtk_tree_model_get_iter( model, &tmp_iter, tmp_path );
		gtk_tree_store_remove( GTK_TREE_STORE( model ), &tmp_iter );
		gtk_tree_path_free( tmp_path );
	}

	/* Remove empty Extras folder */
	if ( empty_extras_folder != NULL ) {
		GtkTreePath *tmp_path =
			gtk_tree_row_reference_get_path( empty_extras_folder );
		GtkTreeIter tmp_iter;
		gtk_tree_model_get_iter( model, &tmp_iter, tmp_path );
		gtk_tree_store_remove( GTK_TREE_STORE( model ), &tmp_iter );
		gtk_tree_path_free( tmp_path );
	}
	
	gtk_tree_row_reference_free( first_separator );
	gtk_tree_row_reference_free( last_separator );
	gtk_tree_row_reference_free( first_folder );
	gtk_tree_row_reference_free( last_folder );
	gtk_tree_row_reference_free( empty_extras_folder );

	return TRUE;
}


gboolean set_menu_contents( GtkTreeModel *model )
{
	g_assert( GTK_IS_TREE_MODEL( model ) );

	gboolean return_value = FALSE;

	xmlBufferPtr buffer;
	xmlTextWriterPtr writer;

	gchar *user_home_dir = NULL;
	gchar *user_menu_conf_file = NULL;
	gchar *user_menu_dir = NULL;

	FILE *fp;

	/* Initialize libxml */
	LIBXML_TEST_VERSION

	/* OK, let's go! */
	buffer = xmlBufferCreate();
	
	if ( buffer == NULL ) {
		ULOG_ERR( "set_menu_contents: failed to create xml buffer." );
		return FALSE;
	}

	writer = xmlNewTextWriterMemory( buffer, 0 );

	if ( writer == NULL ) {
		ULOG_ERR( "set_menu_contents: failed to create xml writer." );
		xmlBufferFree( buffer );
		xmlCleanupParser();
		return FALSE;
	}

	if ( xmlTextWriterStartDocument( writer, NULL, "UTF-8", NULL ) < 0 ) {
		ULOG_ERR( "set_menu_contents: failed to start the document." );
		goto cleanup_and_exit;

	}

	GtkTreeIter iterator;

	gtk_tree_model_get_iter_first( model, &iterator );

	if ( !gtk_tree_store_iter_is_valid(
				GTK_TREE_STORE( model ), &iterator ) ) {
		ULOG_ERR( "set_menu_contents: failed to get iterator." );
		goto cleanup_and_exit;	
	}

	if ( write_menu_conf( writer, GTK_TREE_STORE( model ), &iterator ) == FALSE ) {
		ULOG_ERR( "set_menu_contents: failed to write menu conf." );
		goto cleanup_and_exit;
	}

	/* End the document */
	if ( xmlTextWriterEndDocument( writer ) ) {
		ULOG_ERR( "et_menu_contents: failed to end the document." );
		goto cleanup_and_exit;
	} else {
		xmlFreeTextWriter( writer );
		writer = NULL;
	}

	user_home_dir = getenv( "HOME" );
	user_menu_conf_file = g_build_filename(
			user_home_dir, USER_MENU_FILE, NULL );

	/* Make sure we have the directory for user's menu file */
	user_menu_dir = g_path_get_dirname( user_menu_conf_file );
	
	if ( g_mkdir_with_parents( user_menu_dir, 0755  ) < 0 ) {
		ULOG_ERR( "set_menu_contents: "
				"failed to create directory '%s'.",
				user_menu_dir );
		goto cleanup_and_exit;
	}
	
	/* Always write to the user specific file */
	fp = fopen(user_menu_conf_file, "w");

	if (fp == NULL) {
		ULOG_ERR( "set_menu_contents: failed to open '%s' for writing.",
				user_menu_conf_file );
		goto cleanup_and_exit;
	} else {
		ULOG_DEBUG( "set_menu_contents: writing to '%s'.", user_menu_conf_file );
		return_value = TRUE;
	}

	fprintf(fp, "%s", (const char *) buffer->content);
	fclose(fp);


	cleanup_and_exit:

	if ( writer ) {
		xmlFreeTextWriter( writer );
	}

	xmlBufferFree( buffer );
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
	
	return return_value;
}

