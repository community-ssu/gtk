/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
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

/*
 *
 * Header file for close application dialog
 *
 */

#ifndef CLOSE_APPLICATION_DIALOG_H
#define CLOSE_APPLICATION_DIALOG_H

/* Type of current action */
typedef enum CADActionType CADAction;
enum CADActionType
{
  CAD_ACTION_OPENING,
  CAD_ACTION_SWITCHING,
  CAD_ACTION_OTHER
};

gboolean tn_close_application_dialog(CADAction action);

#endif /* CLOSE_APPLICATION_DIALOG_H */
