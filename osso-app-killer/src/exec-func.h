/**
  @file exec-func.h

  Script/program executing functionality.

  Copyright (C) 2004-2005 Nokia Corporation.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.
   
  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.
   
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA
*/

#ifndef EXEC_FUNC_H_
#define EXEC_FUNC_H_

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <osso-log.h>

#ifdef __cplusplus
extern "C" {
#endif

int exec_prog(const char* cmd, char* const args[]);

#ifdef __cplusplus
}
#endif
#endif /* EXEC_FUNC_H_ */
