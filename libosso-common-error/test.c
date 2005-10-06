/*
 * This file is part of libosso-common-error
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Mohammad Anwari <Mohammad.Anwari@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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

#include <osso-common-error.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <libintl.h>

void main () {
	setlocale (LC_ALL, "");
	fprintf (stderr, "%s\n", osso_common_error (CERM_UNABLE_TO_SEND_FOLDERS));
	fprintf (stderr, "%s\n", osso_common_error (CERM_UNABLE_TO_SEND_MEMORY_CARD));
}
