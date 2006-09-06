/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
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

#include <unistd.h> /* close() */
#include <glib/gtypes.h>
#include <gconf/gconf-client.h>
#include <esd.h>
#include "hn-app-sound.h"

#define PROGRAM_NAME        "MaemoApplicationSwitcher"
#define ALARM_GCONF_PATH    "/apps/osso/sound/system_alert_volume"

static gint
hn_as_sound_get_volume_scale (void);

gint
hn_as_sound_init()
{
    gint sock;
   
    sock = esd_open_sound(NULL);

    return sock;
}


gint
hn_as_sound_register_sample (gint esd_socket, const gchar *sample)
{
    gint sample_id;
    
    sample_id = esd_file_cache (esd_socket, PROGRAM_NAME, sample);

    return sample_id;
}

void
hn_as_sound_deregister_sample (gint esd_socket, gint sample_id)
{
    esd_sample_free (esd_socket, sample_id);
}


gint
hn_as_sound_play_sample (gint esd_socket, gint sample_id)
{
    gint scale = hn_as_sound_get_volume_scale ();

    if (esd_set_default_sample_pan (esd_socket, sample_id, scale, scale) == -1)
        return -1;
    
    if (esd_sample_play (esd_socket, sample_id) == -1)
        return -1;

    return 0;
}


void
hn_as_sound_deinit (gint socket)
{
    close(socket);
}

static gint
hn_as_sound_get_volume_scale ()
{
    GConfClient *client = NULL;
    GError *error = NULL;
    gint volume;

    client = gconf_client_get_default();

    if (!client)
        return 0;

    volume = gconf_client_get_int (client, ALARM_GCONF_PATH, &error);

    g_object_unref (G_OBJECT (client));

    if (error)
    {
        g_error_free (error);
        return 0;
    }


    switch (volume)
    {
        case 1:
            return 0x80;
        case 2:
            return 0xff;
    }

    return 0;
}
