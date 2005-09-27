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

/* Locale include */
#include <locale.h>

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

#include "kstrace.h"

#define USE_AF_DESKTOP_MAIN__
                                                                                       

/* Global external variables */
gboolean config_do_bgkill;
gboolean config_lowmem_dim;
gboolean config_lowmem_notify;
gboolean config_lowmem_pavlov_dialog;

/* Callbacks */
static void initialize_navigator_menus(Navigator *tasknav);

static const char *load_symbols(Navigator *tasknav, void *dlhandle, 
                                  gint symbol_id);

static void create_navigator(Navigator *tasknav);

static void destroy_navigator(Navigator *tasknav);
     
static gpointer create_new_plugin(Navigator *tasknav, gchar *plugin_name); 

static gboolean getenv_yesno(const char *env, gboolean def)
{
    char *val = getenv (env);

    if (val)
	return (strcmp(val, "yes") == 0);
    else
	return def;
}

                
/* This callback creates/loads the button widgets and packs them
   into the vbox */
static void create_navigator(Navigator *tasknav)
{
    GtkBox *box;

    /* Get configuration options from the environment.
     */
    config_do_bgkill = getenv_yesno("NAVIGATOR_DO_BGKILL", TRUE);
    config_lowmem_dim = getenv_yesno("NAVIGATOR_LOWMEM_DIM", FALSE);
    config_lowmem_notify = getenv_yesno("NAVIGATOR_LOWMEM_NOTIFY", FALSE);
    config_lowmem_pavlov_dialog = getenv_yesno("NAVIGATOR_LOWMEM_PAVLOV_DIALOG", FALSE);

    tasknav->main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    gtk_window_set_type_hint(GTK_WINDOW(tasknav->main_window), 
                             GDK_WINDOW_TYPE_HINT_DOCK);
    
    gtk_window_set_accept_focus(GTK_WINDOW(tasknav->main_window), FALSE);
    
    box = GTK_BOX(gtk_vbox_new(FALSE, 0));

    gtk_widget_set_size_request(GTK_WIDGET(box), HILDON_NAVIGATOR_WIDTH,
                                gdk_screen_height());
    gtk_container_add(GTK_CONTAINER(tasknav->main_window),
                      GTK_WIDGET(box));

    /* Initialize bookmark Plugin */
    tasknav->dlhandle_bookmark = create_new_plugin(tasknav, 
                                                   LIBTN_BOOKMARK_PLUGIN);
    if (tasknav->dlhandle_bookmark)
    {
        tasknav->bookmark_data = tasknav->create(); 
        tasknav->bookmark_button = tasknav->navigator_button(
                                                tasknav->bookmark_data);  
        gtk_widget_set_size_request(tasknav->bookmark_button, -1,
                                    BUTTON_HEIGHT );
        g_object_set (G_OBJECT (tasknav->bookmark_button), 
                      "can-focus", FALSE, NULL);

        gtk_box_pack_start(box, tasknav->bookmark_button, 
                           FALSE, FALSE, 0);
    }

    /* Initialize Mail Plugin */
    tasknav->dlhandle_mail = create_new_plugin(tasknav, 
                                               LIBTN_MAIL_PLUGIN);

    
    if (tasknav->dlhandle_mail)
    {
        tasknav->mail_data = tasknav->create();
        tasknav->mail_button = tasknav->navigator_button(
                                            tasknav->mail_data);
        gtk_widget_set_size_request(tasknav->mail_button, -1, 
                                    BUTTON_HEIGHT );

        g_object_set (G_OBJECT (tasknav->mail_button), 
                      "can-focus", FALSE, NULL);
        gtk_box_pack_start(box, tasknav->mail_button, 
                           FALSE, FALSE, 0);
    }
    
    /* Others menu */

    tasknav->others_menu = others_menu_init();

    
    if (tasknav->others_menu)
    {
        tasknav->others_menu_button = others_menu_get_button(
                                                    tasknav->others_menu);
        gtk_widget_set_size_request(tasknav->others_menu_button, -1, 
                                    BUTTON_HEIGHT );
        g_object_set (G_OBJECT (tasknav->others_menu_button), 
                      "can-focus", FALSE, NULL);
        gtk_box_pack_start(box, tasknav->others_menu_button,
                           FALSE, FALSE, 0);
    }
    
    /* Create applications switcher button */
    tasknav->app_switcher = application_switcher_init();
    if (tasknav->app_switcher)
    {
        tasknav->app_switcher_button = application_switcher_get_button(
                                                  tasknav->app_switcher);

        g_object_set (G_OBJECT (tasknav->app_switcher_button), 
                      "can-focus", FALSE, NULL);
        gtk_box_pack_start(box, tasknav->app_switcher_button,
                           FALSE, FALSE, 0);
    }

    /* Initialize navigator menus, then display GUI */
    
    initialize_navigator_menus(tasknav);
    gtk_widget_show_all(tasknav->main_window);
    application_switcher_add_menubutton(tasknav->app_switcher);
    
}

/* Create new plugin. If loading plugin succeed, function returns the 
   pointer to be used in subsequent calls to dlsym and dlclose */
