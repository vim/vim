# Makefile for the Vim message translations.

# TODO make this configurable
# Note: ja.sjis, *.cp1250 and zh_CN.cp936 are only for MS-Windows, they are
# not installed on Unix

LANGUAGES =	af ca cs de en_GB es fr it ja ko no pl ru sk sv uk zh_TW \
		zh_TW.UTF-8 zh_CN zh_CN.UTF-8
MOFILES =	af.mo ca.mo cs.mo de.mo en_GB.mo es.mo fr.mo it.mo ja.mo \
		ko.mo no.mo pl.mo ru.mo sk.mo sv.mo uk.mo \
		zh_TW.mo zh_TW.UTF-8.mo zh_CN.mo zh_CN.UTF-8.mo

PACKAGE = vim
SHELL = /bin/sh

# The OLD_PO_FILE_INPUT and OLD_PO_FILE_OUTPUT are for the new GNU gettext
# tools 0.10.37, which use a slightly different .po file format that is not
# compatible with Solaris (and old gettext implementations) unless these are
# set.  gettext 0.10.36 will not work!
MSGFMT = OLD_PO_FILE_INPUT=yes msgfmt -v
XGETTEXT = OLD_PO_FILE_INPUT=yes OLD_PO_FILE_OUTPUT=yes xgettext
MSGMERGE = OLD_PO_FILE_INPUT=yes OLD_PO_FILE_OUTPUT=yes msgmerge

.SUFFIXES:
.SUFFIXES: .po .mo .pot
.PHONY: all install uninstall check clean distclean $(LANGUAGES)

.po.mo:
	$(MSGFMT) -o $@ $<

all: $(MOFILES)

install: $(MOFILES)
	@$(MAKE) check
	for lang in $(LANGUAGES); do \
	  dir=$(LOCALEDIR)/$$lang/; \
	  if test ! -x "$$dir"; then \
	    mkdir $$dir; chmod 755 $$dir; \
	  fi; \
	  dir=$(LOCALEDIR)/$$lang/LC_MESSAGES; \
	  if test ! -x "$$dir"; then \
	    mkdir $$dir; chmod 755 $$dir; \
	  fi; \
	  if test -r $$lang.mo; then \
	    $(INSTALL_DATA) $$lang.mo $$dir/$(PACKAGE).mo; \
	    chmod $(FILEMOD) $$dir/$(PACKAGE).mo; \
	  fi; \
	done

uninstall:
	@$(MAKE) check
	for cat in $(MOFILES); do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed 's/\$(CATOBJEXT)$$//'`; \
	  rm -f $(LOCALEDIR)/$$lang/LC_MESSAGES/$(PACKAGE).mo; \
	done

converted: ja.sjis.mo cs.cp1250.mo pl.cp1250.mo sk.cp1250.mo zh_CN.cp936.mo \
		ru.cp1251.mo

# Convert ja.po to create ja.sjis.po.  Requires doubling backslashes in the
# second byte.  Don't depend on sjiscorr, it should only be compiled when
# ja.sjis.po is outdated.
ja.sjis.po: ja.po
	@$(MAKE) sjiscorr
	rm -f ja.sjis.po
	iconv -f euc-jp -t cp932 ja.po | ./sjiscorr > ja.sjis.po

sjiscorr: sjiscorr.c
	$(CC) -o sjiscorr sjiscorr.c

# Convert cs.po to create cs.cp1250.po.
cs.cp1250.po: cs.po
	rm -f cs.cp1250.po
	iconv -f iso-8859-2 -t cp1250 cs.po | \
		sed -e 's/charset=ISO-8859-2/charset=cp1250/' -e 's/# Original translations/# Generated from cs.po, DO NOT EDIT/' > cs.cp1250.po

# Convert pl.po to create pl.cp1250.po.
pl.cp1250.po: pl.po
	rm -f pl.cp1250.po
	iconv -f iso-8859-2 -t cp1250 pl.po | \
		sed -e 's/charset=ISO-8859-2/charset=cp1250/' -e 's/# Original translations/# Generated from pl.po, DO NOT EDIT/' > pl.cp1250.po

# Convert sk.po to create sk.cp1250.po.
sk.cp1250.po: sk.po
	rm -f sk.cp1250.po
	iconv -f iso-8859-2 -t cp1250 sk.po | \
		sed -e 's/charset=ISO-8859-2/charset=cp1250/' -e 's/# Original translations/# Generated from sk.po, DO NOT EDIT/' > sk.cp1250.po

# Convert zh_CN.po to create zh_CN.cp936.po.
# set 'charset' to gbk to avoid that msfmt generates a warning
zh_CN.cp936.po: zh_CN.po
	rm -f zh_CN.cp936.po
	iconv -f gb2312 -t cp936 zh_CN.po | \
		sed -e 's/charset=gb2312/charset=gbk/' -e 's/# Original translations/# Generated from zh_CN.po, DO NOT EDIT/' > zh_CN.cp936.po

# Convert ru.po to create ru.cp1251.po.
ru.cp1251.po: ru.po
	rm -f ru.cp1251.po
	iconv -f koi8-r -t cp1251 ru.po | \
		sed -e 's/charset=koi8-r/charset=cp1251/' -e 's/# Original translations/# Generated from ru.po, DO NOT EDIT/' > ru.cp1251.po

check:
	@if test "x" = "x$(prefix)"; then \
	  echo "******************************************"; \
	  echo "  please use make from the src directory  "; \
	  echo "******************************************"; \
	  exit 1; \
	fi

clean:
	rm -f core core.* *.old.po *.mo *.pot sjiscorr

distclean: clean

#
# NOTE: If you get an error for gvimext.cpp not found, you need to unpack the
# extra archive.
#
$(PACKAGE).pot: ../*.c ../if_perl.xs ../GvimExt/gvimext.cpp ../globals.h
	cd ..; $(XGETTEXT) --default-domain=$(PACKAGE) \
		--add-comments --keyword=_ --keyword=N_ \
		*.c if_perl.xs GvimExt/gvimext.cpp globals.h
	mv -f ../$(PACKAGE).po $(PACKAGE).pot

# Don't add a dependency here, we only want to update the .po files manually
$(LANGUAGES):
	@$(MAKE) $(PACKAGE).pot
	if test ! -f $@.po.orig; then cp $@.po $@.po.orig; fi
	mv $@.po $@.po.old
	if $(MSGMERGE) $@.po.old $(PACKAGE).pot -o $@.po; then \
	    rm -f $@.po.old; \
	else \
	    echo "msgmerge for $@.po failed!"; mv $@.po.old $@.po; \
	fi
