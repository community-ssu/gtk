all: evkey

clean:
	rm -f evkey

evkey: evkey.c
	cc -o evkey evkey.c

install:
	install -d $(DESTDIR)/usr/bin
	install -m 755 evkey $(DESTDIR)/usr/bin/evkey
	install -d $(DESTDIR)/usr/sbin
	install -m 755 bootmenu $(DESTDIR)/usr/sbin/bootmenu

