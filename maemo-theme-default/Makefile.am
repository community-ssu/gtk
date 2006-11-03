SUBDIRS = template

install:
	$(mkinstalldirs) $(DESTDIR)$(datadir)/themes/$(THEME_DIR)/
	hildon-theme-install-3					\
            --name $(THEME_NAME)				\
            --template $(srcdir)/template/template.png		\
            --theme $(datadir)/themes/$(THEME_DIR)/		\
            --destdir $(DESTDIR)

MAINTAINERCLEANFILES	= Makefile.in aclocal.m4 configure \
			  config.h.in compile \
			  ltmain.sh depcomp missing \
			  install-sh config.sub \
			  config.guess intltool-extract.in \
			  intltool-merge.in intltool-update.in  \
			  mkinstalldirs build-stamp install-stamp configure-stamp \
			  config.status 

