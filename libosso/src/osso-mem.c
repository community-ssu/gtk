/* ========================================================================= *
 * File: osso-mem.c
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

/* ========================================================================= *
 * Includes
 * ========================================================================= */
#define _GNU_SOURCE

#include <osso-log.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <syslog.h>

#include "osso-mem.h"


/* ========================================================================= *
 * Definitions.
 * ========================================================================= */

/* Compile-time array capacity calculation */
#define CAPACITY(a)  (sizeof(a) / sizeof(*a))

/* Correct division of 2 unsigned values */
#define DIVIDE(a,b)  (((a) + ((b) >> 1)) / (b))

#define NSIZE        ((size_t)(-1))

typedef void* (*malloc_hook_t)(size_t, const void*);


/* ========================================================================= *
 * Meminfo related strings.
 * WARNING: be careful, micro-optimizations here made code a bit fuzzy.
 * ========================================================================= */

/* Defining the labels we could read from the /proc/meminfo               */
/* Done in a little tricky way to have strlen's calculated @ compile time */
#define MEMINFO_LABELS    \
            IV_PAIR(ID_MEMTOTAL,    "MemTotal:" ) \
            IV_PAIR(ID_SWAPTOTAL,   "SwapTotal:") \
            IV_PAIR(ID_MEMFREE,     "MemFree:"  ) \
            IV_PAIR(ID_BUFFERS,     "Buffers:"  ) \
            IV_PAIR(ID_CACHED,      "Cached:"   ) \
            IV_PAIR(ID_SWAPFREE,    "SwapFree:" )

/* Definition of specially labeled string */
/* WARNING: re-defined below !!!          */
#define IV_PAIR(ID,STRING) \
            static const char _localstring_##ID[]=STRING;

/* Introduce labeled strings */
MEMINFO_LABELS

