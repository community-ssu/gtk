#ifndef __GTK_TEXT_MARK_PRIVATE_H__
#define __GTK_TEXT_MARK_PRIVATE_H__

#include <gtk/gtktexttypes.h>
#include <gtk/gtktextlayout.h>

G_BEGIN_DECLS

#define GTK_IS_TEXT_MARK_SEGMENT(mark) (((GtkTextLineSegment*)mark)->type == &gtk_text_left_mark_type || \
                                ((GtkTextLineSegment*)mark)->type == &gtk_text_right_mark_type)

/*
 * The data structure below defines line segments that represent
 * marks.  There is one of these for each mark in the text.
 */

struct _GtkTextMarkBody {
  GtkTextMark *obj;
  gchar *name;
  GtkTextBTree *tree;
  GtkTextLine *line;
  guint visible : 1;
  guint not_deleteable : 1;
};

GtkTextLineSegment *_gtk_mark_segment_new   (GtkTextBTree       *tree,
                                             gboolean            left_gravity,
                                             const gchar        *name);

G_END_DECLS

#endif



