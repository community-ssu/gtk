/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
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
 * @file applet-manager.c
 *
 */

/* System includes */
#include <glib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

/* log include */
#include <osso-log.h>
#include <stdio.h>


/* hildon includes */
#include "applet-manager.h"
#include "hildon-home-interface.h" 
#include "hildon-home-main.h"

/* --------------- applet manager private start ---------------- */

static
gchar *get_factory_configure_file( void )
{
    return g_strconcat(APPLET_MANAGER_FACTORY_PATH,
                       G_DIR_SEPARATOR_S,
                       APPLET_MANAGER_CONFIGURE_FILE, 
                       NULL);
}

static
gchar *get_user_configure_file( void )
{
    return g_strconcat(g_getenv(APPLET_MANAGER_ENV_HOME),
                       G_DIR_SEPARATOR_S,
                       APPLET_MANAGER_USER_PATH,
                       G_DIR_SEPARATOR_S,
                       APPLET_MANAGER_CONFIGURE_FILE, 
                       NULL);
}


/* --------------- applet manager private end ------------------ */
/* --------------- applet manager public start ----------------- */

applet_manager_t *applet_manager_singleton_get_instance(void)
{
    static applet_manager_t *instance = NULL;
    
    if (!instance)
    {
        instance = g_malloc0(sizeof(applet_manager_t));

        instance->applet_list = NULL;
    }
    return instance;
}

void applet_manager_initialize(applet_manager_t *man,
                               gchar *librarypath,
                               gchar *desktoppath,
                               gint applet_x, gint applet_y)
{
    HomeAppletHandler *handler;
    void *state_data = NULL; 
    int state_size = 0;

    if(applet_manager_identifier_exists(man, desktoppath) == TRUE)
    {
        ULOG_ERR( "Identifier %s exists==TRUE \n", desktoppath);
        return;
    }

    handler = home_applet_handler_new(desktoppath, librarypath, 
                                      state_data, &state_size);

    if(handler == NULL)
    {
        ULOG_ERR( "Couldn't retrieve Handler for %s\n", desktoppath);
        return;
    }
    g_assert(handler);

    if(applet_x != APPLET_INVALID_COORDINATE && 
       applet_y != APPLET_INVALID_COORDINATE)
    {
        home_applet_handler_set_coordinates(handler, applet_x, applet_y);
    }
    
    man->applet_list = g_list_append(man->applet_list, handler);
}


void applet_manager_initialize_new(applet_manager_t *man,
                                   gchar *desktoppath)
{
    /* We have no information outside .desktop file. Applet handler is
     * responsible to retrieve and initialize applet in this case.
     */
    gchar *librarypath = NULL;
    gint applet_x = APPLET_INVALID_COORDINATE;
    gint applet_y = APPLET_INVALID_COORDINATE;

    if(applet_manager_identifier_exists(man, desktoppath) == FALSE)
    {
        applet_manager_initialize(man, librarypath, desktoppath, applet_x, applet_y);
    }
}



void applet_manager_deinitialize_handler(applet_manager_t *man,
                                         HomeAppletHandler *handler)
{
    if(handler == NULL)
    {
        return;
    }
    g_assert(handler);

    home_applet_handler_deinitialize(handler);
    man->applet_list = g_list_remove(man->applet_list, handler);
}

void applet_manager_deinitialize(applet_manager_t *man,
                                 gchar *identifier)
{
    HomeAppletHandler *handler = applet_manager_get_handler(man, identifier);

    if(handler == NULL)
    {
        return;
    }
    g_assert(handler);

    applet_manager_deinitialize_handler(man, handler);
}

void applet_manager_deinitialize_all(applet_manager_t *man)
{
    GList *handler;

    for(handler = man->applet_list; handler != NULL; handler = handler->next)
    {
        if(handler->data)
            home_applet_handler_deinitialize((HomeAppletHandler *)handler->data);
    }

    g_list_free (man->applet_list);
}

