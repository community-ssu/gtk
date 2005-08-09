gcc -Wall read_vkb.c -o read_vkb -DTEST `pkg-config --cflags --libs glib-2.0` -lexpat
gcc -Wall gen_vkb.c -o gen_vkb -lexpat `pkg-config --cflags --libs glib-2.0`
