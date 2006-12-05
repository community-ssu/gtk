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

#include "outliner.h"

/* Do the work */
GdkPixbuf*                      process (Template *templ)
{
        GSList *iterator = NULL;
        int counter = 0;
        GdkPixbuf *composite_image = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, templ->Width, templ->Height);
        
        g_return_val_if_fail (templ != NULL, NULL);

        if (composite_image == NULL) {
                g_printerr ("ERROR: Failed to allocate memory for new image (%d x %d)!\n", templ->Width, templ->Height);
                exit (128);
        }

        /* Reset the pixbuf to transparent white */
        gdk_pixbuf_fill (composite_image, 0xffffff00);

        /* Process all the images */
        for (iterator = templ->ElementList; iterator; iterator = g_slist_next (iterator)) {
                Element *element = (Element *) iterator->data;
                GdkPixbuf *image = NULL;

                if (element->X > templ->Width || 
                    element->Y > templ->Height ||
                    element->X + element->Width > templ->Width ||
                    element->Y + element->Height > templ->Height) {
                        g_printerr ("WARNING: Element '%s' is out of bounds (%d %d %d %d)!\n", element->Name,  
                                   element->X, element->Y, element->Width, element->Height);
                } else { 
                        image = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, element->Width, element->Height);

                        if (counter % 5 == 0) 
                                gdk_pixbuf_fill (image, 0x00ff00ff);
                        else if (counter % 5 == 1)
                                gdk_pixbuf_fill (image, 0x00ee00ff);
                        else if (counter % 5 == 2)
                                gdk_pixbuf_fill (image, 0x00dd00ff);
                        else if (counter % 5 == 3)
                                gdk_pixbuf_fill (image, 0x00cc00ff);
                        else
                                gdk_pixbuf_fill (image, 0x00bb00ff);
                }

                if (image != NULL) {
                        /* Draw the stuff over the composite image */
                        gdk_pixbuf_copy_area (image, 0, 0, element->Width, element->Height, 
                                              composite_image, element->X, element->Y);
                        g_print ("Outlined %s\n", element->Name);
                        
                        gdk_pixbuf_unref (image);
                }

                counter++;
        }

        /* Process all the colors */
        for (iterator = templ->ColorList; iterator; iterator = g_slist_next (iterator)) {
        
                Color *color = (Color *) iterator->data;
                GdkPixbuf *image = NULL;

                if (color->X - 4 > templ->Width || 
                    color->Y - 4 > templ->Height ||
                    color->X + 4 > templ->Width ||
                    color->Y + 4 > templ->Height) {
                        g_printerr ("WARNING: Color '%s' is out of bounds (%d %d)!\n", color->Name,  
                                    color->X, color->Y);
                } else { 
                        image = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 8, 8);

                        if (counter % 5 == 0) 
                                gdk_pixbuf_fill (image, 0xffff00ff);
                        else if (counter % 5 == 1)
                                gdk_pixbuf_fill (image, 0xffee00ff);
                        else if (counter % 5 == 2)
                                gdk_pixbuf_fill (image, 0xffdd00ff);
                        else if (counter % 5 == 3)
                                gdk_pixbuf_fill (image, 0xffcc00ff);
                        else
                                gdk_pixbuf_fill (image, 0xffbb00ff);
                }

                if (image != NULL) {
                        /* Draw the stuff over the composite image */
                        gdk_pixbuf_copy_area (image, 0, 0, 8, 8, 
                                              composite_image, color->X - 4, color->Y - 4);
                        g_print ("Outlined color %s\n", color->Name);
                        
                        gdk_pixbuf_unref (image);
                }

                counter++;
        }

        return composite_image;
}

/* Shows the initial copyright/info banner */
void                            show_banner (void)
{
        g_print ("Outliner tool by Michael Dominic K. and Tuomas Kuosmanen\n");
        g_print ("Copyright 2006 by Nokia Corporation.\n\n");
}

/* Show some info about basic usage of the tool */
void                            show_usage (void)
{
        g_print ("Usage: %s <layout> <outputimage>\n\n", g_get_prgname ());
        g_print ("This tool will create an outline image that shows the slicing guides. \n"
                 "You can use this image in your graphics program to check if your drawings\n"
                 "fit the proper areas. \n\n");
}

/* This is the place where we came in... */
int                             main (int argc, char **argv)
{
        gchar *template_file = NULL;
        gchar *output_image_file = NULL;
        int return_val = 0;
        Template *template = NULL;
        GdkPixbuf *output_image = NULL;

	g_type_init ();
        g_set_prgname (g_basename (argv [0]));

        /* Check the args... */
        if (argc < 3) {
                show_banner ();
                show_usage ();
                g_printerr ("Not enough arguments given!\n");
                goto Error;
        }

        /* Get file vals */
        template_file = argv [1];
        output_image_file = argv [2];

        if (template_file == NULL || output_image_file == NULL) {
                show_banner ();
                show_usage ();
                g_printerr ("Bad arguments given!\n");
                goto Error;
        }

        /* Check the template file... */
        if (! g_file_test (template_file, G_FILE_TEST_EXISTS)) {
                g_printerr ("ERROR: %s not found!\n", template_file);
                goto Error;
        }

        /* Read the template file. That spits out errors too */
        template = read_template (template_file);

        if (template == NULL)
                goto Error;

        /* Basic info */
        show_template (template);

        output_image = process (template);
        g_print ("\n");

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
