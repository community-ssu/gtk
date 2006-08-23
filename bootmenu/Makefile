all: evkey

clean:
	rm -f evkey

evkey: evkey.c
	cc -o evkey evkey.c

install:
	install -d $(DESTDIR)/bin
	install -m 755 evkey $(DESTDIR)/bin/evkey
	install -d $(DESTDIR)/sbin
	install -m 755 bootmenu $(DESTDIR)/sbin/bootmenu
