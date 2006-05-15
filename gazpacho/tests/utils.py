import pygtk
pygtk.require('2.0')
import gtk
import os.path
import sys

currentdir = os.path.dirname(os.path.abspath(sys.argv[0]))
prefix = os.path.abspath(os.path.join(currentdir, '..'))

sys.path.append(prefix)
    
from gazpacho import path
path.xml_dir = os.path.join(prefix, 'xml')
path.pixmaps_dir = os.path.join(prefix, 'pixmaps')
path.languages_dir = os.path.join(prefix, 'locale')

from gazpacho import application
    
def init_gazpacho():
    gazpacho = application.Application()
    gazpacho.show_all()
    gazpacho.new_project()
    project = gazpacho.get_current_project()
    refresh_gui()
    return gazpacho, project

def refresh_gui():
    while gtk.events_pending():
        gtk.main_iteration_do(block=gtk.FALSE)

