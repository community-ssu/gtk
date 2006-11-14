/*
 * $Id$
 *
 * Copyright (C) 2005, 2006 Nokia Corporation
 *
 * Authors: Guillem Jover <guillem.jover@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_PRCTL_SET_NAME
#include <sys/prctl.h>
#endif
#include <dlfcn.h>

#include "report.h"
#include "prog.h"

#ifdef DEBUG
extern char **environ;

void
print_prog_env_argv(prog_t *prog)
{
  int i;

  for (i = 0; environ[i] != NULL; i++)
    debug("env[%i]='%s'\n", i, environ[i]);

  for (i = 0; i < prog->argc; i++)
    debug("argv[%i]='%s'\n", i, prog->argv[i]);
}
#else
void
print_prog_env_argv(prog_t *prog)
{
}
#endif

void
load_main(prog_t *prog)
{
  void *module;
  char *error_s;

  /* Load the launched application. */
  module = dlopen(prog->filename, RTLD_LAZY | RTLD_GLOBAL);
  if (!module)
    die(1, "loading invoked application: '%s'\n", dlerror());

  dlerror();
  prog->entry = (entry_t)dlsym(module, "main");
  error_s = dlerror();
  if (error_s != NULL)
    die(1, "loading symbol 'main': '%s'\n", error_s);
}

#ifdef HAVE_PRCTL_SET_NAME
static void
set_process_name(const char *progname)
{
  prctl(PR_SET_NAME, basename(progname));
}
#else
static inline void
set_process_name(const char *progname)
{
}
#endif

void
set_progname(char *progname, int argc, char **argv)
{
  int argvlen = 0;
  int i;

  for (i = 0; i < argc; i++)
    argvlen += strlen(argv[i]) + 1;

  memset(argv[0], 0, argvlen);
  strncpy(argv[0], progname, argvlen - 1);

  set_process_name(progname);

  setenv("_", progname, true);
}

