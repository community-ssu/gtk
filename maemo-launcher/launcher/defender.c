/*
 * $Id$
 *
 * Copyright (C) 2006 Nokia Corporation
 *
 * Author: Guillem Jover <guillem.jover@nokia.com>
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

#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "report.h"

#define OOM_ADJ_VALUE -17

static void
set_oom_adj(char *pid)
{
  FILE *file;
  char filename[128];

  if (snprintf(filename, sizeof(filename), "/proc/%s/oom_adj", pid) < 0)
    die(20, "generating filename string: '%s'\n", filename);

  file = fopen(filename, "w");
  if (!file)
    die(21, "opening file '%s'\n", filename);

  if (fprintf(file, "%d", OOM_ADJ_VALUE) < 0)
    die(22, "writting oom adjust value to file: '%s'\n", filename);

  fclose(file);
}

static bool
is_same_file(char *filea, char *fileb)
{
  struct stat a, b;

  if (stat(filea, &a) < 0)
    die(30, "stating file a: '%s'\n", filea);
  if (stat(fileb, &b) < 0)
    die(31, "stating file b: '%s'\n", fileb);

  return (a.st_dev == b.st_dev && a.st_ino == b.st_ino);
}

static bool
have_valid_parent(void)
{
  pid_t ppid;
  char filename[128];

  ppid = getppid();

  if (snprintf(filename, sizeof(filename), "/proc/%d/exe", ppid) < 0)
    die(30, "generating filename string: '%s'\n", filename);

  return is_same_file(filename, MAEMO_LAUNCHER);
}

int
main(int argc, char **argv)
{
  char *pid;

  if (argc < 2)
    die(10, "not enough arguments\n");

  if (!have_valid_parent())
    die(11, "my parent is not who he claims to be\n");

  pid = argv[1];

  report_set_output(report_syslog);

  set_oom_adj(pid);

  return 0;
}

