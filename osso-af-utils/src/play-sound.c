/*
  Copyright (C) 2006 Nokia Corporation. All rights reserved.
 
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  version 2 as published by the Free Software Foundation.
 
  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

#include <gconf/gconf-client.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <esd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <unistd.h>

#define ALARM_GCONF_PATH "/apps/osso/sound/system_alert_volume"
/* The full volume should be about 30% of maximum, that is 0x4C out of 0xFF.
   The full volume is marked as 2 in gconf, divide and we get 0x26. */
#define SCALE_MULTIPLIER 0x26

/* Based on hildon_play_system_sound */

/* Copypaste from esdfile.c and xmms mixer.c with (very) minor alterations. */
/* Plays a file with esd and with scaled channels. */
static int 
esd_play_file_with_pan( int esd, const char *name_prefix,
                        const char *filename, 
                        int left_scale, 
                        int right_scale )
{
    /* input from libaudiofile... */
    AFfilehandle in_file;
    int in_format, in_width, in_channels, frame_count;
    double in_rate;
    int bytes_per_frame;

    /* output to esound... */
    int out_sock, out_bits, out_channels, out_rate;
    int out_mode = ESD_STREAM, out_func = ESD_PLAY;
    esd_format_t out_format;
    char name[ ESD_NAME_MAX ] = "";

    esd_player_info_t *info;
    esd_info_t *all_info = NULL;

    /* open the audio file */
    in_file = afOpenFile( filename, "rb", NULL );
    if ( !in_file )
	return 0;

    /* get audio file parameters */
    frame_count = afGetFrameCount( in_file, AF_DEFAULT_TRACK );
    in_channels = afGetChannels( in_file, AF_DEFAULT_TRACK );
    in_rate = afGetRate( in_file, AF_DEFAULT_TRACK );
    afGetSampleFormat( in_file, AF_DEFAULT_TRACK, &in_format, &in_width );

    if(getenv("ESDBG")) 
    {
        printf ("frames: %i channels: %i rate: %f format: %i width: %i\n",
                frame_count, in_channels, in_rate, in_format, in_width);
    }

    /* convert audiofile parameters to EsounD parameters */
    if ( in_width == 8 )
        out_bits = ESD_BITS8;
    else if ( in_width == 16 )
        out_bits = ESD_BITS16;
    else
	return 0;

    bytes_per_frame = ( in_width  * in_channels ) / 8;

    if ( in_channels == 1 )
	out_channels = ESD_MONO;
    else if ( in_channels == 2 )
	out_channels = ESD_STEREO;
    else
	return 0;

    out_format = out_bits | out_channels | out_mode | out_func;
    out_rate = (int) in_rate;

    /* construct name */
    if ( name_prefix ) 
    {
        strncpy( name, name_prefix, ESD_NAME_MAX - 2 );
        strcat( name, ":" );
    }
    strncpy( name + strlen( name ), filename, ESD_NAME_MAX - strlen( name ) );

    /* connect to server and play stream */
    out_sock = esd_play_stream( out_format, out_rate, NULL, filename );

    if ( out_sock <= 0 ) {
        /* Close the AF filehandle too */
        afCloseFile ( in_file );
        return 0;
    }

    all_info = esd_get_all_info(esd);
    for (info = all_info->player_list; info != NULL; info = info->next) 
    {
      if (!strcmp(filename, info->name)) 
      {
	break;
      }
    }
    if (info != NULL) 
    {
      esd_set_stream_pan(esd, info->source_id,
	  left_scale, right_scale);
    }
    esd_free_all_info(all_info);
    
    /* play */
    esd_send_file( out_sock, in_file, bytes_per_frame );

    /* close up and go home */
    close( out_sock );
    if ( afCloseFile ( in_file ) )
    {
	return 0;
    }

    return 1;
}


static void
wait_for_esd_server_to_quiesce (int esd_fd)
{
    char *buf = NULL ;
    esd_server_info_t *esd_server_info = NULL ;
    int monitor_fd = -1, read_return = -1, available_bytes = 0, max_avail = 0;
    int first_time = 1;

    if (NULL != (esd_server_info = esd_get_server_info (esd_fd)))
    {
        if ((monitor_fd = esd_monitor_stream (esd_server_info->format, esd_server_info->rate, NULL, NULL)) > 0)
        {
            while (TRUE)
            {
                struct timeval tv;
		
                ioctl (monitor_fd, FIONREAD, &available_bytes) ;
                if (available_bytes > max_avail)
                {
                    buf = realloc (buf, available_bytes) ;
                    max_avail = available_bytes ;
                }
		
                if (available_bytes)
		{
                    read_return = read(monitor_fd, buf, available_bytes);
                    if (read_return < 0)
                        break ;
		}
                else if ( !first_time )
                {
                    break ;
                }

                tv.tv_sec = 0;
                tv.tv_usec = 200000;
                select (0, NULL, NULL, NULL, &tv); /* avoid tight loop */
                first_time = 0;
            }

            if (buf)
                free (buf) ;
            close (monitor_fd) ;
        }
        esd_free_server_info (esd_server_info) ;
    }
}

static int 
play_sound(gchar *sound_filename,
           gchar *data) 
{
    gint sock, pan;
    GConfClient *gc;
    GConfValue *value;

	g_type_init () ;

    gc = gconf_client_get_default();
    value = gconf_client_get(gc, ALARM_GCONF_PATH, NULL);

    if (value && value->type == GCONF_VALUE_INT) 
    {
      pan = gconf_value_get_int(value);
    } 
    else 
    {
      pan = 2;
    }

    if (value) 
    {
        gconf_value_free(value);
    }
    g_object_unref(G_OBJECT(gc));

    pan *= SCALE_MULTIPLIER;
    sock = esd_open_sound(NULL);

    esd_play_file_with_pan(sock, data,
		  sound_filename, pan, pan);

    wait_for_esd_server_to_quiesce (sock) ;
    esd_close(sock);

    return 0;
}

int main(int argc, char **argv) {
  
   if (argc < 2) {
    fprintf(stderr, "usage: %s sound.wav\n",argv[0]);	
    return 1;
   }
   else 
     return play_sound(argv[1], NULL);
}
