/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005, 2006 Nokia Corporation.
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

/* 
 * 
 * @file hildon-navigator-main.c
 *
 */
/* GTK includes */ 
#include <gtk/gtkmain.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtktogglebutton.h>
#include <unistd.h>

/* GDK includes */
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

/* dlfcn include */
#include <dlfcn.h>

/* stdio include */
#include <stdio.h>

/* libintl include */
#include <libintl.h>

/* log include */
#include <log-functions.h>
#include <osso-log.h>

/* Locale include */
#include <locale.h>

/* For user config dir monitoring */
#include <hildon-base-lib/hildon-base-dnotify.h>

/* Hildon includes */
#include "osso-manager.h"
#include "hildon-navigator.h"
#include "application-switcher.h"
#include "others-menu.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* Header file */
#include "hildon-navigator-main.h"
#include "hildon-navigator-interface.h" 


#define PLUGIN_KEY_LIB                  "Library"
#define PLUGIN_KEY_POSITION             "Position"
#define PLUGIN_KEY_MANDATORY            "Mandatory"

#include "kstrace.h"

#define USE_AF_DESKTOP_MAIN__


/* Global external variables */
gboolean config_do_bgkill;
gboolean config_lowmem_dim;
gboolean config_lowmem_notify_enter;
gboolean config_lowmem_notify_leave;
gboolean config_lowmem_pavlov_dialog;
gboolean tn_is_sensitive=TRUE;

/* Callbacks */
static void initialize_navigator_menus(Navigator *tasknav);

static const char *load_symbols(Navigator *tasknav, void *dlhandle, 
                                  gint symbol_id);

static void create_navigator(Navigator *tasknav);

static void destroy_navigator(Navigator *tasknav);
     
static gpointer create_new_plugin(Navigator *tasknav,
                                  const gchar *name,
                                  const gchar *plugin_name);

static GList *insert_navigator_plugin(Navigator *tasknav, GList *list, NavigatorPlugin *plugin);
static GList *load_plugins_from_file(Navigator *tasknav,
                                     const gchar *filename,
                                     gboolean allow_mandatory);
static GList *load_navigator_plugin_list(Navigator *tasknav);
static void destroy_plugin(Navigator *tasknav, NavigatorPlugin *plugin);
static void reload_plugins(Navigator *tasknav, GList *list);
static gint compare_plugins(NavigatorPlugin *a, NavigatorPlugin *b);

static void tasknav_sensitive_cb( GtkWidget *widget, gpointer data);
static void tasknav_insensitive_cb( GtkWidget *widget, gpointer data);

static void plugin_configuration_changed( char *path,
                                          gpointer *data );
/* Plugin configuration files */

#define CFG_FNAME      "plugins.conf"
#define NAVIGATOR_FACTORY_PLUGINS       "/etc/hildon-navigator/" CFG_FNAME
#define NAVIGATOR_USER_DIR              ".osso/hildon-navigator/"
#define NAVIGATOR_USER_PLUGINS          NAVIGATOR_USER_DIR CFG_FNAME
#define NAVIGATOR_WATCH_DIR             ".osso"
#define NAVIGATOR_WATCH_PREFIX          "navigator-"
#define NAVIGATOR_WATCH_SUFFIX          ".watch"


Navigator *task_nav;
gchar *home_dir;

static gboolean getenv_yesno(const char *env, gboolean def)
{
    char *val = getenv (env);

    if (val)
	return (strcmp(val, "yes") == 0);
    else
	return def;
}

static void write_plugin_log(NavigatorPlugin *plugin,
                             const gchar *stage,
                             const gchar *message)
{
    gchar *str;

    g_assert(plugin != NULL);

    if (plugin->watch == NULL)
        return;
    
    str = g_strdup_printf("[%s] %s\n", stage, message);

    fwrite(str, strlen(str), 1, plugin->watch);
    fflush(plugin->watch);
    g_free(str);
}

static void stop_plugin_watch(NavigatorPlugin *plugin)
{
    gchar *watch_name;
    gchar *soname;

    g_assert(plugin != NULL);
    
    if (plugin->watch == NULL)
        return;
    
    fclose(plugin->watch);
    plugin->watch = NULL;

    soname = g_path_get_basename(plugin->library);
    watch_name = g_strdup_printf("%s/%s/%s%s%s",
                                 home_dir,
                                 NAVIGATOR_WATCH_DIR,
                                 NAVIGATOR_WATCH_PREFIX,
                                 soname,
                                 NAVIGATOR_WATCH_SUFFIX);
    remove(watch_name);
    g_free(soname);
    g_free(watch_name);
}

