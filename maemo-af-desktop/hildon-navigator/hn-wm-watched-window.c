#include "hn-wm-watched-window.h"
#include "osso-manager.h"

#include <X11/Xutil.h> 		/* For WMHints */

/* Application relaunch indicator data*/
#define RESTORED "restored"

struct HNWMWatchedWindow
{
  Window                  xwin;         
  gchar                  *name;
  GtkWidget              *menu_widget;   /* Active if no views */
  HNWMWatchableApp       *app_parent;
  GList                  *views; 
  HNWMWatchedWindowView  *view_active;
  GdkPixbuf              *pixb_icon;
  Window                  xwin_group;
  gboolean                is_urgent;
};

struct xwinv
{
  Window *wins;
  gint    n_wins;
};

extern HNWM *hnwm;

/* NOTE: for below caller traps x errors */
static void
hn_wm_watched_window_process_wm_state (HNWMWatchedWindow *win);

static void 
hn_wm_watched_window_process_hildon_view_list (HNWMWatchedWindow *win);

static void
hn_wm_watched_window_process_wm_name (HNWMWatchedWindow *win);

static void
hn_wm_watched_window_process_hibernation_prop (HNWMWatchedWindow *win);

static void
hn_wm_watched_window_process_wm_hints (HNWMWatchedWindow *win);

static void
hn_wm_watched_window_process_net_wm_icon (HNWMWatchedWindow *win);

static void
pixbuf_destroy (guchar *pixels, gpointer data)
{
  /* For hn_wm_watched_window_process_net_wm_icon  */
  g_free(pixels);
}

static void
hn_wm_watched_window_process_net_wm_icon (HNWMWatchedWindow *win)
{
  gulong *data;
  gint    len = 0, offset, w, h, i;
  guchar *rgba_data, *p;

  rgba_data = p = NULL;

  data 
    = hn_wm_util_get_win_prop_data_and_validate (hn_wm_watched_window_get_x_win (win),
						 hnwm->atoms[HN_ATOM_NET_WM_ICON],
						 XA_CARDINAL,
						 0,
						 0,
						 &len);

  HN_DBG("#### grabbing NET icon ####");

  if (data == NULL || len < 2)
    goto out;

  offset = 0;

  /* Do we have an icon of required size ?
   * NOTE: ICON_SIZE set in application-switcher.h, defaults to 26 
   * FIXME: figure out best way to handle scaling here
  */
  do
    {
      w = data[offset]; 
      h = data[offset+1];
      
      HN_DBG("got w:%i, h:%im offset is %i\n", w, h, offset);

      if (w == ICON_SIZE && h == ICON_SIZE)
	break;

      offset += ((w*h) + 2);
    }
  while (offset < len);

  if (offset >= len)
    {
      HN_DBG("w,h not found");
      goto out;
    }

  HN_DBG("#### w,h ok ####");

  p = rgba_data = g_new (guchar, w * h * 4);

  i = offset+2;

  while (i < (w*h))
    {
      *p++ = (data[i] >> 16) & 0xff;
      *p++ = (data[i] >> 8) & 0xff;
      *p++ = data[i] & 0xff;
      *p++ = data[i] >> 24;
      i++;
    }

  if (win->pixb_icon)
    g_object_unref(win->pixb_icon);

  win->pixb_icon = gdk_pixbuf_new_from_data (rgba_data,
					     GDK_COLORSPACE_RGB,
					     TRUE,
					     8,
					     w, h, w * 4,
					     pixbuf_destroy,
					     NULL);

  if (win->pixb_icon == NULL)
    {
      HN_DBG("#### win->pixb_icon == NULL ####");
      g_free(rgba_data);
      goto out;
    }

  /* FIXME: need to just update icon, also could be broke for views */
  app_switcher_item_icon_sync (hnwm->app_switcher, win);

 out:
  if (data)
    XFree(data);
}

