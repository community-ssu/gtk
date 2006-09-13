/*
 * $Id$
 *
 * Copyright (C) 2005, 2006 Nokia Corporation
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

#ifndef LAUNCHER_BOOSTER_H
#define LAUNCHER_BOOSTER_H

typedef void *booster_state_t;

typedef struct {
  int booster_version;
  booster_state_t (*booster_preinit)(int *argc, char **argv[]);
  void (*booster_init)(const char *progfilename, const booster_state_t state);
  void (*booster_reload)(booster_state_t state);
} booster_api_t;

#define MAEMO_LAUNCHER_BOOSTER_API_VERSION 0

typedef struct {
  booster_state_t	state;
  booster_api_t		*api;
  void			*so;
  const char		*name;
} booster_t;

void booster_module_load(booster_t *mod);

#endif

