/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2006 Nokia Corporation.  All rights reserved.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
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

#include <string.h>
#include <glib.h>
#define GTK_FILE_SYSTEM_ENABLE_UNSUPPORTED
#include <gtk/gtkfilesystem.h>
#include <sys/statfs.h>

#include "hildon-file-common-private.h"
#include "hildon-file-system-private.h"
#include "hildon-file-system-mmc.h"
#include "hildon-file-system-settings.h"

static void
hildon_file_system_mmc_class_init (HildonFileSystemMMCClass *klass);
static void
hildon_file_system_mmc_init (HildonFileSystemMMC *device);
static gchar*
hildon_file_system_mmc_get_display_name (HildonFileSystemSpecialLocation
                                         *location, GtkFileSystem *fs);
static gboolean
hildon_file_system_mmc_is_available (HildonFileSystemSpecialLocation *location);
static void
hildon_file_system_mmc_volumes_changed (HildonFileSystemSpecialLocation
                                        *location, GtkFileSystem *fs);
static gchar*
hildon_file_system_mmc_get_unavailable_reason (HildonFileSystemSpecialLocation
                                               *location);
static gchar*
hildon_file_system_mmc_get_extra_info (HildonFileSystemSpecialLocation *location);

G_DEFINE_TYPE (HildonFileSystemMMC,
               hildon_file_system_mmc,
               HILDON_TYPE_FILE_SYSTEM_SPECIAL_LOCATION);

struct _HildonFileSystemMMCPrivate
{
    gboolean internal_card; /* property */
    gboolean available;
};

typedef enum {
    PROP_INTERNAL_CARD = 1
} HildonFileSystemMMCProperty;

#define PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                      HILDON_TYPE_FILE_SYSTEM_MMC, \
                      HildonFileSystemMMCPrivate))

