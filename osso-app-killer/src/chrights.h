/**
  @file chrights.h

  UID changing functions.
 
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

#ifndef CHRIGHTS_H_
#define CHRIGHTS_H_

#include <unistd.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* this is for warnings only */
#define ROOT_UID 0
/* this UID is used as the "lesser" uid by default */
#define USER_UID 29999
#define INVALID_UID -1

#define LESSER_UID_ENVIRONMENT_VARIABLE "KE_RECV_LESSER_UID"

/** save "root" UID
 * */
void init_rights(void);

/** Switch to the original effective user ID (which should be root)
 *  * */ 
void more_rights(void);

/** Switch to the real user ID (which should be root)
 *  *  * */
void less_rights(void);

#ifdef __cplusplus
}
#endif
#endif /* CHRIGHTS_H_ */