static void start_plugin_watch(NavigatorPlugin *plugin)
{
    gchar *watch_name;
    gchar *soname;
    
    g_assert(plugin != NULL);

    /* If it's already open, do nothing */
    if (plugin->watch != NULL)
        return;
    
    soname = g_path_get_basename(plugin->library);
    watch_name = g_strdup_printf("%s/%s/navigator-%s.watch",
                                 home_dir,
                                 NAVIGATOR_WATCH_DIR,
                                 soname);

    plugin->watch = fopen(watch_name, "w");
    g_free(soname);
    g_free(watch_name);

}

static void start_plugin_action(NavigatorPlugin *plugin, gchar *action)
{
    g_assert(plugin != NULL);

    plugin->actions++;
    
    start_plugin_watch(plugin);

    write_plugin_log(plugin, action, "Started");
}

static void stop_plugin_action(NavigatorPlugin *plugin, gchar *action)
{
    g_assert(plugin != NULL);

    plugin->actions--;
    
    write_plugin_log(plugin, action, "Done");
    
    if (plugin->actions == 0)
    {
        stop_plugin_watch(plugin);
    }

}

static gint compare_plugins(NavigatorPlugin *a, NavigatorPlugin *b)
{
    if (a->position == b->position)
        return 0;
    if (a->position < b->position)
        return -1;

   return 1;
}

/* Appends a plugin to the list according to the position assigned
   to the plugin. If a plugin already exists in the position, following
   rules are applied:
    1) If the inserted plugin has the mandatory flag it will be placed in
       the position unconditionally. 
    2) If the inserted plugin has no mandatory flag, but the existing does,
       the plugin is ignored
    3) If neither the inserted nor the existing plugins have the mandatory flag,
       the plugin is inserted in the position
   
   The list is "collapsed" so that if you have positions "0,1,3,6",
   it will be treated as "0, 1, 2, 3" when creating the buttons,
   so no empty spaces.
   
   Returns: the start of the list, which may have changed.
 */
static GList *insert_navigator_plugin(Navigator *tasknav, GList *list, NavigatorPlugin *plugin)
{
    GList *l;
    NavigatorPlugin *existing_plugin;
    
    g_assert(tasknav != NULL);
    
    if (plugin == NULL)
      return list;
    
    existing_plugin = NULL;
    
    l = list;
    /* Look for an existing plugin in the position */
    while (l != NULL)
    {
        existing_plugin = (NavigatorPlugin *)l->data;
        
        if (existing_plugin != NULL
            && existing_plugin->position == plugin->position)
        {
            /* Mandatory plugins are inserted regardless of existing */
            if (plugin->mandatory)
            {
                break;
            }
            /* If the previous plugin was not mandatory, overwrite */
            else if (!existing_plugin->mandatory)
            {
                break;
            }
            /* Otherwise ignore the plugin */
            else
            {
                /* We destroy the plugin here since it won't be used,
                   this way we avoid checking if this function succeeded in
                   other parts of the code. This should be removed if the check
                   becomes necessary. 
                 */
                 g_print("Destroying old plugin %s:%s\n", plugin->library, plugin->name);
                destroy_plugin(tasknav, plugin);
                existing_plugin = NULL;
                return list;
            }
        }
        
        l = l->next;
    }
    
    /* Insert the plugin to correct position */
    list = g_list_insert_before(list, l, plugin);
    if (existing_plugin != NULL
        && existing_plugin->position == plugin->position)
    {
        list = g_list_remove(list, existing_plugin);
        destroy_plugin(tasknav, existing_plugin);
    }

    list = g_list_sort(list, (GCompareFunc)compare_plugins);
    
   return list;
}

static gboolean watchfile_check(const gchar *library)
{
    gchar *watchfile;
    
    watchfile = g_strdup_printf("%s/%s/navigator-%s.watch",
                                  home_dir,
                                  NAVIGATOR_WATCH_DIR,
                                  library);


    if (g_file_test(watchfile, G_FILE_TEST_EXISTS))
    {
        osso_log(LOG_WARNING, 
                "Watchfile for plugin %s exists! "
                "Plugin might be broken so not loading\n", 
                library);

        g_free(watchfile);
        return FALSE;
    }
    
    g_free(watchfile);
    return TRUE;
}

