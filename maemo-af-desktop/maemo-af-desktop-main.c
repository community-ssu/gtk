
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
 * @file maemo-af-desktop-main.c
 * <p>
 * Maemo AF Desktop
 *
 * @brief Provides Task Navigator, Status Bar and Home for the
 *        Maemo Application Framework platform. Task Navigator, Status Bar
 *        and Home are built as libraries and they are linked with the
 *        maemo_af_desktop host application.
 *
 */
 
#define USE_AF_DESKTOP_MAIN__
 
#ifdef USE_AF_DESKTOP_MAIN__
 
/* GTK includes */ 
#include <gtk/gtkmain.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtktogglebutton.h>

/* status bar GTK includes */
#include <gtk/gtkwindow.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkfixed.h>
/* home gtk includes */
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gtk/gtkwidget.h>


#include <unistd.h>

/* GDK includes */
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

/* System includes */
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>


/* GDK includes */
#include <gdk/gdkwindow.h>
#include <gdk/gdkx.h>

/* GLib include */
#include <glib.h>

/* X include */
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>


/* dlfcn include */
#include <dlfcn.h>

/* stdio include */
#include <stdio.h>

/* libintl include */
#include <libintl.h>

/* Locale include */
#include <locale.h>

/* Hildon includes */
#include "hildon-navigator/osso-manager.h"
#include "hildon-navigator/hildon-navigator.h"
#include "hildon-navigator/hn-app-switcher.h"
#include "hildon-navigator/others-menu.h"


/* Hildon status bar includes */
#include "hildon-status-bar/hildon-status-bar-lib.h"
#include "hildon-status-bar/hildon-status-bar-item.h"
#include <hildon-widgets/gtk-infoprint.h>
#include <hildon-widgets/hildon-note.h>


#include <libosso.h>
/*#include <osso-log.h>*/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <locale.h>
#include <libintl.h>
#include <math.h>
#include <stdio.h>
#include <libmb/mbutil.h>

#include <libgnomevfs/gnome-vfs.h>
#include <fcntl.h>		/* for fcntl, O_NONBLOCK */
#include <signal.h>		/* for signal */

#include "libmb/mbdotdesktop.h"
#include <hildon-widgets/hildon-note.h>
#include <hildon-widgets/hildon-file-chooser-dialog.h>
#include <hildon-widgets/hildon-defines.h>

/* log include */
#include <log-functions.h>

/* Header file */
#include "maemo-af-desktop-main.h"
#include "hildon-status-bar/hildon-status-bar-interface.h"
#include "hildon-home/hildon-home-interface.h"
#include "hildon-navigator/hildon-navigator-window.h"
#include "kstrace.h"
/* Moved from main to globals to reduce the amount
   of used stack -- Karoliina Salminen */

HildonNavigatorWindow *tasknav;
osso_context_t *osso;
StatusBar *panel;

static int signal_pipe[2];
static gint keysnooper_id;

/*gboolean IS_SDK = FALSE;*/


/*
 * Signaling pipe handle
 */
void pipe_signals(int signal)
{
    if(write(signal_pipe[1], &signal, sizeof(int)) != sizeof(int))
    {
        osso_log(LOG_ERR, "unix signal %d lost\n", signal);
    }    
}


/* 
 * The event loop callback that handles the unix signals. Must be a GIOFunc.
 * The source is the reading end of our pipe, cond is one of 
 *   G_IO_IN or G_IO_PRI (I don't know what could lead to G_IO_PRI)
 * the pointer d is always NULL
 */
gboolean deliver_signal(GIOChannel *source, GIOCondition cond, gpointer d)
{
    GError *error = NULL;		/* for error handling */

    /* 
     * There is no g_io_channel_read or g_io_channel_read_int, so we read
     * char's and use a union to recover the unix signal number.
     */
    union {
        gchar chars[sizeof(int)];
        int signal;
    } buf;
    GIOStatus status;		/* save the reading status */
    gsize bytes_read;		/* save the number of chars read */
  

    /* 
     * Read from the pipe as long as data is available. The reading end is 
     * also in non-blocking mode, so if we have consumed all unix signals, 
     * the read returns G_IO_STATUS_AGAIN. 
     */
    while((status = g_io_channel_read_chars(source, buf.chars, 
                                            sizeof(int), &bytes_read, &error)) 
          == G_IO_STATUS_NORMAL)
    {
        g_assert(error == NULL);	/* no error if reading returns normal */
        
        /* 
         * There might be some problem resulting in too few char's read.
         * Check it.
         */
        if(bytes_read != sizeof(int))
        {
            osso_log(LOG_ERR, 
                     "lost data in signal pipe (expected %d, received %d)\n",
                     sizeof(int), bytes_read);
            continue;
        }

        switch(buf.signal)
        {
            case SIGINT:
                /* Exit properly */
                gtk_main_quit();
                break;
            case SIGTERM:
                /* Kill all the applications we where handling in TN */
                hildon_navigator_killall();
            default:
                exit(0);
        }
    }
  
    /* 
     * Reading from the pipe has not returned with normal status. Check for 
     * potential errors and return from the callback.
     */
    if(error != NULL)
    {
        osso_log(LOG_ERR, "reading signal pipe failed: %s\n", error->message);
        g_error_free(error);
    }
    if(status == G_IO_STATUS_EOF)
    {
        osso_log(LOG_ERR, "signal pipe has been closed\n");
    }
    
    g_assert(status == G_IO_STATUS_AGAIN);
    return (TRUE);		/* keep the event source */
}


