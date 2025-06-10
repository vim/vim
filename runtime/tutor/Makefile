# Makefile for the Vim tutor.
#
# Use the UTF-8 version as the original and create the others with conversion.
# For some translation files of chapter one, conversion from traditional
# encodings to UTF-8 encoding is performed.

# Common components
include Make_all.mak

.PHONY: all clean

.SUFFIXES:

all: $(CONVERTED)

tutor1.utf-8: tutor1
	iconv -f ISO-8859-1 -t UTF-8 $? >$@

tutor2: tutor2.utf-8
	iconv -f UTF-8 -t ISO-8859-1 $? >$@

tutor1.bar tutor2.bar:
	iconv -f UTF-8 -t ISO-8859-1 $@.utf-8 >$@

tutor1.ca.utf-8: tutor1.ca
	iconv -f ISO-8859-1 -t UTF-8 $? >$@

tutor2.ca: tutor2.ca.utf-8
	iconv -f UTF-8 -t ISO-8859-1 $? >$@

tutor1.cs tutor2.cs :
	iconv -f UTF-8 -t ISO-8859-2 $@.utf-8 >$@

tutor1.cs.cp1250 tutor2.cs.cp1250 :
	iconv -f UTF-8 -t CP1250 $(@:.cp1250=).utf-8 >$@

tutor1.da tutor2.da :
	iconv -f UTF-8 -t ISO-8859-4 $@.utf-8 >$@

tutor1.de.utf-8: tutor1.de
	iconv -f ISO-8859-1 -t UTF-8 $? >$@

tutor2.de: tutor2.de.utf-8
	iconv -f UTF-8 -t ISO-8859-1 $? >$@

tutor1.el tutor2.el:
	iconv -f UTF-8 -t ISO-8859-7 $@.utf-8 >$@

tutor1.el.cp737 tutor2.el.cp737:
	iconv -f UTF-8 -t CP737 $(@:.cp737=.utf-8) >$@

tutor1.eo tutor2.eo:
	iconv -f UTF-8 -t ISO-8859-3 $@.utf-8 >$@

tutor1.es tutor2.es:
	iconv -f UTF-8 -t ISO-8859-1 $@.utf-8 >$@

tutor1.fr.utf-8: tutor1.fr
	iconv -f ISO-8859-1 -t UTF-8 $? >$@

tutor2.fr: tutor2.fr.utf-8
	iconv -f UTF-8 -t ISO-8859-1 $? >$@

tutor1.hr tutor2.hr:
	iconv -f UTF-8 -t ISO-8859-2 $@.utf-8 >$@

tutor1.hr.cp1250 tutor2.hr.cp1250:
	iconv -f UTF-8 -t CP1250 $(@:.cp1250=.utf-8) >$@

tutor1.hu tutor2.hu:
	iconv -f UTF-8 -t ISO-8859-2 $@.utf-8 >$@

tutor1.hu.cp1250 tutor2.hu.cp1250:
	iconv -f UTF-8 -t CP1250 $(@:.cp1250=.utf-8) >$@

tutor1.it.utf-8: tutor1.it
	iconv -f ISO-8859-1 -t UTF-8 $? >$@

tutor2.it: tutor2.it.utf-8
	iconv -f UTF-8 -t ISO-8859-1 $? >$@

tutor1.ja.sjis tutor2.ja.sjis:
	iconv -f UTF-8 -t CP932 $(@:.sjis=.utf-8) >$@

tutor1.ja.euc tutor2.ja.euc:
	iconv -f UTF-8 -t EUC-JP $(@:.euc=.utf-8) >$@

tutor1.ko tutor2.ko:
	ln $@.utf-8 $@

tutor1.ko.euc tutor2.ko.euc:
	iconv -f UTF-8 -t EUC-KR $(@:.euc=.utf-8) >$@

tutor1.nl tutor2.nl:
	iconv -f UTF-8 -t ISO-8859-1 $@.utf-8 >$@

tutor1.no.utf-8: tutor1.no
	iconv -f ISO-8859-1 -t UTF-8 $? >$@

tutor2.no: tutor2.no.utf-8
	iconv -f UTF-8 -t ISO-8859-1 $? >$@

# nb is an alias for no
tutor1.nb tutor2.nb:
	ln $(@:.nb=.no) $@

tutor1.nb.utf-8 tutor2.nb.utf-8:
	ln $(@:.nb.utf-8=.no.utf-8) $@

tutor1.pl tutor2.pl :
	iconv -f UTF-8 -t ISO-8859-2 $@.utf-8 >$@

tutor1.pl.cp1250 tutor2.pl.cp1250 :
	iconv -f UTF-8 -t CP1250 $(@:.cp1250=).utf-8 >$@

tutor1.pt tutor2.pt :
	iconv -f UTF-8 -t ISO-8859-15 $@.utf-8 >$@

tutor1.ru tutor2.ru:
	iconv -f UTF-8 -t KOI8-R $@.utf-8 >$@

tutor1.ru.cp1251 tutor2.ru.cp1251:
	iconv -f UTF-8 -t CP1251 $(@:.cp1251=.utf-8) >$@

tutor1.sk tutor2.sk :
	iconv -f UTF-8 -t ISO-8859-2 $@.utf-8 >$@

tutor1.sk.cp1250 tutor2.sk.cp1250 :
	iconv -f UTF-8 -t CP1250 $(@:.cp1250=).utf-8 >$@

tutor1.sr.cp1250 tutor2.sr.cp1250 :
	iconv -f UTF-8 -t CP1250 $(@:.cp1250=).utf-8 >$@

tutor1.sv.utf-8: tutor1.sv
	iconv -f ISO-8859-15 -t UTF-8 $? >$@

tutor2.sv: tutor2.sv.utf-8
	iconv -f UTF-8 -t ISO-8859-15 $? >$@

tutor1.tr.iso9 tutor2.tr.iso9:
	iconv -f UTF-8 -t ISO-8859-9 $(@:.iso9=.utf-8) >$@

tutor1.zh.utf-8: tutor1.zh.big5
	iconv -f BIG-5 -t UTF-8 -c $? >$@

tutor2.zh.big5: tutor2.zh.utf-8
	iconv -f UTF-8 -t BIG-5 -c $? >$@

clean:
	for G in $(CONVERTED); do if [ -f ./$$G ]; then rm -f ./$$G; fi; done;
