/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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

#define GDK_PIXBUF_ENABLE_BACKEND

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixbuf-animation.h>
#include "hildon-pixbuf-anim-blinker.h"

struct _HildonPixbufAnimBlinker
{
        GdkPixbufAnimation parent_instance;

	gint period;
	gint length;
	
	GdkPixbuf *pixbuf;
	GdkPixbuf *empty;
};

struct _HildonPixbufAnimBlinkerClass
{
        GdkPixbufAnimationClass parent_class;
};


typedef struct _HildonPixbufAnimBlinkerIter HildonPixbufAnimBlinkerIter;
typedef struct _HildonPixbufAnimBlinkerIterClass HildonPixbufAnimBlinkerIterClass;

#define TYPE_HILDON_PIXBUF_ANIM_BLINKER_ITER              (hildon_pixbuf_anim_blinker_iter_get_type ())
#define HILDON_PIXBUF_ANIM_BLINKER_ITER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), TYPE_HILDON_PIXBUF_ANIM_BLINKER_ITER, HildonPixbufAnimBlinkerIter))
#define IS_HILDON_PIXBUF_ANIM_BLINKER_ITER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), TYPE_HILDON_PIXBUF_ANIM_BLINKER_ITER))

#define HILDON_PIXBUF_ANIM_BLINKER_ITER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_HILDON_PIXBUF_ANIM_BLINKER_ITER, HildonPixbufAnimBlinkerIterClass))
#define IS_HILDON_PIXBUF_ANIM_BLINKER_ITER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_HILDON_PIXBUF_ANIM_BLINKER_ITER))
#define HILDON_PIXBUF_ANIM_BLINKER_ITER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_HILDON_PIXBUF_ANIM_BLINKER_ITER, HildonPixbufAnimBlinkerIterClass))

GType hildon_pixbuf_anim_blinker_iter_get_type (void) G_GNUC_CONST;


struct _HildonPixbufAnimBlinkerIterClass
{
        GdkPixbufAnimationIterClass parent_class;
};

struct _HildonPixbufAnimBlinkerIter
{
        GdkPixbufAnimationIter parent_instance;
        
        HildonPixbufAnimBlinker *hildon_pixbuf_anim_blinker;
        
        GTimeVal start_time;
        GTimeVal current_time;
};

static void hildon_pixbuf_anim_blinker_finalize (GObject *object);

static gboolean   is_static_image  (GdkPixbufAnimation *animation);
static GdkPixbuf *get_static_image (GdkPixbufAnimation *animation);

static void       get_size         (GdkPixbufAnimation *anim,
                                    gint               *width, 
                                    gint               *height);
static GdkPixbufAnimationIter *get_iter (GdkPixbufAnimation *anim,
                                         const GTimeVal     *start_time);


G_DEFINE_TYPE(HildonPixbufAnimBlinker, hildon_pixbuf_anim_blinker, GDK_TYPE_PIXBUF_ANIMATION);

static void
hildon_pixbuf_anim_blinker_init (HildonPixbufAnimBlinker *anim)
{
}

static void
hildon_pixbuf_anim_blinker_class_init (HildonPixbufAnimBlinkerClass *klass)
{
        GObjectClass *object_class;
        GdkPixbufAnimationClass *anim_class;

        object_class = G_OBJECT_CLASS (klass);
        anim_class = GDK_PIXBUF_ANIMATION_CLASS (klass);
        
        object_class->finalize = hildon_pixbuf_anim_blinker_finalize;
        
        anim_class->is_static_image = is_static_image;
        anim_class->get_static_image = get_static_image;
        anim_class->get_size = get_size;
        anim_class->get_iter = get_iter;
}

static void
hildon_pixbuf_anim_blinker_finalize (GObject *object)
{
        HildonPixbufAnimBlinker *anim;
        
        anim = HILDON_PIXBUF_ANIM_BLINKER (object);        
        
        g_object_unref (anim->pixbuf);
        
        G_OBJECT_CLASS (hildon_pixbuf_anim_blinker_parent_class)->finalize (object);
}

static gboolean
is_static_image (GdkPixbufAnimation *animation)
{
        return FALSE;
}

static GdkPixbuf *
get_static_image (GdkPixbufAnimation *animation)
{
        HildonPixbufAnimBlinker *anim;
        
        anim = HILDON_PIXBUF_ANIM_BLINKER (animation);
 
 	return anim->pixbuf;       
}

static void
get_size (GdkPixbufAnimation *animation,
          gint               *width, 
          gint               *height)
{
        HildonPixbufAnimBlinker *anim;

        anim = HILDON_PIXBUF_ANIM_BLINKER (animation);
        
        if (width)
        	*width = gdk_pixbuf_get_width (anim->pixbuf);
        if (height)
        	*height = gdk_pixbuf_get_height (anim->pixbuf);
}

static GdkPixbufAnimationIter *
get_iter (GdkPixbufAnimation *anim,
          const GTimeVal    *start_time)
{
        HildonPixbufAnimBlinkerIter *iter;
        
        iter = g_object_new (TYPE_HILDON_PIXBUF_ANIM_BLINKER_ITER, NULL);

        iter->hildon_pixbuf_anim_blinker = HILDON_PIXBUF_ANIM_BLINKER (anim);

        g_object_ref (iter->hildon_pixbuf_anim_blinker);
        
        iter->start_time = *start_time;
        iter->current_time = *start_time;
        
        return GDK_PIXBUF_ANIMATION_ITER (iter);
}