static void 
hn_wm_watched_window_process_hildon_view_active (HNWMWatchedWindow *win)
{
  Window                *new_active_view_id;
  HNWMWatchedWindowView *current_active_view;
  GList                 *iter = NULL;
  HNWMWatchableApp      *app = NULL;

  if (hn_wm_watched_window_get_views (win) == NULL)
    return;

  app = hn_wm_watched_window_get_app (win);

  if (!app)
    return;

  new_active_view_id 
    = hn_wm_util_get_win_prop_data_and_validate (hn_wm_watched_window_get_x_win (win),
						 hnwm->atoms[HN_ATOM_HILDON_VIEW_ACTIVE],
						 XA_WINDOW,
						 32,
						 0,
						 NULL);

  if (!new_active_view_id)
    return;
  
  current_active_view = hn_wm_watched_window_get_active_view (win);

  /* Check the prop value is valid and not alreday active */

  if (current_active_view 
      && hn_wm_watched_window_view_get_id(current_active_view) == *new_active_view_id)
    goto out;

  iter = hn_wm_watched_window_get_views (win);

  /* Find what the view id matches for this window's views and  
   * update.
  */
  while (iter != NULL)
    {
      HNWMWatchedWindowView *view;
      
      view = (HNWMWatchedWindowView *)iter->data;
      
      if (hn_wm_watched_window_view_get_id (view) == *new_active_view_id)
	{
	  app_switcher_update_item (hnwm->app_switcher, win, view,
				    AS_MENUITEM_TO_FIRST_POSITION);

	  hn_wm_watched_window_set_active_view (win, view);
	  goto out;
	}

      iter  = g_list_next(iter);
    }

out:
  
  if (new_active_view_id)
    XFree(new_active_view_id);

  return;
}

static void
hn_wm_watched_window_process_wm_name (HNWMWatchedWindow *win)
{
  HNWMWatchedWindowView *view;

  if (win->name) 
    XFree(win->name);
  
  win->name = NULL;

  /* FIXME: handle UTF8 naming */
  XFetchName(GDK_DISPLAY(), win->xwin, &win->name);

  if (win->name == NULL) 
    win->name = g_strdup("unknown");

  view = hn_wm_watched_window_get_active_view(win);

  /* Duplicate to topped view also */
  if (view)
    hn_wm_watched_window_view_set_name (view, win->name);

  app_switcher_update_item (hnwm->app_switcher, win, view,
			    AS_MENUITEM_SAME_POSITION);
}

static void
hn_wm_watched_window_process_hibernation_prop (HNWMWatchedWindow *win)
{
  Atom             *foo = NULL;
  HNWMWatchableApp *app;

  app = hn_wm_watched_window_get_app (win);
  
  if (!app) 
    return;
  
  /* NOTE: foo has no 'value' if set the app is killable (hibernatable), 
   *       deletes to unset 
   */
  foo = hn_wm_util_get_win_prop_data_and_validate (win->xwin, 
						   hnwm->atoms[HN_ATOM_HILDON_APP_KILLABLE],
						   XA_STRING,
						   8,
						   0,
						   NULL);
  
  hn_wm_watchable_app_set_able_to_hibernate (app,
					     (foo != NULL) ? TRUE : FALSE );
  if (foo)
    XFree(foo);
}

static void
hn_wm_watched_window_process_wm_state (HNWMWatchedWindow *win)
{
  HNWMWatchableApp *app;
  Atom             *state = NULL;

  app = hn_wm_watched_window_get_app (win);

  state 
    = hn_wm_util_get_win_prop_data_and_validate (win->xwin,
						 hnwm->atoms[HN_ATOM_WM_STATE],
						 hnwm->atoms[HN_ATOM_WM_STATE],
						 0, /* FIXME: format */
						 0,
						 NULL);
  
  if (!state 
      || (hn_wm_watchable_app_is_minimised(app) && state[0] == IconicState) )
    goto out;
  
  if (state[0] == IconicState)
    {
      hn_wm_watchable_app_set_minimised (app, TRUE);

      if (hn_wm_watched_window_get_views (win))
	{
	  GList      *iter;
	  iter = hn_wm_watched_window_get_views (win);
	  
	  while (iter != NULL)
	    {
	      HNWMWatchedWindowView *view;
	      
	      view = (HNWMWatchedWindowView *)iter->data;
	      
	      app_switcher_update_item (hnwm->app_switcher, win, view,
					AS_MENUITEM_TO_LAST_POSITION);

	      iter  = g_list_next(iter);
	    }
	}
      else
	{
	  /* Window with no views */
	  app_switcher_update_item (hnwm->app_switcher, win, NULL,
				    AS_MENUITEM_TO_LAST_POSITION );
	}
    }

 out:

  if (state)
    XFree(state);
}

