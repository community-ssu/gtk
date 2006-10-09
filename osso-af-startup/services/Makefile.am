
afinitdir=$(sysconfdir)/osso-af-init
dist_afinit_SCRIPTS = af-defines.sh \
	after-suw-process-restart.sh \
	dbus-sessionbus.sh \
	get-devlock.sh \
	launch-wrapper-tryrestart.sh \
	launch-wrapper.sh \
	nice-kill-launch-wrapper.sh \
	nice-launch-wrapper-tryrestart.sh \
	nice-launch-wrapper.sh \
	temp-reaper-startup.sh

afetcdir = $(sysconfdir)/osso-af-init
afetc_DATA = $(dist_afinit_SCRIPTS)

clean:
	rm -f Makefile Makefile.in
