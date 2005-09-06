#Removed themes from the SUBDIRS, as we do not wish to install
#the themes

if ENABLE_SCHEMAS
  SUBDIRS = schemas
endif

confdir = $(sysconfdir)/matchbox

conf_DATA = kbdconfig


defaultsdir = $(datadir)/matchbox

if WANT_STANDALONE
  defaults_DATA =
else
  defaults_DATA = mbnoapp.xpm
endif

EXTRA_DIST = mbnoapp.xpm kbdconfig

