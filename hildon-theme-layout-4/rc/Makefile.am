layoutdir 				= ${datadir}/@PACKAGE@/

layout_DATA 				= gtkrc gtkrc.maemo_af_desktop

RC_FILES				= afterincludes.rc 		\
					  afterthemespecificgtk.rc 	\
					  base.rc 			\
					  defaultarrows.rc 		\
					  gtkbutton.rc 			\
					  gtkbuttonbox.rc 		\
					  gtkcheckbutton.rc 		\
					  gtkcheckmenuitem.rc 		\
					  gtkcombobox.rc 		\
					  gtkdialog.rc 			\
					  gtkentry.rc 			\
					  gtkentrycompletion.rc 	\
					  gtkexpander.rc 		\
					  gtkfilechooserbutton.rc 	\
					  gtkframe.rc 			\
					  gtkmenu.rc 			\
					  gtkmenuitem.rc 		\
					  gtkmenutoolbutton.rc 		\
					  gtknotebook.rc 		\
					  gtkpaned.rc 			\
					  gtkprogressbar.rc 		\
					  gtkradiobutton.rc 		\
					  gtkrange.rc 			\
					  gtkrc.maemo_af_desktop.rc 	\
					  gtkrc.rc 			\
					  gtkscrollbar.rc 		\
					  gtkscrolledwindow.rc 		\
					  gtkseparator.rc 		\
					  gtksettings.rc 		\
					  gtktextview.rc 		\
					  gtktoggletoolbutton.rc 	\
					  gtktoolbar.rc 		\
					  gtktoolbutton.rc 		\
					  gtktreeview.rc 		\
					  hildonapp.rc 			\
					  hildonbanners.rc 		\
					  hildoncandidatebox.rc 	\
					  hildoncaption.rc 		\
					  hildoncolorchooser.rc 	\
					  hildoncontrolbar.rc 		\
					  hildondateeditor.rc 		\
					  hildongrid.rc 		\
					  hildonhome.rc 		\
					  hildonkeyboard.rc 		\
					  hildonnavigator.rc 		\
					  hildonnavigatorpopuplist.rc 	\
					  hildonnote.rc 		\
					  hildonnumbereditor.rc 	\
					  hildonseekbar.rc 		\
					  hildonstatusbar.rc 		\
					  hildontimeeditor.rc 		\
					  hildontimepicker.rc 		\
					  hildonvolumebar.rc 		\
					  hildonweekdaypicker.rc 	\
					  ossoapplicationinstaller.rc 	\
					  ossoclock.rc 			\
					  ossoglobalsearch.rc 		\
					  ossointernetsearchapplet.rc 	\
					  ossoiradioapplet.rc 		\
					  ossomediaplayer.rc 		\
					  ossorssfeedreader.rc 		\
					  ossortc.rc 			\
					  ossospeeddial.rc 		\
					  themespecificgtk.rc		\
					  hildonthumbscrollbar.rc	\
					  hildoncalendar.rc		\
					  hildonbreadcrumbtrail.rc

gtkrc: $(RC_FILES)
	hildon-theme-rc-parser gtkrc.rc gtkrc

gtkrc.maemo_af_desktop: $(RC_FILES)
	hildon-theme-rc-parser gtkrc.maemo_af_desktop.rc gtkrc.maemo_af_desktop

CLEANFILES				= gtkrc gtkrc.maemo_af_desktop

MAINTAINERCLEANFILES			= Makefile.in 

EXTRA_DIST				= $(RC_FILES)