/* Re-defining pair as a <string, size> */
#undef IV_PAIR
#define IV_PAIR(ID,STRING) \
            { _localstring_##ID, CAPACITY(_localstring_##ID) -1 },

/* Introduce array of strings and sizes */
static const struct
{
   const char* name;
   unsigned    length;
} meminfo_labels[] =
{
   MEMINFO_LABELS
};

/* Re-defining one more time to have IDs for conveinience */
#undef IV_PAIR
#define IV_PAIR(ID,STRING)    ID,

enum
{
    MEMINFO_LABELS
    MAX_MEMINFO_LABELS
};

#undef IV_PAIR

/* Initializer for values array (load_meminfo)*/
#define IV_PAIR(ID,STRING)    0,
#define MEMINFO_INIT          MEMINFO_LABELS


/* ========================================================================= *
 * Local data.
 * ========================================================================= */

/* For lowmem_ functions system limits */
static size_t sys_avail_memory = NSIZE;
static size_t sys_deny_limit   = NSIZE;
static size_t sys_lowmem_limit = NSIZE;

/* SAW-related */
static pthread_mutex_t saw_lock = PTHREAD_MUTEX_INITIALIZER;

#define THREAD_LOCK()      pthread_mutex_lock(&saw_lock)
#define THREAD_UNLOCK()    pthread_mutex_unlock(&saw_lock)

/* Parameters of the latest SAW setup call */
static size_t saw_max_block_size;      /* If allocation is greater - make a test       */
static size_t saw_max_heap_size;       /* If heap is greater that specified - call OOM */

/* This function shall be called when OOM occurs or close */
static osso_mem_saw_oom_func_t saw_user_oom_func = NULL;
static void*  saw_user_context;        /* Extra data for user OOM function */

/* Original malloc hook which may be NULL */
static malloc_hook_t saw_old_malloc_hook = NULL;


/* ========================================================================= *
 * Local methods.
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * get_file_value - gets the value (size or %) from specified /proc file.
 * Returns NSIZE if file is not available or value is invalid.
 * ------------------------------------------------------------------------- */

static size_t get_file_value(const char* filename)
{
   FILE* fp = fopen(filename, "rt");

   if (fp)
   {
      /* File opened successfuly, trying to load value */
      char  buffer[32];

      /* Get the value as a string */
      if (NULL == fgets(buffer, CAPACITY(buffer) - 1, fp))
         *buffer = 0;
      fclose(fp);

      /* Convert it, much faster than fscanf */
      if ( *buffer )
      {
         const long value = strtol(buffer, NULL, 0);

         if (value > 0)
            return (size_t)value;
      }
   }

   /* Any kind of error */
   return NSIZE;
} /* get_file_value */

/* ------------------------------------------------------------------------- *
 * load_meminfo -- open meminfo file and load values.
 * parameters:
 *    vals - array of values to be handled.
 *    size - size of vals array.
 * returns: number of sucessfully loaded values.
 * ------------------------------------------------------------------------- */
static unsigned load_meminfo(size_t *vals, unsigned size)
{
   FILE* meminfo = fopen("/proc/meminfo", "rt");

   if ( meminfo )
   {
      unsigned counter = 0;
      char line[256];

      /* Load all lines in file until we need setup values */
      while (counter < size && fgets(line, CAPACITY(line), meminfo))
      {
         unsigned idx;

         for (idx = 0; idx < size; idx++)
         {
            /* Skip all indicies that already set */
            if ( vals[idx] )
               continue;

            /* Skip values that have different labels */
            if ( strncmp(line, meminfo_labels[idx].name, meminfo_labels[idx].length) )
               continue;

            /* Match, save the value */
            vals[idx] = (size_t)strtoul(line + meminfo_labels[idx].length + 1, NULL, 0);
            counter++;

            /* Exit from scanning loop */
            break;
         } /* for */
      } /* for all values and meminfo lines */

      fclose(meminfo);
      return counter;
   }

   return 0;
} /* load_meminfo */

/* ------------------------------------------------------------------------- *
 * setup_sys_values - loads the values from /proc files or obtain its from
 * system andand setup sys_XXX variables.
 * WARNING: we expect than sys_XXX variables are not changed during runtime.
 * ------------------------------------------------------------------------- */
static void setup_sys_values(void)
{
   /* Setup the page size */
   const size_t pagesize = sysconf(_SC_PAGESIZE);
   /* Load amount of allowed pages */
   const size_t allowed_pages = get_file_value("/proc/sys/vm/lowmem_allowed_pages");

   /* Check the availability of values */
   if (NSIZE == allowed_pages)
   {
      /* Unfortunately, lowmem_ is not available yet (scratchbox?) */
      size_t    total   = 0;    /* ID_MEMTOTAL goes first */
      const int counter = load_meminfo(&total, 1);

      /* Setup available amount of RAM, if no meminfo available set 64MB */
      sys_avail_memory = (counter && total ? total : (64 << 10));
   }
   else
   {
      sys_avail_memory = allowed_pages * (pagesize >> 10);
      sys_deny_limit   = get_file_value("/proc/sys/vm/lowmem_deny_watermark");
      sys_lowmem_limit = get_file_value("/proc/sys/vm/lowmem_notify_high");
   }

   /* Normalize the sys_deny_limit and sys_lowmem_limit according to loaded values */
   if (NSIZE == sys_deny_limit)
      sys_deny_limit = sys_avail_memory - (sys_avail_memory >> 5);
   else
      sys_deny_limit = DIVIDE(sys_avail_memory * sys_deny_limit, 100);

   if (NSIZE == sys_lowmem_limit)
      sys_lowmem_limit = sys_deny_limit;
   else
      sys_lowmem_limit = DIVIDE(sys_avail_memory * sys_lowmem_limit, 100);

   /* Moving from KB to bytes */
   sys_avail_memory <<= 10;
   sys_deny_limit   <<= 10;
   sys_lowmem_limit <<= 10;
} /* setup_sys_values */


/* ------------------------------------------------------------------------- *
 * saw_malloc_hook - Malloc hook. Executed when osso_mem_saw_active is in
 * place. Thread-safe (= slow in some cases).
 * ------------------------------------------------------------------------- */
static void* saw_malloc_hook(size_t size, const void* caller)
{
   void* ptr;  /* Allocated pointer, NULL means OOM situation happened */

   THREAD_LOCK();

   /* Restore the real malloc hook */
   __malloc_hook = saw_old_malloc_hook;

   /* Check for OOM-potential situation */
   if (size >= saw_max_block_size)
   {
      /* We must test amount of memory to predict future */
      const struct mallinfo mi = mallinfo();
      ptr = (mi.arena + mi.hblkhd + size >= saw_max_heap_size ? NULL : malloc(size));
   }
   else
   {
      ptr = malloc(size);
   }

   /* Restore malloc hook to self */
   __malloc_hook = saw_malloc_hook;

   /* Test allocation, call OOM function if necessary  */
   /* Note: SAW may be removed but that is safe for us */
   if (!ptr && saw_user_oom_func)
      saw_user_oom_func(saw_max_heap_size + size, saw_max_heap_size, saw_user_context);

   THREAD_UNLOCK();

#ifdef LIBOSSO_DEBUG
   /* Printing from the critical section is a bad idea, try to do it now */
   if ( !ptr )
      ULOG_INFO_F("SAW: OOM for %u allocation", size);
#endif

   return ptr;
} /* saw_malloc_hook */


/* ========================================================================= *
 * Public methods.
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * osso_mem_get_usage -- returns memory usage for current system in
 * osso_mem_usage_t structure.
 * parameters:
 *    usage - parameters to be updated.
 * returns:
 *    0 if values loaded successfuly OR negative error code.
 * ------------------------------------------------------------------------- */
int osso_mem_get_usage(osso_mem_usage_t* usage)
{
   /* Local variables */
   size_t vals[MAX_MEMINFO_LABELS];

   /* Check the pointer validity first */
   if ( !usage )
      return -1;

   /* Load values from /proc/meminfo file */
   memset(usage, 0, sizeof(*usage));
   memset(vals,  0, sizeof(vals));
   if ( !load_meminfo(vals, CAPACITY(vals)) )
      return -1;

   /* Initialize values for /proc/sys/vm/lowmem_* */
   if (NSIZE == sys_avail_memory)
      setup_sys_values();


   /* Discover memory information using loaded numbers */
   usage->total = vals[ID_MEMTOTAL] + vals[ID_SWAPTOTAL];

   usage->free  = vals[ID_MEMFREE] + vals[ID_BUFFERS] +
                  vals[ID_CACHED] +  vals[ID_SWAPFREE];

   usage->used = usage->total - usage->free;
   usage->util = DIVIDE(100 * usage->used, usage->total);

   /* Translate everything from kilobytes to bytes */
   usage->total <<= 10;
   usage->free  <<= 10;
   usage->used  <<= 10;

   usage->deny = sys_deny_limit;
   usage->low  = sys_lowmem_limit;

   /*
    * From the usage->free we deduct the delta based on deny limit
    * or 87.5% if low limit is disabled
    */
   usage->usable = (usage->low ? sys_avail_memory - usage->low : (sys_avail_memory >> 3));
   usage->usable = (usage->usable < usage->free ? usage->free - usage->usable : 0);

   /* We have succeed */
   return 0;
} /* osso_mem_get_usage */

/* ------------------------------------------------------------------------- *
 * Returns the total allocated RAM in system according to
 * /proc/sys/vm/lowmem_* files.
 *
 * WARNING: Assumes 97% of memory can be allocated if no limits set (kernel)
 * hardcoded threshold.
 * ------------------------------------------------------------------------- */
size_t osso_mem_get_avail_ram(void)
{
   if(NSIZE == sys_avail_memory)
      setup_sys_values();
   return sys_avail_memory;
} /* osso_mem_get_avail_ram */

/* ------------------------------------------------------------------------- *
 * Returns deny limit (in bytes, the total allocated RAM in system)
 * according to /proc/sys/vm/lowmem_* settings.
 *
 * WARNING: Assumes 97% of memory can be allocated if no limits set (kernel)
 * hardcoded threshold.
 * ------------------------------------------------------------------------- */
size_t osso_mem_get_deny_limit(void)
{
   if(NSIZE == sys_deny_limit)
      setup_sys_values();
   return sys_deny_limit;
} /* osso_mem_get_deny_limit */

/* ------------------------------------------------------------------------- *
 * Returns low memory (lowmem_high_limit, the total allocated RAM in system)
 * according to /proc/sys/vm/lowmem_* settings.
 *
 * WARNING: Assumes 97% of memory can be allocated if no limits set (kernel)
 * hardcoded threshold.
 * ------------------------------------------------------------------------- */
size_t osso_mem_get_lowmem_limit(void)
{
   if(NSIZE == sys_lowmem_limit)
      setup_sys_values();
   return sys_lowmem_limit;
} /* osso_mem_get_lowmem_limit */

/* ------------------------------------------------------------------------- *
 * Returns flag about low memory conditions is reached according to
 * /sys/kernel/high_watermark is set to 1.
 *
 * WARNING: under scratchbox always return 0.
 * ------------------------------------------------------------------------- */
int osso_mem_in_lowmem_state(void)
{
   return (1 == get_file_value("/sys/kernel/high_watermark"));
} /* osso_mem_in_lowmem_state */

/* ------------------------------------------------------------------------- *
 * osso_mem_saw_enable - enables Simple Allocation Watchdog.
 * 1. Calculates the possible growth of process' heap based on the
 *    current heap stats, adjusted to the threshold
 * 2. sets up the hook on malloc function; if the particular allocatuion
 *    whose size is bigger than watchblock could violate the limit,
 *    oom_func with user-specified context is called and malloc returns 0
 *
 * Parameters:
 * - threshold - amount of memory that shall be free in system.
 *   If you pass 0 than maximum available should be set (according to lowmem_high_limit)
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
                  void* context
            )
{
   osso_mem_usage_t current;

   /* Validate passed parameters. */
   if ( !watchblock )
      watchblock = sysconf(_SC_PAGESIZE);

   /* Load the values about memory usage */
   if( osso_mem_get_usage(&current) )
   {
      ULOG_CRIT_F("Error:osso_mem_get_usage failed");
      return -EINVAL;
   }

   /* If we're below the threshold, don't make things worse */
   if(current.usable > threshold)
   {
      const struct mallinfo mi = mallinfo();

      THREAD_LOCK();

      saw_user_oom_func  = oom_func;
      saw_max_heap_size  = mi.arena + mi.hblkhd + current.usable - threshold;
      saw_max_block_size = watchblock;
      saw_user_context   = context;
      /* Always dumping memory information (workaround for thumbnailer) */
      syslog(LOG_CRIT, "osso_mem %u = %u + %u + %u - %u", saw_max_heap_size,
             mi.arena, mi.hblkhd, current.usable, threshold);

      if(saw_malloc_hook != __malloc_hook)
      {
         /* SAW hook is not set */
         saw_old_malloc_hook = __malloc_hook;
         __malloc_hook = saw_malloc_hook;
      }

      THREAD_UNLOCK();

      ULOG_INFO_F("SAW hook installed: block size %u, maxheap %u (threshold %u)",
                  saw_max_block_size, saw_max_heap_size, threshold);

      return 0;
   }
   else
   {
      ULOG_WARN_F("SAW: OOM:current.usable(%u) <= threshold(%u)",
                  current.usable, threshold);
      return -EINVAL;
   }
} /* osso_mem_saw_enable */