static void
hn_wm_watched_window_process_wm_hints (HNWMWatchedWindow *win)
{
  HNWMWatchableApp *app;
  XWMHints         *wm_hints;
  gboolean          need_icon_sync = FALSE;

  app = hn_wm_watched_window_get_app (win);  

  wm_hints = XGetWMHints (GDK_DISPLAY(), win->xwin);

  if (!wm_hints)
    return;

  win->xwin_group = wm_hints->window_group;
  
  if (win->is_urgent != (wm_hints->flags & XUrgencyHint))
    need_icon_sync = TRUE;

  win->is_urgent = (wm_hints->flags & XUrgencyHint);

  if (need_icon_sync)
    app_switcher_item_icon_sync (hnwm->app_switcher, win);

  XFree(wm_hints);
}

static void 
hn_wm_watched_window_process_hildon_view_list (HNWMWatchedWindow *win)
{
  struct xwinv xwins;
  int          i;
  GList       *iter = NULL, *next_iter;
  
  if (hn_wm_watched_window_is_hibernating(win))
    return;

  xwins.wins 
    = hn_wm_util_get_win_prop_data_and_validate (win->xwin,
						 hnwm->atoms[HN_ATOM_HILDON_VIEW_LIST],
						 XA_WINDOW,
						 32,
						 0,
						 &xwins.n_wins);
  if (G_UNLIKELY(xwins.wins == NULL))
    return;

  HN_DBG("_HILDON_VIEW_LIST change with %i wins", xwins.n_wins);

  iter = hn_wm_watched_window_get_views (win);

  /* Delete an views we have listed, but are not listed in prop */

  while (iter != NULL)
    {
      HNWMWatchedWindowView *view;
      gboolean               view_found;
      
      view       = (HNWMWatchedWindowView *)iter->data;
      view_found = FALSE;

      next_iter  = g_list_next(iter);
      
      for (i=0; i < xwins.n_wins; i++)
	if (xwins.wins[i] == hn_wm_watched_window_view_get_id (view))
	  {
	    view_found = TRUE;
	    break;
	  }
      
      if (!view_found)
	{
	  /* view is not listed on client - delete the list entry */
	  hn_wm_watched_window_remove_view  (win, view);
	  hn_wm_watched_window_view_destroy (view);
	}

      iter = next_iter;
    }

  /* Now add any new views in prop that we dont have listed */

  for (i=0; i < xwins.n_wins; i++)
    {
      gboolean view_found;
      
      iter       = hn_wm_watched_window_get_views (win);
      view_found = FALSE;

      while (iter != NULL)
	{
	  HNWMWatchedWindowView *view;

	  view = (HNWMWatchedWindowView *)iter->data;

	  if (hn_wm_watched_window_view_get_id (view) == xwins.wins[i])
	    {
	      view_found = TRUE;
	      break;
	    }
	  iter = g_list_next(iter);
	}

      if (!view_found)
	{
	  HNWMWatchedWindowView *new_view;

	  /* Not in internal list so its new, add it */
	  new_view = hn_wm_watched_window_view_new (win, xwins.wins[i]);

	  if (new_view)
	    hn_wm_watched_window_add_view (win, new_view);
	}
    }

  if (xwins.wins)
    XFree(xwins.wins);
}


