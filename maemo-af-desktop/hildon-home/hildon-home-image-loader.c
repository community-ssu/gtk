/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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


/**
 * @file hildon-home-image-loader.c
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <libgnomevfs/gnome-vfs.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

/* Gconf for reading MMC cover status */
#include <gconf/gconf-client.h>

#include "hildon-home-image-loader.h"

/*SAW (allocation watchdog facilities)*/
#include <osso-mem.h>

/**
 * @save_original_bg_image_uri
 * 
 * @param uri uri to original image
 * @param uri_save_file file to save uri
 * 
 * Saves original uri to file
 * 
 */
static 
void save_original_bg_image_uri(const gchar *uri, 
                                const gchar *uri_save_file)
{
    FILE *fp;
    
    unlink(uri_save_file);
    fp = fopen (uri_save_file, "w");
    if(fp == NULL) 
    {
        return;
    }
    if(uri != NULL && uri != "") 
    {
        if(fprintf(fp, HILDON_HOME_URISAVE_FILENAME_FORMAT, uri) < 0)
        {
            return;
        }
    }

    if(fp != NULL && fclose(fp) != 0) 
    {
        return;
    }
    return;
}

/*An out-of-memory callback for load_image_from_uri*/
static void 
load_image_oom_cb(size_t current_sz, size_t max_sz,void *context)
{
    /*&oom is passed in context*/
    *(gboolean *)context = TRUE;
}


/**
 * @load_image_from_uri
 *
 * @param pixbuf Storing place pixbuf of loaded image
 * @param uri URI for the image to load
 *
 * @returns loader status
 *
 * Load an image from given URI. Places to pixbuf
*/
static 
gint load_image_from_uri(GdkPixbuf **pixbuf, const gchar *uri)
{     
    GError           *error = NULL;
    GdkPixbufLoader  *loader;
    GnomeVFSHandle   *handle;
    GnomeVFSResult    result;
    guchar            buffer[BUF_SIZE];
    GnomeVFSFileSize  bytes_read;
    GConfClient      *gconf_client; 	 
    gboolean          mmc_in_use = FALSE; 	 
    gboolean          oom = FALSE; /*OOM during pixbuf op*/
    gboolean          mmc_cover_open = FALSE; 	 
    gchar            *mmc_uri_prefix;

    gconf_client = gconf_client_get_default(); 	 
    g_assert(gconf_client); 	 

    mmc_uri_prefix = 
        g_strdup_printf("%s%s", 
                        HILDON_HOME_URI_PREFIX, 
                        g_getenv(HILDON_HOME_ENV_MMC_MOUNTPOINT));

    if (g_str_has_prefix(uri, mmc_uri_prefix))
    { 	 
        mmc_in_use = TRUE; 	 
        mmc_cover_open = 	 
            gconf_client_get_bool(gconf_client, 	 
                                  HILDON_HOME_GCONF_MMC_COVER_OPEN, 	 
                                  &error ); 	 
        if(error != NULL)
        { 	 
            g_error_free( error ); 	 
            g_object_unref(gconf_client);
            return HILDON_HOME_IMAGE_LOADER_ERROR_SYSTEM_RESOURCE;
        } 	 

        if(mmc_cover_open) 	 
        { 	 
            g_object_unref(gconf_client); 	 
            return HILDON_HOME_IMAGE_LOADER_ERROR_MMC_OPEN; 	 
        } 	 
    }
    g_free(mmc_uri_prefix);
    result = gnome_vfs_open(&handle, uri, GNOME_VFS_OPEN_READ);
    /*Setting up a watchdog*/
    if( osso_mem_saw_enable( 3 << 20, 32767, load_image_oom_cb, (void *)&oom) )
    {
         oom = TRUE;
    }
    
    loader = gdk_pixbuf_loader_new();

    while (!oom && (result == GNOME_VFS_OK) && (!mmc_in_use || !mmc_cover_open)) {
        result = gnome_vfs_read(handle, buffer, BUF_SIZE, &bytes_read);
        if(!oom) 
        {
            if(!gdk_pixbuf_loader_write(loader, buffer, bytes_read, NULL))
            {
                return HILDON_HOME_IMAGE_LOADER_ERROR_SYSTEM_RESOURCE; 	 
            }
        }

        if(!oom && mmc_in_use)
        {
            mmc_cover_open =
                gconf_client_get_bool(gconf_client,
                                      HILDON_HOME_GCONF_MMC_COVER_OPEN,
                                      &error );
            if(error != NULL )
            { 	 
                g_error_free( error ); 
                gdk_pixbuf_loader_close(loader, NULL); 	 
                g_object_unref(gconf_client);
                gnome_vfs_close(handle);
                return HILDON_HOME_IMAGE_LOADER_ERROR_SYSTEM_RESOURCE; 	 
            } 	 
            if(mmc_cover_open) 	 
            { 	 
                gdk_pixbuf_loader_close(loader, NULL); 	 
                g_object_unref(gconf_client); 	 
                gnome_vfs_close(handle);
                return HILDON_HOME_IMAGE_LOADER_ERROR_MMC_OPEN; 	 
            } 	 
        }
    }
    g_object_unref(gconf_client); 	 
    
    /*disable watchdog*/
    osso_mem_saw_disable();
    
    if (oom || (result != GNOME_VFS_ERROR_EOF)) 
    {
        gdk_pixbuf_loader_close(loader, NULL);
        gnome_vfs_close(handle);
        return HILDON_HOME_IMAGE_LOADER_ERROR_FILE_CORRUPT;
    }
    
    gdk_pixbuf_loader_close(loader, NULL);
    *pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
    gnome_vfs_close(handle);
    return HILDON_HOME_IMAGE_LOADER_OK;
}

