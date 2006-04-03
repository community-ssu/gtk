/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005,2006 Nokia Corporation.
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


#include "hn-wm-watchable-app.h"
#include <hildon-widgets/hildon-banner.h>

/* watchable apps */

extern HNWM *hnwm;

struct HNWMWatchableApp
{
  gchar     *icon_name;
  gchar     *service;
  gchar     *app_name; 		/* window title */
  gchar     *bin_name; 		/* class || exec field ? */
  gchar     *class_name;        
  gboolean   able_to_hibernate; /* Can this be killed, but appear running ? */
  gboolean   hibernating;
  gboolean   is_minimised;
  gboolean   startup_notify;
  GtkWidget *ping_timeout_note; /* The note that is shown when the app quits responding */
  HNWMWatchedWindow * active_window;
};

static gboolean
hn_wm_watchable_app_has_windows_find_func (gpointer key,
					   gpointer value,
					   gpointer user_data);


static void
hn_wm_launch_banner_info_free (HNWMLaunchBannerInfo* info)
{
  g_return_if_fail(info);
  
  g_free(info->msg);
  g_free(info);
}

HNWMWatchableApp*
hn_wm_watchable_app_new (MBDotDesktop *desktop)
{
  HNWMWatchableApp *app;
  gchar            *service, *icon_name, *startup_notify;
  gchar            *startup_wmclass, *bin_name, *app_name;

  
  app_name = (gchar *) mb_dotdesktop_get(desktop, 
					 DESKTOP_VISIBLE_FIELD);
  startup_wmclass = (gchar *) mb_dotdesktop_get(desktop, 
						DESKTOP_SUP_WMCLASS);

  /* NOTE: DESKTOP_BIN_FIELD maps to exec  */
  bin_name = (gchar *) mb_dotdesktop_get(desktop,
					 DESKTOP_BIN_FIELD);
  
  if (app_name == NULL || (bin_name == NULL && startup_wmclass == NULL))
    {
      osso_log(LOG_ERR,
	       "HN: Desktop file has invalid fields");
      return NULL;
    }
  
  /* DESKTOP_LAUNCH_FIELD maps to X-Osso-Service */
  service = (gchar *) mb_dotdesktop_get(desktop, 
				     DESKTOP_LAUNCH_FIELD);
  icon_name = (char *) mb_dotdesktop_get(desktop, 
					 DESKTOP_ICON_FIELD);
  startup_notify = (char *)mb_dotdesktop_get(desktop,
					    DESKTOP_STARTUPNOTIFY); 
  
  app = g_new0 (HNWMWatchableApp, 1);

  if (!app)
    {
      mb_dotdesktop_free(desktop);
      return NULL;
    }

  app->icon_name      = g_strdup(icon_name); 
  app->service        = g_strdup(service);    
  app->app_name       = g_strdup(app_name); 
  app->startup_notify = TRUE;                /* Default */

  if (startup_notify)
    app->startup_notify = (g_ascii_strcasecmp(startup_notify, "false"));

  if (startup_wmclass != NULL)
    app->class_name = g_strdup(startup_wmclass);
  else
    /* note bin_name actually maps to the Exec key */
    app->class_name = g_path_get_basename(bin_name);

  app->active_window = NULL;
  HN_DBG("Registered new watchable app\n\tapp_name: %s\n\tclass name: %s\n\texec name (service): %s", 
	 app->app_name, app->class_name, app->service);

  return app;
}



static gboolean
hn_wm_watchable_app_has_windows_find_func (gpointer key,
					   gpointer value,
					   gpointer user_data)
{
  HNWMWatchedWindow *win;  

  win = (HNWMWatchedWindow*)value;

  HN_DBG("Checking %p vs %p", 
	 hn_wm_watched_window_get_app(win),
	 (HNWMWatchableApp *)user_data);

  if (hn_wm_watched_window_get_app(win) == (HNWMWatchableApp *)user_data)
    return TRUE;

  return FALSE;
}

gboolean
hn_wm_watchable_app_has_windows (HNWMWatchableApp *app)
{
  return (g_hash_table_find (hnwm->watched_windows,
			     hn_wm_watchable_app_has_windows_find_func,
			     (gpointer)app) != NULL);
}

gboolean
hn_wm_watchable_app_is_hibernating (HNWMWatchableApp *app)
{
  return app->hibernating;
}

void
hn_wm_watchable_app_set_hibernate (HNWMWatchableApp *app,
				   gboolean          hibernate)
{
  HN_MARK();
  app->hibernating = hibernate;
}


gboolean
hn_wm_watchable_app_is_able_to_hibernate (HNWMWatchableApp *app)
{
  return app->able_to_hibernate;
}

void
hn_wm_watchable_app_set_able_to_hibernate (HNWMWatchableApp *app,
					   gboolean          hibernate)
{
  app->able_to_hibernate = hibernate;
}


const gchar*
hn_wm_watchable_app_get_service (HNWMWatchableApp *app)
{
  return app->service;
}

const gchar*
hn_wm_watchable_app_get_name (HNWMWatchableApp *app)
{
  return app->app_name;
}

