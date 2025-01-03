#
# Makefile for UNIX-like for create self-installing exe of Vim.
# 15/12/2024, Restorer restorer@mail2k.ru
#


.SUFFIXES:
.PHONY: all makeinst prepare rename clean 

ifdef VIMSRC
MKNSISFLAGS := -D"VIMSRC=$(VIMSRC)"
endif

ifdef VIMRT
MKNSISFLAGS := $(MKNSISFLAGS) -D"VIMRT=$(VIMRT)"
endif

ifdef VIMTOOLS
MKNSISFLAGS := $(MKNSISFLAGS) -D"VIMTOOLS=$(VIMTOOLS)"
endif

ifdef GETTEXT
MKNSISFLAGS := $(MKNSISFLAGS) -D"GETTEXT=$(GETTEXT)"
endif

ifdef HAVE_UPX
MKNSISFLAGS := $(MKNSISFLAGS) -DHAVE_UPX=$(HAVE_UPX)
endif

ifdef HAVE_NLS
MKNSISFLAGS := $(MKNSISFLAGS) -DHAVE_NLS=$(HAVE_NLS)
endif

ifdef HAVE_MULTI_LANG
MKNSISFLAGS := $(MKNSISFLAGS) -DHAVE_MULTI_LANG=$(HAVE_MULTI_LANG)
endif

ifdef WIN64
MKNSISFLAGS := $(MKNSISFLAGS) -DWIN64=$(WIN64)
endif

ifdef INCLUDE_LIBGCC
MKNSISFLAGS := $(MKNSISFLAGS) -DINCLUDE_LIBGCC=$(INCLUDE_LIBGCC)
endif

ifdef X
XX := -X"$(X:;=" -X")"
endif

MAJOR != grep -E 'VIM_VERSION_MAJOR\s{2,}' ../src/version.h | \
	awk '{ printf "%d",$$3 }'
MINOR != grep -E 'VIM_VERSION_MINOR\s{2,}' ../src/version.h | \
	awk '{ printf "%d",$$3 }'
PATCH != awk '/number below this line/,/,/' ../src/version.c | \
	awk 'NR == 3 { printf "%04d",$$1 }' | sed -e 's/[ ,]//g'

MKNSISFLAGS := -INPUTCHARSET UTF8 $(MKNSISFLAGS)

all: makeinst

makeinst: prepare
	makensis $(MKNSISFLAGS) gvim.nsi $(XX)

prepare: unzipicons gvim_version.nsh license rename

unzipicons: icons.zip
	if test -d `basename $? .zip` ; then rm -rf `basename $? .zip` ; fi
	unzip $?

gvim_version.nsh: Makefile
	echo "# Generated from Makefile: define the version numbers" > $@
	echo "!ifndef __GVIM_VER__NSH__"  >> $@
	echo "!define __GVIM_VER__NSH__"  >> $@
	echo "!define VER_MAJOR $(MAJOR)" >> $@
	echo "!define VER_MINOR $(MINOR)" >> $@
	echo "!define PATCHLEVEL $(PATCH)" >> $@
	echo "!endif" >> $@

license: ../lang/LICENSE.*.txt ../LICENSE
	for lic in $? ; do \
		bn=`basename $$lic .txt` ; \
		awk 'sub("$$", "\r")' < $$lic | \
		iconv -f UTF-8 -t UTF-16 > ../lang/$$bn.nsis.txt ; \
	done

rename:
	../tools/rename.bat "$(SRC)" "$(DST)"

clean:
	if test -f gvim_version.nsh ; then rm -f gvim_version.nsh ; fi
	rm -f ../lang/LICENSE*.nsis.txt
	if test -d icons ; then rm -rf icons ; fi
	if test -f gvim??.exe ; then rm -f gvim??.exe ; fi

# vim: set noet sw=8 ts=8 sts=0 wm=0 tw=0 ft=make:
