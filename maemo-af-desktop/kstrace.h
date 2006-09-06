#ifndef __KSTRACE__H__
#define __KSTRACE__H__

/* This file is part of maemo-af-desktop 
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
***************************************************************************
*
* @file kstrace.h
*
***************************************************************************
*
* @author Karoliina Salminen karoliina@karoliinasalminen.com
*
* Copyright (c) 2006 Nokia Corporation. All rights reserved.
*
* This file is licensed under LGPL
*
***************************************************************************
*
* @brief
* DESCRIPTION: some simple tracing macros
*
***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// will be changed later to bits
#define TEXCEPTION 1
#define TWARNING 2
#define TDEBUG 3

#define TRACE_ERR(_x,_y) fprintf(stdout,"ERROR: %s\n",_y);
#define TRACE_ERR2(_x,_y,_z) fprintf(stdout,"ERROR: %s. Error code=%ld\n",_y,_z);
#define TRACE(_x,_y) fprintf(stdout,"TRACE LOG: %s\n",_y);

#endif