/**
 * @load_image_pixbuf
 *
 * @param pixbuf Storing place pixbuf of loaded image
 * @param path Path from where to load the pixbuf
 *
 * @returns loader status
 *
 * Load the image pixbuff from given file path.
 */
static 
gint load_image_pixbuf(GdkPixbuf **pixbuf, const gchar *path)
{
    GError *error = NULL;

    *pixbuf = gdk_pixbuf_new_from_file(path, &error);

    if(error != NULL)
    {
        g_error_free(error);
        return HILDON_HOME_IMAGE_LOADER_ERROR_FILE_UNREADABLE;
    }

    if (*pixbuf == NULL)
    {
        return HILDON_HOME_IMAGE_LOADER_ERROR_FILE_CORRUPT;        
    }

    return HILDON_HOME_IMAGE_LOADER_OK;
}

/**
 * @save_image
 *
 * @param pixbuf Pixbuf to save
 * @param path Path where to save the file
 *
 * @returns loader status
 *
 * Save the image to given path. The file is first written to temprary file
 * and only after it's successfully saved it's renamed to the given path.
 */
static 
gint save_image(GdkPixbuf *pixbuf, const gchar *path)
{
    GError *error = NULL;
    gchar *temp_path;
    
    if(pixbuf == NULL)
    {
        return HILDON_HOME_IMAGE_LOADER_ERROR_SYSTEM_RESOURCE;        
    }

    temp_path = 
        g_strconcat(path, HILDON_HOME_IMAGE_LOADER_TEMP_EXTENSION, NULL);
    
    if (!gdk_pixbuf_save(pixbuf, temp_path, 
                         HILDON_HOME_IMAGE_FORMAT, &error, NULL)) 
    {
        g_error_free(error);
        unlink(temp_path);
        g_free(temp_path);
        return HILDON_HOME_IMAGE_LOADER_ERROR_SYSTEM_RESOURCE;
    }
    if (rename(temp_path, path) < 0) 
    {
        unlink(temp_path);
        g_free(temp_path);
        return HILDON_HOME_IMAGE_LOADER_ERROR_SYSTEM_RESOURCE;
    }
    g_free(temp_path);
    return HILDON_HOME_IMAGE_LOADER_OK;
}

