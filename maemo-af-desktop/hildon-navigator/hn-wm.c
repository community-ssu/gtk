/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "hn-wm.h"

#define SAVE_METHOD      "save"
#define KILL_APPS_METHOD "kill_app"

static GdkFilterReturn
hn_wm_x_event_filter (GdkXEvent *xevent,
		      GdkEvent  *event,
		      gpointer   data);

static GHashTable*
hn_wm_watchable_apps_init (void);

static HNWMWatchableApp*
hn_wm_x_window_is_watchable (Window xid);

static void
hn_wm_process_x_client_list (void);

static gboolean
hn_wm_relaunch_timeout(gpointer data);

struct xwinv
{
  Window *wins;
  gint    n_wins;
};

HNWM *hnwm; 			/* Single, global soon to go... */

static gboolean
hn_wm_add_watched_window (HNWMWatchedWindow *win);

void
hn_wm_top_view (GtkMenuItem *menuitem)
{
  HNWMWatchedWindow     *win = NULL;
  HNWMWatchedWindowView *view = NULL;
  gboolean               view_found;
  HNWMWatchableApp      *app;

  win = hn_wm_lookup_watched_window_view (GTK_WIDGET(menuitem));

  if (win && hn_wm_watched_window_get_app (win))
    {
      GList *iter;

      app = hn_wm_watched_window_get_app (win);

      HN_DBG("Window found with views, is '%s'\n",
	     hn_wm_watched_window_get_name (win));

      if (hn_wm_watched_window_is_hibernating(win))
	{
	  HN_DBG("Window hibernating, calling top_service()");
	  hn_wm_top_service(hn_wm_watchable_app_get_service (app));
	  return;
	}
      
      iter = hn_wm_watched_window_get_views (win);
      view_found = FALSE;
      
      HN_DBG("Window has views finding out which needs activating");

      while (iter != NULL)
	{
	  view = (HNWMWatchedWindowView *)iter->data;
	  
	  if (hn_wm_watched_window_view_get_menu(view) == (GtkWidget*)menuitem)
	    {
	      view_found = TRUE;
	      break;
	    }
	  iter  = g_list_next(iter);
	}
      
      if (view_found)
	{
	  HN_DBG("found relevant view, sending hildon activate message");
	  
	  hn_wm_util_send_x_message (hn_wm_watched_window_view_get_id (view),
				     hn_wm_watched_window_get_x_win (win),
				     hnwm->atoms[HN_ATOM_HILDON_VIEW_ACTIVE],
				     SubstructureRedirectMask
				     |SubstructureNotifyMask,
				     0,
				     0,
				     0,
				     0,
				     0);
	  
	  hn_wm_watched_window_set_active_view(win, view);
	  
	  HN_DBG("... and topping service");
	  hn_wm_top_service(hn_wm_watchable_app_get_service (app));
	}
      
      return;
    }
  
  /* View wasn't found, maybe the widget is for a top level window
   * with no views
   */
  HN_DBG("### unable to find win via views ###");

  win = hn_wm_lookup_watched_window_via_menu_widget (GTK_WIDGET(menuitem));

  /* window doesn't have views - non hildon app maybe so top that via WM */
  if (win && hn_wm_watched_window_get_app (win))
    {
      XEvent ev;

      app = hn_wm_watched_window_get_app (win);

      HN_DBG("window is viewless");

      if (hn_wm_watchable_app_is_hibernating (app))
	{
	  HN_DBG("window is hibernating");
	  hn_wm_top_service(hn_wm_watchable_app_get_service (app));
	  return;
	}

      HN_DBG("toping non view window ( %li ) via _NET_ACTIVE_WINDOW message",
	     hn_wm_watched_window_get_x_win (win) );

      /* FIXME: hn_wm_util_send_x_message() should be used here but wont
       *         work!
       */
      memset(&ev, 0, sizeof(ev));

      ev.xclient.type         = ClientMessage;
      ev.xclient.window       = hn_wm_watched_window_get_x_win (win);
      ev.xclient.message_type = hnwm->atoms[HN_ATOM_NET_ACTIVE_WINDOW];
      ev.xclient.format       = 32;

      gdk_error_trap_push();
      XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
		 SubstructureRedirectMask, &ev);
      XSync(GDK_DISPLAY(),FALSE);
      gdk_error_trap_pop();

      return;
    }

  HN_DBG("### unable to find views ###");
}

