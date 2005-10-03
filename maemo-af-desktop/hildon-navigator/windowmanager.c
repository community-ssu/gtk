/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005 Nokia Corporation.
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

#include <gtk/gtkmenu.h>

#include "windowmanager.h"
#include "application-switcher.h"
#include "osso-manager.h"
#include "others-menu.h"
#include "hildon-navigator-interface.h"
#include <libmb/mbutil.h>
#include <X11/X.h>

/* log include */
#include <log-functions.h>

/* Needed for the "launch banner" */
#include <hildon-widgets/gtk-infoprint.h>
#include <hildon-widgets/hildon-note.h>
#include <sys/time.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

#include <libosso.h>

/* Acquire this from outside and not create here! */
osso_context_t *osso;

gint timer_id = 0;

gulong lowmem_banner_timeout = 0;
gulong lowmem_min_distance = 0;
/* Added by Karoliina Salminen 26092005. 
   This global variable contains
   the multiplier for the launch banner timeout in a low memory situation */
gulong lowmem_timeout_multiplier = 0;
/* End of addition 26092005 */

gboolean lowmem_situation = FALSE;
gboolean bgkill_situation = FALSE;

/* Structure that contains pointers to the callback functions */

typedef struct
{
    wm_new_window_cb *new_win_cb;
    wm_removed_window_cb *removed_win_cb;
    wm_updated_window_cb *updated_win_cb;
    wm_desktop_topped_cb *topped_desktop_cb;
    gpointer cb_data;
    
    GtkTreeModel *model;
    GtkWidget *killer_dialog;
    GList *purge_win_list;
} wm_callbacks;

wm_callbacks wm_cbs;


/* X Atoms we'll use all the time */

Atom active_win, clientlist, net_wm_state, pid_atom;
Atom subname, transient_for, typeatom;
Atom wm_class_atom, wm_name, wm_state, wm_type, utf8, showing_desktop;
Atom mb_active_win;

/* FIXME: much nicer to declare global atoms like this 
 * or better put in an array..
*/
Atom Atom_HILDON_APP_KILLABLE;
Atom Atom_NET_WM_WINDOW_TYPE_MENU;
Atom Atom_NET_WM_WINDOW_TYPE_NORMAL;
Atom Atom_NET_WM_WINDOW_TYPE_DIALOG;
Atom Atom_NET_WM_WINDOW_TYPE_DESKTOP;
Atom Atom_NET_WM_STATE_MODAL;

/* Struct for launch banner info */
typedef struct
{
	GtkWidget     *parent;
	gchar         *service_name;
	struct timeval launch_time;
} launch_banner_info;



/* Method call handling function */
static DBusHandlerResult method_call_handler( DBusConnection *connection,
                                              DBusMessage *message,
                                              void *data );


/* Internal function definitions */

static void update_lowmem_situation(gboolean lowmem);

/* Function for showing "launch banner" */
static void show_launch_banner( GtkWidget *parent, gchar *app_name,
                                gchar *service_name );

/* Function for closing "launch banner" */
static void close_launch_banner( GtkWidget *parent );

/* Function for figuring out when to close the banner */
static gboolean launch_banner_timeout( gpointer data );

/* Function for displaying relaunch failure banner */

static gboolean relaunch_timeout( gpointer data );

/** The actual X event handler that delegates real event handling
 *  elsewhere.
 */

static GdkFilterReturn x_event_filter(GdkXEvent *xev, GdkEvent *gevent,
                                      gpointer data);

/* Various X event handlers */

static void create_notify_handler(GdkXEvent *xev, GtkTreeModel *model);

static void destroy_notify_handler(GdkXEvent *xev, GtkTreeModel *model);

static void property_notify_handler(GdkXEvent *xev, GtkTreeModel *model);



static void map_notify_handler(GdkXEvent *xev, GtkTreeModel *model);


/** This helper function creates a list of only views to remove; used
 *  to synchronize the application views when _NET_CLIENT_LIST changes
 */


static gboolean remove_view_helper(GtkTreeModel *model, GtkTreePath *path,
                                   GtkTreeIter *iter, gpointer data);

/**
 * This helper function creates a list of view/window entries to remove
 *
 */

static gboolean remove_helper(GtkTreeModel *model, GtkTreePath *path,
                              GtkTreeIter *iter, gpointer data);

/**
 * Creates the tree model that contains the applications
 */

static GtkTreeModel *create_model(void);


/* Routine to find an application from tree model
 *  @param model Model to search from
 *  @param iter The iterator to set to point to the app
 *                      found
 *  @param name the WM_CLASS name of the application to look for
 *  @return non-zero if an app was found */

static int find_application_from_tree(GtkTreeModel * model,
				      GtkTreeIter * iter,
				      const gchar * name);


/* Routine to find an application from tree model by using D-BUS
 * service name as a key instead of WM_CLASS
 * @param model Model to search from
 * @param iter The iterator to set to point to the app
 *             found
 * @param name the D-BUS service name of the application to look for
 * @return non-zero if the corresponding name was found
 */

static int find_service_from_tree(GtkTreeModel * model,
                                  GtkTreeIter * iter,
                                  const gchar * service);



/* Routine that checks whether a window exists in tree.
 * @param model the model in question
 * @param parent parent application of the window
 * @param id XID of the window to look for
 * @param view_id The unique ID of the view (or 0, if does not matter;
 *                useful because applications without views exist)
 *
 * @return non-zero if window exists in tree */

static int window_exists(GtkTreeModel * model, GtkTreeIter * parent,
                         GtkTreeIter * w_iter,
                         unsigned long id, unsigned long view_id);


/* Insert new window to the tree
 *  @param model the model to insert to 
 *  @param parent the parent application for the window
 *  @param message a structure containing window info */

static void insert_new_window(GtkTreeModel * model, GtkTreeIter * parent,
			      window_props * properties);


/* Update an existing window/view info
 * @param model the GtkTreeModel that contains the window/view data
 * @param parent the parent application node for the window
 * @param id the window ID
 * @param view_id the ID for the view
 * @param view_name The name for the view
 * @param position_change Position change for the window/view, if any
 */

static void update_window(GtkTreeModel * model,
			  GtkTreeIter * parent, unsigned long id,
			  unsigned long view_id,
                          unsigned long new_view_id,
                          const gchar *view_name,
                          const guint position_change);


/*
 * Find the WM_CLASS of a certain window/dialog.
 * The string must be free'd by the caller.
 * @param model The GtkTreeModel that contains the window/view data
 * @param win_id The window ID
 */
static gchar *find_window_wmclass(GtkTreeModel *model,
                                  GtkTreeIter *parent, Window win_id);


/* Handles _NET_ACTIVE_WINDOW property changes
 * @param model the GtkTreeModel that contains the window data
 * @param wm_class_str The WM_CLASS property of the application
 * @param win_id The window ID of the applications window
 */


static void handle_active_window_prop(GtkTreeModel *model,
                                      const gchar *wm_class_str,
                                      const Window win_id);

/* Handles the _HILDON_APP_KILLABLE property changes
 * @param model the GtkTreeModel that contains the window data
 * @param wm_class_str The WM_CLASS property of the application
 * @param win_id The window ID of the applications window
 * @param state The X value for the change mode of the property
 */

static void handle_killable_prop(GtkTreeModel *model,
                                 const gchar *wm_class_str,
                                 const Window win_id,
                                 int state);


/* Handles the minimization of an application
 * @param model the GtkTreeModel that contains the window data
 * @param wm_class_str The WM_CLASS property of the application
 * @param win_id The window if of the applications window
 */

static void handle_minimization(GtkTreeModel *model,
                                const gchar *wm_class_str,
                                const Window win_id);

/* Handles _MB_WIN_SUB_NAME property changes
 * @param model the GtkTreeModel that contains the window/view data
 * @param wm_class_str The WM_CLASS property of the application
 * @param win_id The window ID of the applications window
 */


static void handle_subname_window_prop(GtkTreeModel *model,
                                       const gchar *wm_class_str,
                                       const Window win_id);


/* Handles _NET_CLIENT_LIST property changes
 * @param model the GtkTreeModel that contains the window/view data
 * @param wm_class_str The WM_CLASS property of the application
 * @param win_id The window ID of the applications window
 */

static void handle_client_list_prop(GtkTreeModel *model,
                                    const gchar *wm_class_str,
                                    const Window win_id);


/* Attempts to determine the type of the window
 * @param win_id The window ID of the applications window
 */

static int determine_window_type(Window win_id);


/* Gets the title (i.e. WM_NAME) of the window
 * @param win_id The window ID whose title we try to get
 * @return The title of the window, or "Unknown" if not set
 */

static guchar *get_window_title(Window win_id);


/* Gets the active view for the window (i.e. _NET_ACTIVE_WINDOW)
 * @param win_id the window ID whose active view we try to get
 * @return The view ID, or 0 if it could not be found out.
 */

static gulong get_active_view(Window win_id);


/* Finds the window ID that corresponds to the active view of a particular
 * application.
 * @param model the GtkTreeModel that contains the window/view data
 * @param parent parent application of the window
 * @param target_iter will be set to
 *                    GtkTreeIterator that matches to the active view
 * @param view_id the active view ID
 * @return The window ID that belongs to the particular application
 */

static gulong find_window_id_for_view(GtkTreeModel *model,
                                      GtkTreeIter *parent,
                                      GtkTreeIter *target_iter,
                                      const gulong view_id);


/* Performs the actual removal of view/window entries
 */

static void do_cleanup(GtkTreeModel *model);


/* Checks whether the application is an HildonApp by checking the
 * existence of various custom X properties
 * @param xid The window ID
 */

static gboolean is_window_hildonapp(Window xid);


/* Tops an X window by using the X / window manager features
 * @param xid the Window ID
 */

static void top_non_hildonapp(Window xid);

/*
 * Get the name of the view.
 * @param win_id The window ID
 * @return The subname property of the window (aka view name)
 */

static gchar *get_subname(Window win_id);


/*
 * Check if we need to autotop an application and do it, if necessary.
*/

static void handle_autotopping( void );


/*
 * Appkiller - related function definitions
 */

/* Kills Least Recently Used applications */
static int kill_lru( void );


/* Kills all applications that are safely killable
 * 
 * (This and shutdown_ind() should be combined, as there is a
 * significant functionality overlap)
 */

static int kill_all( gboolean killable_only );


/* The osso_manager callback for the kill_app method required by
 * the OOM killer support
 * @param arguments The D-BUS message arguments
 * @param pointer to the callback-specific data
 */

static int killmethod(GArray *arguments, gpointer data);


/* Finds the application window and uses it to determine the PID
 * and kill the process.
 * @param model The GtkTreeModel
 * @param parent The iterator for the application top node
 * @param mark_as_killed When TRUE, do not remove from AS.
 */

static void kill_application(GtkTreeModel *model, GtkTreeIter *parent,
			     gboolean mark_ask_killed);


/* Updates the X window IDs of an applications window after the
 * process has been OOM killed and restarted.
 * @param model The GtkTreeModel
 * @param parent The iterator for the application top node
 * @parem win_id The new window ID for the application
 * 
 */

static void update_application_window_id(GtkTreeModel *model,
                                         GtkTreeIter *parent,
                                         Window win_id);

/* Finds the corresponding GtkTreeIter for a given menuitem pointer
 *
*/

static gboolean menuitem_match_helper(GtkTreeModel *model,
                                      GtkTreePath *path,
                                      GtkTreeIter *iter,
                                      gpointer data);


/*
 * Saves the session, i.e. the list of applications and their views
 * that were open during at the moment the session was saved 
 */


static int save_session(GArray *arguments, gpointer data);

/*
 * Restores a saved session, i.e. launches all applications that were
 * present at the time the session was saved. Additionally, reorders
 * the applications and their views in the AS menu as close to that
 * as possible.
 */

static void restore_session(osso_manager_t *man);


/*
 * Is called from the Others menu when its dnotify callback is executed and
 * reads a .desktop file. Windowmanager uses this to check whether the app
 * should be added into its treemodel or not.
 *
 * @param desktop Pointer to the .desktop file
*/

static void dnotify_callback(MBDotDesktop *desktop);

/*
 * Shuts _all_ applications down, regardless of their killability.
 */

static void shutdown_handler(void);
/*
 */

static void lowmem_handler(gboolean is_on);
static void bgkill_handler(gboolean is_on);

/*
 * Handle the appkiller signal(s).
 * @param connection The D-BUS connection where the message comes
 * @param message The D-BUS message
 * @param 
 */

static DBusHandlerResult exit_handler(DBusConnection *connection,
                                      DBusMessage *message,
                                      void *data );


/*
 * Sends the SIGTERM signals to applications.
 * @param data Pointer to a GArray of application PIDs.
 * @return TRUE if the sending of signals was succesful.
 */

static gboolean send_sigterms(gpointer data);


/*
 * Initializes the dialog for killing applications
 */

static void create_killer_dialog(void);



/*
 * Handler for the menu-like application killer dialog selections.
 */

static gboolean treeview_button_press(GtkTreeView *treeview,
				      GdkEventButton *event,
				      GtkCellRenderer *renderer);
     
/* Implementations begin */


static GdkFilterReturn x_event_filter(GdkXEvent *xev, GdkEvent *gevent,
                                      gpointer data)
{
    XAnyEvent *xany = xev;
    GtkTreeModel *model = GTK_TREE_MODEL(data);
    switch (xany->type)
    { 
    case CreateNotify:
        create_notify_handler(xev, model);
        return GDK_FILTER_CONTINUE;
    case DestroyNotify:
        destroy_notify_handler(xev, model);
        return GDK_FILTER_CONTINUE;
    case PropertyNotify:
	property_notify_handler(xev, model);
	return GDK_FILTER_CONTINUE;
    case MapNotify:
        map_notify_handler(xev, model);
        return GDK_FILTER_CONTINUE;
    default:
    /* Other events are of no interest to us (for now)... */
    return GDK_FILTER_CONTINUE;
    }
}


