#!/bin/bash
# for ubuntu20.04
sudo apt-get install python3-dev liblua5.3-dev ruby-dev libperl-dev tcl-dev libncurses5-dev libgtk-3-dev libatk1.0-dev libx11-dev libxpm-dev libxt-dev -y
./configure --prefix=$HOME/.vim --with-features=huge --enable-gui=gtk3 --enable-python3interp=yes --enable-perlinterp=yes --enable-luainterp=yes --enable-tclinterp=yes --enable-rubyinterp=yes --enable-cscope --with-x --enable-gnome-check --enable-xim --enable-fontset --enable-multibyte --with-modified-by=boddy
make clean
bear -l libear.so make -j12
