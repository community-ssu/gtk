/**
  @file runas.c

  Run program as another user.

  Copyright (C) 2004-2005 Nokia Corporation. All rights reserved.
  
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  version 2 as published by the Free Software Foundation.
  
  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.
   
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

#include "runas.h"

static uid_t user_uid = INVALID_UID; /* "lesser" UID */

extern char **environ;

static void less_rights(void)
{
        struct passwd *pwd = NULL;
        int ret = -1;
        char *user_name = NULL;

        pwd = getpwuid(user_uid);
        if (pwd == NULL) {
            ULOG_CRIT_F("getpwuid() failed");
            exit(1);
        }
        user_name = pwd->pw_name;
        assert(user_name != NULL);
        /* init groups */
        ret = initgroups(user_name, pwd->pw_gid);
        if (ret < 0) {
            ULOG_CRIT_F("initgroups() failed: %s", strerror(errno));
            exit(1);
        }

        if (geteuid() == user_uid) {
                ULOG_WARN_F("euid is already %d", user_uid);
        }
        if (setuid(user_uid) < 0) {
                ULOG_CRIT_F("couldn't change to euid %d", user_uid);
                exit(1);
        }
}

/**
 * Execute a command with arguments.
 *
 * @param cmd command to execute
 * @param args NULL-terminated array of arguments
 * @return PID of the child or -1 on fork() error.
*/
static int exec_prog(const char* cmd, char* const args[])
{
        pid_t pid = -1;
        pid = fork();
        if (pid < 0) {
                ULOG_ERR_F("fork() failed");
                return -1;
        } else if (pid == 0) {
                ULOG_DEBUG_F("before execve()");
                execve(cmd, args, environ);
                ULOG_ERR_F("execve() returned error: %s",
                                strerror(errno));
                exit(-1);
        }
        assert(pid > 0);
        return pid;
}


int main(int argc, char *argv[])
{
        int ret = -1;
        if (argc < 3 || !isdigit(argv[1][0])) {
                printf("Usage: %s <UID> <program> [arg1...argN]\n",
                                argv[0]);
                ULOG_CRIT("started with invalid arguments");
                exit(1);
        }
        user_uid = atoi(argv[1]);
        less_rights();
        ret = exec_prog(argv[2], &argv[2]);
        printf("%d\n", ret);
        if (ret != -1) {
                exit(0);
        } else {
                ULOG_CRIT("fork() failed");
                exit(1);
        }
}

