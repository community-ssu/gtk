#!/usr/bin/python2.4
import osso
import time

c = osso.Context("test_server", "0.0.0", True)
print "Sending test start..."
c.rpc_run("com.nokia.test_client", "/com/nokia/python/test_client", "com.nokia.test_client", "start_test")
#print "Waiting 10 seconds..."
#time.sleep(10)
#print "Sending test stop..."
#c.rpc_run("com.nokia.python.test", "/com/nokia/python/test", "com.nokia.python.test", "stop_test")

c.close()

