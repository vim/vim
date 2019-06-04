echo INSTALL: $cur__install
make -f Make_cyg_ming.mak installlibvim DESTDIR=$cur__install CC=x86_64-w64-mingw32-gcc WINDRES=x86_64-w64-mingw32-windres CXX=x86_64-w64-mingw32-g++