void applet_manager_configure_save_all(applet_manager_t *man)
{
    gint i=0;
    gchar **groups;
    
    GKeyFile *keyfile;
    GError *error = NULL;
    gchar *configure_file = get_user_configure_file();
    struct stat buf;

    GList *handler_listitem;
    gchar *conf_data;
    gsize length;
    FILE *new_config_file;

    if(configure_file == NULL)
    {
        ULOG_ERR( "configure_file == NULL\n");
        return;
    }
    keyfile = g_key_file_new();

    if(stat(configure_file, &buf) != -1 && buf.st_size != 0)
    {

        error = NULL;
        g_key_file_load_from_file(keyfile, configure_file,
                                  G_KEY_FILE_NONE, &error);
        if (error != NULL)
        {
            ULOG_WARN("Config file error %s: %s\n", 
                      configure_file, error->message);    
            
            g_key_file_free(keyfile);
            g_error_free(error);
            g_free(configure_file);
            return;
        }
    
        /* Groups are a list of applets in this context */
        groups = g_key_file_get_groups(keyfile, NULL);

        while (groups[i] != NULL)
        {
            g_key_file_remove_group (keyfile,
                                     groups[i],
                                     &error);
            i++;
        }
        g_strfreev(groups);
    }

    for(handler_listitem = man->applet_list; handler_listitem != NULL; 
        handler_listitem = handler_listitem->next)
    {
        gchar *desktop, *library;
        gint applet_x, applet_y;
        HomeAppletHandler *handlerItem = 
            (HomeAppletHandler *)handler_listitem->data;

        desktop = home_applet_handler_get_desktop_filepath(handlerItem);
        library = home_applet_handler_get_libraryfile(handlerItem);
        home_applet_handler_get_coordinates(handlerItem, &applet_x, &applet_y);
        g_key_file_set_string(keyfile, desktop, APPLET_KEY_LIBRARY, library);
        g_key_file_set_string(keyfile, desktop, APPLET_KEY_DESKTOP, desktop);
        g_key_file_set_integer(keyfile, desktop, APPLET_KEY_X, applet_x);
        g_key_file_set_integer(keyfile, desktop, APPLET_KEY_Y, applet_y);
    }    

    conf_data = g_key_file_to_data(keyfile, &length, &error);    

    new_config_file = fopen(configure_file, "w"); 
    if(new_config_file == NULL || &length == NULL || 
            fprintf(new_config_file, "%s", conf_data) < 0)
    {
        ULOG_WARN("FAILED to write new conf data into %s\n", configure_file);
        show_flash_full_note();
        g_key_file_free(keyfile);
        g_free(configure_file);
        g_free(conf_data);
        return;
    }
    fclose(new_config_file);
    g_key_file_free(keyfile);
    g_free(configure_file);
    g_free(conf_data);
}