void
hn_wm_top_service(const gchar *service_name)
{
  osso_manager_t    *osso_man;
  HNWMWatchedWindow *win;
  guint              pages_used = 0, pages_available = 0;

  HN_DBG(" Called with '%s'", service_name);

  if (service_name == NULL)
    {
      osso_log(LOG_ERR, "There was no service name!\n");
      return;
    }

  /* Check how much memory we do have until the lowmem threshold */

  if (!hn_wm_memory_get_limits (&pages_used, &pages_available))
    HN_DBG("### Failed to read memory limits, using scratchbox ??");

  /* Here we should compare the amount of pages to a configurable
   *  threshold. Value 0 means that we don't know and assume
   *  slightly riskily that we can start the app...
   */
  if (pages_available > 0 && pages_available < hnwm->lowmem_min_distance)
    {
      gtk_infoprintf(NULL, _("memr_ib_unable_to_switch_to_application"));
      return;
    }

  if ((win = hn_wm_lookup_watched_window_via_service (service_name)) == NULL)
    {
      /* We dont have a watched window for this service currently
       * so just launch it.
       */
      HN_DBG("unable to find service name '%s' in running wins", service_name);
      HN_DBG("Thus launcing via osso_manager_launch()");
      osso_man = osso_manager_singleton_get_instance();
      osso_manager_launch(osso_man, service_name, NULL);
      return;
    }
  else
    {
      HNWMWatchableApp      *app;
      HNWMWatchedWindowView *view = NULL;
      Window                 xid;

      app = hn_wm_watched_window_get_app (win);

      if (hn_wm_watched_window_is_hibernating(win))
	{
	  guint interval = LAUNCH_SUCCESS_TIMEOUT * 1000;
	  
	  HN_DBG("app is hibernating, attempting to reawaken"
		 "via osso_manager_launch()");
	  
	  hn_wm_watched_window_awake (win);
	  
	  /* FIXME: What does below do ?? */
	  if (hnwm->bg_kill_situation == TRUE)
	    {
	      HN_DBG("adding relaunch_timeout() callback");
	      g_timeout_add( interval,
			     hn_wm_relaunch_timeout,
			     (gpointer) g_strdup(service_name) );
	    }
	  return;
	}
      
      if (hn_wm_watched_window_get_views (win))
	{
	  view = hn_wm_watched_window_get_active_view(win);
	  
	  if (!view) /* There is no active so just grab the first one */
	    {
	      view = (HNWMWatchedWindowView *)((hn_wm_watched_window_get_views (win))->data);
	      hn_wm_watched_window_set_active_view(win, view);
	    }
	}
      
      HN_DBG("sending x message to activate app");
      
      if (view)
	xid = hn_wm_watched_window_view_get_id (view);
      else
	xid = hn_wm_watched_window_get_x_win (win);
      
      hn_wm_util_send_x_message (xid,
				 hn_wm_watched_window_get_x_win (win),
				 hnwm->atoms[HN_ATOM_HILDON_VIEW_ACTIVE],
				 SubstructureRedirectMask
				 |SubstructureNotifyMask,
				 0,
				 0,
				 0,
				 0,
				 0);
      
      /* FIXME: Should we just wait till its actually raised by wm ? */
      app_switcher_update_item (hnwm->app_switcher, win, view,
				AS_MENUITEM_TO_FIRST_POSITION);
    }
}


void
hn_wm_top_desktop(void)
{
  XEvent ev;

  memset(&ev, 0, sizeof(ev));

  ev.xclient.type         = ClientMessage;
  ev.xclient.window       = GDK_ROOT_WINDOW();
  ev.xclient.message_type = hnwm->atoms[HN_ATOM_NET_SHOWING_DESKTOP];
  ev.xclient.format       = 32;
  ev.xclient.data.l[0]    = 1;

  gdk_error_trap_push();
  XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
	     SubstructureRedirectMask, &ev);
  XSync(GDK_DISPLAY(),FALSE);
  gdk_error_trap_pop();
}


static void
hn_wm_atoms_init()
{
  /*
   *   The list below *MUST* be kept in the same order as the corresponding
   *   emun in tm-wm-types.h above or *everything* will break.
   *   Doing it like this avoids a mass of round trips on startup.
   */

  char *atom_names[] = {

    "WM_CLASS",			/* ICCCM */
    "WM_NAME",
    "WM_STATE",
    "WM_TRANSIENT_FOR",
    "WM_HINTS",
    "WM_WINDOW_ROLE",

    "_NET_WM_WINDOW_TYPE",	/* EWMH */
    "_NET_WM_WINDOW_TYPE_MENU",
    "_NET_WM_WINDOW_TYPE_NORMAL",
    "_NET_WM_WINDOW_TYPE_DIALOG",
    "_NET_WM_WINDOW_TYPE_DESKTOP",
    "_NET_WM_STATE",
    "_NET_WM_STATE_MODAL",
    "_NET_SHOWING_DESKTOP",
    "_NET_WM_PID",
    "_NET_ACTIVE_WINDOW",
    "_NET_CLIENT_LIST",
    "_NET_WM_ICON",
    "_NET_WM_USER_TIME",

    "_HILDON_APP_KILLABLE",	/* Hildon only props */
    "_HILDON_ABLE_TO_HIBERNATE",	/* alias for the above */
    "_NET_CLIENT_LIST",         /* NOTE: Hildon uses these values on app wins*/
    "_NET_ACTIVE_WINDOW",       /* for views, thus index is named different  */
                                /* to improve code readablity.               */
                                /* Ultimatly the props should be renamed with*/
                                /* a _HILDON prefix  */

    "_MB_WIN_SUB_NAME",		/* MB Only props */
    "_MB_COMMAND",              /* FIXME: Unused */
    "_MB_CURRENT_APP_WINDOW",
    "_MB_APP_WINDOW_LIST_STACKING",

    "UTF8_STRING",
  };

  XInternAtoms (GDK_DISPLAY(),
		atom_names,
		HN_ATOM_COUNT,
                False,
		hnwm->atoms);
}


