/* HAIL - The Hildon Accessibility Implementation Library
 * Copyright (C) 2005, 2006 Nokia Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#include "hailfactory.h"
#include "hail-hildon-libs/hail-hildon-libs.h"
#include "hail-hildon-fm/hail-hildon-fm.h"
#include "hail-hildon-desktop/hail-hildon-desktop.h"
#include "hail.h"

/*
 *   These exported symbols are hooked by gnome-program
 * to provide automatic module initialization and shutdown.
 */
extern void hildon_accessibility_module_init     (void);
extern void hildon_accessibility_module_shutdown (void);

static int hail_initialized = FALSE;

/* Module initialization. It gets instances of hail factories, and
 * binds them to the Hildon widgets */
static void
hail_accessibility_module_init (void)
{
  if (hail_initialized)
    {
      return;
    }
  hail_initialized = TRUE;


  fprintf (stderr, "Hildon Accessibility Module initialized\n");

  hail_hildon_libs_init ();
  hail_hildon_fm_init ();
  hail_hildon_desktop_init ();

}

/**
 * hildon_accessibility_module_init:
 * 
 **/
void
hildon_accessibility_module_init (void)
{
  hail_accessibility_module_init ();
}

/**
 * hildon_accessibility_module_shutdown:
 * 
 **/
void
hildon_accessibility_module_shutdown (void)
{
  if (!hail_initialized)
    {
      return;
    }
  hail_initialized = FALSE;

  fprintf (stderr, "Hildon Accessibility Module shutdown\n");

}

/* Gtk module initialization for Hail. It's called on Gtk 
 * initialization, when it request the module to be loaded */
int
gtk_module_init (gint *argc, char** argv[])
{
  hail_accessibility_module_init ();

  return 0;
}