/* Loads a list of plugins from a configuration file
   If allow_mandatory is FALSE, the "mandatory" flags in the file will be
   ignored
 */
static GList *load_plugins_from_file(Navigator *tasknav,
                                     const gchar *filename,
                                     gboolean allow_mandatory)
{
    gint i;
    gchar **groups;
    GList *list;
    NavigatorPlugin *plugin;
    GKeyFile *keyfile;
    GError *error;

    g_assert(tasknav != NULL);

    list = NULL;

    keyfile = g_key_file_new();

    error = NULL;
    g_key_file_load_from_file(keyfile, filename, G_KEY_FILE_NONE, &error);

    if (error != NULL)
    {
        g_key_file_free(keyfile);
        g_error_free(error);
        return NULL;
    }

    /* Groups are a list of plugins in this context */
    groups = g_key_file_get_groups(keyfile, NULL);
    i = 0;
    while (groups[i] != NULL)
    {
        gchar *library = NULL;
        gint position;
        gboolean mandatory;


        library = g_key_file_get_string(keyfile, groups[i],
                                        PLUGIN_KEY_LIB, &error);
        if (error != NULL)
        {
            osso_log(LOG_WARNING, 
                    "Invalid plugin specification for %s: %s\n", 
                    groups[i], error->message);

            g_error_free(error);
            error = NULL;
            i++;
            g_free(library);
            library=NULL;
            continue;
        }

        if (!watchfile_check(library))
        {
            i++;
            g_free(library);
            library=NULL;
            continue;
        }
        
        position = g_key_file_get_integer(keyfile, groups[i],
                                          PLUGIN_KEY_POSITION, &error);
        if (error != NULL)
        {
            position = 0;
            g_error_free(error);
            error = NULL;
        }            

        mandatory = g_key_file_get_boolean(keyfile, groups[i],
                                           PLUGIN_KEY_MANDATORY, &error);
        if (error != NULL)
        {
            mandatory = FALSE;
            g_error_free(error);
            error = NULL;
        }            

        plugin = create_new_plugin(tasknav, groups[i], library);
        
        if (plugin == NULL)
        {
            osso_log(LOG_WARNING, 
                    "Failed to load plugin %s (%s)\n", 
                    groups[i], library);

            g_free(library);
            library = NULL;
            i++;
            continue;
            
        }        
        
        g_free(library);
        library = NULL;

        plugin->position = position;

        /* If mandatory is not allowed, leave it as FALSE */
        plugin->mandatory = FALSE;
        if (allow_mandatory)
        {
            plugin->mandatory = mandatory;
        }

        list = g_list_append(list, plugin);

        i++;
    }

    g_strfreev(groups);

    g_key_file_free(keyfile);

    return list;
}

/* Loads the list of effective plugins from configuration files
   The process:
    - Get plugins in NAVIGATOR_FACTORY_PLUGINS file
    - Get plugins in NAVIGATOR_USER_PLUGINS file
    - Merge the plugins to one list according the policy
      (see insert_navigator_plugin())
    - If the list is empty after this, fall back to default config
 */
