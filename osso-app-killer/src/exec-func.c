/**
  @file exec-func.c

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

#include "exec-func.h"

extern char **environ;

/**
 * Execute a command with arguments.
 * @param cmd command to execute
 * @param args NULL-terminated array of arguments
 * @return return code of the command or -1 on fork(), -2 on
 * exec() error, -3 if the child terminated abnormally, and
 * -4 if waitpid() failed.
 * */
int exec_prog(const char* cmd, char* const args[])
{
        pid_t pid = -1, waitrc = -1;
        int status;

        pid = fork();
        if (pid < 0) {
                ULOG_ERR_F("fork() failed");
                return -1;
        } else if (pid == 0) {
                int ret = -1;
		ULOG_DEBUG_F("before execve()");
                ret = execve(cmd, args, environ);
                ULOG_ERR_F("execve() returned error: %s", 
                         strerror(errno));
                exit(-2);
        }
        assert(pid > 0);

waitpid_again:
	ULOG_DEBUG_F("before waitpid");
        waitrc = waitpid(pid, &status, 0);
	ULOG_DEBUG_F("after waitpid");
        if (waitrc == -1) {
                if (errno == EINTR) {
                        /* interrupted by a signal */
                        goto waitpid_again;
                }
                ULOG_ERR_F("waitpid() returned error: %s",
                           strerror(errno));
                return -4;
        }
        assert(waitrc == pid);
        if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
        }
        ULOG_ERR_F("child terminated abnormally");
        return -3;
}
