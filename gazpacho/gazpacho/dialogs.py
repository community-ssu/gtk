# Copyright (C) 2004,2005 by SICEm S.L. and Async Open Source
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

import gettext
import os

import gtk

__all__ = ['error', 'info', 'messagedialog', 'open', 'save', 'warning',
           'yesno']

_ = gettext.gettext

def messagedialog(dialog_type, short, long=None, parent=None,
                  buttons=gtk.BUTTONS_OK, additional_buttons=None):
    d = gtk.MessageDialog(parent=parent, flags=gtk.DIALOG_MODAL,
                          type=dialog_type, buttons=buttons)

    if additional_buttons:
        d.add_buttons(*additional_buttons)
    
    d.set_markup(short)
    
    if long:
        if isinstance(long, gtk.Widget):
            widget = long
        elif isinstance(long, basestring):
            widget = gtk.Label()
            widget.set_markup(long)
        else:
            raise TypeError("long must be a gtk.Widget or a string")
        
        expander = gtk.Expander(_("Click here for details"))
        expander.set_border_width(6)
        expander.add(widget)
        d.vbox.pack_end(expander)
        
    d.show_all()
    response = d.run()
    d.destroy()
    return response
    
def error(short, long=None, parent=None):
    """Displays an error message."""
    return messagedialog(gtk.MESSAGE_ERROR, short, long, parent)

def info(short, long=None, parent=None):
    """Displays an info message."""
    return messagedialog(gtk.MESSAGE_INFO, short, long, parent)

def warning(short, long=None, parent=None):
    """Displays a warning message."""
    return messagedialog(gtk.MESSAGE_WARNING, short, long, parent)

def yesno(text, parent=None):
    return messagedialog(gtk.MESSAGE_WARNING, text, None, parent,
                         buttons=gtk.BUTTONS_YES_NO)
  
def open(title='', parent=None, patterns=[], folder=None):
    """Displays an open dialog."""
    filechooser = gtk.FileChooserDialog(title or _('Open'),
                                        parent,
                                        gtk.FILE_CHOOSER_ACTION_OPEN,
                                        (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                                         gtk.STOCK_OPEN, gtk.RESPONSE_OK))

    if patterns:
        file_filter = gtk.FileFilter()
        for pattern in patterns:
            file_filter.add_pattern(pattern)
        filechooser.set_filter(file_filter)
    filechooser.set_default_response(gtk.RESPONSE_OK)

    if folder:
        filechooser.set_current_folder(folder)
        
    response = filechooser.run()
    if response != gtk.RESPONSE_OK:
        filechooser.destroy()
        return
    
    path = filechooser.get_filename()
    if path and os.access(path, os.R_OK):
        filechooser.destroy()
        return path
        
    abspath = os.path.abspath(path)

    error(_('Could not open file "%s"') % abspath,
          _('The file "%s" could not be opened. '
            'Permission denied.') %  abspath)

    filechooser.destroy()
    return path

def save(title='', parent=None, current_name='', folder=None):
    """Displays a save dialog."""
    filechooser = gtk.FileChooserDialog(title or _('Save'),
                                        parent,
                                        gtk.FILE_CHOOSER_ACTION_SAVE,
                                        (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                                         gtk.STOCK_SAVE, gtk.RESPONSE_OK))
    if current_name:
        filechooser.set_current_name(current_name)       
    filechooser.set_default_response(gtk.RESPONSE_OK)
    
    if folder:
        filechooser.set_current_folder(folder)
        
    path = None
    while True:
        response = filechooser.run()
        if response != gtk.RESPONSE_OK:
            path = None
            break
        
        path = filechooser.get_filename()
        if not os.path.exists(path):
            break

        submsg1 = _('A file named "%s" already exists') % os.path.abspath(path)
        submsg2 = _('Do you which to replace it with the current project?')
        text = '<span weight="bold" size="larger">%s</span>\n\n%s\n' % \
              (submsg1, submsg2)
        result = messagedialog(gtk.MESSAGE_ERROR,
                               text,
                               parent=parent,
                               buttons=gtk.BUTTONS_NONE,
                               additional_buttons=(gtk.STOCK_CANCEL,
                                                   gtk.RESPONSE_CANCEL,
                                                   _("Replace"),
                                                   gtk.RESPONSE_YES))
        # the user want to overwrite the file
        if result == gtk.RESPONSE_YES:
            break

    filechooser.destroy()
    return path
    
def test():
    globals()['_'] = lambda s: s
    print open(title='Open a file', patterns=['*.py'])
    print save(title='Save a file', current_name='foobar.py')
    error('An error occurred', gtk.Button('Woho'))
    error('An error occurred',
          'Long description bla bla bla bla bla bla bla bla bla\n'
          'bla bla bla bla bla lblabl lablab bla bla bla bla bla\n'
          'lbalbalbl alabl l blablalb lalba bla bla bla bla lbal\n')
    
if __name__ == '__main__':
    test()