HNWMWatchedWindow*
hn_wm_watched_window_new (Window            xid,
			  HNWMWatchableApp *app)
{
  HNWMWatchedWindow *win = NULL;

  win = g_hash_table_lookup(hnwm->watched_windows_hibernating, 
			    (gconstpointer)hn_wm_watchable_app_get_class_name (app));
  
  if (win)
    {
      /* Window already exists in our hibernating window hash.  
       * There for we can reuse by just updating its var with
       * new window values	  
       */
      g_hash_table_steal(hnwm->watched_windows_hibernating, 
			 hn_wm_watchable_app_get_class_name (app)); 
    }
  else 
    win = g_new0 (HNWMWatchedWindow, 1);  
  
  if (!win) 	 /* FIXME: Handle OOM */
    return NULL;

  win->xwin       = xid;
  win->app_parent = app;

  /* Grab some initial props */
  hn_wm_watched_window_props_sync (win, 
				   HN_WM_SYNC_NAME
				   |HN_WM_SYNC_WMHINTS
				   |HN_WM_SYNC_ICON
				   |HN_WM_SYNC_HILDON_APP_KILLABLE);

  return win;
}


HNWMWatchableApp*
hn_wm_watched_window_get_app (HNWMWatchedWindow *win)
{
  return win->app_parent;
}

Window
hn_wm_watched_window_get_x_win (HNWMWatchedWindow *win)
{
  return win->xwin;
}

gboolean
hn_wm_watched_window_is_urgent (HNWMWatchedWindow *win)
{
  return win->is_urgent;
}

const gchar*
hn_wm_watched_window_get_name (HNWMWatchedWindow *win)
{
  return win->name;
}

void
hn_wm_watched_window_set_name (HNWMWatchedWindow *win,
			       gchar             *name)
{
  if (win->name) g_free(win->name);
  win->name = g_strdup(name);
}


GtkWidget*
hn_wm_watched_window_get_menu (HNWMWatchedWindow *win)
{
  return win->menu_widget;
}

GdkPixbuf*
hn_wm_watched_window_get_custom_icon (HNWMWatchedWindow *win)
{
  if (win->pixb_icon == NULL)
    return NULL;

  return gdk_pixbuf_copy(win->pixb_icon);
}

void
hn_wm_watched_window_set_menu (HNWMWatchedWindow *win,
			       GtkWidget         *menu)
{
  win->menu_widget = menu;
}

GList*
hn_wm_watched_window_get_views (HNWMWatchedWindow *win)
{
  return win->views;
}

void
hn_wm_watched_window_add_view (HNWMWatchedWindow     *win,
			       HNWMWatchedWindowView *view)
{
  win->views = g_list_append(win->views, view);
}

void
hn_wm_watched_window_remove_view (HNWMWatchedWindow     *win,
				  HNWMWatchedWindowView *view)
{
  GList *view_link;

  view_link = g_list_find (win->views, view);             

  if (view_link)
    win->views = g_list_delete_link(win->views, view_link);
}

void
hn_wm_watched_window_set_active_view (HNWMWatchedWindow     *win,
				      HNWMWatchedWindowView *view)
{
  win->view_active = view;
}

HNWMWatchedWindowView *
hn_wm_watched_window_get_active_view (HNWMWatchedWindow     *win)
{
  return win->view_active;
}


gboolean
hn_wm_watched_window_attempt_pid_kill (HNWMWatchedWindow *win)
{
  guchar *pid_result = NULL;

  pid_result = hn_wm_util_get_win_prop_data_and_validate (win->xwin,
							  hnwm->atoms[HN_ATOM_NET_WM_PID],
							  XA_CARDINAL,
							  32,
							  0,
							  NULL);

  if (pid_result == NULL)
    return FALSE;

  if(kill(pid_result[0]+256*pid_result[1], SIGTERM) != 0)
    {
      osso_log(LOG_ERR, "Failed to kill pid %d with SIGTERM", 
	       pid_result[0]+256*pid_result[1]);
      
      XFree(pid_result);
      return FALSE;
    }

  XFree(pid_result);
  return TRUE;
}

