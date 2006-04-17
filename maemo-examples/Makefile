VERSION=1.0
DESTDIR=/
CC=gcc
CFLAGS=`pkg-config --cflags --libs gconf-2.0 hildon-libs hildon-fm gtk+-2.0 libosso gdk-2.0` -DTEXT_FILE=\"`echo ~/MyDocs/foo.txt`\"
EXAMPLES=\
	example_backup_server \
	example_backup_client \
	example_color_selector \
	example_context \
	example_findtoolbar \
	example_file_chooser \
	example_file_details \
	example_font_selector \
	example_gconf \
	example_gtk_print_info \
	example_hard_keys \
	example_hildonapp \
	example_hildonappview \
	example_libosso \
	example_menu \
	example_message \
	example_quickstart \
	example_toolbar

all: $(EXAMPLES) stamp-testfile

install:
	mkdir -p $(DESTDIR)/usr/share/doc/maemo-examples
	cp *.c Makefile README README.gconf COPYING \
		$(DESTDIR)/usr/share/doc/maemo-examples

$(EXAMPLES):
	    $(CC) $(CFLAGS) $(LDFLAGS) -o $@ $@.c

clean:
	-rm -f $(EXAMPLES)
	-rm -rf stamp-*

stamp-testfile:
	-mkdir -p ~/MyDocs
	echo "Test file." > ~/MyDocs/foo.txt
	touch stamp-testfile

dist: clean
	cd .. && \
	cp -a maemo_examples maemo-examples-$(VERSION) && \
	tar czf maemo-examples-$(VERSION).tar.gz \
		maemo-examples-$(VERSION) && \
	rm -rf maemo-examples-$(VERSION)