void applet_manager_configure_load_all(applet_manager_t *man)
{
    gint i;
    gchar **groups;
    GList *conf_identifier_list = NULL;
    GList *conf_identifier_list_iter = NULL;
    GList *handler_listitem;

    GKeyFile *keyfile;
    GError *error;
    gchar *configure_file = get_user_configure_file();
    struct stat buf;

    /* If no configure file found, try factory configure file
     * If its also empty, applets should not exist
     */
    if(configure_file == NULL || stat(configure_file, &buf) == -1)
    {
        g_free(configure_file);
        configure_file = get_factory_configure_file();
        if(configure_file == NULL || stat(configure_file, &buf) == -1)
        {
            g_free(configure_file);
            applet_manager_deinitialize_all(man);
            return; 
        }
    }

    keyfile = g_key_file_new();

    error = NULL;
    g_key_file_load_from_file(keyfile, configure_file,
                              G_KEY_FILE_NONE, &error);
    if (error != NULL)
    {
        ULOG_WARN("Config file error %s: %s\n", 
                  configure_file, error->message);    

        g_key_file_free(keyfile);
        g_error_free(error);
        g_free(configure_file);
        return;
    }
    g_free(configure_file);
    
    /* Groups are a list of applets in this context */
    groups = g_key_file_get_groups(keyfile, NULL);
    i = 0;
    while (groups[i] != NULL)
    {
        gchar *libraryfile = NULL;
        gchar *desktopfile = NULL;
        gint applet_x;
        gint applet_y;

        libraryfile = g_key_file_get_string(keyfile, groups[i],
                                            APPLET_KEY_LIBRARY, &error);
        if (error != NULL)
        {
            ULOG_WARN("Invalid applet specification for %s: %s\n", 
                      groups[i], error->message);

            g_error_free(error);
            error = NULL;
            i++;
            g_free(libraryfile);
            continue;
        }

        desktopfile = g_key_file_get_string(keyfile, groups[i],
                                            APPLET_KEY_DESKTOP, &error);
        if (error != NULL)
        {
            ULOG_WARN("Invalid applet specification for %s: %s\n", 
                      groups[i], error->message);

            g_error_free(error);
            error = NULL;
            i++;
            g_free(libraryfile);
            g_free(desktopfile);
            continue;
        }
        
        applet_x = g_key_file_get_integer(keyfile, groups[i],
                                          APPLET_KEY_X, &error);
        if (error != NULL || applet_x == APPLET_INVALID_COORDINATE)
        {
            ULOG_WARN("Invalid applet specification or invalid coordinate '%d'"
                      "for %s: %s\n", 
                      applet_x, groups[i], error->message);
            applet_x = APPLET_INVALID_COORDINATE;
            g_error_free(error);
            error = NULL;
            i++;
            g_free(libraryfile);
            g_free(desktopfile);
            continue;
        }

        applet_y = g_key_file_get_integer(keyfile, groups[i],
                                          APPLET_KEY_Y, &error);
        if (error != NULL || applet_y == APPLET_INVALID_COORDINATE)
        {
            ULOG_WARN("Invalid applet specification or invalid coordinate '%d'"
                      "for %s: %s\n", 
                      applet_y, groups[i], error->message);
            applet_y = APPLET_INVALID_COORDINATE;
            g_error_free(error);
            error = NULL;
            i++;
            g_free(libraryfile);
            g_free(desktopfile);
            continue;
        }

        if(applet_manager_identifier_exists(man, desktopfile) == FALSE)
        {
            applet_manager_initialize(man, libraryfile, desktopfile, 
                                      applet_x, applet_y);
        } else
        {
            applet_manager_set_coordinates(man, desktopfile, 
                                           applet_x, applet_y);
        }

        conf_identifier_list = g_list_append(conf_identifier_list, desktopfile);
        i++;
    }
            
    for(handler_listitem = man->applet_list; handler_listitem != NULL; 
        handler_listitem = handler_listitem->next)
    {
        gboolean identifier_exists = FALSE;
        gchar *identifier = 
            applet_manager_get_identifier_handler(man, 
                (HomeAppletHandler *)handler_listitem->data);
        
        for(conf_identifier_list_iter = conf_identifier_list; 
            conf_identifier_list_iter != NULL; 
            conf_identifier_list_iter = conf_identifier_list_iter ->next)
        {
            
            if(identifier != NULL && conf_identifier_list_iter->data != NULL &&
               g_str_equal(identifier, (gchar *)conf_identifier_list_iter->data))
            {
                identifier_exists = TRUE;
                break;
            }
        }
        if (identifier_exists == FALSE)
        {
            applet_manager_deinitialize_handler(man, 
                (HomeAppletHandler *)handler_listitem->data);
        }
    }

    g_strfreev(groups);
    g_key_file_free(keyfile);
}

void applet_manager_foreground_handler(applet_manager_t *man, 
                                       HomeAppletHandler *handler)
{
    if(handler == NULL)
    {
        return;
    }
    g_assert(handler);

    home_applet_handler_foreground(handler);
}

