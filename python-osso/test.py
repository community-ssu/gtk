#!/usr/bin/python2.4
import gtk
import hildon
import osso

def dbus_req_handler(*args):
    print args

def exit_event_handler(*args):
    print "saindo...", args

c = osso.Context("Foo", "0.0.0", True)

print c.get_name()
print c.get_version()
#c.system_note_dialog("Foobar!", osso.GN_WARNING)
#c.application_set_exit_callback(exit_event_handler, "ola")
print c.rpc_run("com.nokia.backup", "/com/nokia/backup", "com.nokia.backup", "backup_finish") 
print c.rpc_run("com.nokia.backup", "/com/nokia/backup", "com.nokia.backup", "backup_finish", wait_reply=False) 
print c.rpc_run("com.nokia.backup", "/com/nokia/backup", "com.nokia.backup", "backup_finish", wait_reply=True) 
#c.rpc_run_with_defaults("tn_mail", "send_recv_button_focus", (2,))
#c.rpc_set_callback("com.nokia.backup","/com/nokia/backup", "com.nokia.backup", dbus_req_handler) 

#app = hildon.App("bar")
#view = hildon.AppView("foo")
#view.add(gtk.Button("Hi!"))
#app.set_appview(view)
#app.connect('destroy', gtk.main_quit)
#app.show_all()
#gtk.main()
#c.close()