static HNWMWatchableApp*
hn_wm_x_window_is_watchable (Window xid)
{
  HNWMWatchableApp *app;
  XClassHint        class_hint;
  Atom             *wm_type_atom;
  Status            status = 0;

  app = NULL;

  memset(&class_hint, 0, sizeof(XClassHint));

  gdk_error_trap_push();

  status == XGetClassHint(GDK_DISPLAY(), xid, &class_hint);
  
  if (gdk_error_trap_pop() || status != Success || class_hint.res_name == NULL)
    goto out;
  
  /* Does this window class belong to a 'watched' application ? */
  
  app = g_hash_table_lookup(hnwm->watched_apps,
			    (gconstpointer)class_hint.res_name);
  
  if (!app)
    goto out;
  
  if (hn_wm_watchable_app_is_hibernating(app))
    {
      HN_DBG(" ## new window referencing this app but its hibernating!");
      HN_DBG(" ## Must be sync issue with hash and WatchedWindow hash,");
      HN_DBG(" ## Aborting ");

      app = NULL;
      goto out;
    }
  
  /* FIXME: below checks are really uneeded assuming we trust new MB list prop
   */
  wm_type_atom
    = hn_wm_util_get_win_prop_data_and_validate (xid,
						 hnwm->atoms[HN_ATOM_NET_WM_WINDOW_TYPE],
						 XA_ATOM,
						 32,
						 0,
						 NULL);
  if (!wm_type_atom)
    {
      app = NULL;
      goto out;
    }

  /* Only care about desktop and app wins */
  if (wm_type_atom[0] != hnwm->atoms[HN_ATOM_NET_WM_WINDOW_TYPE_NORMAL]
      && wm_type_atom[0] != hnwm->atoms[HN_ATOM_NET_WM_WINDOW_TYPE_DESKTOP])
    {
      app = NULL;
    }

  XFree(wm_type_atom);

 out:
  
  if (class_hint.res_class)
    XFree(class_hint.res_class);
  
  if (class_hint.res_name)
    XFree(class_hint.res_name);

  return app;
}

/* various lookup functions. */

static gboolean
hn_wm_lookup_watched_window_via_service_find_func (gpointer key,
						   gpointer value,
						   gpointer user_data)
{
  HNWMWatchedWindow *win;
  HNWMWatchableApp  *app;

  win = (HNWMWatchedWindow*)value;

  if (win == NULL || user_data == NULL)
    return FALSE;

  app = hn_wm_watched_window_get_app (win);

  if (!app)
    return FALSE;

  if (hn_wm_watchable_app_get_service (app)
      && !strcmp(hn_wm_watchable_app_get_service (app), (gchar*)user_data))
    return TRUE;

  return FALSE;
}

HNWMWatchedWindow*
hn_wm_lookup_watched_window_via_service (const gchar *service_name)
{
  HNWMWatchedWindow *win = NULL;

  win = g_hash_table_find (hnwm->watched_windows,
			   hn_wm_lookup_watched_window_via_service_find_func,
			   (gpointer)service_name);
  
  if (!win)
    {
      /* Maybe its stored in our hibernating hash */
      win
	= g_hash_table_find (hnwm->watched_windows_hibernating,
			     hn_wm_lookup_watched_window_via_service_find_func,
			     (gpointer)service_name);
    }
  
  return win;
}

static gboolean
hn_wm_lookup_watched_window_via_menu_widget_find_func (gpointer key,
                                                       gpointer value,
                                                       gpointer user_data)
{
  HNWMWatchedWindow *win;
  
  win = (HNWMWatchedWindow*)value;

  if (hn_wm_watched_window_get_menu (win) == (GtkWidget*)user_data)
    return TRUE;

  return FALSE;
}


HNWMWatchedWindow*
hn_wm_lookup_watched_window_via_menu_widget (GtkWidget *menu_widget)
{
  HNWMWatchedWindow *win = NULL;

  win
    = g_hash_table_find (hnwm->watched_windows,
			 hn_wm_lookup_watched_window_via_menu_widget_find_func,
			 (gpointer)menu_widget);

  if (!win)
    {
      /* Maybe its stored in our hibernating hash
       */
      win = g_hash_table_find (hnwm->watched_windows_hibernating,
			       hn_wm_lookup_watched_window_via_menu_widget_find_func,
			       (gpointer)menu_widget);
    }
  
  return win;
}

