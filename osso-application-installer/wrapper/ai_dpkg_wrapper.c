/* The dpkg wrapper for AI */

/*
 * This file is part of osso-application-installer
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>
#include <config.h>

#include <glib.h>
#include <glib/gspawn.h>
#include <glib/gstring.h>

#define APP_NAME "/usr/bin/aiwrapper"
#define INSTALL_USER "install"

extern char **environ;

int main(int argc, char **argv)
{
  gint i = 0;
  gint ret = 0;
  uid_t ruid = getuid();
  uid_t euid = geteuid();
  struct passwd *pentry;
  GString *cmdline = g_string_new("");

  /* Where we started */
  //fprintf(stderr, "Wrapper process initial RUID/EUID: %i/%i\n", ruid, euid);
  
  /* Dropping the wrapper from front */
  /*
  i = 0;
  while (argv[i+1] != NULL) {
    argv[i] = argv[i+1];
    i++;
  }
  argv[i] = (char *)NULL;
  */

  i = 1;
  while (argv[i] != NULL) {
    //fprintf(stderr, "argv[%d]: %s\n", i, argv[i]);
    if (i > 1) cmdline = g_string_append(cmdline, " ");
    cmdline = g_string_append(cmdline, argv[i++]);
  }
  //fprintf(stderr, "Full cmdline: '%s'\n", cmdline->str);


  /* use getpwent() here..*/
  pentry = getpwnam(INSTALL_USER);
  euid = ruid = (uid_t)pentry->pw_uid;
   
  fprintf(stderr, "Trying to set proper ids::ruid:value: %i\n", ruid);
  if((ret = setreuid(ruid, euid)) == -1) {
    fprintf(stderr,"**Failed to set euid to install (ret=%d)\n",
	    ret); 
    exit(1);
  }

  fprintf(stderr, "Success (with ret=%d) on RUID for install::value: %i\n", 
	  ret, ruid);
  gint retval = -1000;
  gchar *cmd_stdout, *cmd_stderr;

  //fprintf (stderr, "Trying to launch dpkg... \n");
  g_spawn_command_line_sync(cmdline->str, &cmd_stdout, &cmd_stderr, &retval, NULL);
  fprintf(stderr, "wrapper complete, retval %d.\n", retval);
 
  //gint p_out[2], p_err[2];
  //pipe(p_out);
  //close(1);
  //dup(p_out[1]);
  fprintf(stdout, cmd_stdout);
  //close(p_out[0]);
  //close(p_out[1]);
  
  //pipe(p_err);
  //close(2);
  //dup(p_err[1]);
  fprintf(stderr, cmd_stderr);
  //close(p_err[0]);
  //close(p_err[1]);

  return 0;
}
