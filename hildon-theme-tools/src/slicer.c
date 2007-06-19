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

#include "slicer.h"

/* Check if image actually uses alpha by scanning the pixels */
gboolean                        check_if_pixbuf_needs_alpha (GdkPixbuf *pixbuf)
{
        guchar *pixels = gdk_pixbuf_get_pixels (pixbuf);
        int bytes_per_pixel = gdk_pixbuf_get_n_channels (pixbuf);
        int width = gdk_pixbuf_get_width (pixbuf);
        int height = gdk_pixbuf_get_height (pixbuf);
        long rowstride = gdk_pixbuf_get_rowstride (pixbuf);
        int x = 0;
        int y = 0;

        g_return_val_if_fail (pixbuf != NULL, FALSE);
        g_return_val_if_fail (bytes_per_pixel == 4, FALSE);

        for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                        if (pixels [(y * rowstride) + (x * bytes_per_pixel) + 3] != 255)
                                return TRUE;
                }
        }

        return FALSE;
}

/* Create a pixbuf copy with just the alpha channel */
GdkPixbuf*                      extract_alpha_from_pixbuf (GdkPixbuf *pixbuf)
{
        GdkPixbuf *new = NULL;
        guchar *new_pixels = NULL;
        guchar *pixels = gdk_pixbuf_get_pixels (pixbuf);
        int bytes_per_pixel = gdk_pixbuf_get_n_channels (pixbuf);
        int width = gdk_pixbuf_get_width (pixbuf);
        int height = gdk_pixbuf_get_height (pixbuf);
        long rowstride = gdk_pixbuf_get_rowstride (pixbuf);
        int x = 0;
        int y = 0;

        g_return_val_if_fail (width > 0, NULL);
        g_return_val_if_fail (height > 0, NULL);
        g_return_val_if_fail (bytes_per_pixel == 4, NULL);
        g_return_val_if_fail (rowstride > 0, NULL);

        new_pixels = g_malloc (width * height * 4);
        g_return_val_if_fail (new_pixels != NULL, NULL);

        for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                        new_pixels [(y * width * 4) + (x * 4)] = pixels [(y * rowstride) + (x * bytes_per_pixel) + 3];
                        new_pixels [(y * width * 4) + (x * 4) + 1] = pixels [(y * rowstride) + (x * bytes_per_pixel) + 3];
                        new_pixels [(y * width * 4) + (x * 4) + 2] = pixels [(y * rowstride) + (x * bytes_per_pixel) + 3];
                        new_pixels [(y * width * 4) + (x * 4) + 3] = pixels [(y * rowstride) + (x * bytes_per_pixel) + 3];
                }
        }

        new = gdk_pixbuf_new_from_data (new_pixels, 
                                        GDK_COLORSPACE_RGB, 
                                        TRUE, 
                                        8, 
                                        width, 
                                        height,
                                        width * 4, 
                                        g_free,
                                        (gpointer) new_pixels);

        return new;
}

/* Create a copy of the pixbuf with alpha removed */
GdkPixbuf*                      strip_alpha_from_pixbuf (GdkPixbuf *pixbuf)
{
        GdkPixbuf *new = NULL;
        guchar *new_pixels = NULL;
        guchar *pixels = gdk_pixbuf_get_pixels (pixbuf);
        int bytes_per_pixel = gdk_pixbuf_get_n_channels (pixbuf);
        int width = gdk_pixbuf_get_width (pixbuf);
        int height = gdk_pixbuf_get_height (pixbuf);
        long rowstride = gdk_pixbuf_get_rowstride (pixbuf);
        int x = 0;
        int y = 0;
        int new_x = 0;
        int new_y = 0;

        g_return_val_if_fail (width > 0, NULL);
        g_return_val_if_fail (height > 0, NULL);
        g_return_val_if_fail (bytes_per_pixel == 4, NULL);
        g_return_val_if_fail (rowstride > 0, NULL);

        new_pixels = g_malloc (width * height * 3);
        g_return_val_if_fail (new_pixels != NULL, NULL);

        for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                        new_pixels [(new_y * width * 3) + (new_x * 3)] = pixels [(y * rowstride) + (x * bytes_per_pixel)];
                        new_pixels [(new_y * width * 3) + (new_x * 3) + 1] = pixels [(y * rowstride) + (x * bytes_per_pixel) + 1];
                        new_pixels [(new_y * width * 3) + (new_x * 3) + 2] = pixels [(y * rowstride) + (x * bytes_per_pixel) + 2];
                        new_x++;
                }
                new_y++;
                new_x = 0;
        }

        new = gdk_pixbuf_new_from_data (new_pixels, 
                                        GDK_COLORSPACE_RGB, 
                                        FALSE, 
                                        8, 
                                        width, 
                                        height,
                                        width * 3, 
                                        g_free,
                                        (gpointer) new_pixels);

        return new;
}

