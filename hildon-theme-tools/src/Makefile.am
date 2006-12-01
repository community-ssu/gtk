bin_PROGRAMS 				= hildon-theme-slicer 		\
					  hildon-theme-colourizer 	\
					  hildon-theme-regenerator 	\
					  hildon-theme-outliner		

# Slicer
hildon_theme_slicer_CFLAGS 		= $(GTK_CFLAGS)
hildon_theme_slicer_LDADD 		= $(GTK_LIBS)
hildon_theme_slicer_SOURCES 		= slicer.c common.c

# Colourizer
hildon_theme_colourizer_CFLAGS 		= $(GTK_CFLAGS)
hildon_theme_colourizer_LDADD 		= $(GTK_LIBS)
hildon_theme_colourizer_SOURCES	 	= colourizer.c common.c

# Regenerator
hildon_theme_regenerator_CFLAGS 	= $(GTK_CFLAGS)
hildon_theme_regenerator_LDADD 		= $(GTK_LIBS)
hildon_theme_regenerator_SOURCES 	= regenerator.c common.c

# Outliner
hildon_theme_outliner_CFLAGS 		= $(GTK_CFLAGS)
hildon_theme_outliner_LDADD 		= $(GTK_LIBS)
hildon_theme_outliner_SOURCES 		= outliner.c common.c

MAINTAINERCLEANFILES 			= Makefile.in

noinst_HEADERS				= common.h 			\
					  colourizer.h			\
					  regenerator.h			\
					  outliner.h			\
					  slicer.h
