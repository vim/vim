#
# Makefile for Vim on MiNT				vim:ts=8:sw=8:tw=78
#
# This is a wrapper around the Unix Makefile. It is configured to accompany
# the MiNT distribution of Vim.
#
# See "Makefile" for instructions how to run "make".
#
# BUT: Always run: "make -f Make_mint.mak config",
#      and then:  "make -f Make_mint.mak"!
# Otherwise the postprocessing won't get done.
#

### This Makefile has been succesfully tested on these systems.
### Check the (*) column for remarks, listed below.
### Later code changes may cause small problems, otherwise Vim is supposed to
### compile and run without problems.

#system:	      configurations:			version (*) tested by:
#-------------	      ------------------------	     -------  -  ----------
#MiNT 1.12.5	      gcc gcc-2.6.1			3.29	 Jens Felderhoff
#MiNT 1.12.6	      gcc gcc-2.6.1	  -GUI		4.6b	 Jens Felderhoff
#MiNT 1.12.6	      gcc gcc-2.6.1	  -GUI		4.6	 Jens Felderhoff

# set this to the pathname prefix of your symbol link editor, i.e. if it is
# /usr/local/bin/sym-ld set:
#
SYMLDPREFIX = /usr/local/bin/sym-
#SYMLDPREFIX = /gnu/bin/sym-

POSTPROCESS = fixstk 20k $(VIMTARGET)
DBGPOSTPROCESS = fixstk 20k $(DBGTARGET)
DBGLDFLAGS = -B$(SYMLDPREFIX)
DBGTARGET = $(VIMTARGET).sym


# Default target is making the executable and then do the post processing
all: $(VIMTARGET) $(TOOLS)
	$(POSTPROCESS)

debug: $(DBGTARGET)
	$(DBGPOSTPROCESS)

#################### include the Unix Makefile ###############

include Makefile


### (M)  MiNT with gcc 2.6.1 and gdb 3.5
CC = gcc -mint
CFLAGS = -g -O -Iproto

$(DBGTARGET): $(OBJ) version.c version.h
	$(CC) -c $(ALL_CFLAGS) version.c
	$(CC) $(LDFLAGS) $(DBGLDFLAGS) -o $(DBGTARGET) -g $(OBJ) \
		version.o $(ALL_LIBS)
