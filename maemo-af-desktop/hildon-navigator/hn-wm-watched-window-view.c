#include "hn-wm-watched-window-view.h"

/* Watched Window views */

extern HNWM *hnwm;

struct HNWMWatchedWindowView
{
  GtkWidget         *menu_widget;
  int                id;
  HNWMWatchedWindow *win_parent;
  char              *name;
};

HNWMWatchedWindowView*
hn_wm_watched_window_view_new (HNWMWatchedWindow *win,
			       int                view_id)
{
  HNWMWatchedWindowView *view;
  HNWMWatchableApp      *app;

  view = g_new0 (HNWMWatchedWindowView, 1);  

  if (!view) 	 /* FIXME: Handle OOM */
    return NULL;

  view->id         = view_id;
  view->win_parent = win; 

  app = hn_wm_watched_window_get_app (win);

  /* The window may have been 'viewless' before this 
   * view was created to we need to remove the widget 
   * ref for a viewless window  
  */
  if (hn_wm_watched_window_get_menu (win))
    {
      app_switcher_remove_item (hnwm->app_switcher,
				hn_wm_watched_window_get_menu (win));

      hn_wm_watched_window_set_menu (win, NULL);
    }

  view->menu_widget = app_switcher_add_new_item (hnwm->app_switcher, 
						 win, 
						 view);
  return view;
}

HNWMWatchedWindow*
hn_wm_watched_window_view_get_parent (HNWMWatchedWindowView *view)
{
  return view->win_parent;
}

gint
hn_wm_watched_window_view_get_id (HNWMWatchedWindowView *view)
{
  return view->id;
}

GtkWidget*
hn_wm_watched_window_view_get_menu (HNWMWatchedWindowView *view)
{
  return view->menu_widget;
}

void
hn_wm_watched_window_view_set_name (HNWMWatchedWindowView *view,
				    const gchar           *name)
{
  if (name == NULL)
    return;

  if (view->name)
    g_free(view->name);

  view->name = g_strdup(name);
}

const gchar*
hn_wm_watched_window_view_get_name (HNWMWatchedWindowView *view)
{
  if (view->name != NULL)
    return view->name;

  if (view->win_parent && hn_wm_watched_window_get_name (view->win_parent))
    return hn_wm_watched_window_get_name (view->win_parent);

  return "unknown";
}

void
hn_wm_watched_window_view_destroy (HNWMWatchedWindowView *view)
{
  HNWMWatchableApp *app;

  app = hn_wm_watched_window_get_app(view->win_parent);

  if (hn_wm_watched_window_get_active_view(view->win_parent) == view)
    hn_wm_watched_window_set_active_view(view->win_parent, NULL);

  if (view->name)
    g_free(view->name);

  /* Only remove the widget if the app is *really* killed */
  if (!hn_wm_watchable_app_is_hibernating(app))
    app_switcher_remove_item (hnwm->app_switcher, view->menu_widget);

  free(view);
}
