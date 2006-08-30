from distutils.core import setup, Extension
from distutils.command.build import build
import os
import sys

datadir = '/usr/share'
defsdir = datadir+'/pygtk/2.0/defs'
includedir = '/usr/include'

class PyHildonBuild(build):
	def run(self):
		"""Create the temporary files used to compile the hildon module:
		-hildon.c
		-hildon-types.h.in
		-hildon-types.c.in"""
		# Generate enum/flags run-time information
		HILDON_TYPE_FILES = includedir+'/hildon-fm/hildon-widgets/hildon-file-system-model.h	\
			'+includedir+'/hildon-widgets/hildon-date-editor.h	\
			'+includedir+'/hildon-widgets/hildon-font-selection-dialog.h	\
			'+includedir+'/hildon-widgets/hildon-time-editor.h	\
			'+includedir+'/hildon-widgets/hildon-grid-item.h	\
			'+includedir+'/hildon-widgets/hildon-telephone-editor.h	\
			'+includedir+'/hildon-base-lib/hildon-base-types.h	\
			'+includedir+'/glib-2.0/glib/gdate.h'
		
		filename = 'hildon-types.h.in'
		cmdinput, cmdresult, error = os.popen3('./gen-enum-h '+HILDON_TYPE_FILES+' >'+filename+'.tmp && mv '+filename+'.tmp '+filename)
		print error.readlines()
		filename = 'hildon-types.c.in'
		cmdinput, cmdresult, error = os.popen3('./gen-enum-c '+HILDON_TYPE_FILES+' >'+filename+'.tmp && mv '+filename+'.tmp '+filename)
		print error.readlines()
		
		# Creation of ".c" files, using pygtk-codegen-2.0
		filenames = ['hildon',]
		for filename in filenames:
			cmdtoexec = 'pygtk-codegen-2.0 \
			    --register '+defsdir+'/gdk.defs \
			    --register '+defsdir+'/gtk-types.defs \
			    --register '+defsdir+'/gtk.defs \
			    --register '+defsdir+'/pango-types.defs \
        		    --register defs/hildon-grid-item.defs \
        		    --register defs/hildon-file-system-model.defs \
			    --override '+filename+'.override \
			    --prefix py'+filename+' '+filename+'.defs > gen-'+filename+'.c \
			&& cp gen-'+filename+'.c '+filename+'.c \
			&& rm -f gen-'+filename+'.c'
			
			cmdinput, cmdresult, error = os.popen3(cmdtoexec)
			print error.readlines()
		build.run(self)
		
hildon = Extension('hildon',
	sources = [
		'hildon.c',
		'hildonmodule.c',
		'hildon-extra.c',
		'hildon-types.c',
	],
	libraries = [
		'hildonbase',
		'hildonwidgets',
		'hildonfm',
		'dbus-1',
                'dbus-glib-1',
                'glib-2.0',
                'gobject-2.0',
                'gmodule-2.0',
                'atk-1.0',
                'pangoxft-1.0',
                'pangox-1.0',
                'pango-1.0',
                'gdk-x11-2.0',
                'gdk_pixbuf-2.0',
                'gdk-x11-2.0',
	],
	include_dirs=[
                '/usr/include',
                '/usr/include/freetype2',
                '/usr/include/dbus-1.0',
                '/usr/include/glib-2.0',
                '/usr/include/atk-1.0',
                '/usr/include/pango-1.0',
                '/usr/include/gtk-2.0',
                '/usr/include/pygtk-2.0',
                '/usr/include/hildon-fm',
		'/usr/include/hildon-widgets'
		'/usr/lib/dbus-1.0/include',
                '/usr/lib/glib-2.0/include',
                '/usr/lib/gtk-2.0/include',
                '/usr/X11R6/include',
	],
	extra_compile_args=[
		'-Os',
#		'-ansi',
		'-DXTHREADS',
		'-DXUSE_MTSAFE_API',
		'-DHILDON_DISABLE_DEPRECATED',
#		'-pedantic',
#		'-Wno-long-long',
#		'-g',
#		'-rdynamic',
	],
)

setup(
	name = 'hildon',
	version = '0.1',
	description = 'Python bindings for libhildon components.',
	author = 'Osvaldo Santana Neto',
	author_email = 'osvaldo.santana@indt.org.br',
	url = 'http://www.maemo.org',
	ext_modules = [hildon],
	cmdclass={'build': PyHildonBuild}
)
