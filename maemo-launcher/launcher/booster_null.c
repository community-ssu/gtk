/*
 * $Id$
 *
 * Copyright (C) 2006 Nokia Corporation
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

#include <stdio.h>

#include "booster.h"
#include "report.h"

static booster_state_t
booster_null_preinit(int *argc, char ***argv)
{
  debug("booster null module preinit (%s)\n", *argv[0]);

  return NULL;
}

static void
booster_null_init(const char *progfilename, const booster_state_t state)
{
  debug("booster null module init (%s)\n", progfilename);
}

static void
booster_null_reload(booster_state_t state)
{
  debug("booster null module reload\n");
}

booster_api_t booster_null_api = {
  .booster_version = MAEMO_LAUNCHER_BOOSTER_API_VERSION,
  .booster_preinit = booster_null_preinit,
  .booster_init = booster_null_init,
  .booster_reload = booster_null_reload,
};

