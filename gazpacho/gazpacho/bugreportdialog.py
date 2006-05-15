# Copyright (C) 2005 by Sicem S.L.
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
import sys
import time
import urllib

import gtk

_ = gettext.gettext

class BugReportDialog(gtk.Dialog):
    def __init__(self, parent=None):
        gtk.Dialog.__init__(self, 'Send Bug report', parent,
                            gtk.DIALOG_MODAL | gtk.DIALOG_NO_SEPARATOR,
                            (gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT,
                             gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))
        self.set_border_width(6)
        # 304 = 430 / sqrt(2) = golden number
        self.set_default_size(430, 304)
        self.vbox.set_spacing(12)

        text1 = _('What were you doing before the error occured?')
        frame1 = self._create_frame(text1)
        sw = gtk.ScrolledWindow()
        sw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        sw.set_shadow_type(gtk.SHADOW_IN)
        self.description_textview = gtk.TextView()
        sw.add(self.description_textview)
        frame1.add(sw)
        self.vbox.pack_start(frame1, True, True)

        text2 = _('Personal information (optional)') + ':'
        frame2 = self._create_frame(text2)
        table = gtk.Table(2, 2)
        table.set_border_width(6)
        table.set_row_spacings(6)
        table.set_col_spacings(6)
        label1 = gtk.Label(_('Name') + ':')
        self.name_entry = gtk.Entry()
        label2 = gtk.Label(_('E-Mail') + ':')
        self.email_entry = gtk.Entry()
        table.attach(label1, 0, 1, 0, 1, 0, 0)
        table.attach(self.name_entry, 1, 2, 0, 1, gtk.EXPAND|gtk.FILL, 0)
        table.attach(label2, 0, 1, 1, 2, 0, 0)
        table.attach(self.email_entry, 1, 2, 1, 2, gtk.EXPAND|gtk.FILL, 0)
        frame2.add(table)
        self.vbox.pack_start(frame2, False, False)

        self.vbox.show_all()

    def _create_frame(self, labeltext):
        frame = gtk.Frame()
        frame.set_border_width(6)
        frame.set_shadow_type(gtk.SHADOW_NONE)
        label = gtk.Label("<b>%s</b>" % labeltext)
        label.set_use_markup(True)
        frame.set_label_widget(label)
        return frame

    def get_description(self):
        buffer = self.description_textview.get_buffer()
        return buffer.get_text(*buffer.get_bounds())

    def get_name(self):
        return self.name_entry.get_text()

    def get_email(self):
        return self.email_entry.get_text()

BUG_TEMPLATE = """Package: Gazpacho
Version: %(version)s
Synopsis: %(subject)s
Bugzilla-Product: Gazpacho
Bugzilla-Component: general

Date: %(date)s

User name: %(username)s %(email)s

The user was doing the following task:
%(description)s

Exception traceback:
%(traceback)s

Gazpacho version: %(version)s
Python version: %(python_version)s
GTK+ version: %(gtk_version)s
PyGTK version: %(pygtk_version)s

Platform: %(platform)s"""


def send_bugreport(exception_text, parent=None):
    """
    Send a bugreport to http://bugzilla.gnome.org in the product
    name Gazpacho.
    
    Uses an http proxy at http://gazpacho.sicem.biz/bugproxy because
    not all the users has a local smtp available, specially win32 users.
    
    Return True if the bug was sent or False otherwise
    """
    from gazpacho.application import __version__

    d = BugReportDialog(parent)
    r = d.run()
    retval = False
    if r != gtk.RESPONSE_ACCEPT:
        d.destroy()
        return False

    date = time.strftime("%Y-%m-%d %H:%M")
    subject = 'Crash on %s' % date
    email = d.get_email()
    if email:
        email = '<%s>' % email
        
    bug_info = dict(description=d.get_description() or 'Unknown',
                    traceback=exception_text or 'No traceback available',
                    username=d.get_name() or 'Unknown',
                    email=email,
                    date=date,
                    subject=subject,
                    platform=sys.platform,
                    python_version=sys.version,
                    version=__version__,
                    gtk_version=gtk.gtk_version,
                    pygtk_version=gtk.pygtk_version)
    
    params = dict(subject=subject, body=BUG_TEMPLATE % bug_info)
    params['from'] = d.get_email() or 'lgs@sicem.biz'
    
    # send the report via HTTP POST
    data = urllib.urlencode(params)
    f = urllib.urlopen("http://gazpacho.sicem.biz/bugproxy", data)
    print f.read()
    
    d.destroy()
    return True