/**
 * @create_background_pixbuf
 *
 * @param pixbuf Storing place of created pixbuf
 * @param src_pixbuf Source image to use for the background
 * @param width Width of the destination pixbuf to create
 * @param height Height of the destination pixbuf to create
 *
 * @returns loader status
 *
 * Create a background from given source image. If the image is larger than
 * the destination size, it's cropped. If it's smaller, it's tiled to fill space.
 */

static
gint create_background_pixbuf(GdkPixbuf **pixbuf, const GdkPixbuf *src_pixbuf,
                              gint width, gint height)
{
    gint src_width = -1, src_height = -1;
    gint part_width = -1, part_height = -1;
    gint src_x, src_y, dest_x, dest_y;
    GdkPixbuf *part_pixbuf = NULL;
    
    if (src_pixbuf == NULL) 
    {
        return HILDON_HOME_IMAGE_LOADER_ERROR_SYSTEM_RESOURCE;        
    }

    src_width = gdk_pixbuf_get_width(src_pixbuf);
    src_height = gdk_pixbuf_get_height(src_pixbuf);
    
    if (src_width == width && src_height == height) 
    {
        /* source pixbuf is in the exactly wanted size,
           we can use it directly */
        *pixbuf = gdk_pixbuf_copy(src_pixbuf);
        return HILDON_HOME_IMAGE_LOADER_OK; 
    }

    /* if image is larger than wanted, crop from the middle */
    if(src_width > width || src_height > height)
    {
        part_width = MIN(src_width, width);
        part_height = MIN(src_height, height);

        part_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 
                                     part_width, part_height);

        if (part_pixbuf == NULL) 
        {
            return HILDON_HOME_IMAGE_LOADER_ERROR_SYSTEM_RESOURCE;
        }

        src_x = src_width > width ? (src_width - width) / 2 : 0;
        src_y = src_height > height ? (src_height - height) / 2 : 0;

        gdk_pixbuf_copy_area(src_pixbuf, src_x, src_y,
                             part_width,
                             part_height,
                             part_pixbuf, 0, 0);

        
        if (part_width == width && part_height == height) 
        {
            /* cropped pixbuf is in the exactly wanted size,
               we can use it directly */
            *pixbuf = gdk_pixbuf_copy(part_pixbuf);
            if (part_pixbuf != NULL) 
            {
                g_object_unref(part_pixbuf);
            }
            return HILDON_HOME_IMAGE_LOADER_OK; 
        }
    } else {
        part_width = src_width;
        part_height = src_height;
    }

    /* image is smaller than wanted, fill image area by tiling it. */
    *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);
        
    if (*pixbuf == NULL) 
    {
        if (part_pixbuf != NULL) 
        {
            g_object_unref(part_pixbuf);
        }
        return HILDON_HOME_IMAGE_LOADER_ERROR_SYSTEM_RESOURCE;
    }
        
    for(dest_x = 0; dest_x < width; dest_x += part_width)
    {
        for(dest_y = 0; dest_y < height; dest_y += part_height )
        {
            gdk_pixbuf_copy_area(src_pixbuf, 0, 0,
                                 MIN(part_width, width-dest_x),
                                 MIN(part_height, height-dest_y),
                                 *pixbuf, dest_x, dest_y);
        }
    }
    
    if (part_pixbuf != NULL) 
    {
        g_object_unref(part_pixbuf);
    }
    return HILDON_HOME_IMAGE_LOADER_OK;
}

