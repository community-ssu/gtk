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

/**
 * SECTION:hail
 * @short_description: hail module initialization
 *
 * Hail module initialization. Hail uses gtk-modules system to do the
 * load and definition of atk implementations.
 */


#include "hailappview.h"
#include "hailcaption.h"
#include "hailcodedialog.h"
#include "hailcolorbutton.h"
#include "hailcolorselector.h"
#include "haildateeditor.h"
#include "haildialog.h"
#include "hailfileselection.h"
#include "hailfindtoolbar.h"
#include "hailgrid.h"
#include "hailgriditem.h"
#include "hailnumbereditor.h"
#include "hailrangeeditor.h"
#include "hailtimeeditor.h"
#include "hailtimepicker.h"
#include "hailvolumebar.h"
#include "hailweekdaypicker.h"
#include "hailwindow.h"