static gboolean
hn_wm_lookup_watchable_app_via_service_find_func (gpointer key,
						  gpointer value,
						  gpointer user_data)
{
  HNWMWatchableApp *app;

  app = (HNWMWatchableApp *)value;

  if (app == NULL || user_data == NULL)
    return FALSE;

  if (hn_wm_watchable_app_get_service (app) == NULL)
    return FALSE;

  if (hn_wm_watchable_app_get_service (app) &&
      !strcmp(hn_wm_watchable_app_get_service (app), (gchar*)user_data))
    return TRUE;

  return FALSE;
}

HNWMWatchableApp*
hn_wm_lookup_watchable_app_via_service (const gchar *service_name)
{
  return g_hash_table_find ( hnwm->watched_apps,
			     hn_wm_lookup_watchable_app_via_service_find_func,
			     (gpointer)service_name);
}

HNWMWatchableApp*
hn_wm_lookup_watchable_app_via_menu (GtkWidget *menu)
{
  HNWMWatchedWindow     *win;

  win = hn_wm_lookup_watched_window_via_menu_widget (menu);

  if (!win)
    win = hn_wm_lookup_watched_window_view (menu);

  if (win)
    return hn_wm_watched_window_get_app (win);

  return NULL;
}

static gboolean
hn_wm_lookup_watched_window_view_find_func (gpointer key,
					    gpointer value,
					    gpointer user_data)
{
  HNWMWatchedWindow *win;
  GList             *iter;

  win = (HNWMWatchedWindow*)value;

  iter = hn_wm_watched_window_get_views (win);
  
  while (iter != NULL)
    {
      HNWMWatchedWindowView *view;

      view = (HNWMWatchedWindowView *)iter->data;

      if (hn_wm_watched_window_view_get_menu (view) == (GtkWidget*)user_data)
	return TRUE;

      iter  = g_list_next(iter);
    }

  return FALSE;
}

HNWMWatchedWindow*
hn_wm_lookup_watched_window_view (GtkWidget *menu_widget)
{
  HNWMWatchedWindow *win;

  win = g_hash_table_find ( hnwm->watched_windows,
			    hn_wm_lookup_watched_window_view_find_func,
			    (gpointer)menu_widget);
  
  if (!win)
    {
      HN_DBG("checking WatchedWindowsHibernating hash, has %i items",
	     g_hash_table_size (hnwm->watched_windows_hibernating));

      win = g_hash_table_find ( hnwm->watched_windows_hibernating,
				hn_wm_lookup_watched_window_view_find_func,
				(gpointer)menu_widget);
    }
  
  return win;
}


/* Root win property changes */

static void
hn_wm_process_mb_current_app_window (void)
{
  static Window      previous_app_xwin = 0;

  HNWMWatchedWindow *win;
  Window            *app_xwin;

  app_xwin =  hn_wm_util_get_win_prop_data_and_validate (GDK_ROOT_WINDOW(),
							 hnwm->atoms[HN_ATOM_MB_CURRENT_APP_WINDOW],
							 XA_WINDOW,
							 32,
							 0,
							 NULL);
  if (!app_xwin || *app_xwin == previous_app_xwin)
    return;

  previous_app_xwin = *app_xwin;

  win = g_hash_table_lookup(hnwm->watched_windows, (gconstpointer)app_xwin);
  
  if (win)
    {
      HNWMWatchableApp *app;
      
      app = hn_wm_watched_window_get_app (win);
      
      if (!app)
	return;
      
      /* Note: this is whats grouping all views togeather */
      if (hn_wm_watched_window_get_views (win))
	{
	  GList      *iter;
	  iter = hn_wm_watched_window_get_views (win);
	  
	  while (iter != NULL)
	    {
	      HNWMWatchedWindowView *view;
	      
	      view = (HNWMWatchedWindowView *)iter->data;
	      
	      app_switcher_update_item (hnwm->app_switcher, win, view,
					AS_MENUITEM_TO_FIRST_POSITION);
	      iter  = g_list_next(iter);
	    }
	}
      else
	{
	  /* Window with no views */
	  app_switcher_update_item (hnwm->app_switcher, win, NULL,
				    AS_MENUITEM_TO_FIRST_POSITION);
	}
    }
  
  XFree(app_xwin);
}

static gboolean
client_list_remove_foreach_func (gpointer key,
				 gpointer value,
				 gpointer userdata)
{
  HNWMWatchedWindow   *win;
  struct xwinv *xwins;
  int    i;
  
  xwins = (struct xwinv*)userdata;
  win   = (HNWMWatchedWindow *)value;
  
  for (i=0; i < xwins->n_wins; i++)
    if (G_UNLIKELY((xwins->wins[i] == hn_wm_watched_window_get_x_win (win))))
      return FALSE;

  return TRUE;
}

