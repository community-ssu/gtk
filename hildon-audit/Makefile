
PROGRAM=hildon-audit

bindir=/usr/bin
datadir=/usr/share/$(PROGRAM)

all :
install :

	# program
	install -d $(DESTDIR)$(bindir)
	install -m755 $(PROGRAM) $(DESTDIR)$(bindir)/

	# symbol files
	install -d $(DESTDIR)$(datadir)
	install -d $(DESTDIR)$(datadir)/bora
	install -d $(DESTDIR)$(datadir)/sardine
	cp -fr data/bora/* $(DESTDIR)$(datadir)/bora
	cp -fr data/sardine/* $(DESTDIR)$(datadir)/sardine

clean :
	rm -f *~