gboolean
hn_wm_watched_window_is_hibernating (HNWMWatchedWindow *win)
{
  HNWMWatchableApp      *app; 

  app = hn_wm_watched_window_get_app(win);

  if (app && hn_wm_watchable_app_is_hibernating(app))
    return TRUE;

  return FALSE;
}

void
hn_wm_watched_window_hibernate (HNWMWatchedWindow *win)
{
  HNWMWatchableApp      *app; 

  app = hn_wm_watched_window_get_app(win);

  if (g_hash_table_steal (hnwm->watched_windows, (gconstpointer)&win->xwin))
    {
      hn_wm_watchable_app_set_hibernate (app, TRUE);

      win->xwin           = None;
      g_hash_table_insert (hnwm->watched_windows_hibernating, 
			   g_strdup(hn_wm_watchable_app_get_class_name (app)),
			   win);
      HN_DBG("'%s' now hibernating, moved to WatchedWindowsHibernating hash",
	     win->name);
    }
}

void
hn_wm_watched_window_awake (HNWMWatchedWindow *win)
{
  HNWMWatchableApp      *app; 
  osso_manager_t        *osso_man;

  app = hn_wm_watched_window_get_app(win);

  if (app)
    {
      /* Relaunch it with RESTORED */
      osso_man = osso_manager_singleton_get_instance();
      osso_manager_launch(osso_man, hn_wm_watchable_app_get_service (app), 
			  RESTORED);

      /* Remove it from our hibernating hash, on mapping it should
       * get readded to regular WatchedWindows hash
       */
      hn_wm_watchable_app_set_hibernate (app, FALSE);
    }
}

void
hn_wm_watched_window_destroy (HNWMWatchedWindow *win)
{
  GList                 *iter, *next_iter;
  HNWMWatchedWindowView *view;

  HN_DBG("Removing '%s'", win->name); 

  /* Dont destroy windows that are hiberating */
  if (hn_wm_watched_window_is_hibernating(win))
    {
      HN_DBG("### Aborting destroying '%s' as hibernating ###", win->name); 
      return;
    }

  if (win->menu_widget)
    {
      /* Only remove the widget if the app is *really* killed */
      app_switcher_remove_item (hnwm->app_switcher, win->menu_widget);
    }


  /* Destroy the views too */
  iter = win->views;

  while (iter)
    {
      next_iter = g_list_next(iter);

      view = (HNWMWatchedWindowView *)iter->data;

      hn_wm_watched_window_view_destroy (view);

      win->views = g_list_delete_link(win->views, iter);

      iter = next_iter;
    }

  if (win->name) 
    XFree(win->name);

  if (win->pixb_icon)
    g_object_unref(win->pixb_icon);

  g_free(win);
}

gboolean
hn_wm_watched_window_props_sync (HNWMWatchedWindow *win, gulong props)
{
  gdk_error_trap_push();

  if (props & HN_WM_SYNC_NAME)
    {
      hn_wm_watched_window_process_wm_name (win);      
    }
  
  if (props & HN_WM_SYNC_HILDON_APP_KILLABLE)
    {
      hn_wm_watched_window_process_hibernation_prop (win);
    }
  
  if (props & HN_WM_SYNC_WM_STATE)
    {
      hn_wm_watched_window_process_wm_state (win);
    }
  
  if (props & HN_WM_SYNC_HILDON_VIEW_LIST)
    {
      hn_wm_watched_window_process_hildon_view_list (win);
    }

  if (props & HN_WM_SYNC_HILDON_VIEW_ACTIVE)
    {
      hn_wm_watched_window_process_hildon_view_active (win);
    }

  if (props & HN_WM_SYNC_WMHINTS)
    {
      hn_wm_watched_window_process_wm_hints (win);
    }

  if (props & HN_WM_SYNC_ICON)
    {
      hn_wm_watched_window_process_net_wm_icon (win);
    }

  gdk_error_trap_pop();

  return TRUE;
}
