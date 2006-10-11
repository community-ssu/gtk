/* 
 * GPL license, Copyright (c) 2006 by Nokia Corporation                       
 *                                                                            
 * Authors:                                                                   
 *      Michael Dominic Kostrzewa <michael.kostrzewa@nokia.com>               
 *                                                                            
 * This program is free software; you can redistribute it and/or modify it    
 * under the terms of the GNU General Public License as published by the      
 *  Free Software Foundation; either version 2, or (at your option) any later
 * version.                                                                   
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

#include "common.h"
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

int                             main (int argc, char **argv);

void                            show_usage (void);

gchar*                          get_color (Color *color, GdkPixbuf *pixbuf);

void                            process (Template *templ, GdkPixbuf *pixbuf);

void                            show_banner (void);
