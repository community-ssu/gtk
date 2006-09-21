/* ========================================================================= *
 * File: osso-mem.h
 *
 * This file is part of libosso
 *
 * Copyright (C) 2005-2006 Nokia Corporation. All rights reserved.
 *
 * Contact: Leonid Moiseichuk <leonid.moiseichuk@nokia.com>
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
 */

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
    size_t  total;      /* Total amount of memory in system: RAM + swap */
    size_t  free;       /* Free memory in system, bytes                 */
    size_t  used;       /* Used memory in system, bytes                 */
    size_t  util;       /* Memory utilization in percents               */
    size_t  low;        /* Low memory limit, bytes.0 if not set or >100%*/
    size_t  deny;       /* Deny limit, bytes.0 if not set or >100%      */
    size_t  usable;     /* How much memory available for applications   */
} osso_mem_usage_t;

/* ------------------------------------------------------------------------- *
 * A OOM notification function used when SAW determines an OOM condition
 *	current_sz -- current heap size
 *	max_sz -- maximum heap size
 *	context -- user-specified context (see osso_mem_saw_enable)
 * ------------------------------------------------------------------------- */
typedef void (*osso_mem_saw_oom_func_t)(size_t current_sz, size_t max_sz,void *context);

/* ========================================================================= *
 * Methods.
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * osso_mem_get_usage -- returns memory usage for current system in
 * osso_mem_usage_t structure.
 * parameters:
 *    usage - parameters to be updated.
 * returns:
 *    0 if values loaded successfuly OR negative error code.
 * ------------------------------------------------------------------------- */
int osso_mem_get_usage(osso_mem_usage_t* usage);

/* ------------------------------------------------------------------------- *
 * Returns the total allocated RAM in system	according to
 * /proc/sys/vm/lowmem_* files.
 *
 * WARNING: Assumes 97% of memory can be allocated if no limits set (kernel)
 * hardcoded threshold.
 * ------------------------------------------------------------------------- */
size_t osso_mem_get_avail_ram(void);

/* ------------------------------------------------------------------------- *
 * Returns deny limit (in bytes, the total allocated RAM in system)
 * according to /proc/sys/vm/lowmem_* settings.
 *
 * WARNING: Assumes 97% of memory can be allocated if no limits set (kernel)
 * hardcoded threshold.
 * ------------------------------------------------------------------------- */
size_t osso_mem_get_deny_limit(void);

/* ------------------------------------------------------------------------- *
 * Returns low memory (lowmem_high_limit, the total allocated RAM in system)
 * according to /proc/sys/vm/lowmem_* settings.
 *
 * WARNING: Assumes 97% of memory can be allocated if no limits set (kernel)
 * hardcoded threshold.
 * ------------------------------------------------------------------------- */
size_t osso_mem_get_lowmem_limit(void);

/* ------------------------------------------------------------------------- *
 * Returns 1 if the device is in the low-memory state.
 *
 * WARNING: under Scratchbox always returns 0.
 * ------------------------------------------------------------------------- */
int osso_mem_in_lowmem_state(void);

/* ------------------------------------------------------------------------- *
 * osso_mem_saw_enable - enables Simple Allocation Watchdog.
 * 1. Calculates the possible growth of process' heap based on the
 *    current heap stats, adjusted to the threshold
 * 2. sets up the hook on malloc function; if the particular allocatuion
 *    whose size is bigger than watchblock_sz could violate the limit,
 *    oom_func with user-specified context is called and malloc returns 0
 *
 * Parameters:
 * - threshold - amount of memory used in system. If you pass 0 than maximum
 *   available should be set (according to lowmem_high_limit)
 * - watchblock - if allocation size more than specified the amount of
 *   available memory must be tested. If 0 passed this parameter should be
 *   set to page size.
 * - oom_func - this function shall be called if we reach high memory
 *   consumption (OOM level), specified by threshold or NULL malloc occurs.
 *   May be NULL.
 * - context - additional parameter that shall be passed into oom_func.
 *
 * Returns: 0 on success, negative on error
 *
 * Note: can be safely called several times.
 *
 * WARNING: if SAW can not be installed the old one will be active.
 * ------------------------------------------------------------------------- */
int osso_mem_saw_enable(size_t threshold,
               size_t watchblock,
               osso_mem_saw_oom_func_t oom_func,
               void *context
            );

/* ------------------------------------------------------------------------- *
 * osso_mem_saw_disable - disables Simple Allocation Watchdog and restore
 * default malloc hook. If no watchdog setup do nothing.
 * ------------------------------------------------------------------------- */
void osso_mem_saw_disable(void);

#ifdef __cplusplus
}
#endif

#endif /* OSSO_MEM_H */
