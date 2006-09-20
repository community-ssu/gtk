CLEANFILES = \
	$(THEMEDIR)/gtk-2.0/gtkrc.cache \
	$(THEMEDIR)/gtk-2.0/gtkrc \
	$(THEMEDIR)/gtk-2.0/gtkrc.maemo_af_desktop.cache \
	$(THEMEDIR)/gtk-2.0/gtkrc.maemo_af_desktop \
	$(THEMEDIR)/gtk-2.0/ \
	$(THEMEDIR)/matchbox/theme.xml \
	$(THEMEDIR)/metchbox/ \
	$(THEMEDIR)/index.theme \
	$(THEMEDIR)/ \
	$(shell for i in gtkrc $$(grep include temp/gtkrc|grep -v '\#'|sed -e 's/^include "//' -e 's/".*$$//'); do echo temp/$$i; done) \
	temp/

$(THEMEDIR)/gtk-2.0/gtkrc:: themespecificgtk
	mkdir -p temp/
	cp $(COMMONRCDIR)/* temp/
	cp $(srcdir)/themespecificgtk temp/themespecificgtk

	mkdir -p $(THEMEDIR)/gtk-2.0/
	
	pushd temp; \
	for i in gtkrc $$(grep include gtkrc|grep -v '\#'|sed -e 's/^include "//' -e 's/".*$$//'); do cat $$i; done|grep -v '^include' > $(srcdir)/$(THEMEDIR)/gtk-2.0/gtkrc; \
	popd;

$(THEMEDIR)/gtk-2.0/gtkrc.maemo_af_desktop:: themespecificgtk $(THEMEDIR)/gtk-2.0/gtkrc
	pushd temp; \
        for i in gtkrc.maemo_af_desktop $$(grep include gtkrc.maemo_af_desktop|grep -v '\#'|sed -e 's/^include "//' -e 's/".*$$//'); do cat $$i; done|grep -v '^include' > $(srcdir)/$(THEMEDIR)/gtk-2.0/gtkrc.maemo_af_desktop; \
        popd;

$(THEMEDIR)/gtk-2.0/gtkrc.cache: $(THEMEDIR)/gtk-2.0/gtkrc
	cache-generator $(THEMEDIR) 2> /dev/null
 
$(THEMEDIR)/gtk-2.0/gtkrc.maemo_af_desktop.cache: $(THEMEDIR)/gtk-2.0/gtkrc.maemo_af_desktop
	cache-generator $(THEMEDIR) 2> /dev/null

$(THEMEDIR)/matchbox/theme.xml: theme.xml
	mkdir -p $(THEMEDIR)/matchbox/
	cp $(srcdir)/theme.xml $(THEMEDIR)/matchbox/

$(THEMEDIR)/index.theme: index.theme
	mkdir -p $(THEMEDIR)/
	cp $(srcdir)/index.theme $(THEMEDIR)/

