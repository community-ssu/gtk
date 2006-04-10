/* exec.h
 * Copyright (C) 2001, 2002, 2005 Free Software Foundation, Inc.
 *
 * This file is part of GnuPG.
 *
 * GnuPG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GnuPG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

#ifndef _EXEC_H_
#define _EXEC_H_

#include <unistd.h>
#include <stdio.h>
#include "iobuf.h"

struct exec_info
{
  int progreturn,binary,writeonly,madedir,use_temp_files,keep_temp_files;
  pid_t child;
  FILE *tochild;
  IOBUF fromchild;
  char *command,*name,*tempdir,*tempfile_in,*tempfile_out;
};

int exec_write(struct exec_info **info,const char *program,
	       const char *args_in,const char *name,int writeonly,int binary);
int exec_read(struct exec_info *info);
int exec_finish(struct exec_info *info);
int set_exec_path(const char *path);

#endif /* !_EXEC_H_ */
