/*
 * This file is part of hildon-base-lib
 *
 * Copyright (C) 2005, 2006 Nokia Corporation.
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

/**
 * @file hildon-base-dir-change.h
 * This file is the private include file for directory monitoring.
 * <p />
 * @brief This is the private header for Directory Monitoring.
 */
#ifndef __HILDON_BASE_DIR_CHANGE_H__
# define  __HILDON_BASE_DIR_CHANGE_H__

/* disable debuging, this works (;
 * --Wolf (2004-01-19)
 */
#ifdef DEBUG
# undef DEBUG
#endif

/* includes */
# define _GNU_SOURCE     /* needed to get the defines */
# include <fcntl.h>
# include <signal.h>
# include <stdio.h>
# include <unistd.h>
# include <malloc.h>
# include <limits.h> 
# include <glib.h>
# include <string.h>
# include "hildon-base-dnotify.h"

/**
 * Our chosen signal
#define HILDON_DNOTIFY_SIG (SIGRTMIN)
 */
#define HILDON_DNOTIFY_SIG (SIGRTMIN+1)

/**
 * Typedef for the linked list of monitored directories.
 */
typedef struct _dir_map_st _dir_map_t;

/**
 * A linked list of monitored directories.
 */
struct _dir_map_st{
	gboolean changed; /**< True when dnotify has detected some change in this directory*/
	int fd; /**< The file descriptor for this directory */
	char *path; /**< the path to this directory */
	hildon_dnotify_cb_f *cb; /**< The callback function to call when a change
							  * has been detected */
        gpointer data; /* Pointer to the user data to be passed to 
                            the callback*/
	_dir_map_t *next; /**< NULL for the last node */
};

/**
 * Handles the signal from dnotify, that will notify of a 
 * change in a directory.
 * It will look at info->si_fd, to find the descriptor for the directory.
 * It will also set the _dir_map_t.changed entries to TRUE, for all entries 
 * with this descriptor.
 * @param sig the signal that was caught by this handler.
 * @param info a structure that contains the payload for this real-time
 * signal.
 * @param data currently ignored.
 */
static void _dir_sig_handler(int sig, siginfo_t *info, void *data);
/**
 * the prepare function used by GMainLoop.
 * @param source The source of this event.
 * @param timeout_ Ignored.
 * @return TRUE when an change in any monitored directory is found. It 
 * will tell g_main_loop that we are ready to dispatch.
 */
static gboolean _dir_prepare(GSource *source, gint *timeout_);
/**
 * the check function used by GMainLoop
 * @param source The source of this event.
 * @return TRUE when an change in any monitored directory is found. It 
 * will tell g_main_loop that we are ready to dispatch.
 */
static gboolean _dir_check(GSource *source);
/**
 * the dispatch function used by GMainLoop. Will call 
 * hildon_dnotify_check_run_callbacks()
 * @param source The source of this event.
 * @param callback Ignored.
 * @param user_data Ignored.
 * @return TRUE
 */
static gboolean _dir_dispatch(GSource *source,	GSourceFunc callback,
					   gpointer user_data);

#endif /*  __HILDON_BASE_DIR_CHANGE_H__ */