/**
 * @create_original_pixbuf_area
 *
 * @param pixbuf Storing place of created pixbuf
 * @param src_pixbuf Source image to use for the background
 * @param src_x X coordinate from where image is cropped
 * @param src_y Y coordinate from where image is cropped
 * @param width Width of image area to be cropped
 * @param height Height of  image area to be cropped
 *
 * @returns loader status
 *
 * Crops image area of orginal background image from given position and
 * size.
 */
static
gint create_original_pixbuf_area(GdkPixbuf **pixbuf, 
                                 GdkPixbuf *src_pixbuf,
                                 gint src_x, gint src_y, 
                                 gint width, gint height)
{
    if (src_pixbuf == NULL) 
    {
        return HILDON_HOME_IMAGE_LOADER_ERROR_SYSTEM_RESOURCE;
    }

    *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);
    if (*pixbuf == NULL) 
    {
        return HILDON_HOME_IMAGE_LOADER_ERROR_SYSTEM_RESOURCE;
    }

    gdk_pixbuf_copy_area(src_pixbuf, 
                         src_x, 
                         src_y, 
                         width, 
                         height, 
                         *pixbuf, 
                         0, 
                         0);
    return HILDON_HOME_IMAGE_LOADER_OK;
}

/**
 * @blend_new_skin_to_image_area
 *
 * @param pixbuf_orig Storing place of blended image pixbuf
 * @param path_blend Path for the skin image
 * @param path_orig Path for the orginal area of image
 *
 * Read original and skin image from given paths and blend skin image
 * to original. If blend skin is not retrievable, only original is 
 * image is provided.
 */
static 
void blend_new_skin_to_image_area(GdkPixbuf **pixbuf_orig, 
                                  const gchar *path_blend, 
                                  const gchar *path_orig)
{
    GdkPixbuf *pixbuf_blend;
    GdkPixbuf *pixbuf_temp;
    gint width_blend, height_blend;
    gint width_orig, height_orig, loader_status;
    
    loader_status = load_image_pixbuf(pixbuf_orig, path_orig);
    if(loader_status != HILDON_HOME_IMAGE_LOADER_OK) 
    {
        return;
    }

    loader_status = load_image_pixbuf(&pixbuf_blend, path_blend);
    if(loader_status != HILDON_HOME_IMAGE_LOADER_OK) 
    {
        return;
    }

    width_orig = gdk_pixbuf_get_width(*pixbuf_orig);
    height_orig = gdk_pixbuf_get_height(*pixbuf_orig);
    width_blend = gdk_pixbuf_get_width(pixbuf_blend);
    height_blend = gdk_pixbuf_get_height(pixbuf_blend);

    if((width_orig != width_blend || height_orig != height_blend) &&
        width_orig == HILDON_HOME_SIDEBAR_WIDTH_LOADER &&
        height_orig == HILDON_HOME_SIDEBAR_HEIGHT_LOADER)
    {
        pixbuf_temp = gdk_pixbuf_scale_simple(
            pixbuf_blend,
            HILDON_HOME_SIDEBAR_WIDTH_LOADER,
            HILDON_HOME_SIDEBAR_HEIGHT_LOADER,
            GDK_INTERP_NEAREST);                
        g_object_unref(pixbuf_blend);
        pixbuf_blend = gdk_pixbuf_copy(pixbuf_temp);
        g_object_unref(pixbuf_temp);
        width_blend = HILDON_HOME_SIDEBAR_WIDTH_LOADER;
        height_blend = HILDON_HOME_SIDEBAR_HEIGHT_LOADER;
    }

    gdk_pixbuf_composite(pixbuf_blend, *pixbuf_orig,
                         0, 0,
                         width_blend,
                         height_blend,
                         0, 0,
                         1.0, 1.0,
                         GDK_INTERP_BILINEAR,
                         HILDON_HOME_IMAGE_ALPHA_FULL);

    g_object_unref(pixbuf_blend);
    return;
}

