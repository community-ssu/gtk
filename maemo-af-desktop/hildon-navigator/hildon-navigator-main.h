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

/** 
 *
 * @file hildon-navigator-main.h
 *
 */
 
#ifndef HILDON_NAVIGATOR_MAIN_H
#define HILDON_NAVIGATOR_MAIN_H

G_BEGIN_DECLS

/* Plugin names */
#define LIBTN_MAIL_PLUGIN        "libtn_mail_plugin.so"
#define LIBTN_BOOKMARK_PLUGIN    "libtn_bookmark_plugin.so"

/* Max. amount of plugin symbols */
#define MAX_SYMBOLS 4

/* Hardcoded pixel values */
#define HILDON_NAVIGATOR_WIDTH 80
#define BUTTON_HEIGHT 90

/* Type definitions for the plugin API */
typedef void *(*PluginCreateFn)(void);
typedef void (*PluginDestroyFn)(void *data);
typedef GtkWidget *(*PluginGetButtonFn)(void *data);
typedef void (*PluginInitializeMenuFn)(void *data);

G_END_DECLS


typedef struct
{
    GtkWidget *main_window;
    
    GtkWidget *bookmark_button;
    GtkWidget *mail_button;
    GtkWidget *app_switcher_button;
    GtkWidget *others_menu_button;
    
    void *mail_data;
    void *bookmark_data;
    void *dlhandle_bookmark;
    void *dlhandle_mail;
    
    PluginCreateFn create;
    PluginDestroyFn destroy;
    PluginGetButtonFn navigator_button;
    PluginInitializeMenuFn initialize_menu;

    ApplicationSwitcher_t *app_switcher;
    OthersMenu_t *others_menu;

    void *app_switcher_dnotify_cb;

} Navigator;

/*Symbols */
enum {
   CREATE_SYMBOL = 0,
   DESTROY_SYMBOL,
   GET_BUTTON_SYMBOL,
   INITIALIZE_MENU_SYMBOL
};

/* Configuration options
 */

/* When CONFIG_DO_BGKILL is TRUE, applications are killed when they
   are in the background and we are in the 'bgkill' situation.
*/
extern gboolean config_do_bgkill;

/* When CONFIG_LOWMEM_DIM is TRUE, all buttons that can start new
   applications are dimmed when we are in the 'lowmem' situation.
*/
extern gboolean config_lowmem_dim;

/* When CONFIG_LOWMEM_NOTIFY_ENTER is TRUE, notify this information to the
 * user upon entering the 'lowmem' situation.
 */
extern gboolean config_lowmem_notify_enter;

/* When CONFIG_LOWMEM_NOTIFY_LEAVE is TRUE, notify this information to the
 * user upon leaving the 'lowmem' situation.
 */
extern gboolean config_lowmem_notify_leave;

/* When CONFIG_LOWMEM_PAVLOV_DIALOG is TRUE, display a dialog instead of
   an infoprint upon entering the 'lowmem' situation.
*/
extern gboolean config_lowmem_pavlov_dialog;

#endif /* HILDON_NAVIGATOR_MAIN_H */

 
