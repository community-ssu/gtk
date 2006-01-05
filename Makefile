# Copyright (C) 2006 Nokia Corporation

INSTALL_DIR := install -d
INSTALL_DATA := install -o root -g root --mode=644

PCDIR := $(DESTDIR)/usr/lib/pkgconfig
INCLUDEDIR := $(DESTDIR)/usr/include/mce

TOPDIR := $(shell /bin/pwd)
INCDIR := $(TOPDIR)/include/mce

PCFILE  := mce.pc
INCLUDE_FILES := $(INCDIR)/dbus-names.h $(INCDIR)/mode-names.h

.PHONY: install
install:
	$(INSTALL_DIR) $(PCDIR) $(INCLUDEDIR)				&&\
	$(INSTALL_DATA) $(PCFILE) $(PCDIR)				&&\
	$(INSTALL_DATA) $(INCLUDE_FILES) $(INCLUDEDIR)
