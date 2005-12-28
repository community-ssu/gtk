languages := en_GB

CLEANFILES = $(foreach mo,$(languages),$(mo).mo)
EXTRA_DIST = $(foreach po,$(languages),$(po).po) README \
	debian/compat \
	debian/rules \
	debian/control\
	debian/hildon-fm-l10n-engb.install \
	POTFILES.in

%.mo: %.po
	msgfmt --statistics -v -o$(top_builddir)/po/$@ $(srcdir)/$<

all: $(foreach mo,$(languages),$(mo).mo)

merge:
	for mo in $(languages); do \
		xgettext -fPOTFILES --foreign-user -C -o$$mo.pot -k_; \
		msgmerge -v $$mo.po $$mo.pot >messages.tmp; \
		mv messages.tmp $$mo.po; \
		rm $$mo.pot; \
	done

install-data-local: all
	for mo in $(languages); do \
		$(mkinstalldirs) $(DESTDIR)$(localedir)/$$mo/LC_MESSAGES; \
		$(INSTALL_DATA) $(top_builddir)/po/$$mo.mo $(DESTDIR)$(localedir)/$$mo/LC_MESSAGES/$(PACKAGE).mo; \
	done

uninstall-local:
	for mo in $(languages); do \
		rm -f $(DESTDIR)$(localedir)/$$mo/LC_MESSAGES/$(PACKAGE).mo; \
	done