static gpointer create_new_plugin(Navigator *tasknav, gchar *plugin_name)
{
    char *lib_path,*full_path;
    void *dlhandle;

    lib_path = hildon_navigator_get_lib_dir();
    full_path = g_build_path("/", lib_path, plugin_name, NULL);
        
    dlhandle = dlopen(full_path, RTLD_NOW);
    g_free(full_path);
    g_free(lib_path);
        
    if (!dlhandle)
    {   
        osso_log(LOG_WARNING, 
                 "Unable to open Task Navigator Plugin %s\n",
                 plugin_name);

        return NULL;
    }

    else
    {
        const char *error_str = NULL;
        
        error_str = load_symbols(tasknav, dlhandle, CREATE_SYMBOL);
            
        if (error_str)
        {
            osso_log(LOG_WARNING, 
                    "Unable to load symbols from TN Plugin %s: %s\n", 
                    plugin_name, error_str);
            
            dlclose(dlhandle);

            return NULL;
        }
    }
    return (gpointer) dlhandle;
}

/* Destroy navigator data */
static void destroy_navigator(Navigator *tasknav)
{
    const char *error_str = NULL;
    
    gtk_widget_destroy(tasknav->bookmark_button);
    gtk_widget_destroy(tasknav->mail_button);
    gtk_widget_destroy(GTK_WIDGET(tasknav->others_menu_button));
    gtk_widget_destroy(GTK_WIDGET(tasknav->app_switcher_button));
     
    /* Destory bookmark menu*/
    error_str = load_symbols(tasknav, tasknav->dlhandle_bookmark, 
                             DESTROY_SYMBOL);
      
    if (error_str)
    {
        osso_log(LOG_WARNING, 
                 "Unable to load symbols from TN Plugin %s: %s\n", 
                 LIBTN_BOOKMARK_PLUGIN, error_str);
        
        if (tasknav->dlhandle_bookmark)
        {
            dlclose(tasknav->dlhandle_bookmark);
        }
    }
    else
    {
        tasknav->destroy(tasknav->bookmark_data);
    }
    
    /* Destroy mail menu*/
    error_str = load_symbols(tasknav, tasknav->dlhandle_mail, 
                             DESTROY_SYMBOL);
            
    if (error_str)
    {
        osso_log(LOG_WARNING, 
                 "Unable to load symbols from TN Plugin %s: %s\n", 
                 LIBTN_MAIL_PLUGIN, error_str);
        
        if (tasknav->dlhandle_mail)
        {
            dlclose(tasknav->dlhandle_mail);
        }
    }
    else
    {    
        tasknav->destroy(tasknav->mail_data);   
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
    void *symbol[ MAX_SYMBOLS ];
    
    if (symbol_id == CREATE_SYMBOL)
    {
        symbol[CREATE_SYMBOL] = dlsym(dlhandle, 
                                       "hildon_navigator_lib_create");

        result = dlerror();
        
        if ( result )
        {
            return result;
        }
        tasknav->create = (PluginCreateFn)symbol[CREATE_SYMBOL];
             
        symbol[GET_BUTTON_SYMBOL] = dlsym(dlhandle,
                             "hildon_navigator_lib_get_button_widget");
        
        result = dlerror();
        
        if ( result )
        {
            return result;
        }
        tasknav->navigator_button = (PluginGetButtonFn)symbol[
                                                    GET_BUTTON_SYMBOL];
    }

    else if (symbol_id == INITIALIZE_MENU_SYMBOL)
    {
        symbol[INITIALIZE_MENU_SYMBOL] = dlsym(dlhandle,
                                 "hildon_navigator_lib_initialize_menu");

        result = dlerror();
        
        if ( result )
        {
            return result;
        }
        tasknav->initialize_menu = (PluginInitializeMenuFn)symbol[
                                                INITIALIZE_MENU_SYMBOL];
    }

    else if (symbol_id == DESTROY_SYMBOL)
    {
        symbol[DESTROY_SYMBOL] = dlsym(dlhandle, 
                                        "hildon_navigator_lib_destroy");

        result = dlerror();
        
        if ( result )
        {
            return result;
        }
        tasknav->destroy = (PluginDestroyFn)symbol[DESTROY_SYMBOL];
    }
    
    return NULL;
}

/* Function to initialize navigator menus */
static void initialize_navigator_menus(Navigator *tasknav)
{
    const char *error_str = NULL;

    /* Initialize bookmark menu*/
    error_str = load_symbols(tasknav, tasknav->dlhandle_bookmark, 
                             INITIALIZE_MENU_SYMBOL);

    if (error_str)
    {
        osso_log(LOG_WARNING, 
                 "Unable to load symbols from TN Plugin %s: %s\n", 
                 LIBTN_BOOKMARK_PLUGIN, error_str);

        if (tasknav->dlhandle_bookmark)
        {
            dlclose(tasknav->dlhandle_bookmark);
        }
    }
    else
    {
        tasknav->initialize_menu(tasknav->bookmark_data);
    }

    /* Initialize mail menu*/
    error_str = load_symbols(tasknav, tasknav->dlhandle_mail, 
                             INITIALIZE_MENU_SYMBOL);
            
    if (error_str)
    {
        osso_log(LOG_WARNING, 
                 "Unable to load symbols from TN Plugin %s: %s\n", 
                 LIBTN_MAIL_PLUGIN, error_str);
        
        if (tasknav->dlhandle_mail)
        {
            dlclose(tasknav->dlhandle_mail);
        }
    }
    
    else
    {
        tasknav->initialize_menu(tasknav->mail_data);   
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


    destroy_navigator(tasknav);
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
