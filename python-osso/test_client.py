#!/usr/bin/python2.4 -u
import gtk
import hildon
import osso

def handler_test(*args):
    print "CALLBACK handler_test with args:", args

def handler_test_application_top(widget, c):
    c.application_top("osso_global_search")

c = osso.Context("test_client", "0.0.0", True)
print "c.get_name() ->", c.get_name()
print "c.get_version() ->", c.get_version()
print "c.get_rpc_timeout() ->", c.get_rpc_timeout()
print "c.set_rpc_timeout(15) ->", c.set_rpc_timeout(15)
print "c.get_rpc_timeout() ->", c.get_rpc_timeout()
c.set_rpc_callback("com.nokia.test_client", "/com/nokia/test_client",
        "com.nokia.test_client", handler_test, (1,2,3, "hahaha"))
#c.system_note_dialog("Test Notice Dialog!")
#c.system_note_dialog("Test Notice Dialog Again!", "notice")
#c.system_note_dialog("Test Wait Dialog!", "wait")
#c.system_note_dialog("Test Error Dialog!", "error")
#c.system_note_dialog("Test Warning Dialog!", "warning")


# Interface
app = hildon.App()
view = hildon.AppView("Client")
app.set_title("Test")
app.set_appview(view)
app.connect("destroy", gtk.main_quit)
vbox = gtk.VBox()

label = gtk.Label("Tests")
vbox.add(label)

test_application_top = gtk.Button("Application Top")
test_application_top.connect("clicked", handler_test_application_top, c)
vbox.add(test_application_top)

view.add(vbox)
app.show_all()
gtk.main()
c.close()