/**
 * @blend_in_with_pixbuf
 *
 * @param pixbuf Storing place of blended image pixbuf
 * @param blended_pixbuf Pixbuf of blend area with new blend
 * @param dest_x Destination for X coordinate
 * @param dest_y Destination for Y coordinate
 *
 * Read an image from given path and blend it into destination pixbuf
 * in the given location.
 */
static 
void blend_in_with_pixbuf(GdkPixbuf **pixbuf, GdkPixbuf *blended_pixbuf, 
                          gint dest_x, gint dest_y)
{
    gint blended_height, blended_width;

    if (blended_pixbuf == NULL) 
    {
        return;
    }

    blended_width = gdk_pixbuf_get_width(blended_pixbuf);
    blended_height = gdk_pixbuf_get_height(blended_pixbuf);

    blended_pixbuf = gdk_pixbuf_add_alpha(blended_pixbuf, FALSE,0,0,0);
    *pixbuf = gdk_pixbuf_add_alpha(*pixbuf, FALSE,0,0,0);
        
    gdk_pixbuf_copy_area(blended_pixbuf,
                         0, 0,
                         blended_width,
                         blended_height,
                         *pixbuf,
                         dest_x, dest_y);
}


/**
 * @blend_in_from_path
 *
 * @param pixbuf Storing place of blended image pixbuf
 * @param path Path for the image to read
 * @param save_path Path for the original image area to save
 * @param x Destination for X coordinate
 * @param y Destination for Y coordinate
 *
 * Read an image from given path and blend it into destination pixbuf
 * in the given location.
 */
static 
void blend_in_from_path(GdkPixbuf **pixbuf, 
                        const gchar *path, 
                        const gchar *save_path, 
                        gint x, gint y)
{
    GdkPixbuf *src_pixbuf;
    GdkPixbuf *src_pixbuf_orig = NULL;
    gint width, height;
    gint src_height, src_width;
    gint loader_status;

    if (*pixbuf == NULL) 
    {
        return;
    }

    width = gdk_pixbuf_get_width(*pixbuf);
    height = gdk_pixbuf_get_height(*pixbuf);

    if (x > width || y > height) 
    {
        return;
    }
    
    loader_status = load_image_pixbuf(&src_pixbuf, path);
    if(loader_status != HILDON_HOME_IMAGE_LOADER_OK) 
    {
        return;
    }

    src_width = gdk_pixbuf_get_width(src_pixbuf);
    src_height = gdk_pixbuf_get_height(src_pixbuf);

    loader_status = 
        create_original_pixbuf_area(&src_pixbuf_orig,
                                    *pixbuf,
                                    0, 0,
                                    src_width,
                                    src_height);
    if(loader_status != HILDON_HOME_IMAGE_LOADER_OK) 
    {
        return;
    }

    save_image(src_pixbuf_orig, save_path);

    if (src_pixbuf_orig != NULL)
    {
        g_object_unref(src_pixbuf_orig);
    }

    gdk_pixbuf_composite(src_pixbuf, *pixbuf,
                         x, y,
                         src_width,
                         src_height,
                         0, 0,
                         1.0, 1.0,
                         GDK_INTERP_BILINEAR,
                         HILDON_HOME_IMAGE_ALPHA_FULL);

    if (src_pixbuf != NULL)
    {
        g_object_unref(src_pixbuf);
    }
    return;
}

/**
 * @blend_in_from_path
 *
 * @param pixbuf Storing place of blended image pixbuf
 * @param path Path for the image to read
 * @param save_path Path for the original image area to save
 * @param x Destination for X coordinate
 * @param y Destination for Y coordinate
 *
 * Read an image from given path and blend it into destination pixbuf
 * in the given location.
 */
