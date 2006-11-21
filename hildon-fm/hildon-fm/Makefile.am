
INCLUDES = $(GTK_CFLAGS) $(GCONF_CFLAGS) $(OSSO_THUMBNAIL_CFLAGS)	\
	   $(GWCONNECT_CFLAGS) $(OSSOIC_CFLAGS) $(DBUSGLIB_CFLAGS)	\
           $(LIBOSSO_CFLAGS)						\
           $(MCE_CFLAGS) -DLOCALEDIR=\"$(localedir)\"			\
           $(LIBOSSOMIME_CFLAGS) -I$(srcdir)/..				\
           -DHILDON_FM_HPANED

lib_LTLIBRARIES = libhildonfm.la


MAINTAINERCLEANFILES = \
	Makefile.in hildon-fm-enum-types.c hildon-fm-enum-types.h 

EXTRA_DIST = 

libhildonfm_la_LIBADD = $(GTK_LIBS) $(GCONF_LIBS) $(OSSO_THUMBNAIL_LIBS) \
	$(GWCONNECT_LIBS) $(DBUSGLIB_LIBS) $(LIBOSSOMIME_LIBS)

libhildonfm_la_LDFLAGS = -version-info 1:0:0
libhildonfm_la_SOURCES = \
	hildon-file-selection.c \
	hildon-file-selection.h \
	hildon-file-system-model.c \
	hildon-file-system-model.h \
	hildon-file-chooser-dialog.c \
	hildon-file-chooser-dialog.h \
	hildon-file-system-private.h \
	hildon-file-system-private.c \
	hildon-file-system-common.h \
	hildon-file-system-info.h \
	hildon-file-system-info.c \
	hildon-file-system-settings.h \
	hildon-file-system-settings.c \
	hildon-file-details-dialog.h \
	hildon-file-details-dialog.c \
	hildon-file-system-special-location.h \
	hildon-file-system-special-location.c \
	hildon-file-system-local-device.h \
	hildon-file-system-local-device.c \
	hildon-file-system-remote-device.h \
	hildon-file-system-remote-device.c \
	hildon-file-system-mmc.h \
	hildon-file-system-mmc.c \
	hildon-file-system-old-gateway.h \
	hildon-file-system-old-gateway.c \
	hildon-file-system-dynamic-device.h \
	hildon-file-system-dynamic-device.c \
	hildon-file-system-upnp.h \
	hildon-file-system-upnp.c \
	hildon-file-system-obex.h \
	hildon-file-system-obex.c \
 	$(hildonfm_built_headers) \
 	$(hildonfm_built_cfiles)

hildonwidgetsincludeinstdir=$(includedir)/hildon-fm/hildon-widgets
hildonwidgetsincludeinst_DATA = \
	hildon-file-selection.h \
	hildon-file-system-model.h \
	hildon-file-chooser-dialog.h \
	hildon-file-system-common.h \
	hildon-file-system-info.h \
	hildon-file-details-dialog.h


headers_to_scan_for_enums = $(hildonwidgetsincludeinst_DATA)
 
# Generate the enums source code, with glib-mkenums:
# This is based on the same Makefile.am stuff in pango:
hildonfm_built_headers = hildon-fm-enum-types.h
hildonfm_built_cfiles = hildon-fm-enum-types.c
 
# Don't build the library until we have built the header that it needs:
$(OBJECTS) $(libhildonfm_la_OBJECTS): $(hildonfm_built_headers)
 
hildon-fm-enum-types.h: @REBUILD@ $(headers_to_scan_for_enums) Makefile
	(cd $(srcdir) && $(GLIB_MKENUMS) \
			--fhead "#ifndef __HILDON_FM_ENUM_TYPES_H__\n#define __HILDON_FM_ENUM_TYPES_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void);\n#define HILDON_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" 	\
			--ftail "G_END_DECLS\n\n#endif /* __HILDON_FM_ENUM_TYPES_H__ */" \
		$(headers_to_scan_for_enums)) > $@

#FIXME: This would be shorter if there was a hildon-fm.h file:
hildon-fm-enum-types.c: @REBUILD@ $(headers_to_scan_for_enums) Makefile
	(cd $(srcdir) && $(GLIB_MKENUMS) \
			--fhead "#include \"hildon-file-chooser-dialog.h\"\n" \
			--fhead "#include \"hildon-file-details-dialog.h\"\n" \
			--fhead "#include \"hildon-file-selection.h\"\n" \
			--fhead "#include \"hildon-file-system-common.h\"\n" \
			--fhead "#include \"hildon-file-system-info.h\"\n" \
			--fhead "#include \"hildon-file-system-model.h\"\n" \
			--fhead "#include \"hildon-fm-enum-types.h\"\n" \
			--fhead "#include <glib-object.h>" \
		      	--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" 	\
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(headers_to_scan_for_enums)) > $@

