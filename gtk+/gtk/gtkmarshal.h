#ifndef GTK_DISABLE_DEPRECATED

#ifndef __gtk_marshal_MARSHAL_H__
#define __gtk_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* BOOL:NONE (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:1) */
extern void gtk_marshal_BOOLEAN__VOID (GClosure     *closure,
                                       GValue       *return_value,
                                       guint         n_param_values,
                                       const GValue *param_values,
                                       gpointer      invocation_hint,
                                       gpointer      marshal_data);
#define gtk_marshal_BOOL__NONE	gtk_marshal_BOOLEAN__VOID

/* BOOL:POINTER (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:2) */
extern void gtk_marshal_BOOLEAN__POINTER (GClosure     *closure,
                                          GValue       *return_value,
                                          guint         n_param_values,
                                          const GValue *param_values,
                                          gpointer      invocation_hint,
                                          gpointer      marshal_data);
#define gtk_marshal_BOOL__POINTER	gtk_marshal_BOOLEAN__POINTER

/* BOOL:POINTER,POINTER,INT,INT (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:3) */
extern void gtk_marshal_BOOLEAN__POINTER_POINTER_INT_INT (GClosure     *closure,
                                                          GValue       *return_value,
                                                          guint         n_param_values,
                                                          const GValue *param_values,
                                                          gpointer      invocation_hint,
                                                          gpointer      marshal_data);
#define gtk_marshal_BOOL__POINTER_POINTER_INT_INT	gtk_marshal_BOOLEAN__POINTER_POINTER_INT_INT

/* BOOL:POINTER,INT,INT (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:4) */
extern void gtk_marshal_BOOLEAN__POINTER_INT_INT (GClosure     *closure,
                                                  GValue       *return_value,
                                                  guint         n_param_values,
                                                  const GValue *param_values,
                                                  gpointer      invocation_hint,
                                                  gpointer      marshal_data);
#define gtk_marshal_BOOL__POINTER_INT_INT	gtk_marshal_BOOLEAN__POINTER_INT_INT

/* BOOL:POINTER,INT,INT,UINT (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:5) */
extern void gtk_marshal_BOOLEAN__POINTER_INT_INT_UINT (GClosure     *closure,
                                                       GValue       *return_value,
                                                       guint         n_param_values,
                                                       const GValue *param_values,
                                                       gpointer      invocation_hint,
                                                       gpointer      marshal_data);
#define gtk_marshal_BOOL__POINTER_INT_INT_UINT	gtk_marshal_BOOLEAN__POINTER_INT_INT_UINT

/* BOOL:POINTER,STRING,STRING,POINTER (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:6) */
extern void gtk_marshal_BOOLEAN__POINTER_STRING_STRING_POINTER (GClosure     *closure,
                                                                GValue       *return_value,
                                                                guint         n_param_values,
                                                                const GValue *param_values,
                                                                gpointer      invocation_hint,
                                                                gpointer      marshal_data);
#define gtk_marshal_BOOL__POINTER_STRING_STRING_POINTER	gtk_marshal_BOOLEAN__POINTER_STRING_STRING_POINTER

/* ENUM:ENUM (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:7) */
extern void gtk_marshal_ENUM__ENUM (GClosure     *closure,
                                    GValue       *return_value,
                                    guint         n_param_values,
                                    const GValue *param_values,
                                    gpointer      invocation_hint,
                                    gpointer      marshal_data);

/* INT:POINTER (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:8) */
extern void gtk_marshal_INT__POINTER (GClosure     *closure,
                                      GValue       *return_value,
                                      guint         n_param_values,
                                      const GValue *param_values,
                                      gpointer      invocation_hint,
                                      gpointer      marshal_data);

/* INT:POINTER,CHAR,CHAR (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:9) */
extern void gtk_marshal_INT__POINTER_CHAR_CHAR (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data);

/* NONE:BOOL (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:10) */
#define gtk_marshal_VOID__BOOLEAN	g_cclosure_marshal_VOID__BOOLEAN
#define gtk_marshal_NONE__BOOL	gtk_marshal_VOID__BOOLEAN

/* NONE:BOXED (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:11) */
#define gtk_marshal_VOID__BOXED	g_cclosure_marshal_VOID__BOXED
#define gtk_marshal_NONE__BOXED	gtk_marshal_VOID__BOXED

/* NONE:ENUM (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:12) */
#define gtk_marshal_VOID__ENUM	g_cclosure_marshal_VOID__ENUM
#define gtk_marshal_NONE__ENUM	gtk_marshal_VOID__ENUM

