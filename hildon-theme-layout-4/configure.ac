AC_INIT(Makefile.am)
AM_INIT_AUTOMAKE(hildon-theme-layout-4, 0.10.2)
AM_CONFIG_HEADER(config.h)

AC_PROG_INSTALL

# Check the tools installation
PKG_CHECK_MODULES(TOOLS, hildon-theme-tools >= 0.2.0)

AC_OUTPUT([Makefile                             \
	   rc/Makefile                          \
           pkgconfig/hildon-theme-layout-4.pc   \
           pkgconfig/Makefile                   \
           data/Makefile                        \
           data/bundle.ini])
