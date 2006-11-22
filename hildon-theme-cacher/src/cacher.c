/* 
 * GPL license, Copyright (c) 2006 by Nokia Corporation                       
 *                                                                            
 * Authors:                                                                   
 *      Tommi Komulainen <tommi.komulainen@nokia.com>
 *      Michael Dominic K. <michael.kostrzewa@nokia.com>
 *                                                                            
 * This program is free software; you can redistribute it and/or modify it    
 * under the terms of the GNU General Public License as published by the      
 * Free Software Foundation, version 2.                                                                   
 *                                                                            
 * This program is distributed in the hope that it will be useful, but        
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   
 * for more details.                                                          
 *                                                                            
 * You should have received a copy of the GNU General Public License along    
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * Temple Place - Suite 330, Boston, MA 02111-1307, USA.                      
 *
 */

#include <gtk/gtk.h>
#include "cacher.h"

/* Parses (resursively) the directory and adds gtkrc matches.
 * We assume everything that HAS *gtkrc* and DOESN'T HAVE *cache*
 * is a valid gtkrc cache. */
GSList*                         parse_dir (const gchar *dir_name, GSList *list, int level)
{
        const gchar *file_name = NULL;
        GDir *dir = NULL;

        g_return_val_if_fail (dir_name != NULL, list);

        /* This is a poor-man's protection so that we don't 
         * go into infinite loop due to broken symlinking */
        if (level > SANITY_LEVEL)
                return list;
        
        dir = g_dir_open (dir_name, 0, NULL);
        if (dir == NULL) {
                g_warning ("Failed to read '%s' directory!", dir_name);
                return list;
        }

        while ((file_name = g_dir_read_name (dir)) != NULL) {
                gchar *path = g_build_filename (dir_name, file_name, NULL);

                if (g_file_test (path, G_FILE_TEST_IS_REGULAR) &&
                    g_strrstr (file_name, "gtkrc") != NULL     &&
                    g_strrstr (file_name, "cache") == NULL)
                        list = g_slist_append (list, path);
                else if (g_file_test (path, G_FILE_TEST_IS_DIR)) 
                        list = parse_dir (path, list, level + 1);
        }

        return list;
}

/* Shows the initial copyright/info banner */
void                            show_banner (void)
{
        g_print ("Theme cache tool by Tommi Komulainen and Michael Dominic Kostrzewa\n");
        g_print ("Copyright 2006 by Nokia Corporation.\n\n");
}

/* Show some info about basic usage of the tool */
void                            show_usage (void)
{
        g_print ("Usage: %s <theme-directory>\n\n", g_get_prgname ());
        g_print ("This tool will pre-generate the cache (.cache) files for gtkrc files.\n"
                 "It will scan recursively the specified directory and generate the cache for ALL\n"
                 "the gtk rc files found.\n\n");
}

int                             main (int argc, char **argv)
{
        GtkSettings *settings;
        GSList *rc_list = NULL;
        GSList *iterator = NULL;

        show_banner ();
        gtk_init_check (&argc, &argv);

        if (argc < 2) {
                show_usage ();
                return 1;
        }

        g_setenv ("GSCANNERCACHE_CREATE", "1", TRUE);

        /* Get the list of all gtkrc files */
        rc_list = parse_dir (argv [1], NULL, 1);

        /* Iterate through all themes and generate cache */
        iterator = rc_list;
        while (iterator != NULL) {
                g_print ("Generating %s.cache\n", (char *) iterator->data);
                gtk_rc_parse ((char *) iterator->data);
                iterator = g_slist_next (iterator);
        }

        settings = g_object_new (GTK_TYPE_SETTINGS, NULL);
        gtk_rc_reparse_all_for_settings (settings, TRUE);

        /* Free list */
        iterator = rc_list;
        while (iterator != NULL) {
                if (iterator->data != NULL)
                        g_free (iterator->data);
                iterator = g_slist_next (iterator);
        }
        g_slist_free (rc_list);

        g_print ("\nDone!\n");

        return 0;
}