static void create_notify_handler(GdkXEvent *xev, GtkTreeModel *model)
{
    int actual_format, window_type = UNKNOWN;
    unsigned long nitems, bytes_after;

    gchar *exec, *name, *icon;
    guchar *wm_class_str;
    gboolean killed = FALSE;
    
    GtkTreeIter parent;

    XCreateWindowEvent *cev = (XCreateWindowEvent *)xev;
    Atom actual_type;
    XWindowAttributes attrs;

    gdk_error_trap_push();
    XGetWindowProperty(GDK_DISPLAY(), cev->window, wm_class_atom, 0, 24,
                       False, XA_STRING, &actual_type, &actual_format,
                       &nitems, &bytes_after, &wm_class_str);
    gdk_flush();
    if (gdk_error_trap_pop() != 0)
    {
        /* If the window does not have wm_class information available,
           it will be ignored */
        return;
    }

    attrs.override_redirect = 0;
    gdk_error_trap_push();
    XGetWindowAttributes(GDK_DISPLAY(), cev->window, &attrs);
    gdk_error_trap_pop();

    /* We're not interested about decorations, menu windows etc... */

    if (attrs.override_redirect == 1 || 
        (window_type = determine_window_type(cev->window)) == MENU )
    {
        if (wm_class_str != NULL)
        {
            XFree(wm_class_str);
        }
        return;
    }

    if (wm_class_str != NULL)
    {
        window_props *new_win;
        GdkWindow *gdk_win = NULL;
        
        if (find_application_from_tree(model, &parent, wm_class_str))
        {
            unsigned long active_view;

            gtk_tree_model_get(model, &parent, WM_KILLED_ITEM, &killed,
                               WM_VIEW_ID_ITEM, &active_view, -1);

            /* We have to create a GDK wrapper for the window,
               because GDK windows outside the running application
               are not 'reachable' directly */
            gdk_error_trap_push();
            gdk_win = gdk_window_foreign_new (cev->window);
            gdk_error_trap_pop();

            if (gdk_win != NULL)
            {
                gdk_error_trap_push();
                gdk_window_set_events(gdk_win,
                                      gdk_window_get_events(gdk_win)
                                      | GDK_STRUCTURE_MASK
                                      | GDK_PROPERTY_CHANGE_MASK);
                gdk_window_add_filter(gdk_win, x_event_filter, model);
                if (gdk_error_trap_pop() != 0)
                {
                    XFree(wm_class_str);
                    return;
                }

                g_object_unref(gdk_win);
            }

            /* If the application has been killed, we'll have to upgrade
               the window ID information on all application leaf nodes. */

            if (killed == TRUE)
            {
                update_application_window_id(model, &parent, cev->window);
                return;
            }

            /* Other kinds of windows will be handled in map events etc. */
            
            if (window_type == UNKNOWN)
            {
                window_type = determine_window_type(cev->window);
            }

            if (window_type != MODAL_DIALOG)
            {
                XFree(wm_class_str);
                return;
            }
            
            gtk_tree_model_get(model, &parent,
                               WM_ICON_NAME_ITEM, &icon,
                               WM_BIN_NAME_ITEM, &exec,
                               WM_VIEW_ID_ITEM, &active_view,
                               WM_NAME_ITEM, &name, -1);
            
            /* Create a new window and append it to the model. */
            new_win = g_malloc0(sizeof(window_props));
            new_win->icon = g_strdup((gchar *)icon);
            new_win->name = g_strdup((gchar *)name);
            new_win->exec = g_strdup((gchar *)wm_class_str);
            new_win->win_id = (gulong)cev->window;
            new_win->view_id = active_view;
            new_win->menuitem_widget = NULL;
            new_win->is_dialog = 1;
            
            if (new_win->view_id == 0)
            {
                gulong active_view = get_active_view(cev->window);
                if (active_view != 0)
                {
                    new_win->view_id = active_view;
                }
                else
                {
                    g_free(icon);
                    g_free(exec);
                    g_free(name);
                    XFree(wm_class_str);
                    return;
                }
            }
            insert_new_window(model, &parent, new_win);
            
            /* Get dialog name */
            new_win->dialog_name = get_window_title(cev->window);
            
            g_free(exec);
            g_free(name);
            g_free(icon);
            g_free(new_win->dialog_name);
            g_free(new_win);
        }
        XFree(wm_class_str);
    }
}




static void destroy_notify_handler(GdkXEvent *xev, GtkTreeModel *model)
{
    XDestroyWindowEvent *dev = xev;
    view_clean_t cleaner;
    gboolean killed = FALSE;
    gchar *wm_class = NULL;
    GtkTreeIter root;

    /* First, find the WM_CLASS of the application and see if it belongs
       to a program that has been OOM killed - if yes, do nothing */

    if (gtk_tree_model_get_iter_first(model, &root) == TRUE)
    {
        wm_class = find_window_wmclass(model, &root, dev->window);
    }

    if (wm_class != NULL)
    {
        gtk_tree_model_get(model, &root, WM_KILLED_ITEM, &killed, -1);
        if (killed == TRUE)
        {
            update_application_window_id(model, &root, 1);
            g_free(wm_class);
            return;
        }
    }

    /* Walk through the whole tree and search for the ID - necessary
       because we can't query the window properties, so the window
       could be anywhere in the structure... */

    wm_cbs.purge_win_list = NULL;
    cleaner.window_id = dev->window;
    gtk_tree_model_foreach(model, remove_helper, (gpointer)&cleaner);

    /* Call the callback function (if set) and give pointer to the
       menu item that is to be removed */
    do_cleanup(model);
    
}



static void property_notify_handler(GdkXEvent *xev, GtkTreeModel *model)
{
    XPropertyEvent *pev = xev;
    
    int actual_format = 0;
    unsigned long nitems, bytes_after;
    unsigned char *wm_class_str = NULL;
    unsigned char *desktop_shown = NULL;
    union wmstate_value vt;

    Atom actual_type;

    gdk_error_trap_push();
    if (determine_window_type(pev->window) != NORMAL_WINDOW && 
        pev->window != GDK_WINDOW_XID(gdk_get_default_root_window()) )
    {
        gdk_error_trap_pop();
        return;
    }
    gdk_error_trap_pop();

    gdk_error_trap_push();
    XGetWindowProperty(GDK_DISPLAY(), pev->window, wm_class_atom, 0, 24,
                       False, XA_STRING, &actual_type, &actual_format,
                       &nitems, &bytes_after, &wm_class_str);
    gdk_error_trap_pop();

    /* If active view changes and is not in the treestore yet,
       it signifies a new view that must be registered. We do also the
       subname registration here. */
    if (pev->atom == active_win)
    {
        handle_active_window_prop(model, wm_class_str, pev->window);
    }
    else if (pev->atom == showing_desktop)
    {
        gdk_error_trap_push();
        XGetWindowProperty(GDK_DISPLAY(), 
                           GDK_WINDOW_XID(gdk_get_default_root_window()),
                           showing_desktop, 0, 32, False, XA_CARDINAL,
                           &actual_type,
                           &actual_format, &nitems, &bytes_after,
                           (guchar **)&desktop_shown);
            if (gdk_error_trap_pop() == 0 && nitems == 1 &&
                actual_type == XA_CARDINAL && desktop_shown[0] == 1)
            {
                handle_autotopping();
            }
            XFree(desktop_shown);
            if (wm_class_str != NULL)
            {
                XFree(wm_class_str);
            }
            return;
    }

    /* End of handler calls that do not require wm_class info */

    if (wm_class_str == NULL)
    {
        return;
    }
    else if (pev->atom == subname)
    {
        handle_subname_window_prop(model, wm_class_str, pev->window);
    }
    
    else if (pev->atom == clientlist)
    {
        handle_client_list_prop(model, wm_class_str, pev->window);
    }
    else if (pev->atom == Atom_HILDON_APP_KILLABLE)
    {
        handle_killable_prop(model, wm_class_str, pev->window, pev->state);
    }
    else if (pev->atom == wm_state)
    {
        gdk_error_trap_push();
        XGetWindowProperty(GDK_DISPLAY(), pev->window, wm_state, 0,
                           8L, False, wm_state, &actual_type,
                           &actual_format,
                           &nitems, &bytes_after, (guchar **)&vt.char_value);
        gdk_error_trap_pop();
        
        if (nitems > 0 && (vt.state_value[0] == IconicState) )
        {
            handle_minimization(model, wm_class_str, pev->window);
        }
        if (vt.char_value)
            XFree(vt.char_value);
    }

    XFree(wm_class_str);
}


static void map_notify_handler(GdkXEvent *xev, GtkTreeModel *model)
{
    int window_type = OTHER;
    gulong id = 0; 
    window_props *new_win;
    
    XMapEvent *mev = xev;
    Atom actual_type;

    XWindowAttributes attrs;
    Status status = 0;

    int actual_format = 0;
    unsigned long nitems, bytes_after;
    unsigned char *wm_class_str = NULL;
    
    GtkTreeIter parent , w_iter;
    
    gdk_error_trap_push();
    status = XGetWindowAttributes(GDK_DISPLAY(), mev->window, &attrs);
    if (gdk_error_trap_pop() != 0)
    {
        /* Just ignore the error, i.e. do nothing */
    }
    
    if (status != 0 && attrs.override_redirect == True)
    {
        return;
    }

    gdk_error_trap_push();
    XGetWindowProperty(GDK_DISPLAY(), mev->window, wm_class_atom, 0, 24,
                       False, XA_STRING, &actual_type, &actual_format,
                       &nitems, &bytes_after, &wm_class_str);
    if (gdk_error_trap_pop() != 0 || nitems < 1)
    {
        return;
    }
    
    
/* See if this window is a modal dialog */
    
    window_type = determine_window_type(mev->window);
    if (window_type == MODAL_DIALOG)
    {
        new_win = g_malloc0(sizeof(window_props));
        new_win->exec = (gchar *)wm_class_str;
        new_win->icon = DUMMY_STRING;
        new_win->name = DUMMY_STRING;
        new_win->win_id = (guint)mev->window;
        new_win->is_dialog = 1;
        new_win->view_id = 0;
        new_win->menuitem_widget = NULL;
        
        new_win->view_id = get_active_view(mev->window);
        
        if (find_application_from_tree(model, &parent, wm_class_str))
        {
            if (new_win->view_id == 0)
            {
                gtk_tree_model_get(model, &parent, WM_VIEW_ID_ITEM,
                                   &new_win->view_id, -1);
                if (new_win->view_id == 0)
                {
                    XFree(wm_class_str);
                    g_free(new_win);
                    return;
                }
            }
            insert_new_window(model, &parent, new_win);
        }
        
        new_win->dialog_name = get_window_title(mev->window);
        
        g_free(new_win->dialog_name);
        XFree(wm_class_str);
        return;
    }
    
    /* No. It's a normal window, so add it. This needs to be done, because
       the non-Hildon apps would not otherwise get registered. */

    if (window_type != NORMAL_WINDOW)
    {
        XFree(wm_class_str);
        return;
    }
    
    if (find_application_from_tree(model, &parent, wm_class_str))
    {
        
        /* Regardless whether it's HildonApp or not, it's safe to
           say that the application is not minimized. */

        gtk_tree_store_set(GTK_TREE_STORE(model), &parent,
                           WM_MINIMIZED_ITEM, FALSE, -1);

	/* Determine whether it is a HildonApp, because applications
           without view supports needs to have different handling... */
        
        if (is_window_hildonapp(mev->window))
        {
            gboolean killed;
            XEvent xev;
            gtk_tree_model_get(model, &parent, WM_VIEW_ID_ITEM, &id,
                               WM_KILLED_ITEM, &killed, -1);
            if (id != 0 && killed)
            {
                memset(&xev, 0, sizeof(xev));
                xev.xclient.type = ClientMessage;
                xev.xclient.serial = 0;
                xev.xclient.send_event = True;
                xev.xclient.window = (Window)id;
                xev.xclient.message_type = active_win;
                xev.xclient.format = 32;
                xev.xclient.data.l[0] = 0;
                gdk_error_trap_push();
                XSendEvent(GDK_DISPLAY(), mev->window, False,
                           SubstructureRedirectMask
                           | SubstructureNotifyMask, &xev);
                gdk_error_trap_pop();
            }
            if (killed)
            {
                /* Could be merged with the above statement?*/
                gtk_tree_store_set(GTK_TREE_STORE(model), &parent,
                                   WM_KILLED_ITEM, FALSE, -1);
            }
            /* This will be handled by the property handlers */
            XFree(wm_class_str);
            return;
        }
        
        if (!window_exists(model, &parent, &w_iter, mev->window, id))
        {
            new_win = g_malloc0(sizeof(window_props));
            gtk_tree_model_get(model, &parent,
                               WM_ICON_NAME_ITEM, &new_win->icon,
                               WM_NAME_ITEM, &new_win->name, -1);
            
            new_win->exec = (gchar *)wm_class_str;
            new_win->is_dialog = 0;
            new_win->view_id = id;
            new_win->win_id = (guint)mev->window;
            new_win->menuitem_widget =
                wm_cbs.new_win_cb(new_win->icon, new_win->name,
                                  DUMMY_STRING, wm_cbs.cb_data);
            insert_new_window(model, &parent, new_win);
            g_free(new_win);
        }
    }
    XFree(wm_class_str);
}


static gboolean remove_view_helper(GtkTreeModel *model, GtkTreePath *path,
                                   GtkTreeIter *iter, gpointer data)
{
    gulong win_id, view_id;
    int is_dialog = 0;
    gtk_tree_model_get(model, iter, WM_ID_ITEM, &win_id,
                       WM_DIALOG_ITEM, &is_dialog,
                       WM_VIEW_ID_ITEM, &view_id, -1);
    if ((gulong)((view_clean_t *)data)->window_id == win_id &&
        (is_dialog == 0) && (view_id != 0))
    {
        GtkTreeIter parent;
        /* We do not want to touch OOM killed/statesaved views */
        if (gtk_tree_model_iter_parent(model, &parent, iter))
        {
            gboolean killed;
            gtk_tree_model_get(model, &parent, WM_KILLED_ITEM, &killed, -1);
            if (killed == TRUE)
            {
                return FALSE;
            }
        }
        if (((view_clean_t *)data)->viewlist == NULL)
        {
            GtkTreeRowReference  *rowref;
            rowref = gtk_tree_row_reference_new(model, path);
            wm_cbs.purge_win_list =
                g_list_append(wm_cbs.purge_win_list, rowref); 
        }
        else
        {
            gboolean keep = FALSE;
            int view_ctr;
            union win_value *wl = ((view_clean_t *)data)->viewlist;
            gulong numviews = (gulong)((view_clean_t *)data)->num_views;
            
            for (view_ctr = 0; view_ctr < numviews ; view_ctr++)
            {
                if ( wl->window_value[view_ctr] == view_id )
                {
                    keep = TRUE;
                    break;
                }
            }
            if (keep == FALSE)
            {
                GtkTreeRowReference  *rowref;
                rowref = gtk_tree_row_reference_new(model, path);
                wm_cbs.purge_win_list =
                    g_list_append(wm_cbs.purge_win_list, rowref);
            }
            
        }
    }
    return FALSE;
}


static gboolean remove_helper(GtkTreeModel *model, GtkTreePath *path,
                              GtkTreeIter *iter, gpointer data)