static GList *load_navigator_plugin_list(Navigator *tasknav)
{
    gchar *fname;
    GList *l;
    GList *plugins;
    
    g_assert (tasknav != NULL);
    
    plugins = NULL;

    /* Factory plugins */
    for (l = load_plugins_from_file(tasknav, NAVIGATOR_FACTORY_PLUGINS, TRUE);
         l; l = l->next)
    {
        NavigatorPlugin *plugin = (NavigatorPlugin *)l->data;
        if (plugin != NULL)
        {
            plugins = insert_navigator_plugin(tasknav, plugins, plugin);
        }
    }

    /* User plugins */
    fname = g_strdup_printf("%s/%s", home_dir, NAVIGATOR_USER_PLUGINS);
    for (l = load_plugins_from_file(tasknav, fname, FALSE);
         l; l = l->next)
    {
        NavigatorPlugin *plugin = (NavigatorPlugin *)l->data;
        if (plugin != NULL)
        {
            plugins = insert_navigator_plugin(tasknav, plugins, plugin);
        }
    }

    g_free (fname);
    
    /* If the list is empty, fall back to traditional config */
    if (plugins == NULL)
    {
        NavigatorPlugin *plugin;
        
         if (watchfile_check(LIBTN_BOOKMARK_PLUGIN))
        {
           plugin = create_new_plugin(tasknav,
                                       LIBTN_BOOKMARK_PLUGIN,
                                       LIBTN_BOOKMARK_PLUGIN);
            if (plugin == NULL)
              {
                osso_log(LOG_WARNING, 
                        "Default plugin %s not loadable!\n", 
                        LIBTN_BOOKMARK_PLUGIN);
              }
            else
              { 
                plugin->position = 0;
                plugin->mandatory = FALSE;
                plugins = g_list_append(plugins, plugin);
              }
        }


         if (watchfile_check(LIBTN_MAIL_PLUGIN))
        {
            plugin = create_new_plugin(tasknav,
                                       LIBTN_MAIL_PLUGIN,
                                       LIBTN_MAIL_PLUGIN);
            if (plugin == NULL)
              {
                osso_log(LOG_WARNING, 
                        "Default plugin %s not loadable!\n", 
                        LIBTN_MAIL_PLUGIN);
              }
            else
              { 
                plugin->position = 1;
                plugin->mandatory = TRUE;
                plugins = g_list_append(plugins, plugin);
              }
        }
    }
    
    return plugins;
}

static void destroy_plugin(Navigator *tasknav, NavigatorPlugin *plugin)
{
    const char *error_str = NULL;

    g_assert (tasknav != NULL);

    if (plugin == NULL)
        return;

    start_plugin_action(plugin, "Destroy");
    
    if (plugin->handle != NULL)
    {
        error_str = load_symbols(tasknav, plugin->handle,
                                 DESTROY_SYMBOL);
          
        if (error_str)
        {
            osso_log(LOG_WARNING, 
                     "Unable to load symbols from TN Plugin %s: %s\n", 
                     plugin->name, error_str);
            
            if (plugin->handle)
            {
                dlclose(plugin->handle);
            }
        }
        else
        {
            tasknav->destroy(plugin->data);
        }

        if (plugin->button != NULL)
        {
            gtk_container_remove(GTK_CONTAINER(tasknav->box), plugin->button);
        }
        
        dlclose(plugin->handle);
    }

    stop_plugin_action(plugin, "Destroy");
        
    g_free(plugin->name);
    g_free(plugin->library);
    
    g_free (plugin);
    plugin = NULL;
}

static void plugin_configuration_changed( char *path,
                                          gpointer *data )
{
    GList *plugins; 
    Navigator *tasknav;

    g_assert (data != NULL);

    tasknav = (Navigator *)data;

    plugins = load_navigator_plugin_list(tasknav);

    reload_plugins(tasknav, plugins);
}

static void reload_plugins(Navigator *tasknav, GList *list)
{
    GList *l;

    g_assert (tasknav != NULL);

    for (l = tasknav->plugins;
         l ; l = l->next)
    {
        NavigatorPlugin *plugin;
        
        plugin = (NavigatorPlugin *)l->data;
        
        if (plugin == NULL)
          continue;
        
        destroy_plugin(tasknav, plugin);
    }
    
    g_list_free(tasknav->plugins);
    
    tasknav->plugins = list;
    
    for (l = tasknav->plugins ; l ; l = l->next)
    {
        const char *error_str = NULL;
        NavigatorPlugin *plugin;
        
        plugin = (NavigatorPlugin *)l->data;
        
        if (plugin == NULL)
          continue;

        start_plugin_action(plugin, "Load navigator_button");
        error_str = load_symbols(tasknav, plugin->handle, GET_BUTTON_SYMBOL);
        stop_plugin_action(plugin, "Load navigator_button");
            
        if (error_str)
        {
            osso_log(LOG_WARNING, 
                    "Unable to load symbols from TN Plugin %s: %s\n", 
                    plugin->name, error_str);
            
            dlclose(plugin->handle);
            plugin->handle = NULL;

            continue;
        }

        start_plugin_action(plugin, "Run navigator_button");
        plugin->button = tasknav->navigator_button(plugin->data);  
        stop_plugin_action(plugin, "Run navigator_button");

        start_plugin_action(plugin, "Setup");
        gtk_widget_set_size_request(plugin->button, -1, BUTTON_HEIGHT );
        g_object_set (G_OBJECT (plugin->button), 
                      "can-focus", FALSE, NULL);
        stop_plugin_action(plugin, "Setup");

        start_plugin_action(plugin, "Insertion");
        gtk_box_pack_start(tasknav->box, plugin->button, 
                           FALSE, FALSE, 0);
        gtk_widget_show(plugin->button);
        stop_plugin_action(plugin, "Insertion");
    }

}

