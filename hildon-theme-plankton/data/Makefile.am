themedir 				= @THEME_FULL_DIR@

gtkthemedir				= @GTK_FULL_DIR@

matchboxthemedir			= @MATCHBOX_FULL_DIR@

gtktheme_DATA				= gtkrc				\
					  gtkrc.maemo_af_desktop        

matchboxtheme_DATA			= theme.xml

theme_DATA 				= index.theme

gtkrc: 
	hildon-theme-subst $(LAYOUT_DIR)/layout.txt $(srcdir)/../template/template.png $(LAYOUT_DIR)/gtkrc gtkrc
	echo "" >> gtkrc
	echo "# Generated from hildon-theme-layout-4 version `pkg-config --modversion hildon-theme-layout-4`" >> gtkrc

gtkrc.maemo_af_desktop: 
	hildon-theme-subst $(LAYOUT_DIR)/layout.txt $(srcdir)/../template/template.png $(LAYOUT_DIR)/gtkrc.maemo_af_desktop gtkrc.maemo_af_desktop
	echo "" >> gtkrc.maemo_af_desktop
	echo "# Generated from hildon-theme-layout-4 version `pkg-config --modversion hildon-theme-layout-4`" >> gtkrc.maemo_af_desktop

theme.xml: 
	hildon-theme-subst $(LAYOUT_DIR)/layout.txt $(srcdir)/../template/template.png $(LAYOUT_DIR)/theme.xml theme.xml
	sed -i -e 's:@THEME_DIR\@:@THEME_DIR@:' theme.xml
	sed -i -e 's:@THEME_FULL_DIR\@:@THEME_FULL_DIR@:' theme.xml
	sed -i -e 's:@THEME_NAME\@:@THEME_NAME@:' theme.xml

index.theme: 
	hildon-theme-subst $(LAYOUT_DIR)/layout.txt $(srcdir)/../template/template.png $(LAYOUT_DIR)/index.theme index.theme
	sed -i -e 's:@THEME_DIR\@:@THEME_DIR@:' index.theme
	sed -i -e 's:@THEME_FULL_DIR\@:@THEME_FULL_DIR@:' index.theme
	sed -i -e 's:@THEME_NAME\@:@THEME_NAME@:' index.theme

MAINTAINERCLEANFILES			= Makefile.in

CLEANFILES				= gtkrc				\
					  gtkrc.maemo_af_desktop	\
					  theme.xml			\
					  index.theme			