{
    guint id;
    gboolean killed;
    /* Do not add statesaved applications that were killed by the
       OOM operation */
    gtk_tree_model_get(model, iter, WM_ID_ITEM, &id,
                       WM_KILLED_ITEM, &killed, -1);
    if  ((gulong)((view_clean_t *) data)->window_id == id && killed != TRUE)
    {
        GtkTreeRowReference  *rowref;
        rowref = gtk_tree_row_reference_new(model, path);
        wm_cbs.purge_win_list = g_list_append(wm_cbs.purge_win_list, rowref);
    }
    return FALSE;
}



GtkTreeModel *create_model(void)
{
    gchar *path, *exec, *icon_name, *startup_wmclass, *binitem, *appname;
    gchar *snotify_str;
    gboolean startupnotify;
    DIR *directory = NULL;
    struct dirent *entry = NULL;
    GtkTreeStore *store = gtk_tree_store_new(WM_NUM_TASK_ITEMS,
                                             G_TYPE_STRING,
                                             G_TYPE_STRING,
                                             G_TYPE_STRING,
                                             G_TYPE_INT,
                                             G_TYPE_ULONG,
                                             G_TYPE_STRING,
                                             G_TYPE_POINTER,
                                             G_TYPE_ULONG,
                                             G_TYPE_STRING,
                                             G_TYPE_BOOLEAN,
                                             G_TYPE_BOOLEAN,
                                             G_TYPE_BOOLEAN,
                                             G_TYPE_BOOLEAN);
    
     directory = opendir(DESKTOPENTRYDIR);
    if (directory == NULL)
    {
        return NULL;
    }

    

    while ( (entry = readdir(directory) ) != NULL)
    {
	MBDotDesktop *desktop;
	GtkTreeIter iter;
        appname = NULL;
	startupnotify = TRUE;

	if (!g_str_has_suffix(entry->d_name, DESKTOP_SUFFIX))
            continue;


        path = g_build_filename(DESKTOPENTRYDIR, entry->d_name, NULL);
	desktop = mb_dotdesktop_new_from_file(path);

	
	if (!desktop)
        {
            osso_log(LOG_WARNING, "Could not open [%s]\n", path);
            g_free(path);
            continue;
        }

        appname = (char *) mb_dotdesktop_get(desktop, DESKTOP_VISIBLE_FIELD);
        startup_wmclass = 
            (char *) mb_dotdesktop_get(desktop, DESKTOP_SUP_WMCLASS);
        binitem = (char *) mb_dotdesktop_get(desktop, DESKTOP_BIN_FIELD);

        /* Things will break if we have application without valid name
           or the fields that provide WM_CLASS comparison info */
           
        if (appname == NULL ||
            (binitem == NULL && startup_wmclass == NULL))
        {
            osso_log(LOG_ERR,
                     "TN: Desktop file %s has invalid fields", path);
            g_free(path);
            mb_dotdesktop_free(desktop);
            continue;
        }

        g_free(path);
	exec = (char *) mb_dotdesktop_get(desktop, DESKTOP_LAUNCH_FIELD);
        icon_name =
            (char *) mb_dotdesktop_get(desktop, DESKTOP_ICON_FIELD);

	snotify_str = (char *)mb_dotdesktop_get(desktop,
						DESKTOP_STARTUPNOTIFY); 

	if (snotify_str != NULL)

	  {
	    startupnotify = ( g_ascii_strcasecmp(snotify_str, "false"));
	  }

	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set(store, &iter,
			   WM_ICON_NAME_ITEM, icon_name,
                           WM_EXEC_ITEM, exec,
			   WM_NAME_ITEM, _(appname),
                           WM_DIALOG_ITEM, 0,
                           WM_MENU_PTR_ITEM, NULL,
                           WM_VIEW_ID_ITEM, 0,
                           WM_KILLABLE_ITEM, FALSE,
                           WM_KILLED_ITEM, FALSE,
                           WM_MINIMIZED_ITEM, FALSE,
                           WM_STARTUP_ITEM, startupnotify,
                           -1);
        if (startup_wmclass != NULL)
        {
            gtk_tree_store_set(store, &iter, WM_BIN_NAME_ITEM,
                               startup_wmclass, -1);
        }
        else
        {
            gtk_tree_store_set(store, &iter, WM_BIN_NAME_ITEM,
                               binitem, -1);
        }
	/* This frees all .desktop keys also */
	mb_dotdesktop_free(desktop);
    }

    /* Add checks? */
    closedir(directory);
    g_free(entry);
    return GTK_TREE_MODEL(store);

}


static int find_application_from_tree(GtkTreeModel * model,
				      GtkTreeIter * iter,
				      const gchar * name)
{
    int has_more = 0, ret = 0;
    g_assert(model && iter);
    
    if (name == NULL)
    {
        osso_log(LOG_ERR,
                 "TN: NULL name at find_application_from_tree\n");
        return ret;
    }

    for (has_more = gtk_tree_model_get_iter_first(model, iter);
         has_more; has_more = gtk_tree_model_iter_next(model, iter))
    {
        gchar *exec;
        gchar *basename = NULL;
        gtk_tree_model_get(model, iter, WM_BIN_NAME_ITEM, &exec, -1);
        basename = g_path_get_basename(exec);
        if (exec && g_str_equal(name, basename) == TRUE)
	{
            ret = 1;
	}
        g_free(exec);
        g_free(basename);
        if (ret)
            return ret;
    }
    return ret;
}


static int find_service_from_tree(GtkTreeModel * model,
                                  GtkTreeIter * iter,
                                  const gchar * service)
{
    int has_more = 0, ret = 0;
    g_assert(model && iter);
    
    /* If we did not get valid service name as input, we can hardly
       find any matches for it either... */

    if (service == NULL)
    {
        osso_log(LOG_ERR,
                 "TN: NULL service name at find_service_from_tree\n");
        return ret;
    }

    for (has_more = gtk_tree_model_get_iter_first(model, iter);
         has_more; has_more = gtk_tree_model_iter_next(model, iter))
    {
        gchar *exec;
        gtk_tree_model_get(model, iter, WM_EXEC_ITEM, &exec, -1);

        if (exec && g_str_equal(service, exec) == TRUE)

	{
            ret = 1;
	}

        g_free(exec);
	
        if (ret)
            return ret;
    }
    return ret;
}


static gulong find_window_id_for_view(GtkTreeModel *model,
                                      GtkTreeIter *parent,
                                      GtkTreeIter *target_iter,
                                      const gulong view_id)
{
    int hasmore;
    for (hasmore = gtk_tree_model_iter_children(model, target_iter, parent);
	 hasmore; hasmore = gtk_tree_model_iter_next(model, target_iter))
    {
        gulong val;
	gulong view;
        gint is_dialog;
	gtk_tree_model_get(model, target_iter, WM_ID_ITEM, &val,
			   WM_VIEW_ID_ITEM, &view,
                           WM_DIALOG_ITEM, &is_dialog, -1);
        
	if ( (view_id == view) && !is_dialog)
	{
	    return val;
	}
    }
    
    return 0;  
}


static int window_exists(GtkTreeModel * model, GtkTreeIter * parent,
                         GtkTreeIter * w_iter,
			 gulong id, gulong view_id)
{
    int hasmore;
    GtkTreeIter iter;
    for (hasmore = gtk_tree_model_iter_children(model, &iter, parent);
	 hasmore; hasmore = gtk_tree_model_iter_next(model, &iter))
    {
	gulong val;
	gulong view;
        gint is_dialog;
	gtk_tree_model_get(model, &iter, WM_ID_ITEM, &val,
			   WM_VIEW_ID_ITEM, &view,
                           WM_DIALOG_ITEM, &is_dialog, -1);
	if (id == val && (view_id == view) && !is_dialog)
	{
            *w_iter = iter;
	    return 1;
	}
    }
    
    return 0;
}

static void insert_new_window(GtkTreeModel * model, GtkTreeIter * parent,
			      window_props * new_win_props)
{
    GtkTreeIter iter;
    gtk_tree_store_insert(GTK_TREE_STORE(model), &iter, parent, 0);
    gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
                       WM_ICON_NAME_ITEM, new_win_props->icon,
                       WM_NAME_ITEM, new_win_props->name,
                       WM_BIN_NAME_ITEM, new_win_props->exec,
                       WM_VIEW_ID_ITEM, new_win_props->view_id,
                       WM_ID_ITEM, new_win_props->win_id,
                       WM_DIALOG_ITEM, new_win_props->is_dialog,
                       WM_MENU_PTR_ITEM,
                       new_win_props->menuitem_widget,
                       -1);
    
}


static void update_window(GtkTreeModel * model,
			  GtkTreeIter * parent, unsigned long id,
			  unsigned long view_id, unsigned long new_view_id,
                          const gchar *view_name,
                          const guint position_change)
{
    int hasmore, num_children;
    gchar *icon_name, *app_name;
    GtkTreeIter iter;
    gboolean killable, killed;
    GtkWidget *menuwidget;

    for (hasmore = gtk_tree_model_iter_children(model, &iter, parent),
             num_children = 0;
	 hasmore;
	 hasmore = gtk_tree_model_iter_next(model, &iter), ++num_children)
    {
	unsigned long val, view_val;
        gint is_dialog;
	gtk_tree_model_get(model, &iter, WM_NAME_ITEM, &app_name,
			   WM_ICON_NAME_ITEM, &icon_name,
			   WM_VIEW_ID_ITEM, &view_val,
                           WM_DIALOG_ITEM, &is_dialog,
			   WM_ID_ITEM, &val, WM_MENU_PTR_ITEM, &menuwidget,
                           WM_KILLABLE_ITEM, &killable,
                           WM_KILLED_ITEM, &killed,
			   -1);
	if (id == val && view_id == view_val && !is_dialog)
        {
	    break;
        }
        g_free(app_name);
        g_free(icon_name);
    }
    
    g_assert(hasmore);
    
    

    if (view_id >=0 )
    {
	gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
			   WM_VIEW_ID_ITEM, new_view_id,
                           WM_DIALOG_ITEM, 0,
			   WM_VIEW_NAME_ITEM, view_name, -1);
    }

    wm_cbs.updated_win_cb(menuwidget, icon_name, app_name, view_name,
                          position_change, killable, NULL,
                          wm_cbs.cb_data);
    g_free(app_name);
    g_free(icon_name);
    /* Return if we have only one child,
     *  since gtk_tree_store_move_after segfaults in
     *  that case! */
    if (num_children <= 1)
	return;
    
    gtk_tree_store_move_after(GTK_TREE_STORE(model), &iter, NULL);
}


static gchar *find_window_wmclass(GtkTreeModel *model, GtkTreeIter *parent,
                                  Window win_id)
{
    int child_number, i;
    gchar *exec = NULL, *temp_exec = NULL;
    gulong item_win_id;
    
    GtkTreeIter iter;
    
    gtk_tree_model_get(model, parent, WM_ID_ITEM, &item_win_id,
                       WM_BIN_NAME_ITEM, &temp_exec, -1);
    if (item_win_id == win_id)
    {
        return temp_exec;
        
    }
    g_free(temp_exec);
    if (gtk_tree_model_iter_has_child(model, parent))
    {
        child_number = gtk_tree_model_iter_n_children(model, parent);
        for (i = 0; i < child_number; i++)
        {
            gtk_tree_model_iter_nth_child(model, &iter, parent, i);
            if ( (exec = find_window_wmclass(model, &iter, win_id)) != NULL)
            {
                return exec;
            }
        }
    }
    
    if (gtk_tree_model_iter_next(model, parent))
    {
        
        if ( (exec = find_window_wmclass(model, parent, win_id)) != NULL)
        {
            return exec;
        }
    }
    
    return exec;
}


static void handle_active_window_prop(GtkTreeModel *model,
                                      const gchar *wm_class_str,
                                      const Window win_id)
{
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *subname_str = NULL;

    guchar *local_wm_class = NULL;
    guint update_flag = AS_MENUITEM_TO_FIRST_POSITION;

    GtkTreeIter parent, w_iter;

    union win_value value, realwin_value;

    Atom actual_type;

    realwin_value.char_value = NULL;
    gdk_error_trap_push();
    
    XGetWindowProperty(GDK_DISPLAY(), win_id, active_win,
                       0, 32, False, XA_WINDOW, &actual_type,
                       &actual_format, &nitems, &bytes_after,
                       (unsigned char **)&value.char_value);
    if (gdk_error_trap_pop() != 0)
    {
        /* We can't do anything without the active window property */
        return;
    }

    /* We need also to check whether this application is the
       active (i.e. topmost) one, because we shouldn't update
       things in application switcher otherwise... */


    gdk_error_trap_push();
    XGetWindowProperty(GDK_DISPLAY(),
                       GDK_WINDOW_XID(gdk_get_default_root_window()),
                       mb_active_win, 0, 32, False, XA_WINDOW, &actual_type,
                       &actual_format, &nitems, &bytes_after,
                       (unsigned char **)&realwin_value.char_value);
    if (gdk_error_trap_pop() != 0 || nitems < 1)
    {
        /* We couldn't find out - let's not update just in case */
        update_flag = AS_MENUITEM_SAME_POSITION;
    }
    else if (realwin_value.window_value[0] != win_id)
    {
        update_flag = AS_MENUITEM_SAME_POSITION;
    }
    
    /* No WM_CLASS info acquired before? Try to get it one more
       time. This is most common when an application itself has been
       topped, instead of just switching to a view inside an
       application. */

    if (wm_class_str == NULL)
    {
        gdk_error_trap_push();
        XGetWindowProperty(GDK_DISPLAY(), realwin_value.window_value[0],
                           wm_class_atom, 0, 24,
                           False, XA_STRING, &actual_type, &actual_format,
                           &nitems, &bytes_after, &local_wm_class);
        gdk_error_trap_pop();
       
        /* If we can't get wm_class for this window, we're unable to do
           anything right now.*/

        if (local_wm_class == NULL)
        {
            XFree(realwin_value.char_value);
            XFree(value.char_value);
            return;
        }

        if (find_application_from_tree(model, &parent, local_wm_class) )
        {
            gulong id;
            gtk_tree_model_get(model, &parent, WM_VIEW_ID_ITEM, &id, -1);
           
            if (id != 0 && window_exists(model, &parent, &w_iter,
                                         realwin_value.window_value[0], id))
            {
                gtk_tree_model_get(model, &w_iter,
                                   WM_VIEW_NAME_ITEM, &subname_str, -1);
                update_window(model, &parent, realwin_value.window_value[0],
                              id, id, subname_str,
                              AS_MENUITEM_TO_FIRST_POSITION);
                
                g_free(subname_str);
            }
            else if (id == 0 )
            {
                /* Seems that apps without views need this... However,
                   let's still check that this is not an application
                   with registered views (as we get them asynchronously) */

                if (is_window_hildonapp(win_id))
                {
                    XFree(local_wm_class);
                    XFree(value.char_value);
                    XFree(realwin_value.char_value);
                    return;
                }

                gchar *exec, *name, *icon;
                gtk_tree_model_get(model, &parent,
                                   WM_ICON_NAME_ITEM, &icon,
                                   WM_BIN_NAME_ITEM, &exec,
                                   WM_NAME_ITEM, &name, -1);
                
                
                window_props *new_win;
                
                new_win = g_malloc(sizeof(window_props));
                new_win->icon = (gchar *)icon;
                new_win->name = (gchar *)name;
                new_win->exec = (gchar *)exec;
                new_win->win_id = (gulong)realwin_value.window_value[0];
                new_win->view_id = id;
                new_win->is_dialog = 0;
                
                new_win->menuitem_widget = 
                    wm_cbs.new_win_cb(icon, name,
                                      subname_str, wm_cbs.cb_data);
                insert_new_window(model, &parent, 
                                  new_win);
                g_free(name);
                g_free(icon);
                g_free(exec);
            }
        }
        XFree(value.window_value);
        XFree(realwin_value.window_value);
        XFree(local_wm_class);
        return;
    }

    /* It is quite probably an application with more than one view.
       Thus, updating the active view status on Application Switcher. */
    


    if (find_application_from_tree(model, &parent, wm_class_str) )
    {
        gboolean killed;
        gtk_tree_model_get(model, &parent, WM_KILLED_ITEM, &killed, -1);
        if (killed == TRUE)
        {
            XFree(value.char_value);
            return;
        }

        subname_str = get_subname(win_id);
        gtk_tree_store_set(GTK_TREE_STORE(model), &parent,
                           WM_VIEW_ID_ITEM, value.window_value[0], -1);
        if (window_exists(model, &parent, &w_iter, win_id, 0))
        {
            update_window(model, &parent, win_id, 0, value.window_value[0],
                          subname_str, update_flag);
            g_free(subname_str);
            return;
        }
        else if (window_exists(model, &parent, &w_iter, win_id,
                          value.window_value[0]))
        {
            update_window(model, &parent, win_id, value.window_value[0],
                          value.window_value[0], subname_str, update_flag);
        }
    }
    g_free(subname_str);
    g_free(value.char_value);
    if (realwin_value.char_value != NULL)
    {
    g_free(realwin_value.char_value);
    }
    if (local_wm_class != NULL)
    {
        XFree(local_wm_class);
    }
    wm_cbs.model = model;
}