/* NONE:ENUM,FLOAT (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:13) */
extern void gtk_marshal_VOID__ENUM_FLOAT (GClosure     *closure,
                                          GValue       *return_value,
                                          guint         n_param_values,
                                          const GValue *param_values,
                                          gpointer      invocation_hint,
                                          gpointer      marshal_data);
#define gtk_marshal_NONE__ENUM_FLOAT	gtk_marshal_VOID__ENUM_FLOAT

/* NONE:ENUM,FLOAT,BOOL (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:14) */
extern void gtk_marshal_VOID__ENUM_FLOAT_BOOLEAN (GClosure     *closure,
                                                  GValue       *return_value,
                                                  guint         n_param_values,
                                                  const GValue *param_values,
                                                  gpointer      invocation_hint,
                                                  gpointer      marshal_data);
#define gtk_marshal_NONE__ENUM_FLOAT_BOOL	gtk_marshal_VOID__ENUM_FLOAT_BOOLEAN

/* NONE:INT (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:15) */
#define gtk_marshal_VOID__INT	g_cclosure_marshal_VOID__INT
#define gtk_marshal_NONE__INT	gtk_marshal_VOID__INT

/* NONE:INT,INT (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:16) */
extern void gtk_marshal_VOID__INT_INT (GClosure     *closure,
                                       GValue       *return_value,
                                       guint         n_param_values,
                                       const GValue *param_values,
                                       gpointer      invocation_hint,
                                       gpointer      marshal_data);
#define gtk_marshal_NONE__INT_INT	gtk_marshal_VOID__INT_INT

/* NONE:INT,INT,POINTER (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:17) */
extern void gtk_marshal_VOID__INT_INT_POINTER (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);
#define gtk_marshal_NONE__INT_INT_POINTER	gtk_marshal_VOID__INT_INT_POINTER

/* NONE:NONE (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:18) */
#define gtk_marshal_VOID__VOID	g_cclosure_marshal_VOID__VOID
#define gtk_marshal_NONE__NONE	gtk_marshal_VOID__VOID

/* NONE:OBJECT (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:19) */
#define gtk_marshal_VOID__OBJECT	g_cclosure_marshal_VOID__OBJECT
#define gtk_marshal_NONE__OBJECT	gtk_marshal_VOID__OBJECT

/* NONE:POINTER (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:20) */
#define gtk_marshal_VOID__POINTER	g_cclosure_marshal_VOID__POINTER
#define gtk_marshal_NONE__POINTER	gtk_marshal_VOID__POINTER

/* NONE:POINTER,INT (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:21) */
extern void gtk_marshal_VOID__POINTER_INT (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data);
#define gtk_marshal_NONE__POINTER_INT	gtk_marshal_VOID__POINTER_INT

/* NONE:POINTER,POINTER (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:22) */
extern void gtk_marshal_VOID__POINTER_POINTER (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);
#define gtk_marshal_NONE__POINTER_POINTER	gtk_marshal_VOID__POINTER_POINTER

/* NONE:POINTER,POINTER,POINTER (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:23) */
extern void gtk_marshal_VOID__POINTER_POINTER_POINTER (GClosure     *closure,
                                                       GValue       *return_value,
                                                       guint         n_param_values,
                                                       const GValue *param_values,
                                                       gpointer      invocation_hint,
                                                       gpointer      marshal_data);
#define gtk_marshal_NONE__POINTER_POINTER_POINTER	gtk_marshal_VOID__POINTER_POINTER_POINTER

/* NONE:POINTER,STRING,STRING (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:24) */
extern void gtk_marshal_VOID__POINTER_STRING_STRING (GClosure     *closure,
                                                     GValue       *return_value,
                                                     guint         n_param_values,
                                                     const GValue *param_values,
                                                     gpointer      invocation_hint,
                                                     gpointer      marshal_data);
#define gtk_marshal_NONE__POINTER_STRING_STRING	gtk_marshal_VOID__POINTER_STRING_STRING

/* NONE:POINTER,UINT (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:25) */
extern void gtk_marshal_VOID__POINTER_UINT (GClosure     *closure,
                                            GValue       *return_value,
                                            guint         n_param_values,
                                            const GValue *param_values,
                                            gpointer      invocation_hint,
                                            gpointer      marshal_data);
#define gtk_marshal_NONE__POINTER_UINT	gtk_marshal_VOID__POINTER_UINT

/* NONE:POINTER,UINT,ENUM (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:26) */
extern void gtk_marshal_VOID__POINTER_UINT_ENUM (GClosure     *closure,
                                                 GValue       *return_value,
                                                 guint         n_param_values,
                                                 const GValue *param_values,
                                                 gpointer      invocation_hint,
                                                 gpointer      marshal_data);
