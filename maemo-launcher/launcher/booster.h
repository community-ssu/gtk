/*
 * $Id$
 *
 * Copyright (C) 2005, 2006, 2007 Nokia Corporation
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

#ifndef BOOSTER_H
#define BOOSTER_H

#include "booster_api.h"

typedef struct booster {
  booster_state_t	state;
  booster_api_t		*api;
  void			*so;
  const char		*name;
  struct booster	*next;
} booster_t;

void boosters_alloc(booster_t **booster, const char *arg);
void boosters_load(booster_t *booster, int *argc, char **argv[]);
void boosters_init(booster_t *booster, char *argv0);
void boosters_reload(booster_t *booster);

#endif

