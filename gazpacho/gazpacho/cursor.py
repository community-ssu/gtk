# Copyright (C) 2004,2005 by SICEm S.L.
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

import gtk

from gazpacho.environ import environ

class Cursor(object):
    plus = gtk.gdk.pixbuf_new_from_file(environ.find_pixmap('plus.png'))
    hotspot = (6, 6)
    offset = (8, 8)
    size = (32, 32)
    fallback = gtk.gdk.Cursor(gtk.gdk.PLUS)

    def create(pixbuf):
        display = gtk.gdk.display_get_default()
        if not pixbuf or not display.supports_cursor_color():
            # On Windows, the patch from Bug #306101 must be applied to
            # enable cursor color support (and alpha support on XP).
            return Cursor.fallback

        tmp = gtk.gdk.Pixbuf(gtk.gdk.COLORSPACE_RGB, True, 8, *Cursor.size)
        tmp.fill(0)
        Cursor.plus.composite(tmp, 0, 0,
                              Cursor.plus.get_width(),
                              Cursor.plus.get_height(),
                              0, 0, 1, 1, gtk.gdk.INTERP_NEAREST, 255)
        x, y = Cursor.offset
        w, h = pixbuf.get_width(), pixbuf.get_height()
        if x + w >= Cursor.size[0]:
            x = Cursor.size[0] - w
        if y + h >= Cursor.size[1]:
            y = Cursor.size[1] - h
            
        pixbuf.composite(tmp,
                         x, y, w, h,
                         x, y, 1, 1,
                         gtk.gdk.INTERP_NEAREST, 255)
        
        return gtk.gdk.Cursor(display,
                              tmp, *Cursor.hotspot)
    create = staticmethod(create)

    def set_for_widget_adaptor(window, widget_adaptor):
        if not widget_adaptor:
            cursor = None
        else:
            cursor = widget_adaptor.cursor
        window.set_cursor(cursor)
    set_for_widget_adaptor = staticmethod(set_for_widget_adaptor)
