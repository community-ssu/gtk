/* ========================================================================= *
 * File: osso-mem.h
 *
 * Copyright (C) 2005 Nokia. All rights reserved.
 *
 * Contact: Leonid Moiseichuk <leonid.moiseichuk@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/*
 * Description:
 *    Reading /proc/meminfo and return memory important values: free, used, usage, total.
 *
 * History:
 *
 * 27-Sep-2005 Leonid Moiseichuk
 * - initial version created using memlimits.c as a prototype.
 * ========================================================================= */

#ifndef OSSO_MEM_H
#define OSSO_MEM_H

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= *
 * Includes
 * ========================================================================= */

#include <unistd.h>

/* ========================================================================= *
 * Definitions.
 * ========================================================================= */

/* Structure that used to report about memory consumption */
typedef struct
{
    size_t  total;     /* Total amount of memory in system: RAM + swap */
    size_t  free;      /* Free memory in system, bytes                 */
    size_t  used;      /* Used memory in system, bytes                 */
    size_t  util;      /* Memory utilization in percents               */
    size_t  low;       /* Low memory limit, bytes.0 if not set or >100%*/
    size_t  deny;      /* Deny limit, bytes.0 if not set or >100%      */
    size_t  usable;
} osso_mem_usage_t;

/* A OOM notification function used when SAW determines an OOM condition
	current_sz -- current heap size
	max_sz -- maximum heap size
	context -- user-specified context (see osso_mem_saw_enable)
*/
typedef void     (*osso_mem_saw_oom_func_t)(size_t current_sz, size_t max_sz,void *context);

/* ========================================================================= *
 * Methods.
 * ========================================================================= */

/* osso_mem_get_usage -- returns memory usage for current system in osso_mem_usage_t structure.
 * parameters:
 *    usage - parameters to be updated.
 * returns:
     0 if values loaded successfuly OR negative error code.
*/
 
int osso_mem_get_usage(osso_mem_usage_t* usage);

/*
 * returns deny limit (in bytes, the total allocated RAM in system)
 * 		according to /proc/sys/vm/lowmem_* settings
*/
size_t osso_mem_get_deny_limit(void);

/*
 * returns low memory (lowmem_high_limit, the total allocated RAM in system)
 * 		according to /proc/sys/vm/lowmem_* settings
 */
size_t osso_mem_get_lowmem_limit(void);


/* ------------------------------------------------------------------------- *
 * osso_mem_saw_enable:  Enables Simple Allocation Watchdog.
 *      1. Calculates the possible growth of process' heap based on the
 *      current heap stats, adjusted to the threshold
 *      2. sets up the hook on malloc function; if the particular allocatuion
 *         whose size is bigger than watchblock_sz could violate the limit,
 *         oom_func with user-specified context is called and malloc returns 0
 *  Returns: 0 on success, negative on error
 * ------------------------------------------------------------------------- */
int osso_mem_saw_enable(size_t threshold,
						size_t watchblock_sz,
						osso_mem_saw_oom_func_t oom_func,
						void *context);
      
/* Disables Simple Allocation Watchdog */
void osso_mem_saw_disable(void);


#ifdef __cplusplus
}
#endif

#endif /* OSSO_MEM_H */