void navigator_set_sensitive(gboolean sensitivity)
{
    tn_is_sensitive=sensitivity;
    if (sensitivity)
    {
	gtk_container_foreach(GTK_CONTAINER(task_nav->box), 
			      (GtkCallback)(tasknav_sensitive_cb),
			      NULL);

    }
    else 
    {
	gtk_container_foreach(GTK_CONTAINER(task_nav->box), 
			      (GtkCallback)(tasknav_insensitive_cb),
			      NULL);
	
    }
}

static void tasknav_insensitive_cb( GtkWidget *widget, gpointer data)
{
    
    gtk_widget_set_sensitive(widget, FALSE);

    if (GTK_IS_CONTAINER(widget))
    {
	gtk_container_foreach(GTK_CONTAINER(widget), 
			      (GtkCallback)(tasknav_insensitive_cb),
			      NULL);	
    }
}
static void tasknav_sensitive_cb( GtkWidget *widget, gpointer data)
{
    gtk_widget_set_sensitive(widget, TRUE);

    if (GTK_IS_CONTAINER(widget))
    {
	gtk_container_foreach(GTK_CONTAINER(widget), 
			      (GtkCallback)(tasknav_sensitive_cb),
			      NULL);	
    }
}


/* This callback creates/loads the button widgets and packs them
   into the vbox */
static void create_navigator(Navigator *tasknav)
{
    gchar *plugin_dir;
    
    task_nav = tasknav;

    home_dir = getenv("HOME");

    /* Get configuration options from the environment.
     */
    config_do_bgkill = getenv_yesno("NAVIGATOR_DO_BGKILL", TRUE);
    config_lowmem_dim = getenv_yesno("NAVIGATOR_LOWMEM_DIM", FALSE);
    config_lowmem_notify_enter = getenv_yesno("NAVIGATOR_LOWMEM_NOTIFY_ENTER", FALSE);
    config_lowmem_notify_leave = getenv_yesno("NAVIGATOR_LOWMEM_NOTIFY_LEAVE", FALSE);
    config_lowmem_pavlov_dialog = getenv_yesno("NAVIGATOR_LOWMEM_PAVLOV_DIALOG", FALSE);

    tasknav->main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    tasknav->plugins = NULL;

    gtk_window_set_type_hint(GTK_WINDOW(tasknav->main_window), 
                             GDK_WINDOW_TYPE_HINT_DOCK);
    
    gtk_window_set_accept_focus(GTK_WINDOW(tasknav->main_window), FALSE);
    
    tasknav->box = GTK_BOX(gtk_vbox_new(FALSE, 0));

    gtk_widget_set_size_request(GTK_WIDGET(tasknav->box), HILDON_NAVIGATOR_WIDTH,
                                gdk_screen_height());
    gtk_container_add(GTK_CONTAINER(tasknav->main_window),
                      GTK_WIDGET(tasknav->box));

    /* Initialize Plugins */
    reload_plugins(tasknav, load_navigator_plugin_list(tasknav));

    /* This is moved here since it inits the dnotify stuff and initing them
     * twice would result in an error according to
     * hildon-base-lib/hildon-base-dnotify.h
     */
    tasknav->others_menu = others_menu_init();

    plugin_dir = g_strdup_printf("%s/%s", home_dir, NAVIGATOR_USER_DIR);

    if ( hildon_dnotify_set_cb(
			    (hildon_dnotify_cb_f *)plugin_configuration_changed,
			    plugin_dir, tasknav ) != HILDON_OK) {
	    osso_log( LOG_ERR, "Error setting dir notify callback!\n" );
    }

    g_free(plugin_dir);

    /* Create applications switcher button */
    tasknav->app_switcher = application_switcher_init();
    if (tasknav->app_switcher)
    {
        
        tasknav->app_switcher_button = application_switcher_get_button(
                                                  tasknav->app_switcher);

        g_object_set (G_OBJECT (tasknav->app_switcher_button), 
                      "can-focus", FALSE, NULL);
        gtk_box_pack_end(tasknav->box, tasknav->app_switcher_button,
                           FALSE, FALSE, 0);
    }

    /* Others menu */

    if (tasknav->others_menu)
    {
        tasknav->others_menu_button = others_menu_get_button(
                                                    tasknav->others_menu);
        gtk_widget_set_size_request(tasknav->others_menu_button, -1, 
                                    BUTTON_HEIGHT );

        g_object_set (G_OBJECT (tasknav->others_menu_button), 
                      "can-focus", FALSE, NULL);
        gtk_box_pack_end(tasknav->box, tasknav->others_menu_button,
                           FALSE, FALSE, 0);
    }
    

    /* Initialize navigator menus, then display GUI */
    
    initialize_navigator_menus(tasknav);
    gtk_widget_show_all(tasknav->main_window);
    application_switcher_add_menubutton(tasknav->app_switcher);
    
}

