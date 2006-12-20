NULL =

INCLUDES =					\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	$(TEST_CFLAGS)				\
	$(VFS_CFLAGS)				\
	-DG_DISABLE_DEPRECATED

libraries =						\
	$(top_builddir)/libgnomevfs/libgnomevfs-2.la	\
	$(TEST_LIBS)					\
	$(LIBEFS_LIBS)

bin_PROGRAMS =						\
	gnomevfs-cat					\
	gnomevfs-copy					\
	gnomevfs-info					\
	gnomevfs-ls					\
	gnomevfs-monitor				\
	gnomevfs-mkdir					\
	gnomevfs-mv					\
	gnomevfs-rm					\
	gnomevfs-df					\
	$(NULL)

gnomevfs_cat_SOURCES = gnomevfs-cat.c
gnomevfs_cat_LDADD = $(libraries)

gnomevfs_copy_SOURCES = gnomevfs-copy.c
gnomevfs_copy_LDADD = $(libraries)

gnomevfs_info_SOURCES = gnomevfs-info.c
gnomevfs_info_LDADD = $(libraries)

gnomevfs_ls_SOURCES = gnomevfs-ls.c
gnomevfs_ls_LDADD = $(libraries)

gnomevfs_monitor_SOURCES = gnomevfs-monitor.c
gnomevfs_monitor_LDADD = $(libraries)

gnomevfs_mkdir_SOURCES = gnomevfs-mkdir.c
gnomevfs_mkdir_LDADD = $(libraries)

gnomevfs_mv_SOURCES = gnomevfs-mv.c
gnomevfs_mv_LDADD = $(libraries)

gnomevfs_rm_SOURCES = gnomevfs-rm.c
gnomevfs_rm_LDADD = $(libraries)

gnomevfs_df_SOURCES = gnomevfs-df.c
gnomevfs_df_LDADD = $(libraries)

EXTRA_DIST = authentication.c
