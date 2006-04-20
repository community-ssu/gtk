#!/usr/bin/python2.4 -u
import osso

c = osso.Context("test_system_note", "0.0.0", True)

print "c.get_name() ->", c.get_name()
print "c.get_version() ->", c.get_version()
print "c.get_rpc_timeout() ->", c.get_rpc_timeout()
print "c.set_rpc_timeout(15) ->", c.set_rpc_timeout(15)
print "c.get_rpc_timeout() ->", c.get_rpc_timeout()

print "DIALOG:", c.system_note_dialog("Test Notice Dialog!")
#a = raw_input()
print "DIALOG:", c.system_note_dialog("Test Notice Dialog Again!", "notice")
#a = raw_input()
print "DIALOG:", c.system_note_dialog("Test Wait Dialog!", "wait")
#a = raw_input()
print "DIALOG:", c.system_note_dialog("Test Error Dialog!", "error")
#a = raw_input()
print "DIALOG:", c.system_note_dialog("Test Warning Dialog!", "warning")
#a = raw_input()
print "INFOPRINT:", c.system_note_infoprint("Test Infoprint!")
#a = raw_input()

c.close()

