
CFLAGS = -Wall -Wchar-subscripts -Wmissing-declarations \
	-Wnested-externs -Wpointer-arith -Wcast-align -Wsign-compare

CFLAGS += -DDEBUG -g

INSTALL = install
DESTDIR =
BINDIR = $(DESTDIR)/usr/sbin
DBUSDIR = $(DESTDIR)/etc/dbus-1/system.d

all: kdbusd

kdbusd: kdbusd.c
	gcc $(CFLAGS) -o kdbusd kdbusd.c `pkg-config --cflags --libs dbus-1`

clean:
	-rm -f kdbusd

distclean: clean

install: all
	$(INSTALL) -d $(BINDIR) $(DBUSDIR)
	$(INSTALL) -s kdbusd $(BINDIR)
	$(INSTALL) -m644 kernel_sysbus_policy.conf $(DBUSDIR)

