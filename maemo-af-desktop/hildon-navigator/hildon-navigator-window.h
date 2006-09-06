/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Author:  Moises Martinez <moises.martinez@nokia.com>
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
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

#ifndef HILDON_NAVIGATOR_WINDOW_H
#define HILDON_NAVIGATOR_WINDOW_H

#include <gtk/gtkwindow.h>


G_BEGIN_DECLS

#define CFG_FNAME                       "plugins.conf"
#define NAVIGATOR_FACTORY_PLUGINS       "/etc/hildon-navigator/" CFG_FNAME
#define NAVIGATOR_USER_DIR              "/.osso/hildon-navigator/"
#define NAVIGATOR_USER_PLUGINS          NAVIGATOR_USER_DIR CFG_FNAME

#define HILDON_NAVIGATOR_WINDOW_TYPE ( hildon_navigator_window_get_type() )
#define HILDON_NAVIGATOR_WINDOW(obj) (GTK_CHECK_CAST (obj, \
            HILDON_NAVIGATOR_WINDOW_TYPE, \
            HildonNavigatorWindow))
#define HILDON_NAVIGATOR_WINDOW_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), \
            HILDON_NAVIGATOR_WINDOW_TYPE, HildonNavigatorWindowClass))
#define HILDON_IS_NAVIGATOR_WINDOW(obj) (GTK_CHECK_TYPE (obj, \
            HILDON_NAVIGATOR_WINDOW_TYPE))
#define HILDON_IS_NAVIGATOR_WINDOW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), \
            HILDON_NAVIGATOR_WINDOW_TYPE))
#define HILDON_NAVIGATOR_WINDOW_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
        HILDON_NAVIGATOR_WINDOW_TYPE, HildonNavigatorWindowPrivate));


typedef struct _HildonNavigatorWindow HildonNavigatorWindow;
typedef struct _HildonNavigatorWindowClass HildonNavigatorWindowClass;

struct _HildonNavigatorWindow
{
    GtkWindow parent;

};

struct _HildonNavigatorWindowClass
{
    GtkWindowClass parent_class;

    void (*set_sensitive) (HildonNavigatorWindow *window,
			   gboolean sensitive);
    
    void (*set_focus)     (HildonNavigatorWindow *window,
			   gboolean focus);

    GtkWidget *(*get_others_menu) (HildonNavigatorWindow *window);
};

GType hildon_navigator_window_get_type (void);

HildonNavigatorWindow *hildon_navigator_window_new (void);

void hn_window_set_sensitive (HildonNavigatorWindow *window, 
                              gboolean sensitive);

void hn_window_set_focus (HildonNavigatorWindow *window, 
                          gboolean focus);

GtkWidget *hn_window_get_others_menu (HildonNavigatorWindow *window);

GtkWidget *hn_window_get_button_focus (HildonNavigatorWindow *window,
				       gint);

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

G_END_DECLS

#endif
