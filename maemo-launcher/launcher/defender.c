/*
 * $Id$
 *
 * Copyright (C) 2006 Nokia
 *
 * Author: Guillem Jover <guillem.jover@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

int
main(int argc, char **argv)
{
  char *pid;

  if (argc < 2)
    die(10, "not enough arguments\n");

  pid = argv[1];

  report_set_output(report_syslog);

  set_oom_adj(pid);

  return 0;
}

