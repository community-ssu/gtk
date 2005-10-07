/* ========================================================================= *
 * File: osso-mem.c
 *
 * Copyright (C) 2005 Nokia. All rights reserved.
 *
 * Contact: Andrei Laperie <andrei.laperie@nokia.com>
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

#include "osso-mem.h"


/* ========================================================================= *
 * Definitions.
 * ========================================================================= */

/* Compile-time array capacity calculation */
#define CAPACITY(a)  (sizeof(a) / sizeof(*a))

/* Correct division of 2 unsigned values */
#define DIVIDE(a,b)  (((a) + ((b) >> 1)) / (b))

/*A lil' bit extra threshold before the deny limit so we could
 * prevent kernel denies alltogether and try to react 'properly' instead 
 * see osso_mem_saw_enable*/
#define DEFAULT_SAW_EXTRA_THRESHOLD (1 << 10)

/*Name-value pair*/
typedef struct
{
   const char* name;    /* /proc/meminfo parameter with ":" */
   unsigned    data;    /* loaded value                     */
} meminfo_nvp_t;

/* ========================================================================= *
 * Local data.
 * ========================================================================= */

/*For lowmem_ functions */
/*System limits*/
static size_t sys_avail_ram;
static size_t sys_deny_limit =  0;
static size_t sys_lowmem_limit = 0;
static size_t sys_pagesize = 0;

/*---------------------------------------------------------------------------*/
/*Local data*/

/*SAW-related*/

static pthread_mutex_t saw_lock = PTHREAD_MUTEX_INITIALIZER;

#define THREAD_LOCK()	pthread_mutex_lock(&saw_lock)
#define THREAD_UNLOCK() pthread_mutex_unlock(&saw_lock)


/*Enable/disable flash*/
static int osso_mem_saw_enabled = 0;

/*Blocksize*/
static size_t osso_mem_saw_blocksz;

/*Maximum heapsize*/
static size_t osso_mem_saw_max_heapsz;

/*OOM notification*/
static osso_mem_saw_oom_func_t osso_mem_saw_oom_func=0;

/*User context*/
static void *osso_mem_saw_usr_ctx;

/*original malloc hook*/
static void *(*saw_old_malloc_hook)(size_t,const void *);




/* ========================================================================= *
 * Local methods.
 * ========================================================================= */

/*Reading /proc/sys/vm/lowmem* */
static void lm_read_proc(void);

/*Reading /proc/meminfo*/
static unsigned mu_load(const char* path, meminfo_nvp_t* vals, unsigned size);

/*Malloc hook*/
static void *saw_malloc_hook(size_t sz, const void *caller);

/* ------------------------------------------------------------------------- *
 * mu_load -- open meminfo file and load values.
 * parameters:
 *    path - path to file to handle.
 *    vals - array of values to be handled.
 *    size - size of vals array.
 * returns: number of sucessfully loaded values.
 * ------------------------------------------------------------------------- */
