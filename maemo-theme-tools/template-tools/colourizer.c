/* 
 * GPL license, Copyright (c) 2006 by Nokia Corporation                       
 *                                                                            
 * Authors:                                                                   
 *      Michael Dominic Kostrzewa <michael.kostrzewa@nokia.com>               
 *                                                                            
 * This program is free software; you can redistribute it and/or modify it    
 * under the terms of the GNU General Public License as published by the      
 *  Free Software Foundation; either version 2, or (at your option) any later
 * version.                                                                   
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

#include "colourizer.h"

/* Do the work */
void                            process (Template *templ, GdkPixbuf *pixbuf)
{
        GSList *iterator = NULL;
        
        g_return_if_fail (templ != NULL);
        g_return_if_fail (pixbuf != NULL);

        for (iterator = templ->ColorList; iterator; iterator = g_slist_next (iterator)) {
                Color *color = (Color *) iterator->data;

                if (color->X > templ->Width || 
                    color->Y > templ->Height)
                        g_warning ("Color '%s' is out of bounds!", color->Name);
                else {
                        gchar *hex = get_color (color, pixbuf);
                        if (hex != NULL) {
                                g_print ("$substitutions{qw{@%s@}} = \"%s\";\n", color->Name, hex);
                                g_free (hex);
                        }
                }
        }
}

/* Get a hex-decimal web-like representation of the color at the given pixel */
gchar*                          get_color (Color *color, GdkPixbuf *pixbuf)
{
        guchar red = 0;
        guchar green = 0;
        guchar blue = 0;
        long rowstride = gdk_pixbuf_get_rowstride (pixbuf);
        guchar *pixels = gdk_pixbuf_get_pixels (pixbuf);
        int bytes_per_pixel = gdk_pixbuf_get_n_channels (pixbuf);

        g_return_val_if_fail (color != NULL, NULL);
        g_return_val_if_fail (pixbuf != NULL, NULL);

        red = pixels [(color->Y * rowstride) + (color->X * bytes_per_pixel)];
        green = pixels [(color->Y * rowstride) + (color->X * bytes_per_pixel) + 1];
        blue = pixels [(color->Y * rowstride) + (color->X * bytes_per_pixel) + 2];

        return g_strdup_printf ("#%.2x%.2x%.2x", red, green, blue);
}

/* Shows the initial copyright/info banner */
void                            show_banner (void)
{
        g_print ("Colourizer tool by Michael Dominic K. and Tuomas Kuosmanen\n");
        g_print ("Copyright 2006 by Nokia Corporation.\n\n");
}

/* Show some info about basic usage of the tool */
void                            show_usage (void)
{
        g_print ("Usage: colourizer <template> <image>\n\n");
        g_print ("This tool will output a Perl substitution array. \n"
                  "It should be used to substitute the Gtk Stock colors in various RC files.\n\n");
}

/* This is the place where we came in... */
int                             main (int argc, char **argv)
{
        gchar *template_file = NULL;
        gchar *image_file = NULL;
        int return_val = 0;
        Template *template = NULL;
        GdkPixbuf *image = NULL;
        
        g_type_init ();

        /* Check the args... */
        if (argc != 3) {
                show_banner ();
                show_usage ();
                goto Error;
        }

        /* Get file vals */
        template_file = argv [1];
        image_file = argv [2];

        if (template_file == NULL || image_file == NULL) {
                show_usage ();
                goto Error;
        }

        /* Check the template file... */
        if (! g_file_test (template_file, G_FILE_TEST_EXISTS))
                g_error ("%s not found!", template_file);

        /* Check the image file... */
        if (! g_file_test (image_file, G_FILE_TEST_EXISTS))
                g_error ("%s not found!", image_file);

        /* Read the template file. That spits out errorsi/aborts too */
        template = read_template (template_file);

        /* Try loading the actual image */
        image = gdk_pixbuf_new_from_file (image_file, NULL);
        if (image == NULL) 
                g_error ("Failed to load image file!");

        /* Check the color bits */
        if ((gdk_pixbuf_get_n_channels (image) != 3 &&
            gdk_pixbuf_get_n_channels (image) != 4) || 
            gdk_pixbuf_get_bits_per_sample (image) != 8) 
                g_error ("Only RGB and RGBA images are supported!");

        process (template, image);

Error:  
        return_val = 128;

Done:
        if (template != NULL)
                free_template (template);

        if (image != NULL)
                gdk_pixbuf_unref (image);

        g_print ("\n");
        return return_val;
}


