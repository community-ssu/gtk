#!/usr/bin/python2.4 -u
import time
import gtk
import hildon
import osso

def handler_test(*args):
    print "CALLBACK handler_test with args:", args

def handler_test_application_top(widget, c):
    c.application_top("osso_global_search")
    c.userdata_changed()

def handler_test_force_autosave(widget, c):
    print widget.get_label() 
    c.force_autosave()

def handler_test_set_handler(widget, c):
    print widget.get_label() 
    c.set_rpc_default_callback(handler_test, (1,2,3, "hahaha"))

def handler_test_unset_handler(widget, c):
    print widget.get_label() 
    c.set_rpc_default_callback(None)

def handler_test_time_notification(data):
    print data

def handler_test_save_state_handler(widget, c):
    print "saving..."
    config = {'name': c.get_name(), 'version': c.get_name()}
    print "saved:", c.state_write(config)

def handler_test_load_state_handler(widget, c):
    print "loading..."
    config = c.state_read()
    print "loaded:", config

plugins = ["libgwsettings.so",
    "libhelloworld-cp.so",
    "libinternetsettings.so",
    "libosso-backup-cp.so",
    "libpersonalisation.so",
]

def handler_test_cp_plugin_handler(widget, data):
    global count

    c, app = data
    try:
        plugin = plugins.pop()
    except:
        return

    print "executing:", plugin
    c.plugin_execute(plugin, True, app)
    print "saving..."
    c.plugin_save_state(plugin)

c = osso.Context("test_client", "0.0.0", True)
print "c.get_name() ->", c.get_name()
print "c.get_version() ->", c.get_version()
print "c.get_rpc_timeout() ->", c.get_rpc_timeout()
print "c.set_rpc_timeout(15) ->", c.set_rpc_timeout(15)
print "c.get_rpc_timeout() ->", c.get_rpc_timeout()
c.set_rpc_callback("com.nokia.test_client", "/com/nokia/test_client",
        "com.nokia.test_client", handler_test, (1,2,3, "hahaha"))
c.set_time_notification_callback(handler_test_time_notification, "Notification Callback!")
c.set_time(time.localtime())

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

test_force_autosave = gtk.Button("Force Autosave")
test_force_autosave.connect("clicked", handler_test_force_autosave, c)
vbox.add(test_force_autosave)

test_unset_handler = gtk.Button("Unset Handler")
test_unset_handler.connect("clicked", handler_test_unset_handler, c)
vbox.add(test_unset_handler)

test_set_handler = gtk.Button("Set Handler")
test_set_handler.connect("clicked", handler_test_set_handler, c)
vbox.add(test_set_handler)

test_save_state = gtk.Button("Save State")
test_save_state.connect("clicked", handler_test_save_state_handler, c)
vbox.add(test_save_state)

test_load_state = gtk.Button("Load State")
test_load_state.connect("clicked", handler_test_load_state_handler, c)
vbox.add(test_load_state)

test_cp_plugin = gtk.Button("Control Panel Plugin")
test_cp_plugin.connect("clicked", handler_test_cp_plugin_handler, (c,app))
vbox.add(test_cp_plugin)


view.add(vbox)
app.show_all()
gtk.main()
c.close()

