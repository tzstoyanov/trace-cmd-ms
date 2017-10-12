INCLUDE=`pkg-config --cflags gtk+-2.0`
gcc -c -Wall  -fpic plugin_xenomai_gui.c $INCLUDE -L. -lparsevent -ltracecmd
gcc -shared -o plugin_xenomai_gui.so plugin_xenomai_gui.o
