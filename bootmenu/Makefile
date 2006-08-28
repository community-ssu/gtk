all: evkey

clean:
	# We don't remove evkey here since we can't build it
        # while making the package

real-clean:
	rm -f evkey

evkey: evkey.c
	arm-linux-uclibc-cc -o evkey evkey.c

install:
	install -d $(DESTDIR)/usr/lib/bootmenu/
	install -m 755 evkey $(DESTDIR)/usr/lib/bootmenu/evkey
	install -m 755 bootmenu.sh $(DESTDIR)/usr/lib/bootmenu/bootmenu.sh
	install -d $(DESTDIR)/usr/sbin
	install -m 755 bootmenu-installer $(DESTDIR)/usr/sbin/bootmenu-installer
