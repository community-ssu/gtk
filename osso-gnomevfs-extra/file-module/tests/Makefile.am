INCLUDES =                                          \
		$(FILE_MODULE_CFLAGS)          	    \
		$(WARN_FLAGS)                       \
		-I$(top_srcdir)/src                 \
		-DTESTDIR=\"$(top_srcdir)/tests\"

noinst_PROGRAMS =                                   \
		test-monitor			    \
		test-lock			    \
		test-copy-permissions

test_monitor_SOURCES =                              \
		test-monitor.c

test_lock =      	  		            \
		test-lock.c

test_copy_permissions =  		            \
		test-copy-permissions.c

LDADD =                                             \
		$(FILE_MODULE_LIBS)