/* ------------------------------------------------------------------------- *
 * osso_mem_saw_disable - disables Simple Allocation Watchdog and restore
 * default malloc hook.
 *
 * Note: can be safely called several times.
 * ------------------------------------------------------------------------- */
void osso_mem_saw_disable(void)
{
   THREAD_LOCK();
   if(saw_malloc_hook == __malloc_hook)
      __malloc_hook = saw_old_malloc_hook;
   THREAD_UNLOCK();

   ULOG_INFO_F("SAW hook removed!");
} /* osso_mem_saw_disable */



/* ========================================================================= *
 * main function, just for testing purposes.
 * ========================================================================= */
#ifdef UNIT_TEST

static void test_oom_func(size_t current_sz, size_t max_sz, void* context)
{
   printf("%s(%u, %u, 0x%08x) called\n", __FUNCTION__, current_sz, max_sz, (unsigned)context);
} /* test_oom_func */


int main(const int argc, const char* argv[])
{
   osso_mem_usage_t usage;
   const size_t insane = 60 << 20;
   void* ptr;

   printf("\n1. UNIT_TEST MEMUSAGE \n");

   /* Load all values from meminfo file */
   if (0 == osso_mem_get_usage(&usage))
   {
      printf ("%u\t%u\t%u\t%u\t%u\t%u\n", usage.total, usage.free, usage.used, usage.util, usage.deny, usage.low);
   }
   else
   {
      printf ("unable to load values from /proc/meminfo file\n");
      return -1;
   }

   printf("\n2. Testing lowmem\n");

   printf("Lowmem limits: LOW=%u bytes, DENY=%u bytes\n",
            osso_mem_get_lowmem_limit(),
            osso_mem_get_deny_limit()
         );

   printf("\n3. Testing SAW\n");
   ptr = malloc( insane );
   printf("Without SAW, allocating %u bytes: %s\n",insane, ptr ? "Succeeded": "Failed" );

   if(ptr)
      free(ptr);

   if ( osso_mem_saw_enable(0, 0, NULL, NULL) )
   {
      printf("Cannot activate saw\n");
   }

   ptr = malloc( insane );
   printf("With SAW, allocating %u bytes: %s\n", insane, ptr ? "Succeeded" : "Failed");
   if(ptr)
      free(ptr);

   if ( osso_mem_saw_enable(0, 0, test_oom_func, NULL) )
      printf("Cannot activate saw with oom function\n");

   ptr = malloc( insane );
   printf("With SAW, allocating %u bytes: %s\n", insane, ptr ? "Succeeded" : "Failed");

   if ( osso_mem_in_lowmem_state() )
      printf("\n4. Low memory situation is reached\n");
   else
      printf("\n4. Low memory situation is not reached\n");

   if(ptr)
      free(ptr);

   /* That is all */
   return 0;
} /* main */

#endif /* UNIT_TEST */

