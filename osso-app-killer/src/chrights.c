/**
  @file chrights.c

  UID changing functions.
  The program does not require to be run as root -- it is enough if
  it is owned by root and the SUID bit is set. (It is assumed that
  only root can do certain operations the program does.)

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

#include "ke-recv.h"
#include "chrights.h"

static uid_t root_uid = INVALID_UID;
static uid_t user_uid = USER_UID; /* "lesser" UID */

void init_rights(void)
{
        char* v = NULL;
        assert(root_uid == INVALID_UID);
        root_uid = geteuid();
        if (root_uid != ROOT_UID) {
                ULOG_WARN_F("effective user ID is not root");
        }
        /* if the variable is defined, use its value as lesser UID */
        v = getenv(LESSER_UID_ENVIRONMENT_VARIABLE);
        if (v != NULL) {
                uid_t uid = (uid_t) atoi(v);
                user_uid = uid;
                ULOG_INFO_F("using %d as the lesser UID", uid);
        }
}

void more_rights(void)
{
        assert(root_uid != INVALID_UID);
        if (geteuid() == root_uid) {
                ULOG_WARN_F("euid is already %d", root_uid);
        }
        if (seteuid(root_uid) < 0) {
                ULOG_ERR_F("couldn't change to euid %d", root_uid);
                assert(FALSE);
        }
}

void less_rights(void)
{
        assert(root_uid != INVALID_UID);
        if (geteuid() == user_uid) {
                ULOG_WARN_F("euid is already %d", user_uid);
        }
        if (seteuid(user_uid) < 0) {
                ULOG_ERR_F("couldn't change to euid %d", user_uid);
        }
}