static void handle_subname_window_prop(GtkTreeModel *model,
                                       const gchar *wm_class_str,
                                       const Window win_id)
{
    int actual_format;
    unsigned long nitems = 0, bytes_after = 0;
    unsigned char *subname_str = NULL;

    union win_value value;
     
    GtkTreeIter parent, w_iter;

    Atom actual_type;

    gdk_error_trap_push();
    XGetWindowProperty(GDK_DISPLAY(), win_id, active_win,
                       0, 32, False, XA_WINDOW, &actual_type,
                       &actual_format, &nitems, &bytes_after,
                       (unsigned char **)&value.char_value);
    
    /* If the _NET_ACTIVE_WINDOW is not set, we can't do anything */
    if (nitems == 0 || gdk_error_trap_pop() != 0)
    {
        return;
    }

    if (find_application_from_tree(model, &parent, wm_class_str) )
    {
        subname_str = get_subname(win_id);

        if (window_exists(model, &parent, &w_iter, win_id, 0))
        {
            update_window(model, &parent, win_id, 0,
                          value.window_value[0], subname_str,
                          AS_MENUITEM_SAME_POSITION);
            g_free(subname_str);
            XFree(value.char_value);
            return;
        }   
        
        if (window_exists(model, &parent, &w_iter, win_id,
                          value.window_value[0]))
        {
            update_window(model, &parent, win_id, value.window_value[0],
                          value.window_value[0], subname_str,
                          AS_MENUITEM_SAME_POSITION);
            g_free(subname_str);
            XFree(value.char_value);
            return;
        }
        g_free(subname_str);
    }
    XFree(value.char_value);
}



static void handle_client_list_prop(GtkTreeModel *model,
                                    const gchar *wm_class_str,
                                    const Window win_id)
{
    view_clean_t cleaner;
    int actual_format, err;
    unsigned long nitems, bytes_after;
    guchar *subname_str = NULL;
    GtkTreeIter parent, w_iter;
    Atom actual_type;
    
    union win_value value;
    
    wm_cbs.purge_win_list = NULL;
    cleaner.window_id = win_id;
    
    gdk_error_trap_push();
    XGetWindowProperty(GDK_DISPLAY(), win_id, clientlist,
                       0, 128, False, XA_WINDOW, &actual_type,
                       &actual_format, &nitems, &bytes_after,
                       (unsigned char **)&value.char_value);
    gdk_flush();
    err = gdk_error_trap_pop();
    
    if (err != 0)
    {
        return;
    }
    if (nitems == 0)
    {
        /* Clean up all views for this window */
        cleaner.viewlist = NULL;
        gtk_tree_model_foreach(model, remove_view_helper,
                               (gpointer)&cleaner);
        do_cleanup(model);
        
    }
    if (nitems > 0)
    {
        int clist_nitems = nitems;
        /* First clean up all views that are not in the clientlist */
        cleaner.viewlist = &value;
        cleaner.num_views = clist_nitems;
        gtk_tree_model_foreach(model, remove_view_helper,
                               (gpointer)&cleaner);
        do_cleanup(model);
        
        if (find_application_from_tree(model, &parent, wm_class_str) )
        {
            subname_str = get_subname(win_id);
            
            if (window_exists(model, &parent, &w_iter, win_id,
                              0))
            {
                update_window(model, &parent, win_id, 0,
                              value.window_value[clist_nitems-1],
                              subname_str, AS_MENUITEM_SAME_POSITION);
                XFree(value.char_value);
                g_free(subname_str);
                return;
            }
            
            if (window_exists(model, &parent, &w_iter, win_id,
                              value.window_value[clist_nitems-1]) )
            {
                update_window(model, &parent, win_id,
                              value.window_value[clist_nitems-1],
                              value.window_value[clist_nitems-1],
                              subname_str, AS_MENUITEM_SAME_POSITION);
		gtk_tree_store_set(GTK_TREE_STORE(model), &parent,
				   WM_VIEW_ID_ITEM,
				   value.window_value[clist_nitems-1], -1);
			
                XFree(value.char_value);
                g_free(subname_str);
                return;
            }
	    else
              {
		window_props *new_win =  NULL;
                  gchar *icon, *name, *exec;
                  
                  gtk_tree_model_get(model, &parent,
                                     WM_ICON_NAME_ITEM, &icon,
                                     WM_BIN_NAME_ITEM, &exec,
                                     WM_NAME_ITEM, &name, -1);
                  
                  /* Create a new window and append it to the model. */
                  new_win = g_malloc0(sizeof(window_props));
                  new_win->icon = icon;
                  new_win->name = name;
                  new_win->exec = exec;
                  new_win->win_id = (gulong)win_id;
                  new_win->is_dialog = 0;
                  
                  new_win->view_id = value.window_value[clist_nitems-1];

                  new_win->menuitem_widget =
                      wm_cbs.new_win_cb(icon, name,
                                        subname_str, wm_cbs.cb_data);
                  insert_new_window(model, &parent,
                                    new_win);
		  gtk_tree_store_set(GTK_TREE_STORE(model), &parent,
				     WM_VIEW_ID_ITEM, new_win->view_id, -1);
                  g_free(icon);
                  g_free(exec);
                  g_free(name);
                  g_free(new_win);
              }
        g_free(subname_str);
        }
    }
    else
    {
        /* Can we lose all views and revert back to bare X window? */
    }
    XFree(value.window_value);
}


static void handle_killable_prop(GtkTreeModel *model,
                                 const gchar *wm_class_str,
                                 const Window win_id,
                                 int state)
{
    GtkTreeIter parent;

    if (state == PropertyDelete)
    {
        if (find_application_from_tree(model, &parent, wm_class_str) )
        {
            int hasmore;
            gchar *view_name, *icon_name, *app_name;
            gpointer widget_ptr;
            GtkTreeIter iter;
            gtk_tree_store_set(GTK_TREE_STORE(model), &parent,
                               WM_KILLABLE_ITEM, FALSE, -1);

            for (hasmore = gtk_tree_model_iter_children(model,
                                                        &iter,
                                                        &parent);
                 hasmore; hasmore = gtk_tree_model_iter_next(model, &iter))
            {
                     gtk_tree_model_get(model, &iter,
                                        WM_MENU_PTR_ITEM,
                                        &widget_ptr,
                                        WM_VIEW_NAME_ITEM, &view_name,
                                        WM_ICON_NAME_ITEM, &icon_name,
                                        WM_NAME_ITEM, &app_name,
                                        -1);
                     if (widget_ptr != NULL)
                     {
                         wm_cbs.updated_win_cb(widget_ptr, icon_name,
                                               app_name, view_name,
                                               AS_MENUITEM_SAME_POSITION,
                                               FALSE, NULL,
                                               wm_cbs.cb_data);
                         gtk_tree_store_set(GTK_TREE_STORE(model),
                                            &parent, WM_KILLED_ITEM, FALSE,
                                            -1);
                     }

            }
            return;
        }
        
    }
    else
    {
        if (find_application_from_tree(model, &parent, wm_class_str) )
        {
            int hasmore;
            gchar *view_name, *icon_name, *app_name;
            gpointer widget_ptr;
            GtkTreeIter iter;

            gtk_tree_store_set(GTK_TREE_STORE(model), &parent,
                               WM_KILLABLE_ITEM, TRUE, -1);

            /* If we're in bgkill situation, we'll kill the application
               right here. */

            if (bgkill_situation)
            {
                kill_application(model, &parent, TRUE);
                return;
            }
            

            for (hasmore = gtk_tree_model_iter_children(model,
                                                        &iter,
                                                        &parent);
                 hasmore; hasmore = gtk_tree_model_iter_next(model, &iter))
            {
                gtk_tree_model_get(model, &iter,
                                   WM_MENU_PTR_ITEM,
                                   &widget_ptr,
                                   WM_VIEW_NAME_ITEM, &view_name,
                                   WM_ICON_NAME_ITEM, &icon_name,
                                   WM_NAME_ITEM, &app_name,
                                   -1);


                if (widget_ptr != NULL)
                {
                    wm_cbs.updated_win_cb(widget_ptr, icon_name,
                                          app_name, view_name,
                                          AS_MENUITEM_SAME_POSITION,
                                          TRUE, NULL,
                                          wm_cbs.cb_data);

                }
                
            }
            
        }
    }
}


static void handle_minimization(GtkTreeModel *model,
                                const gchar *wm_class_str,
                                const Window win_id)
{
    GtkTreeIter parent, iter;
    gchar *app_name = NULL, *view_name = NULL, *icon_name = NULL;
    gpointer widget_ptr = NULL;
    gboolean killable, is_dialog;
    int hasmore;
   
    if (find_application_from_tree(model, &parent, wm_class_str) )
    {

        for (hasmore = gtk_tree_model_iter_children(model,
                                                    &iter,
                                                    &parent);
             hasmore; hasmore = gtk_tree_model_iter_next(model, &iter))
        {
            
            gtk_tree_model_get(model, &iter, WM_MENU_PTR_ITEM,
                               &widget_ptr,
                               WM_VIEW_NAME_ITEM, &view_name,
                               WM_ICON_NAME_ITEM, &icon_name,
                               WM_NAME_ITEM, &app_name,
                               WM_KILLABLE_ITEM, &killable,
                               WM_DIALOG_ITEM, &is_dialog, -1);
            
            if (widget_ptr == NULL || is_dialog == TRUE)
            {
                continue;
            }
            gtk_tree_store_set(GTK_TREE_STORE(model),
                               &parent, WM_MINIMIZED_ITEM, TRUE, -1);
            wm_cbs.updated_win_cb(widget_ptr, icon_name, app_name,
                                  view_name, AS_MENUITEM_TO_LAST_POSITION,
                                  killable, NULL, wm_cbs.cb_data);
        }
        g_free(app_name);
        g_free(view_name);
        g_free(icon_name);
    }
}


static int determine_window_type(Window win_id)
{
    int window_type = OTHER, actual_format, i;
    unsigned long nitems, nitems_wmstate = 0, bytes_after;
    
    union atom_value wm_type_value, wm_state_value;
    
    Atom actual_type;

    gdk_error_trap_push();
    XGetWindowProperty(GDK_DISPLAY(), win_id, wm_type, 0,
                       G_MAXLONG, False, XA_ATOM, &actual_type,
                       &actual_format, &nitems, &bytes_after,
                       (unsigned char **)&wm_type_value.char_value);
    
    if (gdk_error_trap_pop() != 0)
    {
        return UNKNOWN;
    }

    /* FIXME: Why loop here, only one type atom per window.. */
    for (i = 0; i < nitems; i++)
    {
        /* The menu case is tested first, as it will probably be a very
           common one */

        if (wm_type_value.atom_value[i] == Atom_NET_WM_WINDOW_TYPE_MENU)
        {
            window_type = MENU;
            break;
        }

        if (wm_type_value.atom_value[i] == Atom_NET_WM_WINDOW_TYPE_NORMAL)
            
        {
            window_type = NORMAL_WINDOW;

	    /* FIXME: Below is bogus ?? application windows cant be modal..? 
	    */
#if 0
            gdk_error_trap_push();
            XGetWindowProperty(GDK_DISPLAY(), win_id, net_wm_state, 0,
                               G_MAXLONG, False, XA_ATOM, &actual_type,
                               &actual_format, &nitems_wmstate, &bytes_after,
                               (unsigned char **)&wm_state_value.char_value);
            if (gdk_error_trap_pop() != 0)
            {
                break;
            }
            if (nitems_wmstate > 0)
            {
                if (wm_state_value.atom_value[0] == Atom_NET_WM_STATE_MODAL)
                {
                    window_type = OTHER;
                    XFree(wm_state_value.char_value);
                }
                break;
            }
#endif
        }
        else if (wm_type_value.atom_value[i] == Atom_NET_WM_WINDOW_TYPE_DIALOG)
        {
	  /* FIXME: What is window_type is dialog is not modal ?  */

	  gdk_error_trap_push();
	  XGetWindowProperty(GDK_DISPLAY(), win_id, net_wm_state, 0,
			     G_MAXLONG, False, XA_ATOM, &actual_type,
			     &actual_format, &nitems_wmstate, &bytes_after,
			     (unsigned char **)&wm_state_value.char_value);
	  if (gdk_error_trap_pop() != 0)
            {
	      break;
            }

	  if (nitems_wmstate > 0)
            {
	      int j;

	      for (j=0; j < nitems_wmstate; j++)
		if (wm_state_value.atom_value[j] == Atom_NET_WM_STATE_MODAL)
		  {
		    window_type = MODAL_DIALOG;
		    break;
		  }
            }
	  
	  if (wm_state_value.char_value != NULL)
            {
	      XFree(wm_state_value.char_value);
	      wm_state_value.char_value = NULL;
            }

	  break;
	}
        else if (wm_type_value.atom_value[i] == Atom_NET_WM_WINDOW_TYPE_DESKTOP)
        {
            window_type = DESKTOP;
        }
        
    }

    XFree(wm_type_value.char_value);

    return window_type;
}


