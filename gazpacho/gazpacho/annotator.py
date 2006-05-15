# Copyright (C) 2005 Red Hat, Inc.
#               2005 Johan Dahlin
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

from gtk import gdk

use_cairo = False
try:
    from cairo import gtk as cairogtk
    use_cairo = True
except ImportError:
    pass

SELECTION_NODE_SIZE = 7
BORDER_WIDTH = 3

class IAnnotate:
    def __init__(widget, window):
        pass

    def draw_notes(x, y, width, height):
        pass

    def draw_border(x, y, width, height, color):
        pass

class CairoAnnotator(object):
    def __init__(self, widget, window):
        self._cr = cairogtk.gdk_cairo_create(window)
        self._cr.set_line_width(1.0)
        
    def draw_nodes(self, x, y, width, height):
        cr = self._cr
        
        cr.set_source_rgba(0.40, 0.0, 0.0, 0.75) # translucent dark red
        
        if width > SELECTION_NODE_SIZE and height > SELECTION_NODE_SIZE:
            cr.rectangle(x + 1, y + 1,
                         SELECTION_NODE_SIZE - 1, SELECTION_NODE_SIZE - 1)
            cr.rectangle(x + 1, y + height -SELECTION_NODE_SIZE,
                         SELECTION_NODE_SIZE - 1, SELECTION_NODE_SIZE - 1)
            cr.rectangle(x + width - SELECTION_NODE_SIZE, y + 1,
                         SELECTION_NODE_SIZE - 1, SELECTION_NODE_SIZE - 1)
            cr.rectangle(x + width - SELECTION_NODE_SIZE,
                         y + height - SELECTION_NODE_SIZE,
                         SELECTION_NODE_SIZE - 1, SELECTION_NODE_SIZE -1)
            cr.fill()
            
        cr.rectangle(x + 0.5, y + 0.5, width - 1, height - 1)
        cr.stroke()
         
    def draw_border(self, x, y, width, height, color):
        cr = self._cr
        
        for i in range(0, BORDER_WIDTH):
            if (i <= BORDER_WIDTH / 2):
                alpha = (i + 1.) / (BORDER_WIDTH)
            else:
                alpha = (BORDER_WIDTH - i + 0.) / (BORDER_WIDTH)
                
            cr.set_source_rgba(color[0], color[1], color[2],
                               alpha)
            cr.rectangle(x - BORDER_WIDTH / 2 + i + 0.5,
                         y - BORDER_WIDTH / 2 + i + 0.5,
                         width + 2 * (BORDER_WIDTH / 2 - i) - 1,
                         height + 2 * (BORDER_WIDTH / 2 - i) - 1)
            cr.stroke()

class GdkAnnotator(object):
    def __init__(self, widget, window):
        self._widget = widget
        self._window = window
        self._gc = gdk.GC(window, line_width=BORDER_WIDTH)

    def draw_nodes(self, x, y, width, height):
        window = self._window
        gc = self._widget.style.black_gc
        
        if width > SELECTION_NODE_SIZE and height > SELECTION_NODE_SIZE:
            window.draw_rectangle(gc, True, x, y,
                                  SELECTION_NODE_SIZE, SELECTION_NODE_SIZE)
            window.draw_rectangle(gc, True, x, y + height -SELECTION_NODE_SIZE,
                                  SELECTION_NODE_SIZE, SELECTION_NODE_SIZE)
            window.draw_rectangle(gc, True, x + width - SELECTION_NODE_SIZE, y,
                                  SELECTION_NODE_SIZE, SELECTION_NODE_SIZE)
            window.draw_rectangle(gc, True, x + width - SELECTION_NODE_SIZE,
                                  y + height - SELECTION_NODE_SIZE,
                                  SELECTION_NODE_SIZE, SELECTION_NODE_SIZE)
        window.draw_rectangle(gc, False, x, y, width - 1, height - 1)

    def draw_border(self, x, y, width, height, color):
        gc = self._gc
        gc.set_rgb_fg_color(gdk.Color(int(color[0] * 65535.999),
                                      int(color[1] * 65535.999),
                                      int(color[2] * 65535.999)))
        
        self._window.draw_rectangle(gc, False,
                                    x - (BORDER_WIDTH / 2), 
                                    y - (BORDER_WIDTH / 2),
                                    width, height)

if use_cairo:
   Annotator = CairoAnnotator
else:
   Annotator = GdkAnnotator

