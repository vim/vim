README_os_390.txt for version 7.2 of Vim: Vi IMproved.

Welcome to the OS/390 Unix port of VIM.

ATTENTION: THIS IS AN _ALPHA_ VERSION!!!
I expect you to know how to handle alpha software!

This port was done by Ralf Schandl <schandl@de.ibm.com>.
This port is not maintained or supported by IBM!!


For the list of changes see runtime/doc/os_390.txt.


Compiling:
----------

Note: After the file configure was created via autoconf, it had been
      handedited, to make the test for ICEConnectionNumber work.
      DO NOT run autoconf again!

Without X11:

If you build VIM without X11 support, compiling and building is nearly
straightforward. The only restriction is, that you can't call make from the
VIM toplevel directory. Change to the src directory first and call make from
there. Here is a what to do:

    # Don't use c89!
    # Make additional symbols visible.
    # Allow intermixing of compiler options and files.

    $ export CC=cc
    $ export CFLAGS=-D_ALL_SOURCE
    $ export _CC_CCMODE=1
    $./configure --enable-max-features --without-x --enable-gui=no
    $ cd src
    $ make
    $ make test

      Note: Test 28 will be reported as failed. This is because diff can't
	    compare files containing '\0' characters. Test 11 will fail if you
	    don't have gzip.

    $ make install


With X11:

There are two ways for building VIM with X11 support. The first way is simple
and results in a big executable (~13 Mb), the second needs a few additional
steps and results in a much smaller executable (~4.5 Mb). This examples assume
you want Motif.

  The easy way:
    $ export CC=cc
    $ export CFLAGS="-D_ALL_SOURCE -W c,dll"
    $ export LDFLAGS="-W l,dll"
    $ export _CC_CCMODE=1
    $ ./configure --enable-max-features --enable-gui=motif
    $ cd src
    $ make

    With this VIM is linked statically with the X11 libraries.

  The smarter way:
    Make VIM as described above. Then create a file named 'link.sed' with the
    following contense:

	s/-lXext  *//g
	s/-lXmu  *//g
	s/-lXm	*/\/usr\/lib\/Xm.x /g
	s/-lX11  */\/usr\/lib\/X11.x /g
	s/-lXt	*//g
	s/-lSM	*/\/usr\/lib\/SM.x /g
	s/-lICE  */\/usr\/lib\/ICE.x /g

    Then do:
    $ rm vim
    $ make

    Now Vim is linked with the X11-DLLs.

    See the Makefile and the file link.sh on how link.sed is used.


Hint:
-----
Use the online help! (See weaknesses below.)

Example:
Enter ':help syntax' and then press <TAB> several times, you will switch
through all help items containing 'syntax'. Press <ENTER> on the one you are
interested at. Or press <Ctrl-D> and you will get a list of all items printed
that contain 'syntax'.

The helpfiles contains cross-references. Links are between '|'. Position the
cursor on them and press <Ctrl-]> to follow this link. Use <Ctrl-T> to jump
back.

Known weaknesses:
-----------------

- You can't call make from the toplevel directory, you have to do a 'cd src'
  first.  If you do it, make will call configure again. I don't know why and
  didn't investigate it, there were more important things to do. If you can
  make it work drop me a note.

- The documentation was not updated for this alpha release. It contains lot of
  ASCII dependencies, especially in examples.

- Digraphs are dependent on code page 1047. Digraphs are used to enter
  characters that normally cannot be entered by an ordinary keyboard.
  See ":help digraphs".

- Using 'ga' to show the code of the character under the cursor shows the
  correct dec/hex/oct values, but the other informations might be missing or
  wrong.

- The sed syntax file doesn't work, it is ASCII dependent.

Bugs:
-----
If you find a bug please inform me (schandl@de.ibm.com), don't disturb Bram
Moolenaar. It's most likely a bug I introduced during porting or some ASCII
dependency I didn't notice.

Feedback:
---------
Feedback welcome! Just drop me a note.
