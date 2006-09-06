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

/**
* @file hildon-log.h
* @brief
* 
*/

#ifndef HILDON_LOG_H
#define HILDON_LOG_H

#include <glib-object.h>

G_BEGIN_DECLS

#define HILDON_LOG_TYPE ( hildon_log_get_type() )
#define HILDON_LOG(obj) (G_TYPE_CHECK_INSTANCE_CAST (obj, \
            HILDON_LOG_TYPE, \
            HildonLog))
#define HILDON_LOG_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
            HILDON_LOG_TYPE, HildonLogClass))
#define HILDON_IS_LOG(obj) (G_TYPE_CHECK_INSTANCE_TYPE (obj, \
            HILDON_LOG_TYPE))
#define HILDON_IS_LOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
            HILDON_LOG_TYPE))
#define HILDON_LOG_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
        HILDON_LOG_TYPE, HildonLogPrivate));
   
typedef struct _HildonLog HildonLog; 
typedef struct _HildonLogClass HildonLogClass;

struct _HildonLog
{
    GObject parent;
};

struct _HildonLogClass
{
    GObjectClass parent_class;

    void   (*remove_file)           (HildonLog *log);
    void   (*add_group)             (HildonLog *log,
				     const gchar *group);
    void   (*add_message)           (HildonLog *log, 
				     const gchar *key, 
				     const gchar *message);

    GList* (*get_incomplete_groups) (HildonLog *log, ...);
};

GType hildon_log_get_type (void);

HildonLog *hildon_log_new (const gchar *filename);

void   hildon_log_remove_file (HildonLog *log);
void   hildon_log_add_group (HildonLog *log, const gchar *group);
void   hildon_log_add_message (HildonLog *log, const gchar *key, const gchar *message);
GList *hildon_log_get_incomplete_groups (HildonLog *log, ...);

G_END_DECLS

#endif /* HILDON_LOG_H */