void applet_manager_foreground(applet_manager_t *man,
                               gchar *identifier)
{
    HomeAppletHandler *handler = applet_manager_get_handler(man, identifier);

    if(handler == NULL)
    {
        return;
    }
    g_assert(handler);
    applet_manager_foreground_handler(man, handler);
}

void applet_manager_foreground_all(applet_manager_t *man)
{
    GList *handler;

    for(handler = man->applet_list; handler != NULL; handler = handler->next)
    {
        applet_manager_foreground_handler(man, 
            (HomeAppletHandler *)handler->data);
    }
}

void applet_manager_foreground_configure_all(applet_manager_t *man)
{
    applet_manager_configure_load_all(man);
    applet_manager_foreground_all(man);
}

void applet_manager_state_save_handler(applet_manager_t *man, 
                                       HomeAppletHandler *handler, 
                                       void *state_data, 
                                       int *state_size)
{
    if(handler == NULL)
    {
        return;
    }
    g_assert(handler);

    home_applet_handler_save_state(handler, &state_data, state_size);
}

void applet_manager_state_save(applet_manager_t *man,
                               gchar *identifier,
                               void *state_data,
                               int *state_size)
{
    HomeAppletHandler *handler = applet_manager_get_handler(man, identifier);

    if(handler == NULL)
    {
        return;
    }
    g_assert(handler);
    applet_manager_state_save_handler(man, handler, state_data, state_size);
}

void applet_manager_state_save_all(applet_manager_t *man)
{
    GList *handler;
    void *state_data = NULL; 
    int state_size = 0;

    for(handler = man->applet_list; handler != NULL; handler = handler->next)
    {
        applet_manager_state_save_handler(man, 
            (HomeAppletHandler *)handler->data, state_data, &state_size);
    }
}

void applet_manager_background_handler(applet_manager_t *man, 
                                       HomeAppletHandler *handler)
{
    if(handler == NULL)
    {
        return;
    }
    g_assert(handler);

    home_applet_handler_background(handler);
}

void applet_manager_background(applet_manager_t *man,
                               gchar *identifier)
{
    HomeAppletHandler *handler = applet_manager_get_handler(man, identifier);

    if(handler == NULL)
    {
        return;
    }
    g_assert(handler);

    applet_manager_background_handler(man, handler);
}

void applet_manager_background_all(applet_manager_t *man)
{
    GList *handler;

    for(handler = man->applet_list; handler != NULL; handler = handler->next)
    {
        applet_manager_background_handler(man, 
            (HomeAppletHandler *)handler->data);
    }
}

void applet_manager_background_state_save_all(applet_manager_t *man)
{
    applet_manager_state_save_all(man);
    applet_manager_background_all(man);
}

HomeAppletHandler *applet_manager_get_handler(applet_manager_t *man,
                                              gchar *identifier)
{
    GList *handler_listitem;
    gchar *handler_identifier;

    /* NULL is invalid identifier */
    if (identifier == NULL) 
    {
        ULOG_ERR("Trying to get handler for invalid identifier %s\n", 
                 identifier);
        return NULL;
    }

    for(handler_listitem = man->applet_list; handler_listitem != NULL; 
        handler_listitem = handler_listitem->next)
    {
        handler_identifier = 
            home_applet_handler_get_desktop_filepath(
                (HomeAppletHandler *)handler_listitem->data);
        if(handler_identifier != NULL && identifier != NULL && 
           g_str_equal(identifier, handler_identifier))
        {
            return (HomeAppletHandler *)handler_listitem->data;
        }
    }    
    /* no existing identifier found */
    return NULL;
}

GList *applet_manager_get_handler_all(applet_manager_t *man)
{
    return man->applet_list;
}

GtkEventBox *applet_manager_get_eventbox_handler(applet_manager_t *man, 
                                                 HomeAppletHandler *handler)
{
    if(handler == NULL)
    {
        return NULL;
    }
    g_assert(handler);

    return home_applet_handler_get_eventbox(handler);
}