static void hildon_pixbuf_anim_blinker_iter_finalize (GObject *object);

static gint       get_delay_time             (GdkPixbufAnimationIter *iter);
static GdkPixbuf *get_pixbuf                 (GdkPixbufAnimationIter *iter);
static gboolean   on_currently_loading_frame (GdkPixbufAnimationIter *iter);
static gboolean   advance                    (GdkPixbufAnimationIter *iter,
                                              const GTimeVal         *current_time);

G_DEFINE_TYPE (HildonPixbufAnimBlinkerIter, hildon_pixbuf_anim_blinker_iter, GDK_TYPE_PIXBUF_ANIMATION_ITER);

static void
hildon_pixbuf_anim_blinker_iter_init (HildonPixbufAnimBlinkerIter *iter)
{
}

static void
hildon_pixbuf_anim_blinker_iter_class_init (HildonPixbufAnimBlinkerIterClass *klass)
{
        GObjectClass *object_class;
        GdkPixbufAnimationIterClass *anim_iter_class;

        object_class = G_OBJECT_CLASS (klass);
        anim_iter_class = GDK_PIXBUF_ANIMATION_ITER_CLASS (klass);
        
        object_class->finalize = hildon_pixbuf_anim_blinker_iter_finalize;
        
        anim_iter_class->get_delay_time = get_delay_time;
        anim_iter_class->get_pixbuf = get_pixbuf;
        anim_iter_class->on_currently_loading_frame = on_currently_loading_frame;
        anim_iter_class->advance = advance;
}

static void
hildon_pixbuf_anim_blinker_iter_finalize (GObject *object)
{
        HildonPixbufAnimBlinkerIter *iter;
        
        iter = HILDON_PIXBUF_ANIM_BLINKER_ITER (object);
        
        g_object_unref (iter->hildon_pixbuf_anim_blinker);
        
        G_OBJECT_CLASS (hildon_pixbuf_anim_blinker_iter_parent_class)->finalize (object);
}

static gboolean
advance (GdkPixbufAnimationIter *anim_iter,
         const GTimeVal         *current_time)
{
        HildonPixbufAnimBlinkerIter *iter;
        gint elapsed;
        
        iter = HILDON_PIXBUF_ANIM_BLINKER_ITER (anim_iter);
        
        iter->current_time = *current_time;
        
        if (iter->hildon_pixbuf_anim_blinker->length == -1)
        	return TRUE;
        
        /* We use milliseconds for all times */
        elapsed = (((iter->current_time.tv_sec - iter->start_time.tv_sec) * G_USEC_PER_SEC +
                    iter->current_time.tv_usec - iter->start_time.tv_usec)) / 1000;
        
        if (elapsed < 0) {
                /* Try to compensate; probably the system clock
                 * was set backwards
                 */
                iter->start_time = iter->current_time;
                elapsed = 0;
        }

	if (elapsed < iter->hildon_pixbuf_anim_blinker->length)
		return TRUE;
	else
		return FALSE;
}

static gint
get_delay_time (GdkPixbufAnimationIter *anim_iter)
{
        HildonPixbufAnimBlinkerIter *iter;
        gint elapsed;

        iter = HILDON_PIXBUF_ANIM_BLINKER_ITER (anim_iter);
        
        elapsed = (((iter->current_time.tv_sec - iter->start_time.tv_sec) * G_USEC_PER_SEC +
                    iter->current_time.tv_usec - iter->start_time.tv_usec)) / 1000;

	if ((elapsed < iter->hildon_pixbuf_anim_blinker->length) || (iter->hildon_pixbuf_anim_blinker->length == -1))
		return iter->hildon_pixbuf_anim_blinker->period - (elapsed % iter->hildon_pixbuf_anim_blinker->period);
	else
		return -1;
}

static GdkPixbuf *
get_pixbuf (GdkPixbufAnimationIter *anim_iter)
{
        HildonPixbufAnimBlinkerIter *iter;
        gint elapsed;
        
        iter = HILDON_PIXBUF_ANIM_BLINKER_ITER (anim_iter);
        
        elapsed = (((iter->current_time.tv_sec - iter->start_time.tv_sec) * G_USEC_PER_SEC +
                    iter->current_time.tv_usec - iter->start_time.tv_usec)) / 1000;
	
	if ((elapsed > iter->hildon_pixbuf_anim_blinker->length && iter->hildon_pixbuf_anim_blinker->length != -1) ||
	    ((elapsed / iter->hildon_pixbuf_anim_blinker->period) % 2))
		return iter->hildon_pixbuf_anim_blinker->pixbuf;
	else
		return iter->hildon_pixbuf_anim_blinker->empty;
}

static gboolean
on_currently_loading_frame (GdkPixbufAnimationIter *anim_iter)
{
	return FALSE;
}


/* vals in millisecs, length = -1 for infinity */
HildonPixbufAnimBlinker *
hildon_pixbuf_anim_blinker_new (GdkPixbuf *pixbuf, gint period, gint length)
{
  HildonPixbufAnimBlinker *anim;

  anim = g_object_new (TYPE_HILDON_PIXBUF_ANIM_BLINKER, NULL);
  anim->pixbuf = g_object_ref (pixbuf);
  anim->period = period;
  anim->length = length;
  anim->empty = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
  	gdk_pixbuf_get_width (pixbuf), gdk_pixbuf_get_height (pixbuf));
  gdk_pixbuf_fill (anim->empty, 0x00000000);

  return anim;
}
