/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005 Nokia Corporation.
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
 * @file hildon-status-bar-interface.h
 * <p>
 * @brief Hildon-status-bar interface functions for maemo-af-desktop
 */

#ifndef __HILDON_STAB_IF_KS_H__
#define __HILDON_STAB_IF_KS_H__

#include "hildon-status-bar-main.h"


int status_bar_main(osso_context_t *osso, StatusBar **panel);
void status_bar_deinitialize(osso_context_t *osso, StatusBar **panel);
                                                 
#endif
