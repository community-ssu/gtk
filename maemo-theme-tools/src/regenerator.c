/* 
 * GPL license, Copyright (c) 2006 by Nokia Corporation                       
 *                                                                            
 * Authors:                                                                   
 *      Michael Dominic Kostrzewa <michael.kostrzewa@nokia.com>               
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

#include "regenerator.h"

/* Do the work */
GdkPixbuf*                      process (Template *templ, gchar *directory)
{
        GSList *iterator = NULL;
        GdkPixbuf *composite_image = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, templ->Width, templ->Height);
        
        g_return_val_if_fail (templ != NULL, NULL);

        if (composite_image == NULL)
                g_error ("Failed to allocate memory for new image (%d x %d)!", templ->Width, templ->Height);

        /* Reset the pixbuf to transparent white */
        gdk_pixbuf_fill (composite_image, 0xffffff00);

        if (directory == NULL)
                directory = "./";

        for (iterator = templ->ElementList; iterator; iterator = g_slist_next (iterator)) {
                Element *element = (Element *) iterator->data;
                gchar *fname = (element->Name != NULL) ? 
                                g_build_filename (directory, element->Name, NULL) : 
                                NULL;
                GdkPixbuf *image = NULL;

                if (element->X > templ->Width || 
                    element->Y > templ->Height ||
                    element->X + element->Width > templ->Width ||
                    element->Y + element->Height > templ->Height) {
                        g_warning ("Element '%s' is out of bounds (%d %d %d %d)!", element->Name,  
                                   element->X, element->Y, element->Width, element->Height);
                } else if (! g_file_test (fname, G_FILE_TEST_EXISTS)) {
                        g_warning ("Element file name '%s' not found, using PINK!", element->Name);
                        image = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, element->Width, element->Height);
                        gdk_pixbuf_fill (image, 0xff00ffff);
                } else { 
                        image = gdk_pixbuf_new_from_file (fname, NULL);
                        if (image == NULL) 
                                g_warning ("Failed to load element image '%s', ignoring!", 
                                           element->Name);
                }

                if (image != NULL) {
                        /* Draw the stuff over the composite image */
                        gdk_pixbuf_copy_area (image, 0, 0, element->Width, element->Height, 
                                              composite_image, element->X, element->Y);
                        g_print ("PROCESSED %s\n", element->Name);
                        gdk_pixbuf_unref (image);
                }

                if (fname != NULL)
                        g_free (fname);

        }

        return composite_image;
}

/* Shows the initial copyright/info banner */
void                            show_banner (void)
{
        g_print ("Regenerator tool by Michael Dominic K. and Tuomas Kuosmanen\n");
        g_print ("Copyright 2006 by Nokia Corporation.\n\n");
}

/* Show some info about basic usage of the tool */
void                            show_usage (void)
{
        g_print ("Usage: regenerate <template> <outputimage> [inputdir]\n\n");
        g_print ("This tool will combine individual images (ie. previously sliced) into one huge\n"
                 "image template as specified by the layout template. Optionally you can specify an\n"
                 "input directory. The tool can load in PNG and JPEG formats.\n\n");
}

/* This is the place where we came in... */
int                             main (int argc, char **argv)
{
        gchar *template_file = NULL;
        gchar *directory = NULL;
        gchar *output_image_file = NULL;
        int return_val = 0;
        Template *template = NULL;
        GdkPixbuf *output_image = NULL;

	g_type_init ();

        show_banner ();

        /* Check the args... */
        if (argc < 3) {
                show_usage ();
                goto Error;
        }

        /* Get file vals */
        template_file = argv [1];
        output_image_file = argv [2];

        if (template_file == NULL || output_image_file == NULL) {
                show_usage ();
                goto Error;
        }

        /* Check the template file... */
        if (! g_file_test (template_file, G_FILE_TEST_EXISTS)) 
                g_error ("%s not found!", template_file);

        /* Check the optional directory argument */
        if (argc >= 4 && argv [3] != NULL) 
                directory = argv [3];

        /* Read the template file. That spits out errors too */
        template = read_template (template_file);

        if (template == NULL)
                goto Error;

        /* Basic info */
        show_template (template);

        output_image = process (template, directory);
        g_printf ("\n");

        /* Save the resulting image */
        if (output_image != NULL) {
                g_print ("Saving a '%s' image, this might take a while...\n", output_image_file);
                gdk_pixbuf_save (output_image, output_image_file, "png", NULL, NULL);
        }
        
        g_print ("Done!\n");

        goto Done;

Error:
        return_val = 128;
        
Done:
        if (template != NULL)
                free_template (template);

        if (output_image != NULL)
                gdk_pixbuf_unref (output_image);

        g_print ("\n");
        return return_val;
}
