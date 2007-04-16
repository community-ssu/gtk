MAINTAINERCLEANFILES 		= Makefile.in

CLEANFILES			= slice.stamp

EXTRA_DIST 			= template.png \
				  template.svg

all: slice.stamp

clean-local:
	find -iname '*.png' -a ! -iname template.png -a ! -name coordinates.png -exec rm -f '{}' \;
	find -iname '*.jpg' -exec rm -f '{}' \;
	find -iname '*.jpeg' -exec rm -f '{}' \;

slice.stamp:
	hildon-theme-slicer $(LAYOUT_DIR)/layout.txt $(srcdir)/../template/template.png
	touch slice.stamp

install: all
	$(mkinstalldirs) $(DESTDIR)/$(IMAGES_FULL_DIR)
	find -iname '*.png' -a ! -iname template.png -a ! -iname coordinates.png -exec install -c -m 644 '{}' $(DESTDIR)/$(IMAGES_FULL_DIR)/ \;
	find -iname '*.jpg' -exec install -c -m 644 '{}' $(DESTDIR)/$(IMAGES_FULL_DIR)/ \;
	find -iname '*.jpeg' -exec install -c -m 644 '{}' $(DESTDIR)/$(IMAGES_FULL_DIR)/ \;