static guchar *get_window_title(Window win_id)
{
    int actual_format;
    unsigned long nitems, bytes_after;
    guchar *window_title = NULL;

    Atom actual_type;

    gdk_error_trap_push();
    XGetWindowProperty(GDK_DISPLAY(), win_id, wm_name, 0, 24,
                       False, XA_STRING, &actual_type,
                       &actual_format,
                       &nitems, &bytes_after,
                       &window_title);
    if (gdk_error_trap_pop() != 0)
    {
        window_title = g_strdup(UNKNOWN_TITLE);
    }
    return window_title;
}


static gulong get_active_view(Window win_id)
{
    int actual_format;
    unsigned long nitems, bytes_after;
    
    gulong active_view = 0;
    gulong transient_win = 0;

    union win_value window_value;    
    Atom actual_type;

    gdk_error_trap_push();
    XGetWindowProperty(GDK_DISPLAY(), win_id, transient_for,
                       0, 32, False, XA_WINDOW, &actual_type,
                       &actual_format, &nitems, &bytes_after,
                       (unsigned char **)&window_value.char_value);
    if ((gdk_error_trap_pop() == 0) && nitems > 0)
    {
        transient_win = window_value.window_value[0];
        XFree(window_value.char_value);
    }
    else
    {
        return 0;
    }
    
    gdk_error_trap_push();
    XGetWindowProperty(GDK_DISPLAY(), transient_win, active_win,
                       0, 32, False, XA_WINDOW, &actual_type,
                       &actual_format, &nitems, &bytes_after,
                       (unsigned char **)&window_value.char_value);
    if (gdk_error_trap_pop() != 0)
    {
        return 0;
    }
    
    if (nitems > 0 && window_value.window_value[0] != 0)
    {
        active_view = window_value.window_value[0];
        XFree(window_value.char_value);
        
    }
    
    return active_view;
    
}


static void do_cleanup(GtkTreeModel *model)
{
    GList *node;
    GtkTreeIter iter, parent;
    gchar *application = NULL;
    gchar *basename = NULL;
    
    for ( node = wm_cbs.purge_win_list;  node != NULL;  node = node->next )
    {
        GtkTreePath *path;
        
        path =
            gtk_tree_row_reference_get_path((GtkTreeRowReference*)node->data);
        if (path)
        {
            if (gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path))
            {
                gpointer widget;
                gint is_dialog;

                /* This has high redundancy and should be avoided... */
                
                gtk_tree_model_get(model, &iter, WM_MENU_PTR_ITEM, &widget,
                                   WM_DIALOG_ITEM, &is_dialog,
                                   WM_BIN_NAME_ITEM, &application, -1);
                if (!is_dialog)
                {
                    wm_cbs.removed_win_cb(widget, wm_cbs.cb_data);
                }
                gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
            }
        }
        gtk_tree_path_free(path);
    }
    
    g_list_foreach(wm_cbs.purge_win_list,
                   (GFunc)gtk_tree_row_reference_free, NULL);
    g_list_free(wm_cbs.purge_win_list);

    if (application == NULL)
    {
        return;
    }
    
    basename = g_path_get_basename(application);
    
    if (basename != NULL)
    {
        if (find_application_from_tree(model, &parent, basename))
        {
            if (!gtk_tree_model_iter_has_child (model, &parent))
            {
                gtk_tree_store_set(GTK_TREE_STORE(model), &parent,
                                   WM_VIEW_ID_ITEM, 0, -1);
            }
        }
        g_free(basename);
        g_free(application);
    }
}    
        


static gboolean is_window_hildonapp(Window xid)
{
    int actual_format;
    unsigned long nitems, bytes_after;

    union win_value value;

    Atom actual_type;

    value.char_value = NULL;

    gdk_error_trap_push();
    XGetWindowProperty(GDK_DISPLAY(), xid, active_win,
                       0, 32, False, XA_WINDOW, &actual_type,
                       &actual_format, &nitems, &bytes_after,
                       (unsigned char **)&value.char_value);
    if (gdk_error_trap_pop() == 0 && 
        (actual_type == XA_WINDOW && value.char_value != NULL) )
    {
        XFree(value.char_value);
        return TRUE;
    }

    if (value.char_value != NULL)
    {
        XFree(value.char_value);
        value.char_value = NULL;
    }

    gdk_error_trap_push();
    XGetWindowProperty(GDK_DISPLAY(), xid, clientlist,
                       0, 32, False, XA_WINDOW, &actual_type,
                       &actual_format, &nitems, &bytes_after,
                       (unsigned char **)&value.char_value);
    if (gdk_error_trap_pop() == 0 &&
        (actual_type == XA_WINDOW && value.char_value != NULL))
    {
        XFree(value.char_value);
        return TRUE;
    }
    return FALSE;
}

static void top_non_hildonapp(Window xid)
{
    XEvent xev;
    memset(&xev, 0, sizeof(xev));
    xev.xclient.type = ClientMessage;
    xev.xclient.serial = 0;
    xev.xclient.send_event = True;
    xev.xclient.window = (Window)xid;
    xev.xclient.message_type = active_win;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 0;
    gdk_error_trap_push();
    XSendEvent(GDK_DISPLAY(),
               GDK_WINDOW_XID(gdk_get_default_root_window()),
               False, SubstructureRedirectMask |
               SubstructureNotifyMask, &xev);
    gdk_error_trap_pop();
    /* Should errors be logged? */
}


static gchar *get_subname(Window win_id)
{
    int actual_format;
    long long offset = 0;
    unsigned long nitems, bytes_after;
    guchar *subname_str = NULL;
    Atom actual_type;

    /* Let's be slightly paranoid and check that we got the whole
       property read, if necessary we continue reading */
    
    do
    {
        gdk_error_trap_push();
        guchar *subname_part = NULL;
        bytes_after = 0;
        XGetWindowProperty(GDK_DISPLAY(), win_id, subname,
                           offset, G_MAXLONG, False,
                           utf8,
                           &actual_type, &actual_format,
                           &nitems, &bytes_after,
                           &subname_part);
        gdk_error_trap_pop();

        if (nitems > 0 && subname_part != NULL)
        {

            if (subname_str != NULL)
            {
                /* Partial read. Concat to the end of subname string. */
                subname_str = g_strconcat(subname_str, subname_part, NULL);
            }
            else
            {
                subname_str = g_strdup(subname_part);
            }

            XFree(subname_part);
        }

        offset += nitems;
    } while (bytes_after > 0);

    if (subname_str == NULL)
    {
        subname_str = g_strdup(DUMMY_STRING);
    }

    return subname_str;
}

static void handle_autotopping()
{
    GList *mitems = NULL;
    menuitem_comp_t menu_comp;
    GtkTreeIter iter;
    gboolean killed, minimized;

   
    mitems = application_switcher_get_menuitems(wm_cbs.cb_data);
    
    /* First two items are Home and the separator */
    
    if (mitems != NULL && g_list_length(mitems) > 2 )
    {
        menu_comp.menu_ptr = 
            g_list_nth_data(mitems, 2);
        menu_comp.wm_class = NULL;
        menu_comp.service = NULL;
        menu_comp.window_id = 0;

        /* Use of gtk_tree_model_foreach is a heavy operation.
           However, topping a killed application is so major operation
           that this overhead can be tolerated (for now...) */
        
        gtk_tree_model_foreach(wm_cbs.model, menuitem_match_helper,
                               &menu_comp);
        if (menu_comp.service != NULL && 
            find_service_from_tree(wm_cbs.model,
                                   &iter, menu_comp.service))
        {
            gtk_tree_model_get(wm_cbs.model, &iter,
                               WM_KILLED_ITEM, &killed,
                               WM_MINIMIZED_ITEM,
                               &minimized, -1);
            
            if (minimized == FALSE && killed == TRUE)
            {
                top_service(menu_comp.service);
                return;
            }
            
        }
    }
    /* Nothing had to be autotopped. So top desktop. */
    wm_cbs.topped_desktop_cb(wm_cbs.cb_data);

    return;
}


/* End of private window/X-property related functions */


static void kill_application(GtkTreeModel *model, GtkTreeIter *parent,
			     gboolean mark_as_killed)
{
    int actual_format;
    unsigned long nitems, bytes_after;
    Atom actual_type;
    int hasmore;
    GtkTreeIter iter;
    int num_children;
    guchar *pid_result = NULL;
    gboolean app_terminated = FALSE;

    for (hasmore = gtk_tree_model_iter_children(model, &iter, parent),
             num_children = 0;
	 hasmore;
	 hasmore = gtk_tree_model_iter_next(model, &iter), ++num_children)
    {
        gulong id;
        gtk_tree_model_get(model, &iter, WM_ID_ITEM, &id, -1);
        if (id != 0)
        {
            nitems = 0;
            if (app_terminated == FALSE)
            {
                gdk_error_trap_push();
                XGetWindowProperty(GDK_DISPLAY(), (Window)id, pid_atom,
                                   0, 32, False, XA_CARDINAL, &actual_type,
                                   &actual_format, &nitems, &bytes_after,
                                   (unsigned char **)&pid_result);
                gdk_error_trap_pop();
            }
            
            gchar *app_name = NULL, *view_name = NULL;
            gchar *icon_name = NULL;
            gpointer widget_ptr;
            
            if (app_terminated == FALSE && pid_result != NULL)
            {
		gchar *name;
		gtk_tree_model_get(wm_cbs.model, &iter,
				   WM_NAME_ITEM, &name, -1);
		osso_log(LOG_INFO, "Killing %s", name);
		g_free (name);

                if(kill(pid_result[0]+256*pid_result[1], SIGTERM) != 0)
                {
                    osso_log(LOG_ERR, "Failed to kill pid %d with SIGTERM", 
                             pid_result[0]+256*pid_result[1]);
                }
            }
            gtk_tree_store_set(GTK_TREE_STORE(model), parent,
                               WM_KILLED_ITEM, mark_as_killed, -1);
            /* We have to update things for application switcher */
            gtk_tree_model_get(wm_cbs.model, &iter,
                               WM_MENU_PTR_ITEM,
                               &widget_ptr,
                               WM_VIEW_NAME_ITEM, &view_name,
                               WM_ICON_NAME_ITEM, &icon_name,
                               WM_NAME_ITEM, &app_name,
                               -1);
            wm_cbs.updated_win_cb(widget_ptr, icon_name, app_name,
                                  view_name, AS_MENUITEM_SAME_POSITION,
                                  FALSE, NULL, wm_cbs.cb_data);
            
            XFree(pid_result);
            pid_result = NULL;

	    update_lowmem_situation(lowmem_situation);
        }
    }

}

static void update_application_window_id(GtkTreeModel *model,
                                         GtkTreeIter *parent,
                                         Window win_id)
{
    GtkTreeIter iter;
    int hasmore, num_children;
    
    for (hasmore = gtk_tree_model_iter_children(model, &iter, parent),
             num_children = 0;
	 hasmore;
	 hasmore = gtk_tree_model_iter_next(model, &iter), ++num_children)
    {
        gulong xid = 0;
        gtk_tree_model_get(model, &iter, WM_ID_ITEM, &xid, -1);
        if (xid != 0)
        {
            gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
                               WM_ID_ITEM, win_id, -1);
        }
        
    }    
}


static gboolean menuitem_match_helper(GtkTreeModel *model,
                                      GtkTreePath *path,
                                      GtkTreeIter *iter,
                                      gpointer data)

{
    gpointer ptr, orig_ptr;
    menuitem_comp_t *mc;
    mc = (menuitem_comp_t *)data;
    gchar *wm_class, *service;
    gboolean killable;
    gulong view_id, window_id;
    orig_ptr = mc->menu_ptr;
    gtk_tree_model_get(model, iter, WM_MENU_PTR_ITEM, &ptr, -1);
    if (ptr == orig_ptr)
    {
        GtkTreeIter parent_iter;
        if (gtk_tree_model_iter_parent(model, &parent_iter, iter))
        {
            gtk_tree_model_get(model, &parent_iter,
                               WM_EXEC_ITEM, &service,
                               WM_KILLABLE_ITEM, &killable, -1);
        }
            gtk_tree_model_get(model, iter,
                               WM_BIN_NAME_ITEM, &wm_class,
                               WM_VIEW_ID_ITEM, &view_id,
                               WM_ID_ITEM, &window_id, -1);
        ((menuitem_comp_t *)data)->wm_class = g_path_get_basename(wm_class);
        ((menuitem_comp_t *)data)->service = g_strdup(service);
        ((menuitem_comp_t *)data)->view_id = view_id;
        ((menuitem_comp_t *)data)->window_id = window_id;
        ((menuitem_comp_t *)data)->killable = killable;

        g_free(wm_class);
        g_free(service);
        return TRUE;
    }
    return FALSE;
}


