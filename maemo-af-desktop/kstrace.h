#ifndef __KSTRACE__H__
#define __KSTRACE__H__

/**
***************************************************************************
*
* @file kstrace.h
*
***************************************************************************
*
* @author Karoliina Salminen karoliina at karoliinasalminen dot com
*
* Copyright (c) 2005 Karoliina Salminen
*
***************************************************************************
*
* @brief
* DESCRIPTION: tracing macros
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