static unsigned 
mu_load(const char* path, meminfo_nvp_t* vals, unsigned size)
{
   unsigned counter = 0;
   FILE*    meminfo = fopen(path, "rt");

   if ( meminfo )
   {
      char line[256];

      /* Load all lines in file */
      while ( fgets(line, CAPACITY(line),meminfo) )
      {
         unsigned idx;

         /* Search and setup parameter */
         for (idx = 0; idx < size; idx++)
         {
            if ( line == strstr(line, vals[idx].name) )
            {
               /* Parameter has a format SomeName:\tValue, we expect that meminfo_nvp_t::name contains ":" */
               vals[idx].data = (unsigned)strtoul(line + strlen(vals[idx].name) + 1, NULL, 0);
               counter++;
               break;
            }
         }
      } /* while have data */

      fclose(meminfo);
   }

   return counter;
} /* mu_load */
/*---------------------------------------------------------------------------*/
/*Returns a single unsigned integer read from the file or negative
 if error
*/
static int 
lm_get_file_int(const char *filename) 
{
    FILE *f;
    int u;

    f = fopen (filename, "r");
	if(f)
	{
	    if(fscanf(f, "%u", (unsigned *)&u) < 1) 
		{
        	u = -1;
		}

		fclose(f);
    }
	else
		u=errno;

    return u;
}
/*---------------------------------------------------------------------------*/
/*Reads contents of certain /proc/sys/vm/lowmem entries into static vars*/
static void 
lm_read_proc(void)
{
        /*Reading
         *  /proc/sys/vm/lowmwm_available_pages
         *  /proc/sys/vm/lowmem_deny_threshold
         *  /proc/sys/vm/lowmem_notify_high
         * */
    unsigned percent_to_size;
    sys_pagesize = sysconf(_SC_PAGESIZE);
    sys_avail_ram = lm_get_file_int("/proc/sys/vm/lowmem_allowed_pages") * sys_pagesize;
    percent_to_size = sys_avail_ram / 100;

    sys_deny_limit = lm_get_file_int("/proc/sys/vm/lowmem_deny_watermark")
                     * percent_to_size;

    sys_lowmem_limit = lm_get_file_int("/proc/sys/vm/lowmem_notify_high")
                     * percent_to_size;

}
/*---------------------------------------------------------------------------*/
/* Malloc hook. Executed when osso_mem_saw_active is in place. Note, has a doubtful
   thread-safety
*/
static void 
*saw_malloc_hook(size_t sz, const void *caller)
{
    int oom = 0;
    void *p;

	THREAD_LOCK();

    __malloc_hook = saw_old_malloc_hook;

    if(sz >= osso_mem_saw_blocksz)
    {
        struct mallinfo mi = mallinfo();
        mi.arena += (mi.hblkhd + sz);

        ULOG_DEBUG("sz:%u\tmax:%u\tcur:%u\n",sz, osso_mem_saw_max_heapsz,mi.arena);

        if( mi.arena  >=  osso_mem_saw_max_heapsz)
        /*And we're oom*/
        {
            /*Notify*/
            if(osso_mem_saw_oom_func)
            {
				THREAD_UNLOCK();

				(*osso_mem_saw_oom_func)(mi.arena,osso_mem_saw_max_heapsz,osso_mem_saw_usr_ctx);

				THREAD_LOCK();
            }
            oom = 1;
        }

    }


    if(!oom)
    {
	  /*Please note as malloc inside is thread-safe there's a chance of deadlock(?)*/
      p=malloc(sz);
    }
    else
    {
      p=0;
    }

    __malloc_hook = saw_malloc_hook;
	
	THREAD_UNLOCK();
    
	return p;
}


/* ========================================================================= *
 * Public methods.
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * osso_mem_get_usage -- returns memory usage for current system in osso_mem_usage_t structure.
 * parameters:
 *    usage - parameters to be updated.
 * returns:
 *    0 if values loaded successfuly OR negative error code.
 * ------------------------------------------------------------------------- */
int 
osso_mem_get_usage(osso_mem_usage_t* usage)
{
   /* Check the pointer validity first */
   if ( usage )
   {
      static meminfo_nvp_t vals[] =
      {
         { "MemTotal:",  0 },
         { "SwapTotal:", 0 },
         { "MemFree:",   0 },
         { "Buffers:",   0 },
         { "Cached:",    0 },
         { "SwapFree:",  0 },
         { "Slab:",      0 }
      }; /* vals */
	  
      /* Load values from the meminfo file */
      if ( CAPACITY(vals) == mu_load("/proc/meminfo", vals, CAPACITY(vals)) )
      {
         /* Discover memory information using loaded numbers */
         usage->total = vals[0].data + vals[1].data;
         usage->free  = vals[2].data + vals[3].data + vals[4].data + vals[5].data /*+ vals[6].data*/;
         usage->used  = usage->total - usage->free;
         usage->util  = DIVIDE(100 * usage->used, usage->total);

         /* Translate everything from kilobytes to bytes */
         usage->total <<= 10;
         usage->free  <<= 10;
         usage->used  <<= 10;
/*Getting /proc/sys/vm/lowmem* stuff*/
		 if(!sys_lowmem_limit) lm_read_proc();

		 usage->deny = osso_mem_get_deny_limit();
		 usage->low  = osso_mem_get_lowmem_limit();
	 
		 if(usage->deny >  usage->total) usage->deny = 0;
		 if(usage->low > usage->total) usage->low = 0;

		 /*From the usage->free we deduct the delta
		  * based on deny limit 
		  * or 87.5% if deny limit is disabled */
		 usage->usable = usage->deny 
						  	 ? (usage->total - usage->deny)  
							 : (usage->total >> 3) ;
		 usage->usable = usage->usable  > usage->free 
				 			? 0 
							: usage->free - usage->usable;
						 
         /* We have succeed */
         return 0;
      }
      else
      {
         /* Clean-up values */
         memset(usage, 0, sizeof(usage));
      }
   }

   /* Something wrong, shows as error */
   return -1;
} /* memusage */

/* ------------------------------------------------------------------------- *
 * osso_mem_get_deny_limit -- returns deny limit
 * ------------------------------------------------------------------------- */
