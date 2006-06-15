/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* gnome-vfs-job-queue.h - Job queue for asynchronous operation of the GNOME
   Virtual File System (version for POSIX threads).

   Copyright (C) 2001 Free Software Foundation

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: László Péter <laca@ireland.sun.com>
*/

#ifndef GNOME_VFS_JOB_QUEUE_H
#define GNOME_VFS_JOB_QUEUE_H

#include "gnome-vfs-job.h"

void          _gnome_vfs_job_queue_init       (void);
void          _gnome_vfs_job_queue_shutdown   (void);
gboolean      _gnome_vfs_job_schedule         (GnomeVFSJob       *job);
void          _gnome_vfs_job_queue_run        (void);

#endif /* GNOME_VFS_JOB_QUEUE_H */
