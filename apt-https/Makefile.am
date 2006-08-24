CFLAGS_LAUNCHER  = -shared
LDFLAGS_LAUNCHER = -shared -export-dynamic

methoddir = $(libdir)/apt/methods

method_PROGRAMS = https
bin_PROGRAMS = maemo-mini-curl maemo-mini-curl-cert-gui.launch

https_SOURCES = https.cc
https_CXXFLAGS = $(HTTPS_CFLAGS)
https_LDFLAGS = $(HTTPS_LIBS) -lapt-pkg

maemo_mini_curl_SOURCES = maemo-mini-curl.c
maemo_mini_curl_CFLAGS = $(MINICURL_CFLAGS)
maemo_mini_curl_LDFLAGS = $(MINICURL_LIBS) -lcurl

maemo_mini_curl_cert_gui_launch_SOURCES = maemo-cert-gui.c
maemo_mini_curl_cert_gui_launch_CFLAGS = $(CERTGUI_CFLAGS) $(CFLAGS_LAUNCHER)
maemo_mini_curl_cert_gui_launch_LDFLAGS = $(CERTGUI_LIBS)$ $(LDFLAGS_LAUNCHER)

deb:
	./make-package $(PACKAGE) $(VERSION) .

EXTRA_DIST = make-package

install-exec-local:
	rm -f $(DESTDIR)$(bindir)/maemo-mini-curl-cert-gui
	ln -s /usr/bin/maemo-invoker $(DESTDIR)$(bindir)/maemo-mini-curl-cert-gui
