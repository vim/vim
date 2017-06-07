./configure  --prefix=$HOME \
	--with-features=huge \
       	--enable-pythoninterp=yes \
	--enable-cscope\
       	--enable-fail-if-missing \
	--enable-multibyte \
	--enable-fontset \
       	--with-compiledby="HuanGong" \
	--with-python-config-dir=/usr/lib/python2.7/config-x86_64-linux-gnu/\
       	--disable-gui \
	--without-x

make
make install
