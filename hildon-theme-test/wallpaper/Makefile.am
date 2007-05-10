MAINTAINERCLEANFILES 		= Makefile.in

EXTRA_DIST 			= wallpaper.png theme_bg.desktop

install: 
	$(mkinstalldirs) $(DESTDIR)/$(datadir)/backgrounds
	install -c -m 644 wallpaper.png $(DESTDIR)/$(datadir)/backgrounds/$(THEME_DIR).png
	install -c -m 644 theme_bg.desktop $(DESTDIR)/$(datadir)/backgrounds/$(THEME_DIR).desktop

