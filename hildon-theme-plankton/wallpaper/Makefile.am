MAINTAINERCLEANFILES 		= Makefile.in

EXTRA_DIST 			= wallpaper.png theme_bg.desktop

install: 
	$(mkinstalldirs) $(DESTDIR)/usr/share/backgrounds
	install -c -m 644 wallpaper.png $(DESTDIR)/usr/share/backgrounds/$(THEME_DIR).png
	install -c -m 644 theme_bg.desktop $(DESTDIR)/usr/share/backgrounds/$(THEME_DIR)_bg.desktop