GtkEventBox *applet_manager_get_eventbox(applet_manager_t *man,
                                         gchar *identifier)
{
    HomeAppletHandler *handler = applet_manager_get_handler(man, identifier);

    if(handler == NULL)
    {
        return NULL;
    }
    g_assert(handler);

    return applet_manager_get_eventbox_handler(man, handler);
}

GtkWidget *applet_manager_get_settings_handler(applet_manager_t *man, 
                                               HomeAppletHandler *handler,
                                               GtkWindow *parent)
{
    if(handler == NULL)
    {
        return NULL;
    }
    g_assert(handler);

    return home_applet_handler_settings(handler, parent);
}

GtkWidget *applet_manager_get_settings(applet_manager_t *man,
                                       gchar *identifier,
                                       GtkWindow *parent)
{
    HomeAppletHandler *handler = applet_manager_get_handler(man, identifier);

    if(handler == NULL)
    {
        return NULL;
    }
    g_assert(handler);

    return applet_manager_get_settings_handler(man, handler, parent);
}

gchar *applet_manager_get_identifier_handler(applet_manager_t *man, 
                                             HomeAppletHandler *handler)
{
    if(handler == NULL)
    {
        return NULL;
    }
    g_assert(handler);

    return home_applet_handler_get_desktop_filepath(handler);
}

GList *applet_manager_get_identifier_all(applet_manager_t *man)
{
    GList *identifier_list = NULL;
    GList *handler_listitem;
    gchar *identifier;

    for(handler_listitem = man->applet_list; handler_listitem != NULL; 
        handler_listitem = handler_listitem->next)
    {
        identifier = 
            applet_manager_get_identifier_handler(man, 
                (HomeAppletHandler *)handler_listitem->data);
        if(identifier != NULL)
        {
            identifier_list = g_list_append(identifier_list, identifier);
        }
    }

    return identifier_list;
}

void applet_manager_set_coordinates_handler(applet_manager_t *man, 
                                            HomeAppletHandler *handler,
                                            gint x, gint y)
{
    if(handler == NULL)
    {
        return;
    }
    g_assert(handler);

    home_applet_handler_set_coordinates(handler, x, y);
}

void applet_manager_set_coordinates(applet_manager_t *man,
                                    gchar *identifier,
                                    gint x, gint y)
{
    HomeAppletHandler *handler = applet_manager_get_handler(man, identifier);

    if(handler == NULL)
    {
        return;
    }
    g_assert(handler);

    applet_manager_set_coordinates_handler(man, handler, x, y);
}

void applet_manager_get_coordinates_handler(applet_manager_t *man, 
                                            HomeAppletHandler *handler,
                                            gint *x, gint *y)
{
    if(handler == NULL)
    {
        return;
    }
    g_assert(handler);

    home_applet_handler_get_coordinates(handler, x, y);
}

void applet_manager_get_coordinates(applet_manager_t *man,
                                    gchar *identifier,
                                    gint *x, gint *y)
{
    HomeAppletHandler *handler = applet_manager_get_handler(man, identifier);

    if(handler == NULL)
    {
        return;
    }
    g_assert(handler);

    applet_manager_get_coordinates_handler(man, handler, x, y);
}

gboolean applet_manager_identifier_exists(applet_manager_t *man, 
                                          gchar *identifier)
{
    GList *handler_listitem;
    gchar *handler_identifier;

    /* NULL is invalid identifier */
    if (identifier == NULL) 
    {
        return FALSE;
    }

    for(handler_listitem = man->applet_list; handler_listitem != NULL; 
        handler_listitem = handler_listitem->next)
    {
        handler_identifier = 
            home_applet_handler_get_desktop_filepath(
                (HomeAppletHandler *)handler_listitem->data);
        if(handler_identifier != NULL && identifier != NULL && 
           g_str_equal(identifier, handler_identifier))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/* --------------- applet manager public end ----------------- */