/* A helper function to save a png file */
void                            save_png (GdkPixbuf *pixbuf, gchar *filename)
{
        gdk_pixbuf_save (pixbuf, filename, "png", NULL, NULL);
}

/* A helper function to save a jpeg image */
void                            save_jpeg (GdkPixbuf *pixbuf, gchar *filename)
{
        gdk_pixbuf_save (pixbuf, filename, "jpeg", NULL, "quality", JPEG_QUALITY, NULL);
}

/* Do the work */
void                            process (Template *templ, GdkPixbuf *pixbuf, gchar *directory)
{
        GSList *iterator = NULL;
        g_return_if_fail (templ != NULL);
        g_return_if_fail (pixbuf != NULL);

        if (directory == NULL)
                directory = "./";

        for (iterator = templ->ElementList; iterator; iterator = g_slist_next (iterator)) {
                Element *element = (Element *) iterator->data;

                if (element->X > templ->Width || 
                    element->Y > templ->Height ||
                    element->X + element->Width > templ->Width ||
                    element->Y + element->Height > templ->Height) {
                        g_printerr ("WARNING: Element '%s' is out of bounds (%d %d %d %d)!\n", element->Name,  
                                    element->X, element->Y, element->Width, element->Height);
                } else {

                        /* Create the sub-pixbuf representing the extracted bit */
                        GdkPixbuf *sub = gdk_pixbuf_new_subpixbuf (pixbuf, 
                                                                   element->X, element->Y, 
                                                                   element->Width, element->Height);

                        if (sub == NULL)
                                g_printerr ("WARNING: Failed to process '%s'!\n", element->Name);
                        else {
                                gchar *fname = g_build_filename (directory, element->Name, NULL);
                             
                                // FIXME This only covers one case (not stripping alpha when 
                                // forced alpha == TRUE). We should also support a case when 
                                // alpha needs to be added.
                                if ((gdk_pixbuf_get_n_channels (sub) == 4 && 
                                    check_if_pixbuf_needs_alpha (sub) == FALSE &&
                                    element->ForcedAlpha == FALSE) || 
                                    element->NoAlpha == TRUE) {
                                        GdkPixbuf *oldy = sub;
                                        sub = strip_alpha_from_pixbuf (oldy);
                                        gdk_pixbuf_unref (oldy);
                                }
  
                                if ((strlen (fname) >= 4 && strcmp (fname + (strlen (fname) - 4), ".jpg") == 0) ||
                                    (strlen (fname) >= 5 && strcmp (fname + (strlen (fname) - 5), ".jpeg") == 0))
                                        save_jpeg (sub, fname);
                                else
                                        save_png (sub, fname);

                                g_free (fname);

                                gdk_pixbuf_unref (sub);
                                g_print ("Processed %s\n", element->Name);
                        }
                }
        }
}

/* Shows the initial copyright/info banner */
void                            show_banner (void)
{
        g_print ("Slicer tool by Michael Dominic K. and Tuomas Kuosmanen\n");
        g_print ("Copyright 2006 by Nokia Corporation.\n\n");
}

/* Show some info about basic usage of the tool */
void                            show_usage (void)
{
        g_print ("Usage: %s <layout> <image> [outputdir]\n\n", g_get_prgname ());
        g_print ("This tool will slice the input image into individual graphic pieces,\n"
                  "as specified by the template. Optionally you can specify an output directory.\n"
                  "If the directory is not present, it'll be created.\n\n");
        g_print ("The tool can cave in PNG (.png) and JPEG (.jpg, .jpeg) formats.\n\n");
}

/* This is the place where we came in... */
int                             main (int argc, char **argv)
{
        gchar *template_file = NULL;
        gchar *directory = NULL;
        gchar *image_file = NULL;
        int return_val = 0;
        Template *template = NULL;
        GdkPixbuf *image = NULL;

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
        image_file = argv [2];

        if (template_file == NULL || image_file == NULL) {
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

        /* Check the image file... */
        if (! g_file_test (image_file, G_FILE_TEST_EXISTS)) {
                g_printerr ("ERROR: %s not found!\n", image_file);
                goto Error;
        }

        /* Check the optional directory argument */
        if (argc >= 4 && argv [3] != NULL) {
                directory = argv [3];
                if (! g_file_test (directory, G_FILE_TEST_IS_DIR | G_FILE_TEST_EXISTS)) {
                        g_print ("Creating directory %s\n", directory);
                        if (g_mkdir_with_parents (directory, 0755) != 0) {
                                g_printerr ("ERROR: Failed to create directory!\n");
                                goto Error;
                        }
                }
        }

        /* Read the template file. That spits out errors too */
        template = read_template (template_file);

        if (template == NULL)
                goto Error;

        /* Basic info */
        show_template (template);

        /* Try loading the actual image */
        image = gdk_pixbuf_new_from_file (image_file, NULL);
        if (image == NULL) { 
                g_printerr ("ERROR: Failed to load image file!\n");
                goto Error;
        }

        process (template, image, directory);

        g_print ("Done!\n");

        goto Done;

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