static int save_session(GArray *arguments, gpointer data)
{
    
    gint i, state_position = 0;
    GList *mitems = NULL;
    GArray *pids = g_array_new(FALSE, FALSE, sizeof(pid_t));
    
    osso_state_t session_state;
    menuitem_comp_t menu_comp;
    osso_context_t *osso;
    osso_manager_t *man = (osso_manager_t *)data;
    
    osso = get_context(man);

    /* Assuming 32-bit unsigned int; 2^32 views should suffice... */
    gchar view_id[10];

    /* Acquire list of menu items from the Application Switcher */
    mitems = application_switcher_get_menuitems(wm_cbs.cb_data);

    if (mitems == NULL)
    {
        osso_log(LOG_WARNING, "Unable to save session state.");
        return FALSE;
    }
    memset(&statedata, 0, sizeof(struct state_data));
    for (i = g_list_length(mitems)-1 ; i >= 2; i--)
    {
        menu_comp.menu_ptr = 
            g_list_nth_data(mitems, i);
        menu_comp.wm_class = NULL;
        menu_comp.service = NULL;
        menu_comp.window_id = 0;
        /* We should also get the PIDs here... */
        gtk_tree_model_foreach(wm_cbs.model, menuitem_match_helper,
                               &menu_comp);
        if (menu_comp.service != NULL)
        {
            /* +12 comes from the linefeed, space and the view ID */
            if ((strlen(menu_comp.service) + 12 + state_position) >
                MAX_SESSION_SIZE)
            {
                osso_log(LOG_WARNING, "Can't add entries anymore");
                break;
            }
            else
            {
                strcat(statedata.running_apps, menu_comp.service);
                strncat(statedata.running_apps, " ", 1);
                memset(&(view_id), '\0', sizeof(view_id));
                g_snprintf(view_id, 10, "%lu", menu_comp.view_id);
                strncat(statedata.running_apps, view_id, strlen(view_id));
                strcat(statedata.running_apps, "\n");
                state_position = 
                    state_position + strlen(menu_comp.service) +
                    strlen(view_id);
                
                if (menu_comp.window_id != 0)
                {
                    guchar *pid_result = NULL;
                    int actual_format;
                    unsigned long nitems, bytes_after;
                    Atom actual_type;
                    gdk_error_trap_push();
                    XGetWindowProperty(GDK_DISPLAY(), (Window)menu_comp.window_id, pid_atom,
                                       0, 32, False, XA_CARDINAL, &actual_type,
                                       &actual_format, &nitems, &bytes_after,
                                       (unsigned char **)&pid_result);
                    gdk_error_trap_pop();
                    if (pid_result != NULL)
                    {
                        g_array_append_vals(pids,
                                            pid_result, 1);
                    }
                    
                }
            }
        }
    }
    session_state.state_size = sizeof(struct state_data);
    session_state.state_data = &statedata;
    osso_state_write(osso, &session_state);

    /* We will have to set a timer that sends the sigterms after the exit
       DBUS signals have been sent and before TN is shut down */

    timer_id = g_timeout_add(KILL_DELAY, send_sigterms, pids);

    /* We reply automatically for the appkiller, thanks to LibOSSO */        
    return TRUE;
}


static void restore_session(osso_manager_t *man)
{
    osso_return_t ret;
    osso_state_t state;
    osso_context_t *osso = get_context(man);
    struct stat buf;

    gint i = 0;
    gchar *path = NULL;
    gchar **entries = NULL, *last_app = NULL, *tmpdir_path = NULL;

    memset(&statedata, 0, sizeof(struct state_data));
    state.state_data = &statedata;
    state.state_size = sizeof(struct state_data);
    
    /* Read the state */
    ret = osso_state_read(osso, &state);

    if (ret != OSSO_OK)
    {
        return;
    }

    /* Launch the applications */
    entries = g_strsplit(statedata.running_apps, "\n", 100);


    /* Task Navigator dies of Bad Window error unless this sleep()
       exists, even though everything should be trapped properly :I */
    sleep(5);

    while(entries[i] != NULL && strlen(entries[i]) > 0)
    {
        gchar **launch = g_strsplit(entries[i], " ", 2);

        /* Should we check whether this has already been launched? */
        top_service(launch[0]);

        if (entries[i+1] == NULL || strlen(entries[i+1]) <= 1 )
        {
            last_app = g_strdup(launch[0]);
        }
        i++;

        g_strfreev(launch);
    }

    
/* The application switcher may be left on somewhat inconsistent state
   due to the undeterministic app startup times etc. This is
   supposedly OK for now, but reordering code might be added below
   this someday... */

    g_strfreev(entries); 

    /* Finally, unlink the state file. This should be upgraded when
       bug # is fixed... */

    tmpdir_path = getenv(LOCATION_VAR);
    
    if (tmpdir_path != NULL)
    {
        path = g_strconcat(tmpdir_path, "/",
                           "tasknav", "/",
                           "0.1", NULL);
    }
    else
    {
        path = g_strconcat(FALLBACK_PREFIX,
                           "tasknav", "/", "0.1", "/",
                           NULL);
    }

    if (path == NULL)
    {
        return;
    }
    if (stat(path, &buf) != 0) {
        g_free(path);
        return;
    }
    if (S_ISREG(buf.st_mode)) {
        unlink(path);
    }
    
    g_free(path);
    return;
}


static gboolean send_sigterms(gpointer data)
{
    int i;
    gboolean success = TRUE;
    GArray *pidlist = (GArray *)data;
    for (i = 0; i< pidlist->len; i++)
    {
        int retval;
        retval = kill(g_array_index(pidlist, pid_t, i), SIGTERM);
        if (retval < 0)
        {
            osso_log(LOG_WARNING, "TN: Could not kill an application");
            success = FALSE;
        }
    }
    if (timer_id != 0)
    {
        g_source_remove(timer_id);
    }
    timer_id = 0;
    
    /* Free the pidlist, although the memory would be free'd by
       the TN shutdown anyway. */
    if (pidlist != NULL)
    {
        g_array_free(pidlist, TRUE);
    }
    return success;
}

static gulong getenv_int(gchar *env_str, gulong val_default)
{
    gchar *val_str;
    gulong val = val_default;

    val_str = getenv(env_str);
    if (val_str)
    {
	val = strtoul(val_str, NULL, 10);
    }

    return val;
}

/* Public functions*/


gboolean init_window_manager(wm_new_window_cb *new_win_cb,
			     wm_removed_window_cb *removed_win_cb,
			     wm_updated_window_cb *updated_win_cb,
                             wm_desktop_topped_cb *topped_desktop_cb,
			     gpointer cb_data)
{
    GtkTreeModel *model = NULL;
    DBusConnection *connection;
    DBusError error;
    gchar *match_rule = NULL;

    bgkill_situation = FALSE;

    osso_manager_t *osso_man = osso_manager_singleton_get_instance();
     

    if (new_win_cb == NULL || removed_win_cb == NULL ||
        updated_win_cb == NULL || topped_desktop_cb == NULL ||
        osso_man == NULL)
    {
        return FALSE;
    }

    /* Create a tree model that stores the application window/wiew stuff */

    model = create_model();
    g_assert(model);

    /* Check for configurable lowmem values. */

    lowmem_min_distance = getenv_int(LOWMEM_LAUNCH_THRESHOLD_DISTANCE_ENV,
				     LOWMEM_LAUNCH_THRESHOLD_DISTANCE);
    lowmem_banner_timeout = getenv_int(LOWMEM_LAUNCH_BANNER_TIMEOUT_ENV,
				       LOWMEM_LAUNCH_BANNER_TIMEOUT);
    /* Guard about insensibly long values. */
    if (lowmem_banner_timeout > LOWMEM_LAUNCH_BANNER_TIMEOUT_MAX)
    {
        lowmem_banner_timeout = LOWMEM_LAUNCH_BANNER_TIMEOUT_MAX;
    }
    lowmem_timeout_multiplier = getenv_int(LOWMEM_TIMEOUT_MULTIPLIER_ENV,
                     LOWMEM_TIMEOUT_MULTIPLIER);

    application_switcher_set_dnotify_handler(cb_data, &dnotify_callback);
    application_switcher_set_shutdown_handler(cb_data, &shutdown_handler);
    application_switcher_set_lowmem_handler(cb_data, &lowmem_handler);
    application_switcher_set_bgkill_handler(cb_data, &bgkill_handler);

    /* Initialize the common X atoms */

    /* FIXME: put in array and use XInternAtoms() */
    gdk_error_trap_push();

    active_win = XInternAtom(GDK_DISPLAY(), "_NET_ACTIVE_WINDOW", False);
    clientlist = XInternAtom(GDK_DISPLAY(), "_NET_CLIENT_LIST", False);
    net_wm_state =XInternAtom(GDK_DISPLAY(), "_NET_WM_STATE", False);
    pid_atom = XInternAtom(GDK_DISPLAY(), "_NET_WM_PID", False);
    subname = XInternAtom(GDK_DISPLAY(), "_MB_WIN_SUB_NAME", False);
    transient_for = XInternAtom(GDK_DISPLAY(), "WM_TRANSIENT_FOR", False);
    typeatom = XInternAtom(GDK_DISPLAY(), "_MB_COMMAND", False);
    wm_class_atom = XInternAtom(GDK_DISPLAY(), "WM_CLASS", False);
    wm_name = XInternAtom(GDK_DISPLAY(), "WM_NAME", False);
    wm_state = XInternAtom(GDK_DISPLAY(), "WM_STATE", False);
    wm_type = XInternAtom(GDK_DISPLAY(), "_NET_WM_WINDOW_TYPE", False);
    utf8 = XInternAtom(GDK_DISPLAY(), "UTF8_STRING", False);
    showing_desktop = XInternAtom(GDK_DISPLAY(),
                                  "_NET_SHOWING_DESKTOP", False);
    mb_active_win = XInternAtom(GDK_DISPLAY(),
                                "_MB_CURRENT_APP_WINDOW", False);
    Atom_HILDON_APP_KILLABLE = 
      XInternAtom(GDK_DISPLAY(), "_HILDON_APP_KILLABLE", False);
    Atom_NET_WM_WINDOW_TYPE_MENU = 
      XInternAtom(GDK_DISPLAY(), "_NET_WM_WINDOW_TYPE_MENU", False);
    Atom_NET_WM_WINDOW_TYPE_NORMAL =
      XInternAtom(GDK_DISPLAY(), "_NET_WM_WINDOW_TYPE_NORMAL", False);
    Atom_NET_WM_WINDOW_TYPE_DIALOG = 
      XInternAtom(GDK_DISPLAY(), "_NET_WM_WINDOW_TYPE_DIALOG", False);
    Atom_NET_WM_WINDOW_TYPE_DESKTOP =
      XInternAtom(GDK_DISPLAY(), "_NET_WM_WINDOW_TYPE_DESKTOP", False);
    Atom_NET_WM_STATE_MODAL =
      XInternAtom(GDK_DISPLAY(), "_NET_WM_STATE_MODAL", False);


    gdk_error_trap_pop();

    /* Set up callbacks for the window events */
    
    wm_cbs.new_win_cb = new_win_cb;
    wm_cbs.removed_win_cb = removed_win_cb;
    wm_cbs.updated_win_cb = updated_win_cb;
    wm_cbs.topped_desktop_cb = topped_desktop_cb;
    wm_cbs.cb_data = cb_data;
    wm_cbs.model = model;

    /* Set up the event filter */
    gdk_error_trap_push();
    gdk_window_set_events(gdk_get_default_root_window(),
                          gdk_window_get_events(gdk_get_default_root_window())                          | GDK_SUBSTRUCTURE_MASK);
    
    gdk_window_add_filter(gdk_get_default_root_window(), x_event_filter,
                          model);
    if (gdk_error_trap_pop() != 0)
    {
        return FALSE;
    }

    dbus_error_init (&error); 
    connection = dbus_bus_get( DBUS_BUS_SESSION, &error );
    
    if ( ! connection ) {
        osso_log(LOG_ERR, "Failed to connect to DBUS: %s!\n", error.message );
	    dbus_error_free( &error );
    } else {

	    /* Match rule */
	    match_rule = g_strdup_printf(
			    "type='method', interface='%s'",
			    APP_LAUNCH_BANNER_METHOD_INTERFACE );

	    dbus_bus_add_match( connection, match_rule, NULL ); 
            dbus_connection_add_filter( connection, method_call_handler,
                                        model, NULL );
            g_free(match_rule);   

                /* appkiller handler filter */

            match_rule = g_strdup_printf(
                "type='signal', interface='%s'",
                APPKILLER_SIGNAL_INTERFACE);
            dbus_bus_add_match (connection, match_rule, NULL );
            dbus_connection_add_filter( connection, exit_handler,
                                        model, NULL);

            dbus_connection_flush(connection);
            g_free(match_rule);

            /* Add the com.nokia.tasknav callbacks  */
                 
            add_method_cb(osso_man, KILL_APPS_METHOD, &killmethod, osso_man);
            add_method_cb(osso_man, SAVE_METHOD, &save_session, osso_man);
    }

    restore_session(osso_man);
    
    return TRUE;
}


gboolean is_killed(GtkMenuItem *menuitem)
{
    /* FIXME: this is a copy of top_view, below, reduced to only find out
       whether the application corresponding to MENUITEM is killed. This
       information should of course be available more cheaply...
    */

    GtkTreeModel *model = NULL;
    GtkTreeIter iter;
    gboolean valid;
    gboolean killed;

    model = wm_cbs.model;

    valid = gtk_tree_model_iter_children(model, &iter, NULL);
    while (valid)
    {
	gboolean inner_valid;
	GtkTreeIter inner_iter;
	GtkTreeIter parent_iter;

	inner_valid = gtk_tree_model_iter_children(model, &inner_iter, &iter);
	while (inner_valid)
	{
	    gpointer widget;

	    gtk_tree_model_get(model, &inner_iter,
			       WM_MENU_PTR_ITEM, &widget, -1);

	    if (gtk_tree_model_iter_parent(model, &parent_iter, &inner_iter))
	    {
		gtk_tree_model_get(model, &parent_iter,
				   WM_KILLED_ITEM, &killed, -1);
	    }
	    else
	    {
		killed = FALSE;
	    }

	    if ((gpointer)widget == menuitem)
	    {
		return killed;
	    }

	    inner_valid = gtk_tree_model_iter_next(model, &inner_iter);
	}

	valid = gtk_tree_model_iter_next (model, &iter);
    }

    /* Didn't find it, assume it is alive... */
    return FALSE;
}

