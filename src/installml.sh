#! /bin/sh
# installml.sh --- install or uninstall manpage links for Vim
#
# arguments:
# 1  what: "install" or "uninstall"
# 2  also do GUI pages: "yes" or ""
# 3  target directory                e.g., "/usr/local/man/it/man1"
# 4  vim exe name                    e.g., "vim"
# 5  vimdiff exe name                e.g., "vimdiff"
# 6  evim exe name                   e.g., "evim"
# 7  ex exe name                     e.g., "ex"
# 8  view exe name                   e.g., "view"
# 9  rvim exe name                   e.g., "rvim"
# 10 rview exe name                  e.g., "rview"
# 11 gvim exe name                   e.g., "gvim"
# 12 gview exe name                  e.g., "gview"
# 13 rgvim exe name                  e.g., "rgvim"
# 14 rgview exe name                 e.g., "rgview"
# 15 gvimdiff exe name               e.g., "gvimdiff"
# 16 eview exe name                  e.g., "eview"

errstatus=0

what=$1
gui=$2
destdir=$3
vimname=$4
vimdiffname=$5
evimname=$6
exname=$7
viewname=$8
rvimname=$9
rviewname=${10}
gvimname=${11}
gviewname=${12}
rgvimname=${13}
rgviewname=${14}
gvimdiffname=${15}
eviewname=${16}

if test $what = "install" -a (-e $destdir/$vimname.1 -o -e $destdir/$vimdiffname.1 -o -e $destdir/$eviewname.1); then
   if test ! -d $destdir; then
      echo creating $destdir
      ./mkinstalldirs $destdir
   fi

   # ex
   if test ! -e $destdir/$exname.1 -a -e $destdir/$vimname.1; then
      echo creating link $destdir/$exname.1
      cd $destdir; ln -s $vimname.1 $exname.1
   fi

   # view
   if test ! -e $destdir/$viewname.1 -a -e $destdir/$vimname.1; then
      echo creating link $destdir/$viewname.1
      cd $destdir; ln -s $vimname.1 $viewname.1
   fi

   # rvim
   if test ! -e $destdir/$rvimname.1 -a -e $destdir/$vimname.1; then
      echo creating link $destdir/$rvimname.1
      cd $destdir; ln -s $vimname.1 $rvimname.1
   fi

   # rview
   if test ! -e $destdir/$rviewname.1 -a -e $destdir/$vimname.1; then
      echo creating link $destdir/$rviewname.1
      cd $destdir; ln -s $vimname.1 $rviewname.1
   fi

   # GUI targets are optional
   if test "$gui" = "yes"; then
      # gvim
      if test ! -e $destdir/$gvimname.1 -a -e $destdir/$vimname.1; then
	 echo creating link $destdir/$gvimname.1
	 cd $destdir; ln -s $vimname.1 $gvimname.1
      fi

      # gview
      if test ! -e $destdir/$gviewname.1 -a -e $destdir/$vimname.1; then
	 echo creating link $destdir/$gviewname.1
	 cd $destdir; ln -s $vimname.1 $gviewname.1
      fi

      # rgvim
      if test ! -e $destdir/$rgvimname.1 -a -e $destdir/$vimname.1; then
	 echo creating link $destdir/$rgvimname.1
	 cd $destdir; ln -s $vimname.1 $rgvimname.1
      fi

      # rgview
      if test ! -e $destdir/$rgviewname.1 -a -e $destdir/$vimname.1; then
	 echo creating link $destdir/$rgviewname.1
	 cd $destdir; ln -s $vimname.1 $rgviewname.1
      fi

      # gvimdiff
      if test ! -e $destdir/$gvimdiffname.1 -a -e $destdir/$vimdiffname.1; then
	 echo creating link $destdir/$gvimdiffname.1
	 cd $destdir; ln -s $vimdiffname.1 $gvimdiffname.1
      fi

      # eview
      if test ! -e $destdir/$eviewname.1 -a -e $destdir/$evimname.1; then
	 echo creating link $destdir/$eviewname.1
	 cd $destdir; ln -s $evimname.1 $eviewname.1
      fi
   fi
fi

if test $what = "uninstall"; then
   echo Checking for Vim manual page links in $destdir...

   if test -L $destdir/$exname.1; then
      echo deleting $destdir/$exname.1
      rm -f $destdir/$exname.1
   fi
   if test -L $destdir/$viewname.1; then
      echo deleting $destdir/$viewname.1
      rm -f $destdir/$viewname.1
   fi
   if test -L $destdir/$rvimname.1; then
      echo deleting $destdir/$rvimname.1
      rm -f $destdir/$rvimname.1
   fi
   if test -L $destdir/$rviewname.1; then
      echo deleting $destdir/$rviewname.1
      rm -f $destdir/$rviewname.1
   fi

   # GUI targets are optional
   if test "$gui" = "yes"; then
      if test -L $destdir/$gvimname.1; then
	 echo deleting $destdir/$gvimname.1
	 rm -f $destdir/$gvimname.1
      fi
      if test -L $destdir/$gviewname.1; then
	 echo deleting $destdir/$gviewname.1
	 rm -f $destdir/$gviewname.1
      fi
      if test -L $destdir/$rgvimname.1; then
	 echo deleting $destdir/$rgvimname.1
	 rm -f $destdir/$rgvimname.1
      fi
      if test -L $destdir/$rgviewname.1; then
	 echo deleting $destdir/$rgviewname.1
	 rm -f $destdir/$rgviewname.1
      fi
      if test -L $destdir/$gvimdiffname.1; then
	 echo deleting $destdir/$gvimdiffname.1
	 rm -f $destdir/$gvimdiffname.1
      fi
      if test -L $destdir/$eviewname.1; then
	 echo deleting $destdir/$eviewname.1
	 rm -f $destdir/$eviewname.1
      fi
   fi
fi

exit $errstatus

# vim: set sw=3 :