static void
hildon_file_system_mmc_get_property (GObject *object, guint prop_id,
                                     GValue *value, GParamSpec *pspec)
{
    HildonFileSystemMMCPrivate *priv = PRIVATE (object);

    switch (prop_id)
    {
      case PROP_INTERNAL_CARD:
        g_value_set_boolean (value, priv->internal_card);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
hildon_file_system_mmc_set_property(GObject *object, guint prop_id,
                                    const GValue *value, GParamSpec *pspec)
{
    HildonFileSystemSpecialLocation *location;
    HildonFileSystemMMCPrivate *priv = PRIVATE (object);

    switch (prop_id)
    {
      case PROP_INTERNAL_CARD:
        priv->internal_card = g_value_get_boolean (value);

        location = HILDON_FILE_SYSTEM_SPECIAL_LOCATION (object);
        if (priv->internal_card)
        {
            location->sort_weight = SORT_WEIGHT_INTERNAL_MMC;
        }
        else
        {
            location->sort_weight = SORT_WEIGHT_EXTERNAL_MMC;
        }

        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
hildon_file_system_mmc_class_init (HildonFileSystemMMCClass *klass)
{
    HildonFileSystemSpecialLocationClass *location =
            HILDON_FILE_SYSTEM_SPECIAL_LOCATION_CLASS (klass);
    GObjectClass *object_class;

    g_type_class_add_private (klass, sizeof (HildonFileSystemMMCPrivate));
    object_class = G_OBJECT_CLASS (klass);

    object_class->get_property = hildon_file_system_mmc_get_property;
    object_class->set_property = hildon_file_system_mmc_set_property;

    location->get_display_name = hildon_file_system_mmc_get_display_name;
    location->is_available = hildon_file_system_mmc_is_available;
    location->volumes_changed = hildon_file_system_mmc_volumes_changed;
    location->get_unavailable_reason =
            hildon_file_system_mmc_get_unavailable_reason;
    location->get_extra_info = hildon_file_system_mmc_get_extra_info;

    g_object_class_install_property (object_class, PROP_INTERNAL_CARD,
        g_param_spec_boolean ("internal-card", "Internal card",
                              "Whether the card is the internal card",
                              TRUE, G_PARAM_READWRITE));
}

static void
hildon_file_system_mmc_init (HildonFileSystemMMC *device)
{
    HildonFileSystemSpecialLocation *location;

    location = HILDON_FILE_SYSTEM_SPECIAL_LOCATION (device);
    location->fixed_icon = g_strdup ("qgn_list_filesys_mmc_root");
    location->compatibility_type = HILDON_FILE_SYSTEM_MODEL_MMC;
}

static gchar*
hildon_file_system_mmc_get_display_name (HildonFileSystemSpecialLocation
                                         *location, GtkFileSystem *fs)
{
    gchar *name = NULL;
    GtkFileSystemVolume *vol;

    vol = _hildon_file_system_get_volume_for_location (fs, location);

    if (vol)
    {
        name = gtk_file_system_volume_get_display_name (fs, vol);
        gtk_file_system_volume_free (fs, vol);
    }

    /* check first if it is an unnamed memory card */
    if (name && strncmp (name, "mmc-undefined-name", 18) == 0) 
    {
        g_free (name);
        name = NULL;
    }

    if (!name)
    {
        HildonFileSystemMMCPrivate *priv;
        priv = PRIVATE (location);

        if (priv->internal_card)
        {
            name = g_strdup (_("sfil_li_memorycard_internal"));
        }
        else
        {
            name = g_strdup (_("sfil_li_memorycard_removable"));
        }
    }

    return name;
}

static gboolean
hildon_file_system_mmc_is_available (HildonFileSystemSpecialLocation *location)
{
    HildonFileSystemMMCPrivate *priv;
    priv = PRIVATE (location);

    return priv->available;
}

static void
hildon_file_system_mmc_volumes_changed (HildonFileSystemSpecialLocation
                                        *location, GtkFileSystem *fs)
{
    GtkFileSystemVolume *vol;
    HildonFileSystemMMCPrivate *priv;
    gboolean was_available;

    priv = PRIVATE (location);

    was_available = priv->available;

    ULOG_DEBUG(__FUNCTION__);

    vol = _hildon_file_system_get_volume_for_location (fs, location);

    if (vol)
    {
        if (gtk_file_system_volume_get_is_mounted (fs, vol))
            priv->available = TRUE;
        else
            priv->available = FALSE;

        gtk_file_system_volume_free (fs, vol);
    }
    else
    {
        priv->available = FALSE;
    }

    if (was_available != priv->available)
    {
        g_signal_emit_by_name (HILDON_FILE_SYSTEM_MMC (location),
                               "connection-state");
    }
}

static gchar*
hildon_file_system_mmc_get_unavailable_reason (HildonFileSystemSpecialLocation
                                               *location)
{
    HildonFileSystemSettings *fs_settings;
    HildonFileSystemMMCPrivate *priv;

    gboolean is_connected;
    gboolean mmc_is_used;
    gboolean mmc_is_present;
    gboolean mmc_is_corrupted;
    gboolean mmc_cover_open;
    
    priv = PRIVATE (location);

    if (!priv->available)
    {
        fs_settings = _hildon_file_system_settings_get_instance ();

	/* maybe should start using a bitmask instead of bunch of variables */
        g_object_get (fs_settings, "usb-cable", &is_connected, NULL);
	g_object_get (fs_settings, "mmc-used", &mmc_is_used, NULL);
	g_object_get (fs_settings, "mmc-is-present", &mmc_is_present, NULL);
	g_object_get (fs_settings, "mmc-is-corrupted", &mmc_is_corrupted, NULL);
        g_object_get (fs_settings, "mmc-cover-open", &mmc_cover_open, NULL);

	if (mmc_cover_open)
	{
	  return NULL;
	}
	
	if (mmc_is_corrupted)
	{
	  return g_strdup (KE("card_ib_memory_card_corrupted"));
	}
	
        if (is_connected && mmc_is_used)
	{
    	  return g_strdup (_("sfil_ib_mmc_usb_connected"));
	}
  	else
	{
          return g_strdup (_("hfil_ib_mmc_not_present"));
	}
    }

    return NULL;
}

static gchar*
hildon_file_system_mmc_get_extra_info (HildonFileSystemSpecialLocation *location)
{
    HildonFileSystemMMCPrivate *priv;
    gchar *local_path;
    gint64 free_space = 0;
    gchar buffer[256];

    g_return_val_if_fail (location->basepath, NULL);
    priv = PRIVATE (location);

    local_path = g_filename_from_uri (location->basepath, NULL, NULL);
    if (local_path)
    {
        struct statfs buf;

        if (statfs (local_path, &buf) == 0)
        {
            free_space = ((gint64) buf.f_bavail) * ((gint64) buf.f_bsize);
        }

        g_free (local_path);
    }


    if (!priv->available)
    {
        buffer[0] = 0;
    }
    else if (free_space < 1024 * 1024)
    {
        g_snprintf (buffer, sizeof (buffer), "%d kB free",
                (gint) free_space / 1024);
    }
    else
    {
        g_snprintf (buffer, sizeof (buffer), "%d MB free",
                (gint) (free_space / (1024 * 1024)));
    }

    return g_strdup (buffer);
}

