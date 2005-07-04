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
 * @file hildon-base-dir-change.c
 * This is the implementation for Directory Monitoring.
 * @brief This is the implementation for Directory Monitoring.
 */

#include "hildon-base-dir-change.h"

/**
 * A linked list of the directories that are monitored
 */
static _dir_map_t *_dir_map=NULL;

/**
 * A GSource that handles the dnotify flag polling
 */
static GSource *dir_src=NULL;
static GSourceFuncs *dir_funcs=NULL;
static struct sigaction *oldact=NULL;

static void _dir_sig_handler(int sig, siginfo_t *info, void *data)
{
	_dir_map_t *p = _dir_map;

	while(p!=NULL) { /* traverse the list */
		if (info == NULL) {
			p->changed = TRUE; /* mark it as changed */
		}
		else {
			if(p->fd == info->si_fd) { /* search for this directory */
				p->changed = TRUE; /* mark it as changed */
			}
		}
		p=p->next;
	}
	return;
}

static gboolean _dir_prepare(GSource *source, gint *timeout)
{
	_dir_map_t *p = _dir_map;
	while(p!=NULL) {
		if(p->changed == TRUE) 
			return TRUE; /* there is some change in some directory */
		p=p->next;
	}	
	return FALSE; /* there is no change in any monitored directory */
}

static gboolean _dir_check(GSource *source)
{
	_dir_map_t *p = _dir_map;
	while(p!=NULL) {
		if(p->changed == TRUE) 
			return TRUE; /* there is some change in some directory */
		p=p->next;
	}	
	return FALSE; /* there is no change in any monitored directory */
}
	
static gboolean _dir_dispatch(GSource *source,	GSourceFunc callback,
					   gpointer user_data)
{
	if(hildon_dnotify_check_run_callbacks()==HILDON_OK)
		return TRUE;
	else
		return FALSE;
}

static void _dir_dummy1(void)
{
}

static gboolean _dir_dummy2(gpointer data)
{
	return TRUE;
}

hildon_return_t hildon_dnotify_handler_init(void)
{
	guint gsource_handle;
	struct sigaction act;

	if( (dir_funcs != NULL) || (dir_src != NULL) )
		return HILDON_ERR;
	
	dir_funcs = (GSourceFuncs *)g_malloc(sizeof(GSourceFuncs));

	_dir_map = NULL;

	dir_funcs->prepare = _dir_prepare;
	dir_funcs->check = _dir_check;
	dir_funcs->dispatch = _dir_dispatch;
	dir_funcs->finalize = NULL;
	dir_funcs->closure_callback = _dir_dummy2;
	dir_funcs->closure_marshal = _dir_dummy1;

	dir_src = g_source_new(dir_funcs, sizeof(GSource));
	gsource_handle = g_source_attach(dir_src, NULL);

	act.sa_sigaction = _dir_sig_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_RESTART | SA_NODEFER | SA_SIGINFO;
	sigaction(HILDON_DNOTIFY_SIG, &act, oldact);

	return HILDON_OK;
}

/*
	 * The type of change to monitor.\n
	 * The value may be any of these:\n
	 * File modified: DN_MODIFY  =    0x00000002\n
	 * File created:  DN_CREATE  =    0x00000004\n
	 * File removed:  DN_DELETE  =    0x00000008\n
	 * File renamed:  DN_RENAME  =    0x00000010\n
	 * \n
	 * These values are possible, but they will not be caught.\n
	 * File accessed: DN_ACCESS = 0x00000001\n
	 * File changed attributes: DN_ATTRIB  =    0x00000020
*/
hildon_return_t hildon_dnotify_set_cb(hildon_dnotify_cb_f *func,
        char * path, gpointer data)
{
	_dir_map_t *dir;
	if( (func==NULL) || (path == NULL) || 
		(dir_funcs == NULL) || (dir_src == NULL) )
		return HILDON_ERR;
	dir = g_malloc0(sizeof(_dir_map_t));
	if(dir == NULL)
		return HILDON_ERR_NOMEM;

	
	dir->cb = func;
        dir->data = data;
	dir->path = strdup(path);
	dir->changed = FALSE;
	dir->fd = open(path, O_RDONLY);
	if(dir->fd<0) {
		free(dir->path);
		free(dir);
		return HILDON_ERR;
	}
	dir->next = _dir_map;
	_dir_map = dir;		
	if(fcntl(dir->fd, F_SETSIG, HILDON_DNOTIFY_SIG)<0) {
		_dir_map = dir->next;
		free(dir->path);
		free(dir);
		return HILDON_ERR;
	}	
	if(fcntl(dir->fd, F_NOTIFY, DN_MODIFY|DN_CREATE|DN_DELETE|
			 DN_RENAME|DN_MULTISHOT)<0) {
		_dir_map = dir->next;
		free(dir->path);
		free(dir);
		return HILDON_ERR;
	}
	
	return HILDON_OK;
}

