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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "booster.h"
#include "report.h"

void
booster_module_load(booster_t *booster)
{
  char *booster_path;
  char *booster_sym;
  char *error_s;

  asprintf(&booster_path, MAEMO_BOOSTER_DIR "/maemo-booster-%s.so",
	   booster->name);
  if (!booster_path)
    die(40, "allocating booster module path\n");

  asprintf(&booster_sym, "booster_%s_api", booster->name);
  if (!booster_sym)
    die(40, "allocating booster symbol path\n");

  /* Load the booster module. */
  dlerror();
  booster->so = dlopen(booster_path, RTLD_NOW | RTLD_GLOBAL);
  if (!booster->so)
    die(41, "loading booster module: '%s'\n", dlerror());

  free(booster_path);

  dlerror();
  booster->api = (booster_api_t *)dlsym(booster->so, booster_sym);
  error_s = dlerror();
  if (error_s != NULL)
    die(42, "loading booster symbol '%s': '%s'\n", booster_sym, error_s);

  free(booster_sym);

  if (booster->api->booster_version != MAEMO_LAUNCHER_BOOSTER_API_VERSION)
    die(43, "cannot handle booster module version %d.\n",
        booster->api->booster_version);
}

