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
* @file hildon-navigator.h
*/


#ifndef HILDON_NAVIGATOR_H
#define HILDON_NAVIGATOR_H

#include <libintl.h>

/* Button numbers */
enum
{
    APPLICATIONS_MENU_BUTTON =0,
    BOOKMARK_MENU_BUTTON,
    MAIL_MENU_BUTTON
};

/* To tell a correct header */
enum
{
    BOOKMARK_MANAGER_TYPE =0,
    SEND_RECEIVE_TYPE,
    OUTBOX_TYPE
};

#define TEEMADIR "TEEMADIR"
#define HOME_ENV "HOME"
#define HILDON_NAVIGATOR_MENU_NAME "menu_from_navigator"

#define _(String) gettext(String)

enum
{
    WIN_ICON_ITEM =0, /* Normal icon */
    WIN_NAME_ITEM,
    WIN_EXEC_ITEM,
    WIN_FILENAME_ITEM,
    WIN_ARGS_ITEM,
    WIN_TYPE_ITEM,
    WIN_DIMMED_ITEM, /* True if the window should be drawed dimmed */
    WIN_DIALOG_ITEM,
    WIN_TEXT_COLOR_ITEM,
    WIN_ID_ITEM,
    WIN_DIMMED_ICON_ITEM, /* Icon to be used for dimmed windows */
    WIN_APP_ICON_ITEM, 
    WIN_APP_DIMMED_ICON_ITEM,
    WIN_BIN_NAME_ITEM,
    WIN_BOLD_ITEM,
    NUM_TASK_ITEMS
};

enum
{
    WIN_ITEM_TYPE_APP,
    WIN_ITEM_TYPE_WINDOW
};

G_BEGIN_DECLS

/*activates an application.  This does one of two things: 1) If the
 *application isn't running, then the application is started.  2) If the
 *application is running, then it is raised to become the topmost window.
 *@param name - the name of the application
 *@param exec - the fullpath of the executable
 *@param param Any parameters that need to be passed to the application
 */
void hildon_navigator_activate( const char* name, const char *exec, 
                                const char *param );

/** Kill all applications watched by the task navigator **/
void hildon_navigator_killall( void );

G_END_DECLS

#endif /* HILDON_NAVIGATOR_H*/