const gchar*
hn_wm_watchable_app_get_icon_name (HNWMWatchableApp *app)
{
  return app->icon_name;
}

const gchar*
hn_wm_watchable_app_get_class_name (HNWMWatchableApp *app)
{
  return app->class_name;
}


gboolean
hn_wm_watchable_app_is_minimised (HNWMWatchableApp *app)
{
  return app->is_minimised;
}

gboolean
hn_wm_watchable_app_has_startup_notify (HNWMWatchableApp *app)
{
  return app->startup_notify;
}


void
hn_wm_watchable_app_set_minimised (HNWMWatchableApp *app,
				   gboolean          minimised)
{
  app->is_minimised = minimised;
}

void
hn_wm_watchable_app_destroy (HNWMWatchableApp *app)
{
  ; /* Never currently happens */
}

/* Launch Banner Dialog */

/* FIXME: rename namespace to watched app */
  void 
hn_wm_watchable_app_launch_banner_show (GtkWidget        *parent, 
					/* FIXME: Unused ? */
					HNWMWatchableApp *app) 
{
  HNWMLaunchBannerInfo *info;
  guint                 interval;

  interval = APP_LAUNCH_BANNER_CHECK_INTERVAL * 1000;

  info = g_new0(HNWMLaunchBannerInfo, 1);

  info->app = app;

  gettimeofday( &info->launch_time, NULL );

  gdk_error_trap_push(); 	/* Needed ? */
        
  info->msg = g_strdup_printf(_( APP_LAUNCH_BANNER_MSG_LOADING ),
			     app->app_name ? _(app->app_name) : "" );
        
  info->banner = GTK_WIDGET(hildon_banner_show_animation(NULL, NULL, info->msg));
        
  gdk_error_trap_pop();

  g_timeout_add(interval, hn_wm_watchable_app_launch_banner_timeout, info);

}

void 
hn_wm_watchable_app_launch_banner_close (GtkWidget            *parent,
					 HNWMLaunchBannerInfo *info)
{
  if (!(info && info->msg))
    return;

  if(info->banner)
    gtk_widget_destroy(info->banner);
  
  hn_wm_launch_banner_info_free(info);
}

gboolean 
hn_wm_watchable_app_launch_banner_timeout (gpointer data)
{
  HNWMLaunchBannerInfo *info = data;
  struct timeval        current_time;
  long unsigned int     t1, t2;
  guint                 time_left;
  gulong                current_banner_timeout = 0;

  /* Added by Karoliina Salminen 26092005 
   * Addition to low memory situation awareness, the following 
   * multiplies the launch banner timeout with the timeout 
   * multiplier found from environment variable 
   */
  if(hnwm->lowmem_situation == TRUE) 
    current_banner_timeout = hnwm->lowmem_banner_timeout * hnwm->lowmem_timeout_multiplier;
  else 
    current_banner_timeout = hnwm->lowmem_banner_timeout;

  /* End of addition 26092005 */

#if 0 // needed ???
  if ( find_service_from_tree( hnwm->callbacks.model, 
			       &iter, 
			       info->service_name ) > 0) 
    {
    } else {
      /* This should never happen. Bail out! */
      return FALSE;
    }
#endif	

  gettimeofday( &current_time, NULL );

  t1 = (long unsigned int) info->launch_time.tv_sec;
  t2 = (long unsigned int) current_time.tv_sec;
  time_left = (guint) (t2 - t1);
	
  /* The following uses now current_banner_timeout instead of 
   * lowmem_banner_timeout, changed by
   * Karoliina Salminen 26092005 
   */
  if (time_left >= current_banner_timeout 
      || hn_wm_watchable_app_has_windows (info->app))
    {
      /* Close the banner */
      hn_wm_watchable_app_launch_banner_close( NULL, info );
      
      if (time_left >= APP_LAUNCH_BANNER_TIMEOUT 
	  && hnwm->lowmem_situation == TRUE)
	{
	  hildon_banner_show_information(NULL, NULL, _("memr_ib_unable_to_switch_to_application"));
	}
      
      return FALSE;
    }
  
  return TRUE;
}


void
hn_wm_watchable_app_hibernate (HNWMWatchableApp *app)
{
  g_return_if_fail(app);
  g_hash_table_foreach_steal(hnwm->watched_windows,
                             hn_wm_watched_window_hibernate_func,
                             app);
  
  hn_wm_watchable_app_set_hibernate (app, TRUE);
}

void
hn_wm_watchable_app_set_ping_timeout_note(HNWMWatchableApp *app, GtkWidget *note)
{
	app->ping_timeout_note = note;
}

GtkWidget*
hn_wm_watchable_app_get_ping_timeout_note(HNWMWatchableApp *app)
{
	return app->ping_timeout_note;
}

HNWMWatchedWindow *
hn_wm_watchable_app_get_active_window (HNWMWatchableApp *app)
{
  return app->active_window;
}

void
hn_wm_watchable_app_set_active_window (HNWMWatchableApp *app, HNWMWatchedWindow * win)
{
  app->active_window = win;
}
