THEMENAME = maemo-default

THEMEMAKEFILE = $(MAEMO_THEME_TOOLS_DATADIR)/themeMakefile.mk
THEMEDIR = themes/$(THEMENAME)
COMMONRCDIR = $(MAEMO_THEME_TOOLS_COMMONRCDIR)

nobase_data_DATA = $(THEMEDIR)/gtk-2.0/gtkrc.cache \
				   $(THEMEDIR)/gtk-2.0/gtkrc \
				   $(THEMEDIR)/gtk-2.0/gtkrc.maemo_af_desktop.cache \
				   $(THEMEDIR)/gtk-2.0/gtkrc.maemo_af_desktop \
				   $(THEMEDIR)/matchbox/theme.xml \
				   $(THEMEDIR)/index.theme


EXTRA_DIST = themespecificgtk theme.xml index.theme

include $(THEMEMAKEFILE)

# Hack, default theme currently has no transparent pixmaps, which confuses sapwood
$(THEMEDIR)/gtk-2.0/gtkrc::
	sed -i 's/^\W*shaped = TRUE$$//'  $(srcdir)/$(THEMEDIR)/gtk-2.0/gtkrc

$(THEMEDIR)/gtk-2.0/gtkrc.maemo_af_desktop::
	sed -i 's/^\W*shaped *= *TRUE$$//'  $(srcdir)/$(THEMEDIR)/gtk-2.0/gtkrc.maemo_af_desktop
