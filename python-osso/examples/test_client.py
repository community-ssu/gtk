#!/usr/bin/python2.5 -u
import time
import gtk
import hildon
import osso

def handler_test(interface, method, parameters, args):
    print "CALLBACK handler_test with interface[",interface,"] method[",method,"] parameters[",parameters,"] args[",args,"]"

def handler_test_application_top(widget, applic, as):
    applic.application_top("maemopad")
    as.userdata_changed()

def handler_test_force_autosave(widget, as):
    print widget.get_label() 
    as.force_autosave()

def handler_test_set_handler(widget, rpc):
    print widget.get_label()
    rpc.set_rpc_default_callback(handler_test, (1,2,3,"testing"))
    rpc.rpc_run_with_defaults("test_client", "open")

def handler_test_unset_handler(widget, rpc):
    print widget.get_label() 
    rpc.set_rpc_default_callback(None)

def handler_test_time_notification(data):
    print data

def handler_test_save_state_handler(widget, as, ss):
    print "saving..."
    config = {'name': as.get_name(), 'version': as.get_version()}
    print "saved:", ss.state_write(config)

def handler_test_load_state_handler(widget, ss):
    print "loading..."
    config = ss.state_read()
    print "loaded:", config

plugins = ["libgwsettings.so",
    "libhelloworld-cp.so",
    "libinternetsettings.so",
    "libosso-backup-cp.so",
    "libpersonalisation.so",
]

def handler_test_cp_plugin_handler(widget, plug, app):
    global count

    try:
        plugin = plugins.pop()
    except:
        return

    print "executing:", plugin
    plug.plugin_execute(plugin, True, app)
    print "saving..."
    plug.plugin_save_state(plugin)

c = osso.Context("test_client", "0.0.0", False)
as = osso.Autosave(c)
applic = osso.Application(c)
rpc = osso.Rpc(c)
print "as.get_name() ->", as.get_name()
print "as.get_version() ->", as.get_version()
print "rpc.get_rpc_timeout() ->", rpc.get_rpc_timeout()
print "rpc.set_rpc_timeout(15) ->", rpc.set_rpc_timeout(15)
print "rpc.get_rpc_timeout() ->", rpc.get_rpc_timeout()
timenot = osso.TimeNotification(c)
timenot.set_time_notification_callback(handler_test_time_notification, "Notification Callback!")
timenot.set_time(time.localtime())
plug = osso.Plugin(c)
ss = osso.StateSaving(c)

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
test_application_top.connect("clicked", handler_test_application_top, applic, as)
vbox.add(test_application_top)

test_force_autosave = gtk.Button("Force Autosave")
test_force_autosave.connect("clicked", handler_test_force_autosave, as)
vbox.add(test_force_autosave)

test_unset_handler = gtk.Button("Unset Handler")
test_unset_handler.connect("clicked", handler_test_unset_handler, rpc)
vbox.add(test_unset_handler)

test_set_handler = gtk.Button("Set Handler")
test_set_handler.connect("clicked", handler_test_set_handler, rpc)
vbox.add(test_set_handler)

test_save_state = gtk.Button("Save State")
test_save_state.connect("clicked", handler_test_save_state_handler, as, ss)
vbox.add(test_save_state)

test_load_state = gtk.Button("Load State")
test_load_state.connect("clicked", handler_test_load_state_handler, ss)
vbox.add(test_load_state)

test_cp_plugin = gtk.Button("Control Panel Plugin")
test_cp_plugin.connect("clicked", handler_test_cp_plugin_handler, plug,app)
vbox.add(test_cp_plugin)


view.add(vbox)
app.show_all()
gtk.main()
c.close()

