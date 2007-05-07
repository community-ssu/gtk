/* 
 * GPL license, Copyright (c) 2006 by Nokia Corporation                       
 *                                                                            
 * Authors:                                                                   
 *      Michael Dominic Kostrzewa <michael.kostrzewa@nokia.com>               
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

#include "common.h"
#include <glib-2.0/glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdlib.h>
#include <string.h>

#define                         JPEG_QUALITY "99"

int                             main (int argc, char **argv);

void                            show_usage (void);

void                            show_banner (void);

void                            save_png (GdkPixbuf *pixbuf, gchar *filename);
        
void                            save_jpeg (GdkPixbuf *pixbuf, gchar *filename);

void                            process (Template *templ, GdkPixbuf *pixbuf, gchar *directory);

gboolean                        check_if_pixbuf_needs_alpha (GdkPixbuf *pixbuf);

GdkPixbuf*                      strip_alpha_from_pixbuf (GdkPixbuf *pixbuf);

GdkPixbuf*                      extract_alpha_from_pixbuf (GdkPixbuf *pixbuf);

