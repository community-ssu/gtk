/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "hn-wm-watchable-app.h"

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
};

static gboolean
hn_wm_watchable_app_has_windows_find_func (gpointer key,
					   gpointer value,
					   gpointer user_data);

/* Simple stack to remember last message displayed that can get  
 * lost in certain situations with gtk_infoprints.
 * Probably better for gtk_infoprint to handle this..
 */
static void
banner_msg_stack_push (const gchar *msg)
{
  hnwm->banner_stack = g_list_append(hnwm->banner_stack, g_strdup(msg));
}

static const gchar*
banner_msg_stack_remove (const gchar *msg)
{
  GList *item;

  /* Remove a msg from stack and free. Return either the previous 
   * stored message
  */
  
  item = g_list_find_custom(hnwm->banner_stack, msg, (GCompareFunc)strcmp);

  if (item)
    {
      /* Free the msg */
      g_free(item->data);

      hnwm->banner_stack = g_list_remove_link(hnwm->banner_stack, item);

      g_list_free(item);
    }
  
   item = g_list_last(hnwm->banner_stack);
  
  if (item)
    return item->data;
  
  return NULL;
}

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
			     app->app_name ? app->app_name : "" );
        
  gtk_banner_show_animation( NULL, info->msg );

  /* Store a copy of message so we can work round multuple gtk_banner issues */
  banner_msg_stack_push (info->msg);
        
  gdk_error_trap_pop();

  g_timeout_add(interval, hn_wm_watchable_app_launch_banner_timeout, info);

}

void 
hn_wm_watchable_app_launch_banner_close (GtkWidget            *parent,
					 HNWMLaunchBannerInfo *info)
{
  const gchar *prev_msg;

  if (!(info && info->msg))
    return;

  /* Note: below may not close banner, works on refcnt */
  gtk_banner_close( NULL );
  
  /* remove message from stack and possibly get an old one to redisplay */
  prev_msg = banner_msg_stack_remove(info->msg);

  hn_wm_launch_banner_info_free(info);

  if (prev_msg)
    gtk_banner_set_text(NULL, prev_msg);
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
	  gtk_infoprintf(NULL, _("memr_ib_unable_to_switch_to_application"));
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
