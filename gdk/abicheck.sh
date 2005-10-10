#! /bin/sh

cpp -P -DALL_FILES -DGDK_ENABLE_BROKEN -DGDK_WINDOWING_X11 ${srcdir:-.}/gdk.symbols | sed -e '/^$/d' -e 's/ G_GNUC.*$//' | sort | uniq > expected-abi
nm -D .libs/libgdk-x11-2.0.so | grep " T " | cut -d ' ' -f 3 | sort > actual-abi
diff -u expected-abi actual-abi && rm expected-abi actual-abi
