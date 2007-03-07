/*
 * $Id$
 *
 * Copyright (C) 2006, 2007 Nokia Corporation
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
#include <string.h>
#include <dlfcn.h>

#include "booster.h"
#include "report.h"

static booster_t *
booster_new(const char *name)
{
  booster_t *booster;

  booster = malloc(sizeof(*booster));
  if (!booster)
    die(50, "allocating booster\n");

  booster->next = NULL;
  booster->name = name;

  return booster;
}

static void
booster_module_load(booster_t *booster)
{
  char *booster_path;
  char *booster_sym;
  char *error_s;

  if (asprintf(&booster_path, BOOSTER_DIR "/booster-%s.so", booster->name) < 0)
    die(40, "allocating booster module path\n");

  if (asprintf(&booster_sym, "booster_%s_api", booster->name) < 0)
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

  if (booster->api->booster_version != BOOSTER_API_VERSION)
    die(43, "cannot handle booster module version %d.\n",
        booster->api->booster_version);
}

void
boosters_alloc(booster_t **booster, const char *arg)
{
  char *name, *name_p;

  /* Optimization: allocate once and share among all boosters, in case we want
   * to free the boosters we'll have free only 'name' from the first entry. */
  name = name_p = strdup(arg);
  if (!name_p)
    die(60, "allocating booster names\n");

  for (name = name_p; *name_p; name_p++)
  {
    if (name == name_p)
    {
      *booster = booster_new(name);
      booster = &((*booster)->next);
    }
    else if (*name_p == ',')
    {
      *name_p = '\0';
      name = name_p + 1;
    }
  }
}

void
boosters_load(booster_t *booster, int *argc, char **argv[])
{
  while (booster)
  {
    booster_module_load(booster);
    booster->state = booster->api->booster_preinit(argc, argv);
    booster = booster->next;
  }
}

void
boosters_init(booster_t *booster, char *argv0)
{
  while (booster)
  {
    booster->api->booster_init(argv0, booster->state);
    booster = booster->next;
  }
}

void
boosters_reload(booster_t *booster)
{
  while (booster)
  {
    booster->api->booster_reload(booster->state);
    booster = booster->next;
  }
}

