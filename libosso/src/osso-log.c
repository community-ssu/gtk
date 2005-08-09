/**
 * @file osso-log.c
 * Copyright (C) 2005 Nokia Corporation.
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
 *
 */
                
#include <osso-internal.h>
#include <stdarg.h>
#include <syslog.h>


void osso_log(int level, const char *format, ...) 
{
    va_list args;
    va_start(args,format);

    vsyslog(level,format,args);
    

    va_end(args);
    
}


inline void d_log(const char *file,int line, int level, 
                const char *format, ...)
{
#ifdef DEBUG
    va_list args;
    char *prefix_format;
    int len;

    len = strlen(file)+strlen(":%d: ")+strlen(format)+2;
   
/*    dprint("file [%s], line [%d], format [%s], len [%d]\n",file,line,format,
                    len);*/
    prefix_format = g_malloc(len);
    g_assert(prefix_format);

    g_snprintf(prefix_format,len,"%s:%d: %s",
                    file,line,format);

/*    dprint("Prefixed format [%s]\n",prefix_format);    */
    va_start(args, format);
    
    vsyslog(level|LOG_USER,prefix_format,args);
    
    va_end(args);

    g_free(prefix_format);
#endif
}




