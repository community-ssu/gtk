# This file is part of hildon-desktop
#
# Copyright (c) 2006 Nokia Corporation. All rights reserved.
#
# Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# version 2.1 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301 USA
#

ALL: minihome

CPP_FLAGS+=-I../../src

hildon-home-select-applets-dialog.o: hildon-home-select-applets-dialog.c hildon-home-select-applets-dialog.h
	gcc $(CPP_FLAGS)-Wall -Werror -g -o hildon-home-select-applets-dialog.o -c hildon-home-select-applets-dialog.c `pkg-config --cflags libhildondesktop`

minihome: minihome.c hildon-home-select-applets-dialog.o
	gcc $(LD_FLAGS) $(CPP_FLAGS)-Wall -Werror -g -o minihome minihome.c hildon-home-select-applets-dialog.o `pkg-config --libs --cflags libhildondesktop`