int maemo_af_desktop_main(int argc, char* argv[])
{
    GIOChannel *g_signal_in; 

    GError *error = NULL;	/* handle errors */
    long fd_flags; 	    /* used to change the pipe into non-blocking mode */
    gchar *gtkrc = NULL;

    setlocale (LC_ALL, "");

    bindtextdomain (PACKAGE, LOCALEDIR);
    textdomain (PACKAGE);
    
    /* Read the maemo-af-desktop gtkrc file */
    gtkrc = g_build_filename (g_get_home_dir (), 
                              OSSO_USER_DIR,
                              MAEMO_AF_DESKTOP_GTKRC,
                              NULL);
    
    if (gtkrc && g_file_test ((gtkrc), G_FILE_TEST_EXISTS))
    {
        gtk_rc_add_default_file (gtkrc);
    }

    g_free (gtkrc);

    /* If gtk_init was called already (maemo-launcher is used),
     * re-parse the gtkrc to include the maemo-af-desktop specific one */
    if (gdk_screen_get_default ())
    {
        gtk_rc_reparse_all_for_settings (
                gtk_settings_get_for_screen (gdk_screen_get_default ()),
                TRUE /* Force even if the files haven't changed */);
    }

    gtk_init (&argc, &argv);

    gnome_vfs_init();

        /* initialize osso */
    osso = osso_initialize( HILDON_STATUS_BAR_NAME, 
                            HILDON_STATUS_BAR_VERSION, FALSE, NULL );
    if( !osso )
    {  
        osso_log( LOG_ERR, "osso_initialize failed! Exiting.." );
        return 1;
    }
    

    tasknav = hildon_navigator_window_new ();

    gtk_widget_show_all (GTK_WIDGET (tasknav));
    /*task_navigator_main(&tasknav);*/

    keysnooper_id = 0;
    keysnooper_id=hildon_home_main();

    status_bar_main(osso, &panel);

    if(pipe(signal_pipe)) 
    {
        osso_log(LOG_ERR, "osso_af_desktop_main() pipe\n");
    }
    fd_flags = fcntl(signal_pipe[1], F_GETFL);
    if(fd_flags == -1)
    {
        osso_log(LOG_ERR, "osso_af_desktop_main() read descriptor flags\n");
    }
    if(fcntl(signal_pipe[1], F_SETFL, fd_flags | O_NONBLOCK) == -1)
    {
        osso_log(LOG_ERR, "osso_af_desktop_main() write descriptor flags\n");
    }

    /* Install the unix signal handler pipe_signals 
       for the signals of interest */
    signal(SIGINT, pipe_signals);
    signal(SIGTERM, pipe_signals);
    signal(SIGHUP, pipe_signals);
    
    g_signal_in = g_io_channel_unix_new(signal_pipe[0]);
    
    g_io_channel_set_encoding(g_signal_in, NULL, &error);
    if(error != NULL)
    {		/* handle potential errors */
        osso_log(LOG_ERR, "g_io_channel_set_encoding failed %s\n",
                 error->message);
        g_error_free(error);
    }
    
    /* put the reading end also into non-blocking mode */
    g_io_channel_set_flags(g_signal_in,     
                           g_io_channel_get_flags(g_signal_in) | 
                           G_IO_FLAG_NONBLOCK, 
                           &error);
    
    if(error != NULL)
    {		/* tread errors */
        osso_log(LOG_ERR, "g_io_set_flags failed %s\n", error->message);
        g_error_free(error);
    }

    /* register the reading end with the event loop */
    g_io_add_watch(g_signal_in, G_IO_IN | G_IO_PRI, deliver_signal, NULL);
    gtk_main();
    home_deinitialize(keysnooper_id);
    status_bar_deinitialize(osso,&panel);
    /*task_navigator_deinitialize(&tasknav);*/
    
    osso_deinitialize( osso );
      
    return 0;
}
    
int main( int argc, char* argv[] )
{
    
    return maemo_af_desktop_main(argc,argv);   
        
}


#endif