void top_view(GtkMenuItem *menuitem)
{
    GtkTreeModel *model = NULL;
    XEvent xev;
    GtkTreeIter iter;
    gboolean valid;
    gboolean killed;
    gchar *exec = NULL;
    
    model = wm_cbs.model;

    valid = gtk_tree_model_iter_children (model, &iter, NULL);
    while (valid)
    {
        gboolean inner_valid;
        GtkTreeIter inner_iter;
        GtkTreeIter parent_iter;
        inner_valid = gtk_tree_model_iter_children(model, &inner_iter, &iter);
        while (inner_valid)
        {
            gulong win_id, view_id;
            gpointer widget;
            
            gtk_tree_model_get(model, &inner_iter,
                               WM_EXEC_ITEM, &exec,
                               WM_ID_ITEM, &win_id,
                               WM_VIEW_ID_ITEM, &view_id,
                               WM_MENU_PTR_ITEM, &widget,
                               -1);
            
            if (gtk_tree_model_iter_parent(model, &parent_iter, &inner_iter))
            {

            gtk_tree_model_get(model, &parent_iter,
                               WM_EXEC_ITEM, &exec,
                               WM_KILLED_ITEM, &killed, -1);
            }
            else
            {
                killed = FALSE;
            }
            if ((gpointer)widget == menuitem && (killed == FALSE) )
	  {

              gboolean is_hildonapp = is_window_hildonapp(win_id);
              int retval;
              
              if (!is_hildonapp && view_id == 0)
              {
                  top_non_hildonapp(win_id);
                  g_free(exec);
                  return;
              }

              memset(&xev, 0, sizeof(xev));
              xev.xclient.type = ClientMessage;
              xev.xclient.serial = 0;
              xev.xclient.send_event = True;
              xev.xclient.window = (Window)view_id;
              xev.xclient.message_type = active_win;
              xev.xclient.format = 32;
              xev.xclient.data.l[0] = 0;
              gdk_error_trap_push();
              retval = XSendEvent(GDK_DISPLAY(), (Window)win_id, False,
                                  SubstructureRedirectMask
                                  | SubstructureNotifyMask, &xev);
              gdk_error_trap_pop();
              
              g_free(exec);
	      return;
	  }
            else if ((gpointer)widget == menuitem && killed == TRUE)
            {
                if (exec != NULL)
                {
                    GtkTreeIter parent;
                    if (gtk_tree_model_iter_parent(model,
                                                   &parent,
                                                   &inner_iter))
                    {
                        gtk_tree_store_set(GTK_TREE_STORE(model),
                                           &parent,
                                           WM_VIEW_ID_ITEM, view_id, -1);
                    }
                    top_service(exec);
                    g_free(exec);
                    return;
                }
            }
            inner_valid = gtk_tree_model_iter_next (model, &inner_iter);
        }
        valid = gtk_tree_model_iter_next (model, &iter);
    }
}


void top_service(const gchar *service_name)
{
    osso_manager_t *osso_man;
    Window win_id;
    XEvent xev;

    GtkTreeIter parent, target_iter;
    gulong view_id = 0;
    gboolean killable, killed = FALSE;
    gpointer widget_ptr = NULL;
    gchar *app_name = NULL, *view_name = NULL, *icon_name = NULL;
    int retval = 0;
    FILE *lowmem_allowed_f = NULL, *pages_used_f = NULL;
    guint lowmem_allowed, pages_used = 0;
    guint pages_available = 0;

    if (service_name == NULL)
    {
        osso_log(LOG_ERR, "There was no service name!\n");
        return;
    }

    /* Check how much memory we do have until the lowmem threshold */

    lowmem_allowed_f = fopen(LOWMEM_PROC_ALLOWED, "r");
    pages_used_f = fopen(LOWMEM_PROC_USED, "r");

    if (lowmem_allowed_f != NULL && pages_used_f != NULL)
    {
	/* FIXME: Add checks. */
	fscanf(lowmem_allowed_f, "%u", &lowmem_allowed);
	fscanf(pages_used_f, "%u", &pages_used);
	if (pages_used < lowmem_allowed)
	{
	    pages_available = lowmem_allowed - pages_used;
	}
	else
	{
	    pages_available = 0;
	}
    }
    else
    {
	osso_log(LOG_ERR, "We could not read lowmem page stats.\n");
    }

    if (lowmem_allowed_f)
    {
	fclose(lowmem_allowed_f);
    }
    if (pages_used_f)
    {
	fclose(pages_used_f);
    }

    /* Here we should compare the amount of pages to a configurable
       threshold. Value 0 means that we don't know and assume
       slightly riskily that we can start the app... */

    if (pages_available > 0 && pages_available < lowmem_min_distance)
    {
	gtk_infoprintf(NULL, _("memr_ib_unable_to_switch_to_application"));

	return;
    }

    /* Search for the corresponding service root node */
     if (!find_service_from_tree(wm_cbs.model, &parent, service_name))
      {
          return;
      }
    /* Retrieve the active view information */
     gtk_tree_model_get(wm_cbs.model, &parent, WM_VIEW_ID_ITEM, &view_id,
                        WM_KILLED_ITEM, &killed, -1);
     
     if (view_id == 0 && killed == FALSE)
     {
         /* For now, we don't top any non-hildonapps here but launch
            new instances instead. Thus, we don't have to worry about
            updating the Application Switcher as this is handled
            automatically after application launch. */
         osso_man = osso_manager_singleton_get_instance();
         osso_manager_launch(osso_man, service_name, NULL);
         return;
     }
     else if (killed == TRUE)
     {
        
        guint interval = LAUNCH_SUCCESS_TIMEOUT * 1000;
        
        /* This should probably be merged with the previous condition */
        osso_man = osso_manager_singleton_get_instance();
        osso_manager_launch(osso_man, service_name, RESTORED);
        if (bgkill_situation == TRUE)
        {
            g_timeout_add( interval,
                           relaunch_timeout, 
                           (gpointer) g_strdup(service_name) );
	    }
        return;
     }
     else
     {
         memset(&xev, 0, sizeof(xev));
         xev.xclient.type = ClientMessage;
         xev.xclient.serial = 0;
         xev.xclient.send_event = True;
         xev.xclient.window = (Window)view_id;
         xev.xclient.message_type = active_win;
         xev.xclient.format = 32;
         xev.xclient.data.l[0] = 0;
     }
     
     /* We need also the actual window ID to send the message */

     win_id = find_window_id_for_view(wm_cbs.model, &parent,
                                      &target_iter, view_id);
     if (win_id != 0)
     {
         gdk_error_trap_push();
         retval = XSendEvent(GDK_DISPLAY(), (Window)win_id, False,
                             SubstructureRedirectMask
                             | SubstructureNotifyMask, &xev);
         gdk_error_trap_pop();
         if (retval != Success)
         {
         osso_log(LOG_WARNING, "Sending topping event failed");
         }
     

     /* Finally, update the application switcher. (fix for bug 3268) */
     gtk_tree_model_get(wm_cbs.model, &target_iter,
                        WM_MENU_PTR_ITEM,
                        &widget_ptr,
                        WM_VIEW_NAME_ITEM, &view_name,
                        WM_ICON_NAME_ITEM, &icon_name,
                        WM_NAME_ITEM, &app_name,
                        WM_KILLABLE_ITEM, &killable,
                        -1);
     wm_cbs.updated_win_cb(widget_ptr, icon_name, app_name, view_name,
                           AS_MENUITEM_TO_FIRST_POSITION,
                           killable, NULL, wm_cbs.cb_data);
     }
}


void top_desktop(void)
{
    XEvent ev;

    memset(&ev, 0, sizeof(ev));
    ev.xclient.type = ClientMessage;
    gdk_error_trap_push();
    ev.xclient.window = GDK_ROOT_WINDOW();
    ev.xclient.message_type = showing_desktop;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = 1;
    gdk_error_trap_push();
    XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
               SubstructureRedirectMask, &ev);
    gdk_error_trap_pop();
    
}

static int kill_lru( void )
{
	GtkTreeIter parent;
	menuitem_comp_t menu_comp;
	GtkWidget *widgetptr = NULL;

	widgetptr = application_switcher_get_killable_item(wm_cbs.cb_data);

	if (widgetptr == NULL) {
		return 1;
	}
	
	menu_comp.menu_ptr = widgetptr;
	menu_comp.wm_class = NULL;
	gtk_tree_model_foreach(wm_cbs.model, menuitem_match_helper,
			&menu_comp);

	if (menu_comp.wm_class != NULL) {
		if (find_application_from_tree(wm_cbs.model, &parent,
					menu_comp.wm_class)) {
			gint killable;
			gtk_tree_model_get(wm_cbs.model, &parent,
					WM_KILLABLE_ITEM, &killable, -1);
			if (killable == TRUE) {
				kill_application(wm_cbs.model, &parent, TRUE);
			}
		}
	}


	return 0;
}


static int kill_all( gboolean killable_only )
{
    gint i;
    GList *mitems = NULL;
    
    menuitem_comp_t menu_comp;

    /* Acquire list of menu items (i.e. running apps)
       from the Application Switcher */
    mitems = application_switcher_get_menuitems(wm_cbs.cb_data);

    if (mitems == NULL)
    {
        /* We assume that there are no application to kill */
        return 0;
    }

    /* Get all applications that are killable - the currently active
       app should be non-killable and is automatically not included */

    for (i = g_list_length(mitems)-1 ; i >= 2; i--)
    {
        menu_comp.menu_ptr = 
            g_list_nth_data(mitems, i);
        menu_comp.wm_class = NULL;
        menu_comp.service = NULL;
        menu_comp.window_id = 0;
        menu_comp.killable = FALSE;

        gtk_tree_model_foreach(wm_cbs.model, menuitem_match_helper,
                               &menu_comp);

	/* Do not kill applications that do not have service name
	   information, as we'd be unable to top them... */

        if (menu_comp.window_id != 0 && menu_comp.service != NULL &&
            ((menu_comp.killable == TRUE && killable_only == TRUE) ||
             killable_only == FALSE))
        {

            guchar *pid_result = NULL;
            int actual_format;
            Status status;
            unsigned long nitems, bytes_after;
            Atom actual_type;
            union win_value window_value;

            /* Is the application topped, i.e. topmost? If yes, it
               should not be killed... */

            gdk_error_trap_push();
            XGetWindowProperty(GDK_DISPLAY(),
                               GDK_WINDOW_XID(gdk_get_default_root_window()),
                               active_win, 0, 32, False, XA_WINDOW,
                               &actual_type,
                               &actual_format, &nitems, &bytes_after,
                               (unsigned char **)&window_value.char_value);
            if (gdk_error_trap_pop() == 0 &&
                window_value.window_value[0] == menu_comp.window_id)
            {
                XFree(window_value.char_value);
                continue;
            }

            gdk_error_trap_push();
            status = XGetWindowProperty(GDK_DISPLAY(),
                                        (Window)menu_comp.window_id,
                                        pid_atom, 0, 32, False, XA_CARDINAL,
                                        &actual_type, &actual_format,
                                        &nitems, &bytes_after,
                                        (unsigned char **)&pid_result);
            if (gdk_error_trap_pop() == 0 &&pid_result != NULL &&
                status == Success)
            {
                pid_t pid = pid_result[0]+256*pid_result[1];
                int retval;

                /* As we don't have to wait, kill things right here.
                   Mark them also as killed to the treemodel, if
                   it's an hildonapp-based program and supports
                   killable property. */

                retval = kill((pid_t)pid, SIGTERM);
                if (retval != 0)
                {
                    osso_log(LOG_ERR, "Could not kill pid %d\n", pid);
                    XFree(window_value.char_value);
                    XFree(pid_result);
                    continue;
                }
                if (menu_comp.killable == TRUE )
                {
                    GtkTreeIter iter;
                    /* Find service parent node */
                    if ( find_service_from_tree( wm_cbs.model,
                                                 &iter,
                                                 menu_comp.service ) > 0 ) {
                        gtk_tree_store_set( GTK_TREE_STORE(wm_cbs.model),
                                            &iter, WM_KILLED_ITEM, TRUE, -1);
                    }
                }
                XFree(pid_result);
            }
        }
    }

    update_lowmem_situation(lowmem_situation);

    return 0; 
}


static int killmethod(GArray *arguments, gpointer data)
{
    gchar *appname = NULL, *operation = NULL;
    GtkTreeIter parent;
    if (arguments->len < 1)
    {
        return 1;
    }

    operation = (gchar *)g_array_index(arguments, osso_rpc_t, 0).value.s;

    /* In this case, TN will kill every process that has supposedly
       statesaved */

    if (operation == NULL)
    {
        return 1;
    }

    if (strcmp(operation, "lru") == 0)
    {
	return kill_lru();
    }

    else if (strcmp(operation, "all") == 0)
    {
        return kill_all(TRUE);
    }
    else if (strcmp(operation, "app") != 0 || (arguments->len < 2) )
    {
        return 1;
    }

    /* Kill a certain application */

    appname = (gchar *)g_array_index(arguments, osso_rpc_t, 1).value.s;

    if (appname == NULL)
    {
        return 1;
    }
    if (find_application_from_tree(wm_cbs.model, &parent, appname))
    {
        gint killable;
        gtk_tree_model_get(wm_cbs.model, &parent,
                           WM_KILLABLE_ITEM, &killable, -1);
        if (killable == TRUE)
        {
            kill_application(wm_cbs.model, &parent, TRUE);
            
        }
    }

    return 0;
}