/* Create new plugin. If loading plugin succeed, function returns the 
   pointer to a NavigatorPlugin structure */
static gpointer create_new_plugin(Navigator *tasknav,
                                  const gchar *name,
                                  const gchar *plugin_name)
{
    char *full_path;
    NavigatorPlugin *plugin;

    g_assert (tasknav != NULL);

    if (name == NULL)
        return NULL;
    
    if (plugin_name == NULL)
        return NULL;

    
    if (plugin_name[0] == '/')
    {
        full_path = g_strdup(plugin_name);
    }
    else
    {
        full_path = g_build_path("/", PLUGINDIR, plugin_name, NULL);
    }

    plugin = g_new0(NavigatorPlugin, 1);
    if (plugin == NULL)
    {   
        osso_log(LOG_WARNING, 
                 "Memory exhausted when loading %s\n",
                 plugin_name);

        g_free(full_path);
        return NULL;
    }
        
    plugin->name = g_strdup(name);
    plugin->library = g_strdup(full_path);

    start_plugin_action(plugin, "dlopen()");
    plugin->handle = dlopen(full_path, RTLD_NOW);
    stop_plugin_action(plugin, "dlopen()");

    plugin->watch = NULL;

    g_free(full_path);
        
    if (!plugin->handle)
    {   
        osso_log(LOG_WARNING, 
                 "Unable to open Task Navigator Plugin %s\n",
                 plugin_name);
        g_free(plugin->name);
        g_free(plugin->library);
        g_free(plugin);

        return NULL;
    }

    else
    {
        const char *error_str = NULL;
        
        start_plugin_action(plugin, "Load create");
        error_str = load_symbols(tasknav, plugin->handle, CREATE_SYMBOL);
        stop_plugin_action(plugin, "Load create");
    
        if (error_str)
        {
            osso_log(LOG_WARNING, 
                    "Unable to load symbols from TN Plugin %s: %s\n", 
                    plugin_name, error_str);
            
            dlclose(plugin->handle);
            plugin->handle = NULL;
            g_free(plugin->name);
            g_free(plugin->library);
            g_free(plugin);

            return NULL;
        }
        
        start_plugin_action(plugin, "Run create");
        plugin->data = tasknav->create();
        stop_plugin_action(plugin, "Run create");
    }
    return (gpointer) plugin;
}

/* Destroy navigator data */
static void destroy_navigator(Navigator *tasknav)
{
    GList *l;
    
    g_assert (tasknav != NULL);
    
    gtk_widget_destroy(GTK_WIDGET(tasknav->others_menu_button));
    gtk_widget_destroy(GTK_WIDGET(tasknav->app_switcher_button));
     
    /* Destory plugins*/
    for (l = tasknav->plugins ; l ; l = l->next)
    {
        NavigatorPlugin *plugin;
        
        plugin = (NavigatorPlugin *)l->data;
    
        destroy_plugin(tasknav, plugin);
    }
    
    /* Destroy others menu */
    others_menu_deinit(tasknav->others_menu);

    /* Destroy application switcher */
    application_switcher_deinit(tasknav->app_switcher);
}

