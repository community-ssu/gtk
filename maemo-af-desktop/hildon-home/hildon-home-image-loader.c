/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005 Nokia Corporation.
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
#include <osso-log.h>

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
 * @param postintall TRUE if installing time call
 *
 * @returns loader status
 *
 * Load an image from given URI. Places to pixbuf
*/
static 
gint load_image_from_uri(GdkPixbuf **pixbuf, const gchar *uri, 
                         gboolean postinstall)
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
    
    if(result != GNOME_VFS_OK)
    {
        g_object_unref(gconf_client);
        return HILDON_HOME_IMAGE_LOADER_ERROR_FILE_UNREADABLE;
    }

    /*Setting up a watchdog*/
    if(!postinstall && 
       osso_mem_saw_enable( 3 << 20, 32767, load_image_oom_cb, (void *)&oom) )
    {
        oom = TRUE;
    } 
    loader = gdk_pixbuf_loader_new();

    while (!oom && (result == GNOME_VFS_OK) && 
           (!mmc_in_use || !mmc_cover_open)) {
        result = gnome_vfs_read(handle, buffer, BUF_SIZE, &bytes_read);
        if(!oom) 
        {
            if(!gdk_pixbuf_loader_write(loader, buffer, bytes_read, &error))
            {
                if(error != NULL)
                {
                    if(error->domain == GDK_PIXBUF_ERROR &&
                            (error->code == GDK_PIXBUF_ERROR_CORRUPT_IMAGE ||
                             error->code == GDK_PIXBUF_ERROR_UNKNOWN_TYPE))
                    {
                        g_error_free(error);
                        return HILDON_HOME_IMAGE_LOADER_ERROR_FILE_CORRUPT;
                    }
                    g_error_free(error);
                }
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
    if(postinstall)
    {
        osso_mem_saw_disable();
    }
    
    if (oom || (result != GNOME_VFS_ERROR_EOF)) 
    {
        gdk_pixbuf_loader_close(loader, NULL);
        gnome_vfs_close(handle);
        return HILDON_HOME_IMAGE_LOADER_ERROR_FILE_CORRUPT;
    }
    
    gdk_pixbuf_loader_close(loader, NULL);
    *pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
    gnome_vfs_close(handle);
    if (!*pixbuf)
        return HILDON_HOME_IMAGE_LOADER_ERROR_FILE_CORRUPT;
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
        if (error && error->domain == G_FILE_ERROR 
                  && error->code == G_FILE_ERROR_NOSPC)
        {
            /* Flash full! */
            g_error_free(error);
            unlink(temp_path);
            g_free(temp_path);
            return HILDON_HOME_IMAGE_LOADER_ERROR_FLASH_FULL;
        }
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
 * @create_background_pixbuf_color
 *
 * @param pixbuf Storing place of created pixbuf
 * @param width Width of the destination pixbuf to create
 * @param height Height of the destination pixbuf to create
 * @param color_red Red component of rgb color to set background of pixbuf
 * @param color_green Green component of rgb color to set background of pixbuf
 * @param color_blue Blue component of rgb color to set background of pixbuf
 *
 * @returns loader status
 *
 * Create a background image of given rgb color.
 */

static
void create_background_pixbuf_color(GdkPixbuf **pixbuf, 
                                    gint width, gint height,
                                    gint color_red, 
                                    gint color_green, 
                                    gint color_blue)
{
    guint32 color = 0;
   
    color = (guint8)(color_red >> 8) << 24 | 
            (guint8)(color_green >> 8) << 16 |
            (guint8)(color_blue >> 8) << 8 |
            0xff;

    *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 
                             width, height);
    
    gdk_pixbuf_fill(*pixbuf, color);
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
 * Create a background from given source image. If the image is larger
 * than the destination size, it's cropped. If it's smaller, it's
 * tiled to fill space.
 */

static
gint create_background_pixbuf(GdkPixbuf **pixbuf, const GdkPixbuf *src_pixbuf,
                              gint width, gint height, 
                              gint scaling,
                              gint color_red, 
                              gint color_green, 
                              gint color_blue)
{
    gint src_width = -1, src_height = -1;
    gint dest_x, dest_y;
    double scaling_ratio;

    if (src_pixbuf == NULL) 
    {
        return HILDON_HOME_IMAGE_LOADER_ERROR_SYSTEM_RESOURCE;        
    }


    create_background_pixbuf_color(pixbuf, 
                                   width, height,
                                   color_red, color_green, color_blue);
    if (*pixbuf == NULL) 
    {
        return HILDON_HOME_IMAGE_LOADER_ERROR_SYSTEM_RESOURCE;
    }

    src_width = gdk_pixbuf_get_width(src_pixbuf);
    src_height = gdk_pixbuf_get_height(src_pixbuf);

    if (src_width == width && src_height == height) 
    {
        /* source pixbuf is in the exactly wanted size,
           we can use it directly */
        gdk_pixbuf_composite(src_pixbuf,
                             *pixbuf,
                             0, 0,
                             width, height,
                             0, 0,
                             1.0, 1.0,
                             GDK_INTERP_NEAREST,
                             HILDON_HOME_IMAGE_ALPHA_FULL);

        if (*pixbuf == NULL) 
        {
            return HILDON_HOME_IMAGE_LOADER_ERROR_SYSTEM_RESOURCE;
        }
        return HILDON_HOME_IMAGE_LOADER_OK; 
    }

    switch (scaling)
    {
      case CENTERED:
        gdk_pixbuf_composite(src_pixbuf,
                             *pixbuf,
                             MAX(0, (width-src_width) / 2),
                             MAX(0, (height-src_height) / 2),
                             MIN(src_width, width),
                             MIN(src_height, height),
                             MAX(0, (width-src_width) / 2),
                             MAX(0, (height-src_height) / 2),
                             1.0, 1.0,
                             GDK_INTERP_NEAREST,
                             HILDON_HOME_IMAGE_ALPHA_FULL);
        break;
      case SCALED:
        scaling_ratio = MIN((double)width/src_width, 
                            (double)height/src_height);

        gdk_pixbuf_composite(src_pixbuf,
                             *pixbuf,
                             (int)MAX(0,(width-scaling_ratio*src_width)/2),
                             (int)MAX(0,(height-scaling_ratio*src_height)/2),
                             scaling_ratio*src_width, 
                             scaling_ratio*src_height,
                             MAX(0,(width-scaling_ratio*src_width)/2),
                             MAX(0,(height-scaling_ratio*src_height)/2),
                             scaling_ratio,
                             scaling_ratio,
                             GDK_INTERP_NEAREST,
                             HILDON_HOME_IMAGE_ALPHA_FULL);

        break;
      case STRETCHED:
        gdk_pixbuf_composite(src_pixbuf,
                             *pixbuf,
                             0, 0,
                             width, height,
                             0, 0,
                             (double)width/src_width,
                             (double)height/src_height,
                             GDK_INTERP_NEAREST,
                             HILDON_HOME_IMAGE_ALPHA_FULL);
        break;
      case TILED:

        for(dest_x = 0; dest_x < width; dest_x += src_width)
        {
            for(dest_y = 0; dest_y < height; dest_y += src_height )
            {
                gdk_pixbuf_composite(src_pixbuf,
                                     *pixbuf, 
                                     dest_x, 
                                     dest_y,
                                     MIN(src_width, width-dest_x),
                                     MIN(src_height, height-dest_y),
                                     dest_x, dest_y,
                                     1.0, 1.0,
                                     GDK_INTERP_NEAREST,
                                     HILDON_HOME_IMAGE_ALPHA_FULL);
            }
        }
        break;
      default :
        return HILDON_HOME_IMAGE_LOADER_ERROR_SYSTEM_RESOURCE;        
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
        pixbuf_blend = pixbuf_temp;
        width_blend = HILDON_HOME_SIDEBAR_WIDTH_LOADER;
        height_blend = HILDON_HOME_SIDEBAR_HEIGHT_LOADER;
    }

    gdk_pixbuf_composite(pixbuf_blend, *pixbuf_orig,
                         0, 0,
                         width_blend,
                         height_blend,
                         0, 0,
                         1.0, 1.0,
                         GDK_INTERP_NEAREST,
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
 * @param load_path Path for the image to read
 * @param save_path Path for the original image area to save
 * @param x Destination for X coordinate
 * @param y Destination for Y coordinate
 * @param area_width Destination area width
 * @param area_height Destination area height
 *
 * Read an image from given path and blend it into destination pixbuf
 * in the given location.
 */
static 
void blend_in_from_path(GdkPixbuf **pixbuf, 
                        const gchar *load_path, 
                        const gchar *save_path, 
                        gint area_x, gint area_y,
                        gint area_width, gint area_height)
{
    GdkPixbuf *src_pixbuf;
    GdkPixbuf *src_pixbuf_orig = NULL;
    gint width, height;
    gint loader_status;

    if (*pixbuf == NULL) 
    {
        return;
    }

    width = gdk_pixbuf_get_width(*pixbuf);
    height = gdk_pixbuf_get_height(*pixbuf);

    if (area_x > width || area_y > height) 
    {
        return;
    }
    
    loader_status = load_image_pixbuf(&src_pixbuf, load_path);
    if(loader_status != HILDON_HOME_IMAGE_LOADER_OK) 
    {
        return;
    }

    loader_status = 
        create_original_pixbuf_area(&src_pixbuf_orig,
                                    *pixbuf,
                                    area_x, area_y,
                                    area_width,
                                    area_height);
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
                         area_x, area_y,
                         area_width,
                         area_height,
                         0, 0,
                         1.0, 1.0,
                         GDK_INTERP_NEAREST,
                         HILDON_HOME_IMAGE_ALPHA_FULL);

    if (src_pixbuf != NULL)
    {
        g_object_unref(src_pixbuf);
    }
    return;
}

int main(int argc, char *argv[])
{
    gchar *operation;
    gint scaling;
    gchar *source_uri, *source_uri_savefile, *target_savefile;
    gint dest_width, dest_height;
    gint color_red, color_green, color_blue;

    gchar *area1_origfile, *area1_savefile;
    gint area1_target_x, area1_target_y;

    gchar *area2_origfile, *area2_savefile;
    gint area2_target_x, area2_target_y;

    gboolean postinstall;
    
    GdkPixbuf *pixbuf = NULL, *src_pixbuf = NULL;
    GdkPixbuf *pixbuf_area1 = NULL;
    GdkPixbuf *pixbuf_area2 = NULL;
    gint loader_status, argc_count;

    if (argc != HILDON_HOME_IMAGE_LOADER_ARGC_AMOUNT)
    {
        g_print("<operation>"
                " <scaling mode>"
                " <source-image-uri>"
                " <source-uri-savefile>"
                " <image-savefile>"
                " <image_width>"
                " <image-heigh>"
                " <color_red>"
                " <color_green>"
                " <color_blue>"
                " <area1-image-origfile>"
                " <area1-image-savefile>"
                " <area1-image-x>"
                " <area1-image-y>"
                " <area2-image-origfile>"
                " <area2-image-savefile>"
                " <area2-image-x>"
                " <area2-image-y>"
                " <install-flag>\n"
                );
        return HILDON_HOME_IMAGE_LOADER_ERROR_SYSTEM_RESOURCE;
    }
    
    argc_count = 1;
    operation = argv[argc_count++];
    scaling = atoi(argv[argc_count++]);
    source_uri = argv[argc_count++];
    source_uri_savefile = argv[argc_count++];
    target_savefile = argv[argc_count++];
    dest_width = atoi(argv[argc_count++]);
    dest_height = atoi(argv[argc_count++]);
    color_red = atoi(argv[argc_count++]);
    color_green = atoi(argv[argc_count++]);
    color_blue = atoi(argv[argc_count++]);
    area1_origfile = argv[argc_count++];
    area1_savefile = argv[argc_count++];
    area1_target_x = atoi(argv[argc_count++]);
    area1_target_y = atoi(argv[argc_count++]);
    area2_origfile = argv[argc_count++];
    area2_savefile = argv[argc_count++];
    area2_target_x = atoi(argv[argc_count++]);
    area2_target_y = atoi(argv[argc_count++]);
    postinstall = atoi(argv[argc_count++]);

    if (!gnome_vfs_init()) 
    {
        return HILDON_HOME_IMAGE_LOADER_ERROR_SYSTEM_RESOURCE;
    }
    
    if(g_str_equal(operation, HILDON_HOME_IMAGE_LOADER_SKIN) == TRUE) 
    {
        if(area1_origfile != NULL && area1_savefile != NULL)
        {
            blend_new_skin_to_image_area(&pixbuf_area1,
                                         area1_origfile,
                                         area1_savefile);
        }

        if(area2_origfile != NULL && area2_savefile != NULL)
        {
            blend_new_skin_to_image_area(&pixbuf_area2,
                                         area2_origfile,
                                         area2_savefile);
        }
        
        loader_status = load_image_pixbuf(&pixbuf, target_savefile);
        if(loader_status != HILDON_HOME_IMAGE_LOADER_OK) 
        {
            return loader_status;
        }
        
        
        if(pixbuf_area1 != NULL)
        {
            blend_in_with_pixbuf(&pixbuf, pixbuf_area1, 
                                 area1_target_x,
                                 area1_target_y);
        }
        
        if(pixbuf_area2 != NULL)
        {
            blend_in_with_pixbuf(&pixbuf, pixbuf_area2,
                                 area2_target_x,
                                 area2_target_y);
        }

        loader_status = save_image(pixbuf, target_savefile);
        if(loader_status != HILDON_HOME_IMAGE_LOADER_OK) 
        {
            return loader_status;
        }
    } else if(g_str_equal(operation, HILDON_HOME_IMAGE_LOADER_IMAGE) == TRUE)

    {
        if(g_str_equal(source_uri, HILDON_HOME_IMAGE_LOADER_NO_IMAGE) == TRUE)
        {
            create_background_pixbuf_color(&pixbuf, 
                                           dest_width, dest_height,
                                           color_red, color_green, color_blue);
            if (pixbuf == NULL) 
            {
                return HILDON_HOME_IMAGE_LOADER_ERROR_SYSTEM_RESOURCE;
            }
        } else
        {
            loader_status = 
                load_image_from_uri(&src_pixbuf, source_uri, postinstall);
        
            if(loader_status != HILDON_HOME_IMAGE_LOADER_OK) 
            {
                return loader_status;
            }
            
            loader_status = create_background_pixbuf(&pixbuf, src_pixbuf, 
                                                     dest_width, 
                                                     dest_height,
                                                     scaling,
                                                     color_red, 
                                                     color_green, 
                                                     color_blue);
            if (loader_status != HILDON_HOME_IMAGE_LOADER_OK) 
            {
                return loader_status;
            }

            if (src_pixbuf != NULL)
            {
                g_object_unref(src_pixbuf);
            }
        }
        /* titlebar area */

        if(area1_origfile != NULL && area1_savefile != NULL)
        {
            blend_in_from_path(&pixbuf, 
                               area1_origfile,
                               area1_savefile,
                               area1_target_x,
                               area1_target_y,
                               dest_width,
                               HILDON_HOME_TITLEBAR_HEIGHT_LOADER);
        }

        /* sidebar area */
        if(area2_origfile != NULL && area2_savefile != NULL)
        {
            blend_in_from_path(&pixbuf, 
                               area2_origfile,
                               area2_savefile,
                               area2_target_x,
                               area2_target_y,
                               HILDON_HOME_SIDEBAR_WIDTH_LOADER,
                               HILDON_HOME_SIDEBAR_HEIGHT_LOADER);
        }
        loader_status = save_image(pixbuf, target_savefile);
        if(loader_status != HILDON_HOME_IMAGE_LOADER_OK) 
        {
            return loader_status;
        }
        save_original_bg_image_uri(source_uri, source_uri_savefile);
    } else
    {
        g_print("Unknown operation\n");
    }
        
    if(pixbuf != NULL)
    {
        g_object_unref(pixbuf);
    }

    return HILDON_HOME_IMAGE_LOADER_OK;
}
