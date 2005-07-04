/*
 * This file is part of hildon-base-lib
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Luc Pionchon <luc.pionchon@nokia.com>
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
 * @file hildon-base-types.h
 * This file ncludes misc. type definitions. \n
 * <b>DO NOT include directly, it is simply used by the other components</b>
 * <p />
 * @brief This header includes all type definitions used by hildon-base
 */
#ifndef __HILDON_BASE_TYPES_H__
# define  __HILDON_BASE_TYPES_H__

#include <glib.h>
G_BEGIN_DECLS

/**
 * The return type of most functions.
 */
typedef enum 
{	HILDON_OK, /**<Denotes a successful run of the function */
	HILDON_ERR, /**<Some kind of an error occurred within the function */
	HILDON_ERR_NOMEM /**< Out of memory */
}hildon_return_t;

typedef enum
{
	HILDON_FALSE = 0,
	HILDON_TRUE = 1
}hildon_boolean;


/**
 * Function type definition for the callback that will be called
 * This function will be called every time that the library detects a 
 change in the monitored directory.
 * @param path Path to the directory where the change occurred. 
 * @param data user data 
*/
typedef void (hildon_dnotify_cb_f)(char * path,gpointer data);

G_END_DECLS

#endif /* __HILDON_BASE_TYPES_H__ */
