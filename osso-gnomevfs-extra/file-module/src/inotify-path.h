/*
	Copyright (C) 2006 John McCutchan <john@johnmccutchan.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License version 2 for more details.
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software Foundation,
	Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef __INOTIFY_PATH_H
#define __INOTIFY_PATH_H

#include "inotify-kernel.h"
#include "inotify-sub.h"

gboolean ip_startup (void (*event_cb)(ik_event_t *event, ih_sub_t *sub));
gboolean ip_start_watching (ih_sub_t *sub);
gboolean ip_stop_watching  (ih_sub_t *sub);

#endif
