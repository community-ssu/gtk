# show graphical menu if menu key is pressed
# modify default_root,root_dev,root_fstype,root_fsoptions variables
			
#  Copyright (C) 2006 Frantisek Dufka
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

# HW keys scancodes
# up 103 down 108 left 105 right 106 select 28
# esc 1 menu 62 home 63 fullscreen 64 plus 65 minus 66

#wait 0.5 second for keyup event
GETKEY="evkey -u -t 500 /dev/input/event1"
KEY_SELECT=28
KEY_UP=103
KEY_DOWN=108
KEY_MENU=62

#define menu items

MENU_MIN=1
MENU_IDX=$MENU_MIN
MENU_MAX=4

##first is flasher default
#MENU_1_NAME="Flasher default - $(cal-tool --get-root-device 2> /dev/null)"
#MENU_1_ID=$(cal-tool --get-root-device 2> /dev/null)

MENU_1_NAME="Internal flash"
MENU_1_ID="flash"

MENU_2_NAME="MMC card"
MENU_2_ID="mmc"

MENU_3_NAME="MMC card, partition 2, ext2"
MENU_3_ID="mmc2"
MENU_3_DEVICE="mmcblk0p2"
MENU_3_MODULES="ext2"
MENU_3_FSTYPE="ext2"
MENU_3_FSOPTIONS="noatime"

MENU_4_NAME="USB hard drive"
MENU_4_ID="usb"

load_modules()
{
for mod in $* ; do
if [ -f $MODULE_PATH/${mod}.ko ]; then
insmod $MODULE_PATH/${mod}.ko
fi
done
}
								   
menu_redraw(){
i=$MENU_MIN
y=100
while [ $i -le $MENU_MAX ] ; do
    eval label=\$MENU_${i}_NAME
    if [ $i = $MENU_IDX ] ; then t=0xffff ; b=0 ; else t=0 ; b=0xffff ; fi
    text2screen -x 0 -y $y -w 800 -h 20 -c
    text2screen -s 2 -x 40 -y $y -T $t -B $b -t " $label "
    i=$((i+1))
    y=$((y+30))
done
}

menu_end(){
eval MENU_ID=\$MENU_${MENU_IDX}_ID
text2screen -c
text2screen -s 2 -H center -y 20 -T 0 -t "Booting from $MENU_ID ..."
}

menu_init(){
text2screen -c
text2screen -s 2 -H center -y 30 -T 0 -t "BOOT MENU"
text2screen -s 2 -H center -y 440 -T 0 -t "Press UP/DOWN to move, center to select"
}

menu_debug(){
text2screen -x 0 -y 380 -w 800 -h 20 -c
text2screen -s 2 -H center -y 380 -T 0 -t "$1"
}

menu_loop(){
while true ; do
key=`$GETKEY`
case "$key" in
    ${KEY_SELECT}) # select key, exit menu
#	menu_debug SELECT
	menu_end
	break ;;
    ${KEY_UP}) # up key
	if [ $MENU_IDX -gt $MENU_MIN ] ; then MENU_IDX=$((MENU_IDX-1)) ; else MENU_IDX=$MENU_MAX; fi
#	menu_debug "UP ($MENU_IDX)"
	menu_redraw
	;;
    ${KEY_DOWN}) # down key
	if [ $MENU_IDX -lt $MENU_MAX ] ; then MENU_IDX=$((MENU_IDX+1)) ; else MENU_IDX=$MENU_MIN ; fi
#	menu_debug "DOWN ($MENU_IDX)"
	menu_redraw
	;;
    ?*)
#	menu_debug "key $key"
	;;
esac
done
}

#try to preselect item with same id as preset in default_root
i=$MENU_MIN
while [ $i -le $MENU_MAX ] ; do
    eval label_id=\$MENU_${i}_ID
    [ "$label_id" = "$default_root" ] && MENU_IDX=$i
    i=$((i+1))
done

#show onscreeen menu if menu key was pressed
if [ "$HWKEYSTATE" = "$KEY_MENU" -o "$default_root" = "ask" ] ; then
	menu_init
	menu_redraw
	menu_loop
else
	menu_end
fi

eval i=\$MENU_${MENU_IDX}_DEVICE ; [ -z "$i" ] || root_dev=$i
eval i=\$MENU_${MENU_IDX}_FSTYPE ; [ -z "$i" ] || root_fstype=$i
eval i=\$MENU_${MENU_IDX}_FSOPTIONS ; [ -z "$i" ] || root_fsoptions=$i
eval i=\$MENU_${MENU_IDX}_MODULES ; [ -z "$i" ] || load_modules $i
eval i=\$MENU_${MENU_IDX}_ID ; [ -z "$i" ] || default_root=$i