hildon_return_t hildon_dnotify_remove_cb(char * path)
{
	_dir_map_t *dir, *prev;
	if( (path == NULL) || (dir_funcs == NULL) || (dir_src == NULL) )
		return HILDON_ERR;
	dir = _dir_map;
	prev = NULL;
	while(dir!=NULL) {
		if(strcmp(path,dir->path)==0) {
			_dir_map_t *tmp;
			tmp=dir;
			dir=dir->next;
			if(prev == NULL) {
				_dir_map = _dir_map->next;
			}
			else {
				prev->next=tmp->next;
			}
			free(tmp->path);
			close(tmp->fd);
			free(tmp);
		}
		else {
			prev = dir;
			dir=dir->next;
		}
	}
	
	return HILDON_OK;
}

hildon_return_t hildon_dnotify_handler_clear(void)
{
	_dir_map_t *dir;

	if( (dir_funcs == NULL) || (dir_src == NULL) )
		return HILDON_ERR;
	dir = _dir_map;
	while(dir!=NULL) {
		_dir_map_t *tmp;
		tmp=dir;
		dir=dir->next;
		free(tmp->path);
		close(tmp->fd);
		free(tmp);
	}
	g_source_destroy(dir_src);
	if(dir_funcs != NULL)
		free(dir_src);
	if(dir_src != NULL)
		free(dir_funcs);

	sigaction(HILDON_DNOTIFY_SIG, oldact, NULL);
	
	_dir_map = NULL;
	dir_src = NULL;
	dir_funcs = NULL;
	return HILDON_OK;
}

static void freelist(_dir_map_t *p);

hildon_return_t hildon_dnotify_check_run_callbacks(void)
{
	_dir_map_t *p = _dir_map;
        _dir_map_t callbacks_to_run,*iter;
	if( (dir_funcs == NULL) || (dir_src == NULL) )
		return HILDON_ERR;
        
        callbacks_to_run.next = NULL;
        iter = &callbacks_to_run;
        
        
        
        /* Callect callbacks to run to a list
         * and then run them. This avoids
         * race conditions with callbacks
         * that modifies the callback list. */
	while(p!=NULL) {
		if(p->changed == TRUE) { 
			p->changed = FALSE;
	/*		(p->cb)(p->path,p->data);
          */              
                        iter->next = (_dir_map_t *) g_malloc(sizeof(_dir_map_t));
                        
                        if (iter->next == NULL)
                            return HILDON_ERR_NOMEM;
                        
                        memset(iter->next,0,sizeof(_dir_map_t));

                        iter=iter->next;
                        iter->cb =p->cb;

                        iter->path=p->path;
                        iter->data=p->data;
                        
		}	
		p=p->next;
	}

        iter = callbacks_to_run.next;

        while (iter)
        {
            (iter->cb)(iter->path,iter->data);
            iter = iter->next;
        }


        freelist(callbacks_to_run.next);
        
	return HILDON_OK;
}

static void freelist(_dir_map_t *p)
{
    if (p)
        freelist(p->next);
    free(p);
}
