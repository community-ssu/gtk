install:
	$(mkinstalldirs) $(DESTDIR)$(datadir)/themes/maemo-default
	maemo-theme-install-3					\
            --name "Default Theme"				\
            --template $(srcdir)/theme-images-template.png	\
            --theme $(datadir)/themes/maemo-default/		\
            --destdir $(DESTDIR)

EXTRA_DIST = theme-images-template.png
