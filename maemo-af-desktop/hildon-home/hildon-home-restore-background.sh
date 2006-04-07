#!/bin/sh

IMAGE_LOADER=/usr/bin/home-image-loader

OSSO_CONF_DIR="${HOME}/.osso"
HOME_CONF_DIR="${OSSO_CONF_DIR}/hildon-home"

DEFAULT_BACKGROUND="/usr/share/backgrounds/bg_img_01.png"
DEFAULT_TITLEBAR="/usr/share/themes/default/images/qgn_plat_home_status_bar_background.png"
DEFAULT_SIDEBAR="/usr/share/themes/default/images/qgn_plat_home_border_left.png"


# Remove old home configuration
rm -Rf $HOME_CONF_DIR/*

# Recreate the directory
if [ ! -d $HOME_CONF_DIR ]
then
    mkdir -p $HOME_CONF_DIR
fi

# Generate home background cache
if [ -x $IMAGE_LOADER ]
then
    $IMAGE_LOADER new_image 0 \
        $DEFAULT_BACKGROUND "${HOME_CONF_DIR}/user_filename.txt" \
        "${HOME_CONF_DIR}/hildon_home_bg_user.png" 720 480 0 0 0 \
        $DEFAULT_TITLEBAR "${HOME_CONF_DIR}/original_titlebar.png" 0 0 \
        $DEFAULT_SIDEBAR  "${HOME_CONF_DIR}/original_sidebar.png" 0 60 1
fi
