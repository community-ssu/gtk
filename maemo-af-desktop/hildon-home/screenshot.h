/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
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
 *
 */


/* A tool to grab screenshots from the screen. 
 * We simply read the screen shots from the root window.
 *
 * In this solution it is required that the are to be
 * grabbed is visible on the screen. That's why we call
 * it a screenshot..
 *
 * Another aproach would be to force a widget to render
 * it self to a pixbuf. It would be facinating but
 * it is bit tricky as we can not quarantee that the widget
 * window won't have visible child windows. We would
 * have to replace the windows of each child widget with 
 * a pixbuf. This is left as an exercise for the reader.
 * 
 * TODO: We should provide a general function to grab
 * the area of any GtKWidget too. Not only a GdkWindow.
 * 
 */

#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include "gdk/gdk.h"
#include "glib.h"


G_BEGIN_DECLS



/** Grab a screen shot from a GdkWindow. 
 *  The window must be visible on the screen. For a window on the
 *  background this would grab a slice of the window on top of it.
 *  
 *  Please not that if you pass this function widget->window from
 *  a GtkWidget the result might not be what you want since the
 *  window may refer to a window of one of the parents of the widget.
 *
 *  @param window Pointer to a GdkWindow visible on the screen != NULL.
 *  
 *  @return pointer to a pixbuf grabbed from the window area with
 *  	refcount of one. The caller is responsible of freeing it.
 *  	  
 */
GdkPixbuf *screenshot_grab_from_gdk_window(GdkWindow *window);


/** Grab a screen shot for an rectangle.
 *  Uses the root window of the default screen to read the
 *  pixbuf from. 
 *
 *  @param x  X-coordinate of the upper left coordinate of the
 *  		area to grab. Given in screen coordinates >=0.
 *  		
 *  @param y  Y-coordinate of the upper left coordinate of the
 *  		area to grab. Given in screen coordinates >=0.
 *
 *  @param width The width of the area to grab in pixels >=0.
 *  
 *  @param height The height of the area to grab in pixels >=0.
 *
 *  @return Grabbed serverside pixbuf with refcount of one.
 *  		The caller is responsible in unreffing it.
 *
 */
GdkPixbuf *screenshot_grab_area(const  gint x,const gint y,
		const gint width,const gint height);


G_END_DECLS

#endif /* SCREENSHOT_H */