static 
void blend_in_from_path_sidebar(GdkPixbuf **pixbuf, 
                                const gchar *path, 
                                const gchar *save_path, 
                                gint x, gint y)
{
    GdkPixbuf *src_pixbuf;
    GdkPixbuf *src_pixbuf_orig = NULL;
    GdkPixbuf *pixbuf_sidebar = NULL;
    gint width, height;
    gint src_height, src_width;
    gint loader_status;

    if (*pixbuf == NULL) 
    {
        return;
    }

    width = gdk_pixbuf_get_width(*pixbuf);
    height = gdk_pixbuf_get_height(*pixbuf);

    if (x > width || y > height) 
    {
        return;
    }
    
    loader_status = load_image_pixbuf(&pixbuf_sidebar, path);
    if(loader_status != HILDON_HOME_IMAGE_LOADER_OK) 
    {
        return;
    }

    src_pixbuf = gdk_pixbuf_scale_simple(
        pixbuf_sidebar,
        HILDON_HOME_SIDEBAR_WIDTH_LOADER,
        HILDON_HOME_SIDEBAR_HEIGHT_LOADER,
        GDK_INTERP_NEAREST);                
    if (pixbuf_sidebar != NULL)
    {
        g_object_unref(pixbuf_sidebar);
    }

    src_width = gdk_pixbuf_get_width(src_pixbuf);
    src_height = gdk_pixbuf_get_height(src_pixbuf);

    if( x+src_width >  width )
    {
        src_width = width-x;
    }
    if( y+src_height >  height )
    {
        src_height = height-y;
    }
    
    loader_status = 
        create_original_pixbuf_area(&src_pixbuf_orig,
                                    *pixbuf,
                                    x, y,
                                    src_width,
                                    src_height);
    if(loader_status != HILDON_HOME_IMAGE_LOADER_OK) 
    {
        return;
    }

    save_image(src_pixbuf_orig, save_path);

    if (src_pixbuf_orig != NULL)
    {
        g_object_unref(src_pixbuf_orig);
    }

    gdk_pixbuf_composite(src_pixbuf, *pixbuf,
                         x, y,
                         src_width,
                         src_height,
                         0, 0,
                         1.0, 1.0,
                         GDK_INTERP_BILINEAR,
                         HILDON_HOME_IMAGE_ALPHA_FULL);

    if (src_pixbuf != NULL)
    {
        g_object_unref(src_pixbuf);
    }
    return;
}

