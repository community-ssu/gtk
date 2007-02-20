/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
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

#ifndef __STATUSBAR_ITEM_WRAPPER_H__
#define __STATUSBAR_ITEM_WRAPPER_H__

#include <libhildondesktop/statusbar-item.h>

G_BEGIN_DECLS

#define STATUSBAR_TYPE_ITEM_WRAPPER ( statusbar_item_wrapper_get_type() )
#define STATUSBAR_ITEM_WRAPPER(obj) (GTK_CHECK_CAST (obj, STATUSBAR_TYPE_ITEM_WRAPPER, StatusbarItemWrapper))
#define STATUSBAR_ITEM_WRAPPER_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), STATUSBAR_TYPE_ITEM_WRAPPER, StatusbarItemWrapperClass))
#define STATUSBAR_IS_ITEM_WRAPPER(obj) (GTK_CHECK_TYPE (obj, STATUSBAR_TYPE_ITEM_WRAPPER))
#define STATUSBAR_IS_ITEM_WRAPPER_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), STATUSBAR_TYPE_ITEM_WRAPPER))
#define STATUSBAR_ITEM_WRAPPER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), STATUSBAR_TYPE_ITEM_WRAPPER, StatusbarItemWrapperPrivate));

typedef struct _StatusbarItemWrapper StatusbarItemWrapper; 
typedef struct _StatusbarItemWrapperClass StatusbarItemWrapperClass;
typedef struct _StatusbarItemWrapperAPI_st StatusbarItemWrapperAPI_st; 

struct _StatusbarItemWrapper
{
    StatusbarItem parent;
};

struct _StatusbarItemWrapperClass
{
    StatusbarItemClass parent_class;
    
    void (*statusbar_log_start) (StatusbarItemWrapper * self);
    void (*statusbar_log_end)   (StatusbarItemWrapper * self);
};

/* Type definitions for the plugin API */ 
typedef void (*StatusbarItemWrapperEntryFn)( StatusbarItemWrapperAPI_st *fn );
typedef void *(*StatusbarItemWrapperInitializeFn)( StatusbarItemWrapper *item,
                                                  GtkWidget **button);
typedef void (*StatusbarItemWrapperDestroyFn)( void *data );
typedef void (*StatusbarItemWrapperUpdateFn)( void *data, 
		                             gint value1, 
                                             gint value2, 
					     const gchar *str);
typedef int (*StatusbarItemWrapperGetPriorityFn)( void *data );
typedef void (*StatusbarItemWrapperSetConditionalFn)( void *data, 
		                                     gboolean cond );


/* Struct for the plugin API function pointers */
struct _StatusbarItemWrapperAPI_st 
{
    StatusbarItemWrapperInitializeFn     initialize;
    StatusbarItemWrapperDestroyFn        destroy;
    StatusbarItemWrapperUpdateFn         update;
    StatusbarItemWrapperGetPriorityFn    get_priority;
    StatusbarItemWrapperSetConditionalFn set_conditional;
};

GType statusbar_item_wrapper_get_type (void);

StatusbarItemWrapper *statusbar_item_wrapper_new (const gchar *name, 
						  const gchar *library, 
						  gboolean mandatory);

void statusbar_item_wrapper_update (StatusbarItemWrapper *item, 
                                    gint value1, 
                                    gint value2, 
                                    const gchar *str);

void statusbar_item_wrapper_update_conditional_cb (StatusbarItem *sbitem,
		                                   gboolean user_data );


G_END_DECLS

#endif/*__STATUSBAR_ITEM_WRAPPER_H__*/