#define gtk_marshal_NONE__POINTER_UINT_ENUM	gtk_marshal_VOID__POINTER_UINT_ENUM

/* NONE:POINTER,POINTER,UINT,UINT (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:27) */
extern void gtk_marshal_VOID__POINTER_POINTER_UINT_UINT (GClosure     *closure,
                                                         GValue       *return_value,
                                                         guint         n_param_values,
                                                         const GValue *param_values,
                                                         gpointer      invocation_hint,
                                                         gpointer      marshal_data);
#define gtk_marshal_NONE__POINTER_POINTER_UINT_UINT	gtk_marshal_VOID__POINTER_POINTER_UINT_UINT

/* NONE:POINTER,INT,INT,POINTER,UINT,UINT (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:28) */
extern void gtk_marshal_VOID__POINTER_INT_INT_POINTER_UINT_UINT (GClosure     *closure,
                                                                 GValue       *return_value,
                                                                 guint         n_param_values,
                                                                 const GValue *param_values,
                                                                 gpointer      invocation_hint,
                                                                 gpointer      marshal_data);
#define gtk_marshal_NONE__POINTER_INT_INT_POINTER_UINT_UINT	gtk_marshal_VOID__POINTER_INT_INT_POINTER_UINT_UINT

/* NONE:POINTER,UINT,UINT (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:29) */
extern void gtk_marshal_VOID__POINTER_UINT_UINT (GClosure     *closure,
                                                 GValue       *return_value,
                                                 guint         n_param_values,
                                                 const GValue *param_values,
                                                 gpointer      invocation_hint,
                                                 gpointer      marshal_data);
#define gtk_marshal_NONE__POINTER_UINT_UINT	gtk_marshal_VOID__POINTER_UINT_UINT

/* NONE:POINTER,UINT,UINT (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:30) */

/* NONE:STRING (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:31) */
#define gtk_marshal_VOID__STRING	g_cclosure_marshal_VOID__STRING
#define gtk_marshal_NONE__STRING	gtk_marshal_VOID__STRING

/* NONE:STRING,INT,POINTER (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:32) */
extern void gtk_marshal_VOID__STRING_INT_POINTER (GClosure     *closure,
                                                  GValue       *return_value,
                                                  guint         n_param_values,
                                                  const GValue *param_values,
                                                  gpointer      invocation_hint,
                                                  gpointer      marshal_data);
#define gtk_marshal_NONE__STRING_INT_POINTER	gtk_marshal_VOID__STRING_INT_POINTER

/* NONE:UINT (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:33) */
#define gtk_marshal_VOID__UINT	g_cclosure_marshal_VOID__UINT
#define gtk_marshal_NONE__UINT	gtk_marshal_VOID__UINT

/* NONE:UINT,POINTER,UINT,ENUM,ENUM,POINTER (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:34) */
extern void gtk_marshal_VOID__UINT_POINTER_UINT_ENUM_ENUM_POINTER (GClosure     *closure,
                                                                   GValue       *return_value,
                                                                   guint         n_param_values,
                                                                   const GValue *param_values,
                                                                   gpointer      invocation_hint,
                                                                   gpointer      marshal_data);
#define gtk_marshal_NONE__UINT_POINTER_UINT_ENUM_ENUM_POINTER	gtk_marshal_VOID__UINT_POINTER_UINT_ENUM_ENUM_POINTER

/* NONE:UINT,POINTER,UINT,UINT,ENUM (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:35) */
extern void gtk_marshal_VOID__UINT_POINTER_UINT_UINT_ENUM (GClosure     *closure,
                                                           GValue       *return_value,
                                                           guint         n_param_values,
                                                           const GValue *param_values,
                                                           gpointer      invocation_hint,
                                                           gpointer      marshal_data);
#define gtk_marshal_NONE__UINT_POINTER_UINT_UINT_ENUM	gtk_marshal_VOID__UINT_POINTER_UINT_UINT_ENUM

/* NONE:UINT,STRING (/home/jlehto/teema4/3rdparty/gtk+2.0-2.6/gtk/gtkmarshal.list:36) */
extern void gtk_marshal_VOID__UINT_STRING (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data);
#define gtk_marshal_NONE__UINT_STRING	gtk_marshal_VOID__UINT_STRING

G_END_DECLS

#endif /* __gtk_marshal_MARSHAL_H__ */

#endif /* GTK_DISABLE_DEPRECATED */
