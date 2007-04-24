/*
 * Copyright (C) 2005, 2006, 2007 Nokia Corporation.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
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

#include <string.h>

#include <hildon-thumbnail-factory.h>
#include <hildon-thumber-common.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixbuf-io.h>

#include <id3.h>

void get_frame(ID3Tag *tag, int id, char buf[1024])
{
    ID3Frame *frame;

    if ((frame = ID3Tag_FindFrameWithID(tag, id)) != NULL) {
      ID3Field *field;
      if ((field = ID3Frame_GetField(frame, ID3FN_TEXT)) != NULL) {
        (void) ID3Field_GetASCII(field, buf, 1024);
      } else {
        strcpy(buf, "");
      }
    } else {
        strcpy(buf, "");
    }
}

#define N_PAIRS 5

#define SET_PAIR(name, val) \
    g_assert(i < N_PAIRS); \
    if(val && val[0] != '\0') { \
	keys[i] = g_strdup(HILDON_THUMBNAIL_OPTION_PREFIX name); \
	values[i] = g_strdup(val); \
	i++; \
    }

GdkPixbuf *create_thumb(const gchar *local_file, const gchar *mime_type,
    guint width, guint height, HildonThumbnailFlags flags,
    gchar ***opt_keys, gchar ***opt_values, GError **error)
{
    GdkPixbuf *pixbuf = hildon_thumber_create_empty_pixbuf();

    char **keys, **values;
    int i = 0;

    ID3Tag *tag;
    char buf[1024];

    if((tag = ID3Tag_New()) != NULL) {
        ID3Tag_Link(tag, local_file);

        keys = g_new0(char *, N_PAIRS + 1);
        values = g_new0(char *, N_PAIRS + 1);


        SET_PAIR("MP3", "1");
        SET_PAIR("Noimage", "1");
        
        get_frame(tag, ID3FID_TITLE, buf);
        SET_PAIR("Title", buf);

        get_frame(tag, ID3FID_LEADARTIST, buf);
        SET_PAIR("Artist", buf);

        get_frame(tag, ID3FID_ALBUM, buf);
        SET_PAIR("Album", buf);

        *opt_keys = keys;
        *opt_values = values;
    }

    return pixbuf;
}

int main(int argc, char **argv)
{
    return hildon_thumber_main(&argc, &argv, create_thumb);
}
