`README.md` for version 8.0 of Vim: Vi IMproved.
[![Build Status](https://travis-ci.org/vim/vim.svg?branch=master)](https://travis-ci.org/vim/vim)
[![Coverage Status](https://coveralls.io/repos/vim/vim/badge.svg?branch=master&service=github)](https://coveralls.io/github/vim/vim?branch=master)
[![Appveyor Build status](https://ci.appveyor.com/api/projects/status/o2qht2kjm02sgghk?svg=true)](https://ci.appveyor.com/project/chrisbra/vim)
[![Coverity Scan](https://scan.coverity.com/projects/241/badge.svg)](https://scan.coverity.com/projects/vim)


## What is Vim? ##


this is a private vim fork from offical site for Myself; it will compile without x; 
except it, I also remove clipboard support, because it will cause vim  slow down;



## Compiling ##

step 1: install relative lib on your platrom ; python-dev, ruby, ncurse, etc

```
  sudo apt-get install libncurses5-dev libgnome2-dev \
    libgnomeui-dev libgtk2.0-dev libatk1.0-dev libbonoboui2-dev \
    libcairo2-dev libx11-dev libxpm-dev libxt-dev libncurses-dev \
    libgnome2-dev vim-gnome libgtk2.0-dev libatk1.0-dev libbonoboui2-dev \
    libcairo2-dev libx11-dev libxpm-dev libxt-dev python python-dev ruby ruby-dev
```

step 2: configure & compile
```
# With GUI
./configure --prefix=$HOME --with-features=huge \
            --enable-multibyte \
            --enable-pythoninterp=yes \
            --with-python-config-dir=/usr/lib/python2.7/config-x86_64-linux-gnu \
            --enable-python3interp=yes \
            --with-python3-config-dir=/usr/lib/python3.4/config-3.4m-x86_64-linux-gnu \
            --enable-perlinterp=yes \
            --enable-luainterp=yes \
            --enable-cscope 


# Without GUI & X
./configure  --prefix=$HOME  --with-features=huge --enable-pythoninterp=yes \
	     --enable-cscope --enable-fail-if-missing --enable-multibyte \
	     --enable-fontset --with-compiledby="HuanGong" \
	     --with-python-config-dir=/usr/lib/python2.7/config-x86_64-linux-gnu/ --disable-gui --without-x

```

## Installation ##

in default, I install VIm In my HOME directional


