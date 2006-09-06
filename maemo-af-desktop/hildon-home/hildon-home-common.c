/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

/**
 * @file hildon-home-common.c
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif


#include "hildon-home-common.h"
#include "hildon-home-private.h"

G_LOCK_DEFINE_STATIC (hildon_home_globals);

static gchar *hildon_home_user_home = NULL;
static gchar *hildon_home_user_conf = NULL;
static gchar *hildon_home_user_file = NULL;
static gchar *hildon_home_system_conf = NULL;

static gchar *hildon_home_user_bg_file = NULL;

/* initialize the globals.
 * HOLDS: hildon_home_globals
 */
static void
hildon_home_globals_init_do (void)
{
  if (!hildon_home_user_home)
    hildon_home_user_home = g_strdup (g_getenv (HILDON_HOME_ENV_HOME));
  
  if (!hildon_home_user_conf)
    hildon_home_user_conf = g_build_filename (hildon_home_user_home,
                                              HILDON_HOME_USER_SYSTEM_DIR,
                                              HILDON_HOME_CONF_DIR,
                                              NULL);
  if (!hildon_home_system_conf)
    hildon_home_system_conf = g_build_filename (HILDON_HOME_FACTORY_DIR,
                                                HILDON_HOME_CONF_DIR,
                                                NULL);

  g_free (hildon_home_user_file);
  hildon_home_user_file = g_build_filename (hildon_home_user_conf,
		  			    HILDON_HOME_CONF_FILE,
					    NULL);

  g_free (hildon_home_user_bg_file);
  hildon_home_user_bg_file = g_build_filename (DATADIR,
		                               "backgrounds",
		  			       "bg_img_01.png",
					       NULL);
}

static inline void
hildon_home_globals_init (void)
{
  G_LOCK (hildon_home_globals);
  hildon_home_globals_init_do ();
  G_UNLOCK (hildon_home_globals);
}

/**
 * hildon_home_get_user_home_dir:
 *
 * Gets the current user's home directory.
 *
 * In contrast to g_get_home_dir(), this function prefers the
 * <envar>HOME</envar> environment variable.
 *
 * Return value: the current user's home directory.
 */
G_CONST_RETURN gchar*
hildon_home_get_user_home_dir (void)
{
  hildon_home_globals_init ();
  
  return hildon_home_user_home;
}

/**
 * hildon_home_get_user_config_dir:
 *
 * Gets the current user's configuration directory.
 *
 * Return value: the current user's configuration directory.
 */
G_CONST_RETURN gchar*
hildon_home_get_user_config_dir (void)
{
  hildon_home_globals_init ();
  
  return hildon_home_user_conf;
}

G_CONST_RETURN gchar *
hildon_home_get_user_config_file (void)
{
  hildon_home_globals_init ();

  return hildon_home_user_file;
}

/**
 * hildon_home_get_system_config_dir:
 *
 * Gets the global configuration directory.
 *
 * Return value: the global configuration directory.
 */
G_CONST_RETURN gchar*
hildon_home_get_system_config_dir (void)
{
  hildon_home_globals_init ();
  
  return hildon_home_system_conf;
}

G_CONST_RETURN gchar *
hildon_home_get_user_bg_file (void)
{
  hildon_home_globals_init ();

  return hildon_home_user_bg_file;
}
