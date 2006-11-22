/* 
 * GPL license, Copyright (c) 2006 by Nokia Corporation                       
 *                                                                            
 * Authors:                                                                   
 *      Tommi Komulainen <tommi.komulainen@nokia.com>
 *      Michael Dominic K. <michael.kostrzewa@nokia.com>
 *                                                                            
 * This program is free software; you can redistribute it and/or modify it    
 * under the terms of the GNU General Public License as published by the      
 * Free Software Foundation, version 2.                                                                   
 *                                                                            
 * This program is distributed in the hope that it will be useful, but        
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   
 * for more details.                                                          
 *                                                                            
 * You should have received a copy of the GNU General Public License along    
 * with this program; if not, write to the Free Software Foundation, Inc., 59 
 * Temple Place - Suite 330, Boston, MA 02111-1307, USA.                      
 *
 */

#include <gtk/gtk.h>

#define                         SANITY_LEVEL 10

GSList*                         parse_dir (const gchar *dir_name, GSList *list, int level);

void                            show_banner (void);

void                            show_usage (void);

int                             main (int argc, char **argv);