static DBusHandlerResult method_call_handler( DBusConnection *connection,
                                              DBusMessage *message,
                                              void *data )
{


	/* Catch APP_LAUNCH_BANNER_METHOD */
	if ( dbus_message_is_method_call( message, 
                                          APP_LAUNCH_BANNER_METHOD_INTERFACE,
                                          APP_LAUNCH_BANNER_METHOD ) ) {

		DBusError error;
		gchar *service_name = NULL;
		gchar *service = NULL;
		gchar *app_name = NULL;
		gulong view_id;
		gboolean startup = TRUE;
	
		dbus_error_init (&error);
		
		
		dbus_message_get_args( message, &error, DBUS_TYPE_STRING,
				&service_name, DBUS_TYPE_INVALID );

		
		if ( dbus_error_is_set( &error ) ) {
	
			osso_log(LOG_WARNING, "Error getting message args: %s\n",
					error.message);
			dbus_error_free( &error );
			
		} else {

            if (!g_str_has_prefix(service_name, SERVICE_PREFIX)) {
                dbus_free (service_name);
                return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            }
                        
            service = g_strdup (service_name + strlen(SERVICE_PREFIX));
            dbus_free (service_name);

            GtkTreeIter iter;
        
			if ( find_service_from_tree( wm_cbs.model,
						&iter, service ) > 0 ) {

				gtk_tree_model_get(wm_cbs.model, &iter,
						WM_VIEW_ID_ITEM, &view_id,
						WM_NAME_ITEM, &app_name,
					       	WM_STARTUP_ITEM, &startup,							-1);

				if (view_id == 0 && startup == TRUE &&
				    lowmem_banner_timeout > 0) {
					/* Show the banner */
                    /* Service will be freed later when
                     * banner is removed */
					show_launch_banner( NULL, app_name, service );
				}


				return DBUS_HANDLER_RESULT_HANDLED;
           }
           g_free (service);
		}
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


static void show_launch_banner( GtkWidget *parent, gchar *app_name,
		                gchar *service_name )
{
	launch_banner_info *info = g_malloc0( sizeof( launch_banner_info ) );
        gchar *message;
	
	guint interval = APP_LAUNCH_BANNER_CHECK_INTERVAL * 1000;

	if ( app_name == NULL ) {
		app_name = g_strdup( "" );
	}

	gettimeofday( &info->launch_time, NULL );
	info->service_name = service_name;

	/* Show the banner */
        gdk_error_trap_push();

        message = g_strdup_printf(_( APP_LAUNCH_BANNER_MSG_LOADING ),
                                  app_name );
        
        gtk_banner_show_animation( NULL, message );

        gdk_error_trap_pop();

	g_timeout_add(interval, launch_banner_timeout, info);

	/* Cleanup */
        g_free( app_name );
        g_free( message );
}


static void close_launch_banner( GtkWidget *parent )
{
	gtk_banner_close( NULL );
}



static gboolean launch_banner_timeout(gpointer data)
{
	launch_banner_info *info = data;
	struct timeval current_time;
	long unsigned int t1, t2;
	guint time_left;
	GtkTreeIter iter;
    gulong current_banner_timeout = 0;

    /* Added by Karoliina Salminen 26092005 
       Addition to low memory situation awareness, the following multiplies the
       launch banner timeout with the timeout multiplier found from environment variable */
    if(lowmem_situation == TRUE) {
        current_banner_timeout = lowmem_banner_timeout * lowmem_timeout_multiplier;
    } else {
        current_banner_timeout = lowmem_banner_timeout;
    }
    /* End of addition 26092005 */

	if ( find_service_from_tree( wm_cbs.model,
				&iter, info->service_name ) > 0 ) {
	} else {
		/* This should never happen. Bail out! */
		return FALSE;
	}
	
	gettimeofday( &current_time, NULL );

	t1 = (long unsigned int) info->launch_time.tv_sec;
	t2 = (long unsigned int) current_time.tv_sec;
	time_left = (guint) (t2 - t1);
	
    /* The following uses now current_banner_timeout instead of lowmem_banner_timeout, changed by
       Karoliina Salminen 26092005 */
	if (time_left >= current_banner_timeout ||
	    gtk_tree_model_iter_has_child(wm_cbs.model, &iter) == TRUE ) {
		
		/* Close the banner */
		close_launch_banner( NULL );
		

		if ( time_left >= APP_LAUNCH_BANNER_TIMEOUT &&
		     lowmem_situation == TRUE) {
		  gtk_infoprintf(NULL, _("memr_ib_unable_to_switch_to_application"));
		}
	
		/* Cleanup */
		g_free (info->service_name);
		g_free (info);

		return FALSE;
	}
	
	return TRUE;
}

static gboolean relaunch_timeout(gpointer data)
{
  GtkTreeIter iter;
  gchar *service_name = (gchar *)data;
  gboolean killed;

  if (find_service_from_tree(wm_cbs.model, &iter, service_name))
    {
      gtk_tree_model_get(wm_cbs.model, &iter, WM_KILLED_ITEM, &killed, -1);
      
      /* Did launch fail? */

      if (killed == TRUE && lowmem_situation == TRUE)
	{
	  gtk_infoprintf(NULL, _("memr_ib_unable_to_switch_to_application"));
	}
    }
  
  g_free(service_name);
  return FALSE;
}

	
static void dnotify_callback(MBDotDesktop *desktop)
{
    GtkTreeStore *store = GTK_TREE_STORE(wm_cbs.model);
    GtkTreeIter iter, dummy_iter;
    gchar *exec, *icon_name, *startup_wmclass, *binitem, *snotify_str;
    gboolean startupnotify = TRUE;

    binitem = (char *) mb_dotdesktop_get(desktop, DESKTOP_BIN_FIELD);
    startup_wmclass = 
        (char *) mb_dotdesktop_get(desktop, DESKTOP_SUP_WMCLASS);

    if (binitem == NULL && startup_wmclass == NULL)
    {
        return;
    }

    if (startup_wmclass != NULL)
    {
        if (find_application_from_tree(wm_cbs.model,
                                       &dummy_iter,
                                       startup_wmclass) == 1)
        {
            return;
        }
        gtk_tree_store_append(store, &iter, NULL);
        gtk_tree_store_set(store, &iter, WM_BIN_NAME_ITEM,
                           startup_wmclass, -1);
    }
    else
    {	
	gchar *basename = g_path_get_basename(binitem);
	
        if (find_application_from_tree(wm_cbs.model,
                                       &dummy_iter,
                                       basename) == 1)
        {
	    if ( basename ) {
	       g_free( basename );
	    }
            return;
        } 
        else
        {
            gtk_tree_store_append(store, &iter, NULL);
            gtk_tree_store_set(store, &iter, WM_BIN_NAME_ITEM,
                               binitem, -1);
        }
        
    }
    exec = (char *) mb_dotdesktop_get(desktop, DESKTOP_LAUNCH_FIELD);
    
    icon_name =
        (char *) mb_dotdesktop_get(desktop, DESKTOP_ICON_FIELD);
    

    snotify_str = (char *)mb_dotdesktop_get(desktop,
					    DESKTOP_STARTUPNOTIFY); 
    if (snotify_str != NULL)
      
      {
	startupnotify = ( g_ascii_strcasecmp(snotify_str, "false"));
      }
    
    gtk_tree_store_set(store, &iter,
                       WM_ICON_NAME_ITEM, icon_name,
                       WM_EXEC_ITEM, exec,
                       WM_NAME_ITEM,
                       mb_dotdesktop_get(desktop,
                                         DESKTOP_VISIBLE_FIELD),
                       WM_DIALOG_ITEM, 0,
                       WM_MENU_PTR_ITEM, NULL,
                       WM_VIEW_ID_ITEM, 0,
                       WM_KILLABLE_ITEM, FALSE,
                       WM_KILLED_ITEM, FALSE,
		       WM_STARTUP_ITEM, startupnotify,
                       -1);
        return;
}


static void shutdown_handler(void)
{
    kill_all(FALSE);
}

static void explain_lowmem (void)
{
    /* We are called for every press when the button is insensitive.
       Buttons are sometimes insensitive for other reasons than lowmem,
       so we have to check here. */
    if (lowmem_situation)
    {
	gtk_infoprintf(NULL, _("memr_ib_unable_to_switch_to_application"));
    }
}

void set_lowmem_explain(GtkWidget *widget)
{
    g_signal_connect(G_OBJECT(widget), "insensitive-press",
		     G_CALLBACK(explain_lowmem), NULL);
}

/* FIXME: This is defined in maemo-af-desktop-main.c and we shouldn't
   be using it of course. Cleaning this up can be done when nothing
   else is left to clean up... */
extern Navigator tasknav;

static void update_lowmem_situation(gboolean lowmem)
{
    /* Update the state of the UI according to LOWMEM. This is done by
       just enabling/disabling all relevant widgets. Also, on the first
       time around, we connect some handlers that put up explanations
       when the widget is pressed anyway. */

    static gboolean first_time = TRUE;

    /* If dimming is disabled, we don't do anything here. Also see
       APPLICATION_SWITCHER_UPDATE_LOWMEM_SITUATION. */
    if (!config_lowmem_dim)
    {
	return;
    }

    if (first_time)
    {
	ApplicationSwitcher_t *as = tasknav.app_switcher;

	first_time = FALSE;
	set_lowmem_explain(others_menu_get_button(tasknav.others_menu));
	set_lowmem_explain(tasknav.bookmark_button);
	set_lowmem_explain(tasknav.mail_button);
	set_lowmem_explain(as->toggle_button1);
	set_lowmem_explain(as->toggle_button2);
	set_lowmem_explain(as->toggle_button3);
	set_lowmem_explain(as->toggle_button4);
    }

    gtk_widget_set_sensitive(others_menu_get_button(tasknav.others_menu),
			    !lowmem);
    gtk_widget_set_sensitive(tasknav.bookmark_button, !lowmem);
    gtk_widget_set_sensitive(tasknav.mail_button, !lowmem);

    application_switcher_update_lowmem_situation(tasknav.app_switcher, lowmem);
}

static void show_pavlov_dialog(void)
{
    GtkWidget *dialog =
	hildon_note_new_information (NULL,
				     _("memr_ni_application_memory_low"));
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    create_killer_dialog();
}

static void lowmem_handler(gboolean is_on)
{
    if (lowmem_situation != is_on)
    {
	lowmem_situation = is_on;

	/* The 'lowmem' situation always includes the 'bgkill' situation,
	   but the 'bgkill' signal is not generated in all configurations.
	   So we just call the bgkill_handler here. */
	bgkill_handler(is_on);

	update_lowmem_situation(is_on);

	if (is_on && config_lowmem_notify_enter)
	{
	    if (config_lowmem_pavlov_dialog)
	    {
		show_pavlov_dialog();
	    }
	    else
	    {
		/*
		 * The string supposed to be displayed here is
		 * "memr_ib_unable_to_switch_to_application".
		 */
		gtk_infoprintf(NULL, _("memr_ni_application_memory_low"));
	    }
	}
	else if (!is_on && config_lowmem_notify_leave)
	{
	    gtk_infoprintf(NULL, _("memr_ib_no_more_memory_low"));
	}
    }
}

static void bgkill_handler(gboolean is_on)
{
    /* If bgkilling is disabled, we never turn on the bgkill_situation. */
    if (!config_do_bgkill)
    {
	return;
    }

    if (is_on != bgkill_situation)
    {
	bgkill_situation = is_on;

	if (is_on == TRUE)
	{
	    kill_all(TRUE);
	}
    }
}

static DBusHandlerResult exit_handler(DBusConnection *connection,
                                      DBusMessage *message,
                                      void *data )
{
    /* For now, just reuse the kill_all() code. If the
       TN needs to do different things for these signals,
       this obviously needs to be changed. */
    
    if ( dbus_message_is_signal( message, APPKILLER_SIGNAL_INTERFACE,
                                 APPKILLER_EXIT_SIGNAL ) )
    {
        kill_all(FALSE);
/*        shutdown_handler();*/
    }
    
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static gboolean treeview_button_press(GtkTreeView *treeview,
				      GdkEventButton *event,
				      GtkCellRenderer *renderer)
{
    GtkTreeViewColumn *column;
    GtkTreePath *path;
    gint cell_x, cell_y;
    gint start_pos, width;

    if (event->window != gtk_tree_view_get_bin_window(treeview))
    {
	gtk_widget_destroy(wm_cbs.killer_dialog);
	return FALSE;
    }
    if (!gtk_tree_view_get_path_at_pos(treeview, event->x, event->y,
				       &path, &column, &cell_x, &cell_y))
    {
	gtk_widget_destroy(wm_cbs.killer_dialog);
	return FALSE;
    }
    if (gtk_tree_view_column_cell_get_position(column, renderer,
					       &start_pos, &width))
    {
	if (start_pos <= cell_x && cell_x <= start_pos + width)
	{
	    GtkTreeModel *model;
	    GtkTreeIter iter, parent_iter;

	    model = gtk_tree_view_get_model(treeview);

	    if (gtk_tree_model_get_iter (model, &iter, path))
	    {
		gchar *name;

		gtk_tree_model_get(model, &iter, COL_SERVICE, &name, -1);

		if (find_service_from_tree(wm_cbs.model, &parent_iter, name))
		{
		    kill_application(wm_cbs.model, &parent_iter, FALSE);
		}

		g_free(name);
		gtk_widget_destroy(wm_cbs.killer_dialog);

		return TRUE;
	    }
	}
    }

    gtk_tree_path_free(path);
    gtk_widget_destroy(wm_cbs.killer_dialog);

    return FALSE;
}

static void create_killer_dialog(void)
{
    GtkListStore *store;
    GList *mitems = NULL;
    GtkIconTheme *icon_theme;
    GdkPixbuf *pixbuf;
    GtkWidget *treeview = NULL;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    GtkRequisition req;
    GtkWidget *toplevel;

    GtkWidget *dialog = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_set_type_hint(GTK_WINDOW(dialog),
			     GDK_WINDOW_TYPE_HINT_MENU);
    gtk_widget_set_name(dialog, "hildon-status-bar-popup");
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_decorated(GTK_WINDOW(dialog), TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 20);

    store = gtk_list_store_new (N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);

    icon_theme = gtk_icon_theme_get_default ();

    /* Add running applications to the list */
    /* FIXME: We _really_ should be able to get the name, icon etc.
       from the AS itself without having to romp through the treemodel... */

    mitems = application_switcher_get_menuitems(wm_cbs.cb_data);
    /* First two items are Home and the separator */

    if (mitems != NULL && g_list_length(mitems) > 2)
    {
	gint i;

	for (i = g_list_length(mitems) - 1 ; i >= 2; i--)
	{
	    GtkTreeIter iter;
	    menuitem_comp_t menu_comp;

	    menu_comp.menu_ptr = g_list_nth_data(mitems, i);
	    menu_comp.wm_class = NULL;
	    menu_comp.service = NULL;
	    menu_comp.window_id = 0;

	    gtk_tree_model_foreach(wm_cbs.model, menuitem_match_helper,
				   &menu_comp);
	    if (menu_comp.service != NULL &&
		find_service_from_tree(wm_cbs.model, &iter, menu_comp.service))
	    {
		gchar *icon_name, *app_name;

		gtk_tree_model_get(wm_cbs.model, &iter,
				   WM_ICON_NAME_ITEM, &icon_name,
				   WM_NAME_ITEM, &app_name,
				   -1);
		pixbuf = gtk_icon_theme_load_icon(icon_theme, icon_name,
						  HILDON_ICON_PIXEL_SIZE_SMALL,
						  0, NULL);
		/* g_free(icon_name) or something */

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
				   COL_ICON, pixbuf,
				   COL_NAME, _(app_name),
				   COL_SERVICE, menu_comp.service,
				   -1);
		g_object_unref(pixbuf);
	    }
	}

	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);
	g_object_unref(store);

    }

    if (treeview == NULL)
    {
	return;
    }

    column = gtk_tree_view_column_new();

    renderer = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer, "pixbuf",
					COL_ICON, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_attributes(column, renderer, "text",
					COL_NAME, NULL);

    renderer = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    g_object_set(renderer, "stock-id", GTK_STOCK_CLOSE, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    gtk_container_add(GTK_CONTAINER(dialog), treeview);

    /* Setup kill handlers */

    g_signal_connect(treeview, "button-press-event",
		     G_CALLBACK(treeview_button_press), renderer);
    wm_cbs.killer_dialog = dialog;
    gtk_widget_show_all(dialog);

    /* A quick and dirty 'solution' to position the pseudomenu */
    toplevel = gtk_widget_get_toplevel(treeview);
    gtk_widget_size_request( toplevel, &req );
    gtk_window_move(GTK_WINDOW(dialog), 80,
		    gdk_screen_get_height(gdk_screen_get_default()
		    - req.height));
    gtk_window_present(GTK_WINDOW(dialog));
}