static void
hn_wm_process_x_client_list(void)
{
  struct xwinv xwins;
  int     i;

  /* FIXME: We (or MB!) should probably keep a copy of ordered window list
   * that way we can check against new list for changes before to save
   * some hash lookups etc.
   *
   * Also now we have the list in stacking order could use this to update
   * app switcher ordering more efficiently, but need to figure out how
   * that works first.
   */

  xwins.wins
    = hn_wm_util_get_win_prop_data_and_validate (GDK_ROOT_WINDOW(),
						 hnwm->atoms[HN_ATOM_MB_APP_WINDOW_LIST_STACKING],
						 XA_WINDOW,
						 32,
						 0,
						 &xwins.n_wins);
  if (G_UNLIKELY(xwins.wins == NULL))
    {
      g_warning("Failed to read _MB_APP_WINDOW_LIST_STACKING root win prop, "
		"you probably need a newer matchbox !!!");
    }

  /* Check if any windows in our hash have since dissapeared */
  g_hash_table_foreach_remove ( hnwm->watched_windows,
				client_list_remove_foreach_func,
				(gpointer)&xwins);
  
  /* Now add any new ones  */
  for (i=0; i < xwins.n_wins; i++)
    {
      if (!g_hash_table_lookup(hnwm->watched_windows,
			       (gconstpointer)&xwins.wins[i]))
	{
	  HNWMWatchedWindow   *win;
	  HNWMWatchableApp    *app;
	  
	  /* We've found a window thats listed but not currently watched.
           * Check if it is watchable by us
	   */
	  app = hn_wm_x_window_is_watchable (xwins.wins[i]);
	  
	  if (!app)
	    continue;
	  
	  win = hn_wm_watched_window_new (xwins.wins[i], app);
	  
	  if (!win)
	    continue;

	  if (!hn_wm_add_watched_window(win))
	    continue; 		/* XError likely occured, xwin gone */

	  /* Grab the view prop from the window and add any views.
	   * Note this will add menu items for em.
	   */
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_HILDON_VIEW_LIST);

	  if (hn_wm_watched_window_get_views (win) == NULL
	      && hn_wm_watched_window_get_menu (win) == NULL)
	    {
	      GtkWidget *menu_item;
	      /* This window has no views so we set up a button for
	       * its regular window
	       */
	      HN_DBG("### adding a viewless window ###");

	      menu_item = app_switcher_add_new_item (hnwm->app_switcher,
						     win,
						     NULL);
	      
	      hn_wm_watched_window_set_menu (win, menu_item);
	    }
	}
    }

  if (xwins.wins)
    XFree(xwins.wins);
}

gboolean
hn_wm_add_watched_window (HNWMWatchedWindow *win)
{
  GdkWindow  *gdk_wrapper_win = NULL;
  gint       *key;

  key = g_new0 (gint, 1);
  
  if (!key) 	 /* FIXME: Handle OOM */
    return FALSE;
  
  *key = hn_wm_watched_window_get_x_win(win);
  
  gdk_error_trap_push();
  
  gdk_wrapper_win = gdk_window_foreign_new (hn_wm_watched_window_get_x_win(win));
  
  if (gdk_wrapper_win != NULL)
    {
      /* Monitor the window for prop changes */
      gdk_window_set_events(gdk_wrapper_win,
			    gdk_window_get_events(gdk_wrapper_win)
			    | GDK_PROPERTY_CHANGE_MASK);
      
      gdk_window_add_filter(gdk_wrapper_win,
			    hn_wm_x_event_filter,
			    NULL);
    }
  
  XSync(GDK_DISPLAY(), False);  /* FIXME: Check above does not sync */
  
  if (gdk_error_trap_pop() || gdk_wrapper_win == NULL)
    goto abort;
  
  g_object_unref(gdk_wrapper_win);

  g_hash_table_insert (hnwm->watched_windows, key, (gpointer)win);

  return TRUE;

 abort:

  if (win)
    hn_wm_watched_window_destroy (win);

  if (gdk_wrapper_win)
    g_object_unref(gdk_wrapper_win);
  
  return FALSE;
}


/* Main event filter  */