/* This function loads the plugin symbols */
static const char *load_symbols(Navigator *tasknav, void *dlhandle, 
                                  gint symbol_id)
{

    const char *result = NULL;
    void *symbol;

    g_assert (tasknav != NULL);
    g_assert (dlhandle != NULL);
    
    switch (symbol_id)
    {
    case CREATE_SYMBOL:
        symbol = dlsym(dlhandle, "hildon_navigator_lib_create");
        result = dlerror();
        if ( result )
            return result;
        tasknav->create = (PluginCreateFn)symbol;
    case GET_BUTTON_SYMBOL:
        symbol = dlsym(dlhandle, "hildon_navigator_lib_get_button_widget");
        result = dlerror();
        if ( result )
            return result;
        tasknav->navigator_button = (PluginGetButtonFn)symbol;
    case INITIALIZE_MENU_SYMBOL:
        symbol = dlsym(dlhandle, "hildon_navigator_lib_initialize_menu");
        result = dlerror();
        if ( result )
            return result;
        tasknav->initialize_menu = (PluginInitializeMenuFn)symbol;
    case DESTROY_SYMBOL:
        symbol = dlsym(dlhandle, "hildon_navigator_lib_destroy");
        result = dlerror();
        if ( result )
            return result;
        tasknav->destroy = (PluginDestroyFn)symbol;
    }
    
    return NULL;
}

/* Function to initialize navigator menus */
static void initialize_navigator_menus(Navigator *tasknav)
{
    GList *l;

    g_assert (tasknav != NULL);

    /* Initialize plugin menus */

    for (l = tasknav->plugins ; l ; l = l->next)
    {
        const char *error_str = NULL;
        NavigatorPlugin *plugin;
        
        plugin = (NavigatorPlugin *)l->data;
        
        if (plugin == NULL)
          continue;

        if (plugin->handle == NULL)
          continue;
        
        error_str = load_symbols(tasknav, plugin->handle, INITIALIZE_MENU_SYMBOL);
            
        if (error_str)
        {
            osso_log(LOG_WARNING, 
                    "Unable to load symbols from TN Plugin %s: %s\n", 
                    plugin->name, error_str);
            
            dlclose(plugin->handle);
            plugin->handle = NULL;

            continue;
        }
        else
        {
            tasknav->initialize_menu(plugin->data);
        }
    }


    /*Initialize application switcher menu*/
    application_switcher_initialize_menu(tasknav->app_switcher);
    tasknav->app_switcher_dnotify_cb =
        application_switcher_get_dnotify_handler(tasknav->app_switcher);

    /* Initialize others menu */
    others_menu_initialize_menu(tasknav->others_menu,
                                tasknav->app_switcher_dnotify_cb);

}

/* 
 *
 *  This will be called from af-desktop-main
 *  
 */

#ifdef USE_AF_DESKTOP_MAIN__
int task_navigator_main(Navigator *tasknav){
 
  
    
    create_navigator(tasknav);

    return 0;

}

int task_navigator_deinitialize(Navigator *tasknav){

    gchar *watch_dir;
    const gchar *filename;
    GDir *dir;
    GError *error;

    destroy_navigator(tasknav);
    
    /* Everybody deserves a second chance:
     * Clear the plugin watchfiles after a succesfull lifecycle
     */

    watch_dir = g_strdup_printf("%s/%s", home_dir, NAVIGATOR_WATCH_DIR);
    hildon_dnotify_remove_cb(NAVIGATOR_WATCH_DIR);
    
    error = NULL;
    dir = g_dir_open (watch_dir, 0, &error);
    
    if (dir == NULL)
      {
        ULOG_WARN("Cannot open watch dir %s: %s", watch_dir, error->message);
        g_error_free(error);
        g_free(watch_dir);
        return 0;
      }

    filename = g_dir_read_name(dir);
    while (filename != NULL)
      {
        /* If the format of the name matches, unlink the file */
        if (g_str_has_prefix(filename, NAVIGATOR_WATCH_PREFIX)
            && g_str_has_suffix(filename, ".watch"))
          {
            gchar *path = g_strdup_printf("%s/%s", watch_dir, filename);
            if (remove(path) < 0)
              {
                ULOG_WARN("Unable to remove watch file %s!", path);
              }
            g_free(path);
              
          }
        filename = g_dir_read_name(dir);
      }
    
    g_free(watch_dir);

    return 0;
}
#endif


#ifndef USE_AF_DESKTOP_MAIN__


int main( int argc, char* argv[] )
{
    
    Navigator tasknav;
    setlocale (LC_ALL, "");
    bindtextdomain (PACKAGE, LOCALEDIR);
    textdomain (PACKAGE);
    
    gtk_init(&argc, &argv);
    
    task_navigator_main(&tasknav);
    
    gtk_main();

    task_navigator_deinitialize(&tasknav);
    
    return 0;
}
    
#endif
