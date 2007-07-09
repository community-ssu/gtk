/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005, 2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

/*
 * This is a VERY incomplete and even somewhat incorrect implementation of a
 * reader/parser for fdo's menu-spec compliant menus. Quite Maemo specific.
 *
 */

#include "libhildonmenu.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

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
        gchar **categories;
	gchar *text_domain;
	gboolean allocated;
} desktop_entry_t;

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
    g_free(de->text_domain);
    g_free(de->desktop_id);
    if ( de->categories )
        g_strfreev(de->categories);

    g_free(de);
}

static GdkPixbuf *add_emblem_to_icon( GdkPixbuf *icon, GdkPixbuf *emblem )
{
	if ( icon == NULL ) {
		g_warning ( "add_emblem_to_icon: icon is NULL." );
		return NULL;
	} else if ( emblem == NULL ) {
		g_warning ( "add_emblem_to_icon: emblem is NULL." );
		return NULL;
	}

	GdkPixbuf *result = gdk_pixbuf_copy( icon );

	if ( result == NULL ) {
		g_warning( "add_emblem_to_icon: failed to copy icon.");
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
			g_warning("Error loading icon '%s': %s\n", icon_name,
					error->message);
			g_error_free(error);
			error = NULL;
		}
	} else {
		g_warning("Error loading icon: no icon name\n");
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
            g_object_ref(fallback);
            return fallback;
        }
    
		pixbuf = gtk_icon_theme_load_icon(icon_theme,
				icon_name,
				icon_size,
				GTK_ICON_LOOKUP_NO_SVG, &error);

		if (error) {
			g_warning("Error loading icon '%s': %s\n", icon_name,
					error->message);
			g_error_free(error);
			error = NULL;
		}
	} else {
		g_warning("Error loading icon: no icon name\n");
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
		g_warning("Error reading file '%s'\n", full_path );
		return NULL;
	}

	g_debug( "get_desktop_files: reading dir '%s'", full_path );

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
			g_warning("%s: %s\n",
					current_path, strerror(errno));

		} else if (S_ISDIR(buf.st_mode)) {
			/* Recurse */
			g_debug( "get_desktop_files: recursing into '%s'",
					current_path );

			desktop_files = get_desktop_files(g_strdup(current_path),
					desktop_files);

		} else if (S_ISREG(buf.st_mode) &&
				g_str_has_suffix(current_path,
					DESKTOP_FILE_SUFFIX)) {

            gchar *type = NULL;

			key_file = g_key_file_new();

			/* Let's read the interesting stuff */
			if ( g_key_file_load_from_file( key_file, current_path,
					G_KEY_FILE_NONE, &error ) == FALSE ) {

				g_warning("Error reading '%s': %s\n",
						current_path, error->message);
				
				g_error_free(error);
				error = NULL;
				
	                	g_free(current_path);
				
				continue;
			}
						

			type = g_key_file_get_string(
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

                        item->categories = g_key_file_get_string_list(
					key_file,
					DESKTOP_ENTRY_GROUP,
					DESKTOP_ENTRY_CATEGORIES_FIELD,
                                        NULL,
					NULL);


            if (item->service && !strchr(item->service, '.')) {
                /* Unqualified domain, add the default com.nokia. */
                gchar * s = g_strconcat (SERVICE_PREFIX, item->service, NULL);
                g_free(item->service);
                item->service = s;
            }
			
            item->text_domain = g_key_file_get_string(
					key_file,
					DESKTOP_ENTRY_GROUP,
					DESKTOP_ENTRY_TEXT_DOMAIN_FIELD,
					NULL);

			item->desktop_id = g_strdup( dir_entry->d_name );

			item->allocated = FALSE;


			g_debug( "get_desktop_files: appending .desktop = %s",
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

static gint find_by_category(desktop_entry_t *a, const gchar *category)
{
    gint i = 0;

    if ( !a->categories || !category ) return 1;

    if ( a->allocated ) return 1;

    while ( a->categories[i] )
    {
        if ( strcmp (category, a->categories[i]) == 0 )
            return 0;

        i++;
    }

    return 1;
}

static GSList *get_desktop_entries_from_category(GList *desktop_files,
                                          const gchar *category)
{
    GSList *result = NULL;
    GList *l = NULL;

    while ( (  l = g_list_find_custom(desktop_files,
                                      category,
                                      (GCompareFunc)find_by_category) ) ) {
      result = g_slist_append(result, l->data);

      ((desktop_entry_t *)(l->data))->allocated = TRUE;
    }

    return result;
}

static void add_desktop_entry(desktop_entry_t * item,
                              GtkTreeStore *menu_tree,
                              GtkTreeIter *iterator)
{
    GdkPixbuf *app_icon = NULL;
    GdkPixbuf *thumb_icon = NULL;
    GtkTreeIter child_iter;

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

    if ( !thumb_icon )
      {
        /* .. or use the default */
        thumb_icon = get_icon( ICON_DEFAULT_APP,
                               ICON_THUMB_SIZE );
      }

    gtk_tree_store_set(menu_tree,
                       &child_iter,
                       TREE_MODEL_NAME,
                       item->name,
                       TREE_MODEL_LOCALIZED_NAME,
                       (item->text_domain && *item->text_domain)?
                         g_strdup (dgettext (item->text_domain, item->name)):
                         g_strdup (dgettext (GETTEXT_PACKAGE,   item->name)),
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
                       TREE_MODEL_TEXT_DOMAIN,
                       item->text_domain,
                       TREE_MODEL_COMMENT,
                       item->comment,
                       -1);

    if ( app_icon ){
      g_object_unref( G_OBJECT( app_icon ) );
      app_icon = NULL;
    }

    if ( thumb_icon ){
      g_object_unref( G_OBJECT( thumb_icon ) );
      thumb_icon = NULL;
    }

    /* Mark the item allocated */
    item->allocated = TRUE;

}

static void read_menu_conf(const char *filename, GtkTreeStore *menu_tree,
		xmlDocPtr doc, xmlNodePtr root_element, GtkTreeIter *iterator,
		GList *desktop_files)
{
	gboolean first_level = FALSE;
	
	static GdkPixbuf *folder_icon            = NULL;
	static GdkPixbuf *folder_thumb_icon      = NULL;
	static GdkPixbuf *folder_open_icon       = NULL;
	static GdkPixbuf *folder_closed_icon     = NULL;
	
	static GdkPixbuf *emblem_expander_open   = NULL;
	static GdkPixbuf *emblem_expander_closed = NULL;

        static GtkTreeIter extras_iter;
        GList *l;

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

	if (doc == NULL) {
		doc = xmlReadFile(filename, NULL, 0);

		if (doc == NULL) {
			g_warning( "read_menu_conf: unable to read '%s'", filename );
			goto check_unallocated;
		}
 	        first_level = TRUE;
	}

	if (root_element == NULL) {
		root_element = xmlDocGetRootElement(doc);

		if (root_element == NULL) {
			g_debug( "read_menu_conf: failed to get root element." );
			xmlFreeDoc(doc);
			goto check_unallocated;
		}
	}

	if (xmlStrcmp(root_element->name, (const xmlChar *) "Menu") != 0) {
		/* Not a menu */
		g_debug( "read_menu_conf: not a menu.");
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

		if (strcmp((const char *) current_element->name, "Menu") == 0) {
			/* Submenu */
			gtk_tree_store_append(menu_tree, &child_iterator, iterator);

			read_menu_conf(filename,
                                       menu_tree,
                                       doc,
                                       current_element,
                                       &child_iterator,
                                       desktop_files);
		} else if (strcmp((const char *) current_element->name, "Name") == 0) {

                  key = xmlNodeListGetString(doc,
                                             current_element->xmlChildrenNode,
                                             1);


                  if (iterator &&
                      gtk_tree_store_iter_is_valid (menu_tree, iterator))
			gtk_tree_store_set(menu_tree, iterator,
					TREE_MODEL_NAME,
					key,
                                        TREE_MODEL_LOCALIZED_NAME,
                                        g_strdup (dgettext (GETTEXT_PACKAGE,
                                                            key)),
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

			if ( strcmp((const char *) key, EXTRAS_MENU_STRING ) == 0 ) {
				extras_iter = *iterator;
			}

			xmlFree(key);

		} else if (strcmp((const char *) current_element->name, "MergeFile") == 0) {

			/* FIXME: skip this for now */
			continue;

			key = xmlNodeListGetString(doc,
					current_element->xmlChildrenNode, 1);

			/* Relative path. prepend current dir */
			if (g_str_has_prefix((const char *) key, "/") == FALSE) {
				key = (xmlChar *) g_build_path(G_DIR_SEPARATOR_S,
						g_path_get_dirname(filename), key, NULL);
			}

			/* Recursion */
			read_menu_conf((const char *) key, menu_tree, NULL, NULL, iterator, desktop_files);

			xmlFree(key);

		} else if (strcmp((const char *) current_element->name, "Include") == 0) {

			xmlNodePtr child_element = NULL;

			for (child_element = current_element->xmlChildrenNode;
					child_element != NULL;
					child_element = child_element->next) {

				if (strcmp((const char *) child_element->name, "Filename") == 0) {

					/* Get the children */
					key = xmlNodeListGetString(doc,
							child_element->xmlChildrenNode,
							1);

					if ( !key || strlen((const char *) key) == 0) {
						xmlFree( key );
						continue;
					}

					/* Get the contents */
					item = get_desktop_item(desktop_files, (const char *) key);

					if ( !item ) {
						xmlFree( key );
						continue;
					}

                                        add_desktop_entry(item, menu_tree, iterator);
                                        xmlFree(key);
                                }
                                else if (strcmp((char *)child_element->name, "And") == 0) {
                                  xmlNodePtr cat_element = NULL;
                                  GSList *items, *i;
                                  for (cat_element = child_element->xmlChildrenNode;
                                       cat_element != NULL;
                                       cat_element = cat_element->next) {

                                    if (strcmp((char *)cat_element->name, "Category") != 0)
                                      continue;

                                    if (!cat_element->xmlChildrenNode)
                                      continue;

                                    key = xmlNodeListGetString(doc,
                                                               cat_element->xmlChildrenNode,
                                                               1);

                                    items = get_desktop_entries_from_category (desktop_files,
                                                                               (gchar*)key);

                                    for (i = items ; i ; i = g_slist_next(i))
                                      {
                                        add_desktop_entry((desktop_entry_t *)i->data,
                                                          menu_tree,
                                                          iterator);
                                      }

                                    xmlFree(key);
                                  }
                                }
                        }

 







		} else if (strcmp((const char *) current_element->name, "Separator") == 0) {
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
check_unallocated:

        if ( first_level )
          {
            /* The doc handle was created in this call */
            xmlFreeDoc(doc);


            /* Create the extras menu if it does not exist */
            if ( !gtk_tree_store_iter_is_valid(menu_tree, &extras_iter ) ) {

                gtk_tree_store_append( menu_tree,
                                       &extras_iter,
                                       NULL );

                gtk_tree_store_set(menu_tree,
                                   &extras_iter,
                                   TREE_MODEL_NAME,
                                   EXTRAS_MENU_STRING,
                                   TREE_MODEL_LOCALIZED_NAME,
                                   dgettext (GETTEXT_PACKAGE,
                                             EXTRAS_MENU_STRING),
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
                                   -1 );
            }

            l = desktop_files;
            while ( l ) {
                desktop_entry_t *item;

                item = l->data;

                if ( item->allocated == FALSE ) {
                        add_desktop_entry( item, menu_tree, &extras_iter );
                }

                l = l->next;
            }
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
	return;
}


GtkTreeModel *get_menu_contents(void)
{
	GtkTreeStore *contents;
	GList *desktop_files = NULL;
	gchar *user_home_dir;
	gchar *user_menu_conf_file;

	desktop_files = get_desktop_files(g_strdup(DEFAULT_APPS_DIR), desktop_files);

	contents = gtk_tree_store_new(
			TREE_MODEL_COLUMNS,
			G_TYPE_STRING,     /* Name */
			G_TYPE_STRING,     /* Localized name */
			GDK_TYPE_PIXBUF,   /* Icon */
			GDK_TYPE_PIXBUF,   /* Thumb Icon */
			GDK_TYPE_PIXBUF,   /* Icon & expander open emblem */
			GDK_TYPE_PIXBUF,   /* Icon & expander closed emblem */
			G_TYPE_STRING,	   /* Exec */
			G_TYPE_STRING,	   /* Service  */
			G_TYPE_STRING,	   /* Desktop ID */
			G_TYPE_STRING,     /* Comment */
			G_TYPE_STRING	   /* Text domain */
			);

	/* Get $HOME */
	user_home_dir = getenv( "HOME" );

	/* Build the user menu filename */
	user_menu_conf_file = g_build_filename( user_home_dir, USER_MENU_DIR, MENU_FILE, NULL);
	
	/* If the user has her own menu file.. */
	if ( g_file_test( user_menu_conf_file, G_FILE_TEST_EXISTS ) ) {
		g_debug( "get_menu_contents: using user menu file = '%s'\n",
				user_menu_conf_file );

		/* .. read it */
		read_menu_conf(user_menu_conf_file, contents,
				NULL, NULL, NULL, desktop_files);
	} else {
		/* Use the system-wide menu file */
		read_menu_conf(HILDON_DESKTOP_MENU_DIR "/" MENU_FILE, contents,
				NULL, NULL, NULL, desktop_files);
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
		g_warning(  "write_menu_conf: invalid iterator." );
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
					g_warning( "write_menu_conf: "
					          "failed to end Include element." );
					goto cleanup_and_exit;
				} else {
					g_debug( "write_menu_conf: </Include>" );
				}
				include_open = FALSE;
			}
	
			/* <Menu> */
			if ( xmlTextWriterStartElement( writer,
						BAD_CAST "Menu" ) < 0 ) {
				g_warning( "write_menu_conf: "
				          "failed to start Menu element." );
				goto cleanup_and_exit;
			} else {
				g_debug( "write_menu_conf: <Menu>" );
			}

			/* <Name> ... </Name> */
			if ( xmlTextWriterWriteElement( writer, (const xmlChar *) "Name", (const xmlChar *) name ) < 0 ) {
				g_warning( "write_menu_conf: "
				          "failed to write Name element." );
				goto cleanup_and_exit;
			} else {
				g_debug( "write_menu_conf: <Name>%s</Name>", name );
			}

			GtkTreeIter child_iterator;

			/* Has children.. */
			if ( gtk_tree_model_iter_children( GTK_TREE_MODEL( menu_tree ),
						&child_iterator, iterator ) ) {

				/* Recurse */
				if ( !write_menu_conf( writer, menu_tree,
							&child_iterator ) ) {
					g_warning( "write_menu_conf: failed to recurse." );
					goto cleanup_and_exit;
				}
			}

			/* </Menu> */
			if ( xmlTextWriterEndElement( writer ) < 0 ) {
				g_warning ( "write_menu_conf: "
				           "failed to end Menu element." );
				goto cleanup_and_exit;
			} else {
				g_debug( "write_menu_conf: </Menu>" );
			}

		} else if ( desktop_id && strcmp( desktop_id,
					SEPARATOR_STRING ) == 0 ) {
			/* </Include> */
			if ( include_open == TRUE ) {
				if ( xmlTextWriterEndElement( writer ) < 0 ) {
					g_warning( "write_menu_conf: "
					          "failed to end Include element." );
					goto cleanup_and_exit;
				} else {
					g_debug( "write_menu_conf: </Include>" );
				}
				include_open = FALSE;
			}

			/* <Separator/> */

			/* This returns -1 for some reason. But this seems to work 
			 * and without this it doesn't work. Oh well.. */
			xmlTextWriterWriteElement( writer, (const xmlChar *) "Separator", NULL );
			xmlTextWriterEndElement( writer );

			g_debug( "write_menu_conf: <Separator/>" );

		} else if ( desktop_id && strlen ( desktop_id ) > 0 ) {
			/* <Include> */
			if ( include_open == FALSE ) {
				if ( xmlTextWriterStartElement( writer,
						BAD_CAST "Include") < 0 ) {
					g_warning( "write_menu_conf: "
					          "failed to start Include element." );
					goto cleanup_and_exit;
				} else {
					g_debug( "write_menu_conf: <Include>" );
				}

				include_open = TRUE;
			}

			/* <Filename> ... </Filename> */
			if ( xmlTextWriterWriteElement( writer,
					(const xmlChar *) "Filename", (const xmlChar *) desktop_id ) < 0 ) {
				g_warning( "write_menu_conf: "
				          "failed to write Filename element." );
				goto cleanup_and_exit;
			} else {
				g_debug( "write_menu_conf: "
				            "<Filename>%s</Filename>", desktop_id );
			}

		} else {
			g_warning( "write_menu_conf: 'what happen?'"
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
			g_warning( "write_menu_conf: "
			          "failed to end Include element." );
			goto cleanup_and_exit;
		} else {
			g_debug( "write_menu_conf: </Include>" );
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
		g_debug( "find_first_and_last_root_level_folders: "
				"first_folder and last_folder are both "
				"NULL. Doing nothing." );
		return;
	}

	GtkTreeIter top_iter;
	GtkTreeIter child_iter;
	GtkTreeIter last_iter;

	if ( !gtk_tree_model_get_iter_first( model, &top_iter ) ) {
		g_warning( "find_first_and_last_root_level_folders: model is empty." );
		return;
	}

	if ( !gtk_tree_model_iter_children( model, &child_iter, &top_iter ) ) {
		g_warning( "find_first_and_last_root_level_folders: tree is empty." );
		return;
	}


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
        gchar *desktop_id = NULL;
      
		gtk_tree_model_get( model, &child_iter,
				TREE_MODEL_DESKTOP_ID,
				&desktop_id,
				-1 );

		/* If it's a folder.. */
		if ( !desktop_id || strlen( desktop_id ) == 0 ) {
			/* Get the first folder path */
			if ( first_folder != NULL && *first_folder == NULL ) {
				*first_folder = gtk_tree_model_get_path(
						model, &child_iter );
			}
           
            last_iter = child_iter;
		}

        g_free( desktop_id );
	} while ( gtk_tree_model_iter_next( model, &child_iter ) );

    *last_folder = gtk_tree_model_get_path( model, &last_iter );
}


gboolean set_separators( GtkTreeModel *model )
{
	g_assert( GTK_IS_TREE_MODEL( model ) );

	GtkTreeIter top_iter;
	GtkTreeIter child_iter;

	if ( !gtk_tree_model_get_iter_first( model, &top_iter ) ) {
		g_warning( "set_separators: model is empty." );
		return FALSE;
	}

	if ( !gtk_tree_model_iter_children( model, &child_iter, &top_iter ) ) {
		g_warning( "set_separators: tree is empty." );
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
				g_debug( "set_separators: found first separator." );
			} else if ( last_separator == NULL ) {
				last_separator = gtk_tree_row_reference_new(
						model, path );
				g_debug( "set_separators: found last separator." );
			} else {
				g_warning( "set_separators: more than two separators." );
			}
		} else {
			if ( !desktop_id || strlen( desktop_id ) == 0 ) {
				g_debug( "set_separators: "
						"item is a folder '%s'", name );

				/* Empty Extras -folder is ignored */
				if ( name && strcmp( name,
							EXTRAS_MENU_STRING ) == 0 ) {
					if ( !gtk_tree_model_iter_has_child(
								model, &child_iter ) ) {
						empty_extras_folder =
							gtk_tree_row_reference_new(
								model, path );
						g_debug( "set_separators: "
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
				g_debug( "set_separators: "
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
		g_debug( "set_separators: first item is a folder, no top separator." );
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

		g_debug( "set_separators: first item is an app, top separator added." );
	} else {
		g_debug( "set_separators: menu is empty?" );
	}

	/* Add the nottom separator if needed */
	if ( last_item == 2 ) {
		g_debug( "set_separators: last item is a folder, no bottom separator." );
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

		g_debug( "set_separators: last item is an app, bottom separator added." );
	} else {
		g_debug( "set_separators: menu is empty?" );
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
        GtkTreeIter iter;

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
		g_warning( "set_menu_contents: failed to create xml buffer." );
		return FALSE;
	}

	writer = xmlNewTextWriterMemory( buffer, 0 );

	if ( writer == NULL ) {
		g_warning( "set_menu_contents: failed to create xml writer." );
		xmlBufferFree( buffer );
		xmlCleanupParser();
		return FALSE;
	}

	if ( xmlTextWriterStartDocument( writer, NULL, "UTF-8", NULL ) < 0 ) {
		g_warning( "set_menu_contents: failed to start the document." );
		goto cleanup_and_exit;

	}

        gtk_tree_model_get_iter_first (model, &iter);
        if ( xmlTextWriterStartElement( writer,
                                        BAD_CAST "Menu" ) < 0 ) {
               g_warning ( "set_menu_contents: failed to write root element");
               goto cleanup_and_exit;
        }

	if ( write_menu_conf( writer, GTK_TREE_STORE( model ), &iter ) == FALSE ) {
		g_warning( "set_menu_contents: failed to write menu conf." );
		goto cleanup_and_exit;
	}
        if ( xmlTextWriterEndElement( writer ) < 0 ) {
               g_warning ( "set_menu_contents: failed to write root element");
               goto cleanup_and_exit;
        }

	/* End the document */
	if ( xmlTextWriterEndDocument( writer ) ) {
		g_warning( "et_menu_contents: failed to end the document." );
		goto cleanup_and_exit;
	} else {
		xmlFreeTextWriter( writer );
		writer = NULL;
	}

	user_home_dir = getenv( "HOME" );
	user_menu_conf_file = g_build_filename(
			user_home_dir, USER_MENU_DIR, MENU_FILE, NULL );

	/* Make sure we have the directory for user's menu file */
	user_menu_dir = g_path_get_dirname( user_menu_conf_file );
	
	if ( g_mkdir_with_parents( user_menu_dir, 0755  ) < 0 ) {
		g_warning( "set_menu_contents: "
				"failed to create directory '%s'.",
				user_menu_dir );
		goto cleanup_and_exit;
	}
	
	/* Always write to the user specific file */
	fp = fopen(user_menu_conf_file, "w");

	if (fp == NULL) {
		g_warning( "set_menu_contents: failed to open '%s' for writing.",
				user_menu_conf_file );
		goto cleanup_and_exit;
	} else {
		g_debug( "set_menu_contents: writing to '%s'.", user_menu_conf_file );
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

	if ( user_menu_conf_file ) {
		g_free( user_menu_conf_file );
	}

	if ( user_menu_dir ) {
		g_free( user_menu_dir );
	}
	
	return return_value;
}

