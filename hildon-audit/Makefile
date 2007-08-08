
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
	install -d $(DESTDIR)$(datadir)/chinook
	cp -fr data/bora/* $(DESTDIR)$(datadir)/bora
	cp -fr data/chinook/* $(DESTDIR)$(datadir)/chinook

clean :
	rm -f *~

