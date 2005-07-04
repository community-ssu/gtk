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
 * @file hildon-base-dnotify.h
 * This file contains the API for directory monitoring. 
 * <p />
 * @brief This document is the API for Directory Monitoring.
*/

#ifndef __HILDON_BASE_DNOTIFY_H__
# define __HILDON_BASE_DNOTIFY_H__

/* includes */
# include "hildon-base-types.h"

G_BEGIN_DECLS
	
	/**
	 * Initializes the dir change handler.
	 * @return HILDON_OK on success, HILDON_ERR on error.
	 * 
	 * This function will set the library up. It will create a new GEvent
	 * and attach it to the default context. The GEvent handle will 
	 * monitor a special linked list, that will contain an entry for all
	 * the monitored directories. In this list a boolean variable will be
	 * set to TRUE by the dnotify event handler (which is also set up by
	 * this function).
	 * NOTE! This function should be called before any calls to
	 * to any other function in this library are made!
	 * Calling it more then once will result in an error.
	 * 
	 * This function will register a new GEvent with the g_main_loop of
	 * the default context. It will also set a signal handler for the 
	 * real time signal SIGRTMIN.
	 */	
	hildon_return_t hildon_dnotify_handler_init(void);


	/**
	 * Function for setting the callback for a given directory changes
	 * @param func        The callback function. The library can not 
	 check if this pointer is really a pointer to a function, so the
	 programmer should make sure it is. (NULL is checked for)
	 It will be called every time that one of these events occur:\n
	 * File modified\n
	 * File created\n
	 * File removed\n 
	 * File renamed\n 
	 * @param path        The path name of the directory
         * @param data        Pointer to user data to be passed to the
         *                      callback
	 * @return            HILDON_OK on success, HILDON_ERR on error.

	 The function uses the F_NOTIFY fcntl (man fcntl) causes a signal
	 to be activated when the directory changes. The signal handler
	 sets a boolean flag to TRUE for the file descriptor that that 
	 corresponds to the monitored directory. The GEvent handler will 
	 poll these flags at every g_main_loop iteration (in the default 
	 context). The g_main_loop GEvent dispatch function will be called,
	 and it will call the callbacks. This way the signal handler remains
	 small and executes quickly. (because there are restrictions on what
	 is safe to call from signal handlers).
	 * This function can be called multiple times to monitor multiple 
	 directories, or the same directory with a different callback function.
	 */
	hildon_return_t hildon_dnotify_set_cb(hildon_dnotify_cb_f *func, 
											 char * path, gpointer data);

	/**
	 * Removes dnotify for all callbacks on the directory path.
	 * @param path The path to remove notification from
	 * @return HILDON_OK.
	 */	
	hildon_return_t hildon_dnotify_remove_cb(char * path);
	
	/**
	 * Removes the timeout and clears the dir_change handlers
	 * @return HILDON_OK on success, HILDON_ERR on error.
	 * After calling this function, the library will be reset,
	 and needs to be initialized again.
	 * 
	 * This function destroys the GEvent created by hildon_dnotify_init(), 
	 * and will reset the signal handler of SIGRTMIN to what it was.
	 */
	hildon_return_t hildon_dnotify_handler_clear(void);
	
	/**
	 * This may be called from programs that do not use the glib main
	 loop. It will call the dnotify callbacks if needed.
	 * @return HILDON_OK on success, HILDON_ERR on error.
	 */
	hildon_return_t hildon_dnotify_check_run_callbacks(void);

        

G_END_DECLS

#endif /* __HILDON_BASE_DNOTIFY_H__ */