int main(int argc, char *argv[])
{
    GdkPixbuf *pixbuf = NULL, *src_pixbuf = NULL;
    const gchar *operation, *source_uri, *source_uri_save_file;
    const gchar *dest_filename;
    gint dest_width, dest_height;
    gint argv_counter = 5, loader_status;
    
    GdkPixbuf *pixbuf_titlebar = NULL;
    GdkPixbuf *pixbuf_sidebar = NULL;
    
    if (argc < argv_counter || (((argc - argv_counter) % 2) != 0)) 
    {
        g_print("Usage: <operation> <save-image-filename> "
                "<image-width> <image-height> "
                "[<source-image-uri> <source-uri-save-file>] "
                "[<image-filename> <image-filename-save> "
                "<image-x> <image-y> [..]]\n");
        return HILDON_HOME_IMAGE_LOADER_ERROR_SYSTEM_RESOURCE;
    }
    
    operation = argv[1];
    dest_filename = argv[2];
    dest_width = atoi(argv[3]);
    dest_height = atoi(argv[4]);
    
    if (!gnome_vfs_init()) 
    {
        return HILDON_HOME_IMAGE_LOADER_ERROR_SYSTEM_RESOURCE;
    }
    
    if(g_str_equal(operation, HILDON_HOME_IMAGE_LOADER_SKIN) == TRUE) 
    {
        /* -------------------------------- */
        if(argc >= HILDON_HOME_IMAGE_LOADER_ARGC_TITLEBAR_NEWSKIN) 
        {
            if(argv[argv_counter] && argv[argv_counter+1])
            {
                blend_new_skin_to_image_area(&pixbuf_titlebar,
                                             argv[argv_counter],
                                             argv[argv_counter+1]);
            }
            /* -------------------------------- */
            if(argc == HILDON_HOME_IMAGE_LOADER_ARGC_SIDEBAR_NEWSKIN) 
            {
                if(argv[argv_counter+4] && argv[argv_counter+5])
                {
                    blend_new_skin_to_image_area(&pixbuf_sidebar, 
                                                 argv[argv_counter+4],
                                                 argv[argv_counter+5]);
                }
            }
        }        
        
        /* -------------------------------- */
        loader_status = load_image_pixbuf(&pixbuf, dest_filename);
        if(loader_status != HILDON_HOME_IMAGE_LOADER_OK) 
        {
            return loader_status;
        }
        
        
        /* -------------------------------- */
        if(argc >= HILDON_HOME_IMAGE_LOADER_ARGC_TITLEBAR_NEWSKIN) 
        {
            if(argv[argv_counter] && argv[argv_counter+1] &&
               pixbuf_titlebar != NULL)
            {
                blend_in_with_pixbuf(&pixbuf, pixbuf_titlebar, 
                                     atoi(argv[argv_counter+2]),
                                     atoi(argv[argv_counter+3]));
            }
        
            if(argc == HILDON_HOME_IMAGE_LOADER_ARGC_SIDEBAR_NEWSKIN) 
            {
                if(argv[argv_counter+4] && argv[argv_counter+5] &&
                   pixbuf_sidebar != NULL)
                {
                    blend_in_with_pixbuf(&pixbuf, pixbuf_sidebar,
                                         atoi(argv[argv_counter+6]),
                                         atoi(argv[argv_counter+7]));
                }
            }
        }

        loader_status = save_image(pixbuf, dest_filename);
        if(loader_status != HILDON_HOME_IMAGE_LOADER_OK) 
        {
            return loader_status;
        }
    } else 
    {
        source_uri = argv[argv_counter];
        argv_counter++;
        source_uri_save_file = argv[argv_counter];
        argv_counter++;
        loader_status = load_image_from_uri(&src_pixbuf, source_uri);
        
        if(loader_status != HILDON_HOME_IMAGE_LOADER_OK) 
        {
            return loader_status;
        }
 
        loader_status = create_background_pixbuf(&pixbuf, src_pixbuf, 
                                                 dest_width, dest_height);
        if (loader_status != HILDON_HOME_IMAGE_LOADER_OK) 
        {
            return loader_status;
        }

        if (src_pixbuf != NULL)
        {
            g_object_unref(src_pixbuf);
        }
        /* titlebar */
        if(argc >= HILDON_HOME_IMAGE_LOADER_ARGC_TITLEBAR_NEWIMAGE) 
        {
            if(argv[argv_counter] && argv[argv_counter+1])
            {
                blend_in_from_path(&pixbuf, 
                                   argv[argv_counter], 
                                   argv[argv_counter+1], 
                                   atoi(argv[argv_counter+2]),
                                   atoi(argv[argv_counter+3]));
            }
        
            /* sidebar */
            if(argc == HILDON_HOME_IMAGE_LOADER_ARGC_SIDEBAR_NEWIMAGE) 
            {
                argv_counter += 4;
                if(argv[argv_counter] && argv[argv_counter+1])
                {
                    blend_in_from_path_sidebar(&pixbuf, 
                                               argv[argv_counter], 
                                               argv[argv_counter+1], 
                                               atoi(argv[argv_counter+2]),
                                               atoi(argv[argv_counter+3]));
                }
            }
        }

        loader_status = save_image(pixbuf, dest_filename);
        if(loader_status != HILDON_HOME_IMAGE_LOADER_OK) 
        {
            return loader_status;
        }
        save_original_bg_image_uri(source_uri, source_uri_save_file);
    }
        
    if(pixbuf != NULL)
    {
        g_object_unref(pixbuf);
    }

    gnome_vfs_shutdown();

    return HILDON_HOME_IMAGE_LOADER_OK;
}