static GdkFilterReturn
hn_wm_x_event_filter (GdkXEvent *xevent,
		      GdkEvent  *event,
		      gpointer   data)
{
  XPropertyEvent *prop;
  HNWMWatchedWindow     *win = NULL;

  /* If this isn't a property change event ignore ASAP */
  if (((XEvent*)xevent)->type != PropertyNotify)
    return GDK_FILTER_CONTINUE;

  prop = (XPropertyEvent*)xevent;

  /* Root window property change */

  if (G_LIKELY(prop->window == GDK_ROOT_WINDOW()))
    {
      if (prop->atom == hnwm->atoms[HN_ATOM_MB_APP_WINDOW_LIST_STACKING])
	{
	  hn_wm_process_x_client_list();
	}
      else if (prop->atom == hnwm->atoms[HN_ATOM_MB_CURRENT_APP_WINDOW])
	{
	  hn_wm_process_mb_current_app_window ();
	}
      else if (prop->atom == hnwm->atoms[HN_ATOM_NET_SHOWING_DESKTOP])
	{
	  int *desktop_state;

	  desktop_state =
	    hn_wm_util_get_win_prop_data_and_validate (GDK_WINDOW_XID(gdk_get_default_root_window()),
						       hnwm->atoms[HN_ATOM_NET_SHOWING_DESKTOP],
						       XA_CARDINAL,
						       32,
						       1,
						       NULL);
	  if (desktop_state)
	    {
	      if (desktop_state[0] == 1)
		app_switcher_top_desktop_item (hnwm->app_switcher);
	      XFree(desktop_state);
	    }
	}
    }
  else /* Non root win prop change */
    {
      /* Hopefully one of our watched windows changing a prop..
       *
       * Check if its an atom were actually interested in
       * before doing the assumed to be more expensive hash
       * lookup. FIXME: hmmm..
       */

      if ( prop->atom == hnwm->atoms[HN_ATOM_WM_NAME]
	   || prop->atom == hnwm->atoms[HN_ATOM_WM_STATE]
	   || prop->atom == hnwm->atoms[HN_ATOM_HILDON_VIEW_LIST]
	   || prop->atom == hnwm->atoms[HN_ATOM_HILDON_VIEW_ACTIVE]
	   || prop->atom == hnwm->atoms[HN_ATOM_HILDON_APP_KILLABLE]
	   || prop->atom == hnwm->atoms[HN_ATOM_HILDON_ABLE_TO_HIBERNATE]
	   || prop->atom == hnwm->atoms[HN_ATOM_NET_WM_STATE]
	   || prop->atom == hnwm->atoms[HN_ATOM_WM_HINTS]
	   || prop->atom == hnwm->atoms[HN_ATOM_NET_WM_ICON]
           || prop->atom == hnwm->atoms[HN_ATOM_MB_WIN_SUB_NAME]
           || prop->atom == hnwm->atoms[HN_ATOM_WM_WINDOW_ROLE])
        
	{
	  win = g_hash_table_lookup(hnwm->watched_windows,
				    (gconstpointer)&prop->window);
	}

      if (!win)
	return GDK_FILTER_CONTINUE;

      if (prop->atom == hnwm->atoms[HN_ATOM_WM_NAME]
          || prop->atom == hnwm->atoms[HN_ATOM_MB_WIN_SUB_NAME])
	{
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_NAME);
	}
      else if (prop->atom == hnwm->atoms[HN_ATOM_WM_STATE])
	{
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_WM_STATE);
	}
      else if (prop->atom == hnwm->atoms[HN_ATOM_NET_WM_ICON])
	{
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_ICON);
	}
      else if (prop->atom == hnwm->atoms[HN_ATOM_HILDON_VIEW_ACTIVE])
	{
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_HILDON_VIEW_ACTIVE);
	}
      else if (prop->atom == hnwm->atoms[HN_ATOM_WM_HINTS])
	{
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_WMHINTS);
	}
      else if (prop->atom == hnwm->atoms[HN_ATOM_HILDON_VIEW_LIST])
	{
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_HILDON_VIEW_LIST);
	}
      else if (prop->atom == hnwm->atoms[HN_ATOM_WM_WINDOW_ROLE])
	{
	  /* Windows realy shouldn't do this... */
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_WINDOW_ROLE);
	}
      else if (prop->atom == hnwm->atoms[HN_ATOM_HILDON_APP_KILLABLE]
			   || prop->atom == hnwm->atoms[HN_ATOM_HILDON_ABLE_TO_HIBERNATE])
	{
	  HNWMWatchableApp *app;

	  app = hn_wm_watched_window_get_app(win);

	  if (prop->state == PropertyDelete)
	    {
	      hn_wm_watchable_app_set_able_to_hibernate (app, FALSE);
	    }
	  else
	    {
	      hn_wm_watched_window_props_sync (win,
					       HN_WM_SYNC_HILDON_APP_KILLABLE);
	    }
	}
    }

  return GDK_FILTER_CONTINUE;
}


/* DBus related callbacks */