/*Deny limit, bytes*/
size_t 
osso_mem_get_deny_limit(void) 
{
	if(!sys_deny_limit) lm_read_proc();
	return sys_deny_limit;
}

/* ------------------------------------------------------------------------- *
 * osso_mem_get_lowmem_limit -- returns lowmem notification limit
 * ------------------------------------------------------------------------- */
size_t 
osso_mem_get_lowmem_limit(void) 
{
	if(!sys_lowmem_limit) lm_read_proc();
	return sys_lowmem_limit;
}

/* ------------------------------------------------------------------------- *
 * osso_mem_saw_enable -- Enables Simple Allocation Watchdog. 
 * 		1. Calculates the possible growth of process' heap based on the
 *		current heap stats, adjusted to the threshold
 *		2. sets up the hook on malloc function; if the particular allocatuion 
 *		   whose size is bigger than watchblock_sz could violate the limit, 
 *		   oom_func with user-specified context is called and malloc returns 0
 *	Returns: 0 on success, negative on error
 * ------------------------------------------------------------------------- */
int
osso_mem_saw_enable(size_t threshold,
					size_t watchblock_sz,
					osso_mem_saw_oom_func_t oom_func,
					void *context)
{
	osso_mem_usage_t current;

	if( 0!=osso_mem_get_usage(&current) )
			return -EINVAL;
	
	osso_mem_saw_disable();

    ULOG_DEBUG("Setting hooks\n");

	THREAD_LOCK();

    osso_mem_saw_oom_func = oom_func;
    osso_mem_saw_blocksz   = watchblock_sz;
    osso_mem_saw_usr_ctx = context;
  
	/*If we're below the threshold, don't make things worse*/
   	if(current.usable > threshold)
	{
		struct mallinfo mi=mallinfo();
		/*How much heap could grow*/
		osso_mem_saw_max_heapsz =  mi.arena + mi.hblkhd 
					    + current.usable
						- threshold;
	}
	else
	{	
		osso_mem_saw_max_heapsz = 0;
	}
    
    /*Accounting for deductions we calculated*/
    if(!osso_mem_saw_max_heapsz)
    {
		THREAD_UNLOCK();
        if(osso_mem_saw_oom_func)
        {
           (*osso_mem_saw_oom_func)(0,osso_mem_saw_max_heapsz,osso_mem_saw_usr_ctx);
        }
        else
        {
            return -EINVAL;
        }
    }



    /* setting malloc hook and saving an old one*/
    saw_old_malloc_hook = __malloc_hook;
    __malloc_hook = saw_malloc_hook;
    osso_mem_saw_enabled = 1;
	
	THREAD_UNLOCK();

	ULOG_DEBUG("WDOG: sz %u max %u thr %u\n",
                    osso_mem_saw_blocksz, osso_mem_saw_max_heapsz, DEFAULT_SAW_EXTRA_THRESHOLD);

    return 0;
}

/* ------------------------------------------------------------------------- *
 * osso_mem_saw_disable -- disabled watchdog previously enabled with osso_mem_saw_enable
 *    if no watchdog enabled, does nothing
 * ------------------------------------------------------------------------- */
void
osso_mem_saw_disable(void)
{
    if(!osso_mem_saw_enabled)
        return;
	
	THREAD_LOCK();
    
	osso_mem_saw_enabled = 0;

    /*Restoring hooks*/
    if(__malloc_hook == saw_malloc_hook)
        __malloc_hook = saw_old_malloc_hook;

	THREAD_UNLOCK();

    ULOG_DEBUG("Removing hooks!\n");
}



/* ========================================================================= *
 * main function, just for testing purposes.
 * ========================================================================= */
#ifdef UNIT_TEST

int 
main(const int argc, const char* argv[])
{
   osso_mem_usage_t usage;
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
   
   {
  	   void *ptr;
	   size_t insane = 60 << 20;
	   ptr = malloc( insane );
	   printf("Without SAW, allocating %u bytes: %s\n",insane, ptr ? "Succeeded": "Failed" );
	   if(ptr) free(ptr);
	   if (0!=osso_mem_saw_enable(3<<20,32767,NULL,NULL))
	   {
			printf("Cannot activate saw. Kaput"); 
			return -1;
	   }
       ptr = malloc( insane );
       if(ptr) free(ptr);
       printf("With SAW, allocating %u bytes: %s\n",insane, ptr ? "Succeeded" : "Failed");
	}
   /* That is all */
   return 0;
} /* main */

#endif /* UNIT_TEST */

