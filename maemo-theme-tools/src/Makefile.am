bin_PROGRAMS 				= hildon-theme-slicer 		\
					  hildon-theme-colourizer 	\
					  hildon-theme-regenerator 	\
					  hildon-theme-outliner 	\
					  hildon-theme-cacher

# Slicer
hildon_theme_slicer_CFLAGS 		= $(GTK_CFLAGS) -DEXEC_NAME=\"hildon-theme-slicer\"
hildon_theme_slicer_LDADD 		= $(GTK_LIBS)
hildon_theme_slicer_SOURCES 		= slicer.c common.c

# Colourizer
hildon_theme_colourizer_CFLAGS 		= $(GTK_CFLAGS) -DEXEC_NAME=\"hildon-theme-colourizer\"
hildon_theme_colourizer_LDADD 		= $(GTK_LIBS)
hildon_theme_colourizer_SOURCES	 	= colourizer.c common.c

# Regenerator
hildon_theme_regenerator_CFLAGS 	= $(GTK_CFLAGS) -DEXEC_NAME=\"hildon-theme-regenerator\"
hildon_theme_regenerator_LDADD 		= $(GTK_LIBS)
hildon_theme_regenerator_SOURCES 	= regenerator.c common.c

# Outliner
hildon_theme_outliner_CFLAGS 		= $(GTK_CFLAGS) -DEXEC_NAME=\"hildon-theme-outliner\"
hildon_theme_outliner_LDADD 		= $(GTK_LIBS)
hildon_theme_outliner_SOURCES 		= outliner.c common.c

# Cacher
# FIXME: Add "EXEC_NAME" here too
hildon_theme_cacher_CFLAGS 		= $(GTK_CFLAGS)
hildon_theme_cacher_LDADD 		= $(GTK_LIBS)
hildon_theme_cacher_SOURCES 		= cacher.c

MAINTAINERCLEANFILES 			= Makefile.in