static DBusHandlerResult
hn_wm_dbus_method_call_handler (DBusConnection *connection,
				DBusMessage    *message,
				void           *data )
{
  /* Catch APP_LAUNCH_BANNER_METHOD */
  if (dbus_message_is_method_call (message,
				   APP_LAUNCH_BANNER_METHOD_INTERFACE,
				   APP_LAUNCH_BANNER_METHOD ) )
    {
      DBusError         error;
      gchar            *service_name = NULL;
      gchar            *service = NULL;
      HNWMWatchableApp *app;
      
      dbus_error_init (&error);
      
      dbus_message_get_args (message,
			     &error,
			     DBUS_TYPE_STRING,
			     &service_name,
			     DBUS_TYPE_INVALID );

      if (dbus_error_is_set (&error))
	{
	  osso_log(LOG_WARNING, "Error getting message args: %s\n",
		   error.message);
	  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

      if (!g_str_has_prefix(service_name, SERVICE_PREFIX))
	{
	  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

      service = g_strdup (service_name + strlen(SERVICE_PREFIX));

      HN_DBG("Checking if service: '%s' is watchable", service);

      /* Is this 'service' watchable ? */
      if ((app = hn_wm_lookup_watchable_app_via_service (service)) != NULL)
	{
	  if (hn_wm_watchable_app_has_startup_notify (app)
	      && hnwm->lowmem_banner_timeout > 0
	      /* FIXME: is_hibernating() should just work here rather
	       *        than hash lookup - must avoid showing banner
               *        for re launched apps.
	       */
	      && !g_hash_table_lookup(hnwm->watched_windows_hibernating,
				      hn_wm_watchable_app_get_class_name (app))
	      && !hn_wm_watchable_app_has_windows (app))
	    {
	      HN_DBG("Showing Launchbanner...");
	      hn_wm_watchable_app_launch_banner_show ( NULL, app );
	    }
	}
      g_free (service);
    }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/* Application switcher callback funcs */

static void
hn_wm_dnotify_func(MBDotDesktop *desktop)
{
  HNWMWatchableApp *app;

  app = hn_wm_watchable_app_new (desktop);

  /* FIXME: what if this update already exists ? */
  if (app)
    {
      g_hash_table_insert (hnwm->watched_apps,
			   g_strdup(hn_wm_watchable_app_get_class_name (app)),
			   (gpointer)app);
    }
}

static int
hn_wm_osso_kill_method(GArray *arguments, gpointer data)
{
  gchar             *appname = NULL, *operation = NULL;
  HNWMWatchedWindow *win = NULL;
  HNWMWatchableApp  *app;

  if (arguments->len < 1)
    return 1;

  operation = (gchar *)g_array_index(arguments, osso_rpc_t, 0).value.s;

  /* In this case, HN will kill every process that has supposedly
     statesaved */

  if (operation == NULL)
    return 1;

  if (strcmp(operation, "lru") == 0)
    {
      return hn_wm_memory_kill_lru();
    }
  else if (strcmp(operation, "all") == 0)
    {
      return hn_wm_memory_kill_all_watched(TRUE);
    }
  else if (strcmp(operation, "app") != 0 || (arguments->len < 2) )
    {
      return 1;
    }

  /* Kill a certain application */

  appname = (gchar *)g_array_index(arguments, osso_rpc_t, 1).value.s;

  if (appname == NULL)
    return 1;

  win = hn_wm_lookup_watched_window_via_service (appname);
  app = hn_wm_watched_window_get_app (win);

  if (win)
    {
      if (hn_wm_watchable_app_is_able_to_hibernate (app))
	hn_wm_watched_window_attempt_pid_kill (win);
    }

  return 0;
}

static gboolean
hn_wm_relaunch_timeout(gpointer data)
{
  gchar             *service_name = (gchar *)data;
  HNWMWatchedWindow *win = NULL;

  win = hn_wm_lookup_watched_window_via_service (service_name);

  if (!win && hnwm->lowmem_situation)
    {
      gtk_infoprintf(NULL, _("memr_ib_unable_to_switch_to_application"));
    }

  g_free(service_name);

  return FALSE;
}

static GHashTable*
hn_wm_watchable_apps_init (void)
{
  GHashTable        *watchable_apps;
  HNWMWatchableApp  *app;
  DIR               *directory;
  struct dirent     *entry = NULL;

  if ((directory = opendir(DESKTOPENTRYDIR)) == NULL)
    {
      HN_DBG(" ##### Failed in opening " DESKTOPENTRYDIR " ##### ");
      return NULL;
    }

  watchable_apps = g_hash_table_new_full (g_str_hash,
					  g_str_equal,
					  (GDestroyNotify)g_free,
					  (GDestroyNotify)hn_wm_watchable_app_destroy);

  while ((entry = readdir(directory)) != NULL)
    {
      gchar        *path;
      MBDotDesktop *desktop;

      if (!g_str_has_suffix(entry->d_name, DESKTOP_SUFFIX))
	continue;

      path = g_build_filename(DESKTOPENTRYDIR, entry->d_name, NULL);

      desktop = mb_dotdesktop_new_from_file(path);

      if (!desktop)
	{
	  osso_log(LOG_WARNING, "Could not open [%s]\n", path);
	  continue;
	}

      app = hn_wm_watchable_app_new (desktop);

      if (app)
	{
	  g_hash_table_insert (watchable_apps,
			       g_strdup(hn_wm_watchable_app_get_class_name (app)),
			       (gpointer)app);
	}

      mb_dotdesktop_free(desktop);
      g_free(path);
    }

  closedir(directory);

  return watchable_apps;
}

gchar*
hn_wm_compute_watched_window_hibernation_key (Window            xwin,
					      HNWMWatchableApp *app)
{
  gchar *role, *hibernation_key = NULL;

  HN_DBG("#### computing hibernation key ####");

  g_return_val_if_fail(app, NULL);

  gdk_error_trap_push();

  role = hn_wm_util_get_win_prop_data_and_validate (xwin,
						    hnwm->atoms[HN_ATOM_WM_WINDOW_ROLE],
						    XA_STRING,
						    8,
						    0,
						    NULL);

  if (!role || !*role || gdk_error_trap_pop())
    hibernation_key = g_strdup(hn_wm_watchable_app_get_class_name (app));
  else
    hibernation_key = g_strdup_printf("%s%s", 
				      hn_wm_watchable_app_get_class_name (app),
				      role);
  if (role)
    XFree(role);

  HN_DBG("#### hibernation key: %s ####", hibernation_key);

  return hibernation_key;
}

gboolean
hn_wm_init (ApplicationSwitcher_t *as)
{
  DBusConnection *connection;
  DBusError       error;
  gchar          *match_rule = NULL;

  hnwm = g_new0(HNWM, 1);

  hnwm->bg_kill_situation = FALSE;

  osso_manager_t *osso_man = osso_manager_singleton_get_instance();

  hnwm->app_switcher = as;

  /* Check for configurable lowmem values. */

  hnwm->lowmem_min_distance
    = hn_wm_util_getenv_long (LOWMEM_LAUNCH_THRESHOLD_DISTANCE_ENV,
			      LOWMEM_LAUNCH_THRESHOLD_DISTANCE);
  hnwm->lowmem_banner_timeout
    = hn_wm_util_getenv_long (LOWMEM_LAUNCH_BANNER_TIMEOUT_ENV,
			      LOWMEM_LAUNCH_BANNER_TIMEOUT);

  /* Guard about insensibly long values. */
  if (hnwm->lowmem_banner_timeout > LOWMEM_LAUNCH_BANNER_TIMEOUT_MAX)
    hnwm->lowmem_banner_timeout = LOWMEM_LAUNCH_BANNER_TIMEOUT_MAX;

  hnwm->lowmem_timeout_multiplier
    = hn_wm_util_getenv_long (LOWMEM_TIMEOUT_MULTIPLIER_ENV,
			      LOWMEM_TIMEOUT_MULTIPLIER);

  /* Various app switcher callbacks */

  application_switcher_set_dnotify_handler (as, &hn_wm_dnotify_func);
  application_switcher_set_shutdown_handler (as, &hn_wm_shutdown_func);
  application_switcher_set_lowmem_handler (as, &hn_wm_memory_lowmem_func);
  application_switcher_set_bgkill_handler (as, &hn_wm_memory_bgkill_func);

  /* build our hash of watchable apps via .desktop key/values */

  hnwm->watched_apps = hn_wm_watchable_apps_init ();

  /* Initialize the common X atoms */

  hn_wm_atoms_init();

  /* Hash to track watched windows */

  hnwm->watched_windows
    = g_hash_table_new_full (g_int_hash,
			     g_int_equal,
			     (GDestroyNotify)g_free,
			     (GDestroyNotify)hn_wm_watched_window_destroy);

  /* Hash for windows that dont really still exists but HN makes them appear
   * as they do - they are basically backgrounded.
   */
  hnwm->watched_windows_hibernating
    = g_hash_table_new_full (g_str_hash,
			     g_str_equal,
			     (GDestroyNotify)g_free,
			     (GDestroyNotify)hn_wm_watched_window_destroy);

  gdk_error_trap_push();

  /* select X events */

  gdk_window_set_events(gdk_get_default_root_window(),
			gdk_window_get_events(gdk_get_default_root_window())
			| GDK_PROPERTY_CHANGE_MASK );

  gdk_window_add_filter(gdk_get_default_root_window(),
			hn_wm_x_event_filter,
			NULL);

  gdk_error_trap_pop();

  /* Get on the DBus */

  dbus_error_init (&error);

  connection = dbus_bus_get( DBUS_BUS_SESSION, &error );

  if (!connection )
    {
      osso_log(LOG_ERR, "Failed to connect to DBUS: %s!\n", error.message );
      dbus_error_free( &error );
    }
  else
    {
      /* Match rule */

      match_rule = g_strdup_printf("type='method',interface='%s'",
				   APP_LAUNCH_BANNER_METHOD_INTERFACE );

      dbus_bus_add_match( connection, match_rule, NULL );

      dbus_connection_add_filter( connection, hn_wm_dbus_method_call_handler,
				  /* model */ NULL, NULL );
      g_free(match_rule);

      dbus_connection_flush(connection);

      /* Add the com.nokia.tasknav callbacks - FIXME: what are these for ? */
      add_method_cb(osso_man, KILL_APPS_METHOD,
		    &hn_wm_osso_kill_method, osso_man);

      add_method_cb(osso_man, SAVE_METHOD,
		    &hn_wm_session_save, osso_man);
    }

  /* We need to postpone the restoration of applications until
     maemo-af-desktop initialization is done, as it will
     behave erratically under unified desktop otherwise... */
  /* FIXME: Check whats happening here */
  g_timeout_add(5000, hn_wm_session_restore_via_timeout, (gpointer)osso_man);

  return TRUE;
}

