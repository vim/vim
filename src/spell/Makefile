# Makefile for Vim spell files.

SPELLDIR = ../../runtime/spell
VIM	 = ../vim

all: en
	
en: $(SPELLDIR)/en.latin1.spl \
	$(SPELLDIR)/en.utf-8.spl \
	$(SPELLDIR)/en.ascii.spl

pl: $(SPELLDIR)/pl.iso-8859-2.spl \
	$(SPELLDIR)/pl.utf-8.spl

nl: $(SPELLDIR)/nl.latin1.spl \
	$(SPELLDIR)/nl.utf-8.spl

fr: $(SPELLDIR)/fr.latin1.spl \
	$(SPELLDIR)/fr.utf-8.spl

diff:
	-diff -a -C 1 en_US.orig.aff en_US.aff >en_US.diff
	-diff -a -C 1 en_US.orig.dic en_US.dic >>en_US.diff
	-diff -a -C 1 en_AU.orig.aff en_AU.aff >en_AU.diff
	-diff -a -C 1 en_AU.orig.dic en_AU.dic >>en_AU.diff
	-diff -a -C 1 en_CA.orig.aff en_CA.aff >en_CA.diff
	-diff -a -C 1 en_CA.orig.dic en_CA.dic >>en_CA.diff
	-diff -a -C 1 en_GB.orig.aff en_GB.aff >en_GB.diff
	-diff -a -C 1 en_GB.orig.dic en_GB.dic >>en_GB.diff
	-diff -a -C 1 en_NZ.orig.aff en_NZ.aff >en_NZ.diff
	-diff -a -C 1 en_NZ.orig.dic en_NZ.dic >>en_NZ.diff
	
	-diff -a -C 1 nl_NL.orig.aff nl_NL.aff >nl_NL.diff
	-diff -a -C 1 nl_NL.orig.dic nl_NL.dic >>nl_NL.diff
	
	-diff -a -C 1 pl_PL.orig.aff pl_PL.aff >pl_PL.diff
	-diff -a -C 1 pl_PL.orig.dic pl_PL.dic >>pl_PL.diff
	
	-diff -a -C 1 fr_FR.orig.aff fr_FR.aff >fr_FR.diff
	-diff -a -C 1 fr_FR.orig.dic fr_FR.dic >>fr_FR.diff

$(SPELLDIR)/en.latin1.spl : $(VIM) \
			en_US.aff en_US.dic \
			en_AU.aff en_AU.dic \
			en_CA.aff en_CA.dic \
			en_GB.aff en_GB.dic \
			en_NZ.aff en_NZ.dic
	env LANG=en_US.ISO8859-1 \
		$(VIM) -e -c "mkspell! $(SPELLDIR)/en en_US en_AU en_CA en_GB \
			en_NZ" -c q

$(SPELLDIR)/en.utf-8.spl : $(VIM) \
			en_US.aff en_US.dic \
			en_AU.aff en_AU.dic \
			en_CA.aff en_CA.dic \
			en_GB.aff en_GB.dic \
			en_NZ.aff en_NZ.dic
	env LANG=en_US.UTF-8 \
		$(VIM) -e -c "mkspell! $(SPELLDIR)/en en_US en_AU en_CA en_GB \
			en_NZ" -c q

$(SPELLDIR)/en.ascii.spl : $(VIM) \
			en_US.aff en_US.dic \
			en_AU.aff en_AU.dic \
			en_CA.aff en_CA.dic \
			en_GB.aff en_GB.dic \
			en_NZ.aff en_NZ.dic
	$(VIM) -e -c "mkspell! -ascii $(SPELLDIR)/en en_US en_AU en_CA en_GB \
			en_NZ" -c q

$(SPELLDIR)/pl.iso-8859-2.spl : $(VIM) \
			pl_PL.aff pl_PL.dic
	env LANG=pl_PL.ISO8859-2 $(VIM) -e -c "mkspell! $(SPELLDIR)/pl pl_PL" -c q

$(SPELLDIR)/pl.utf-8.spl : $(VIM) \
			pl_PL.aff pl_PL.dic
	env LANG=pl_PL.UTF-8 $(VIM) -e -c "mkspell! $(SPELLDIR)/pl pl_PL" -c q

$(SPELLDIR)/nl.latin1.spl : $(VIM) \
			nl_NL.aff nl_NL.dic
	env LANG=nl_NL.ISO8859-1 $(VIM) -e -c "mkspell! $(SPELLDIR)/nl nl_NL" -c q

$(SPELLDIR)/nl.utf-8.spl : $(VIM) \
			nl_NL.aff nl_NL.dic
	env LANG=nl_NL.UTF-8 $(VIM) -e -c "mkspell! $(SPELLDIR)/nl nl_NL" -c q

$(SPELLDIR)/fr.latin1.spl : $(VIM) \
			fr_FR.aff fr_FR.dic
	env LANG=fr_FR.ISO8859-1 $(VIM) -e -c "mkspell! $(SPELLDIR)/fr fr_FR" -c q

$(SPELLDIR)/fr.utf-8.spl : $(VIM) \
			fr_FR.aff fr_FR.dic
	env LANG=fr_FR.UTF-8 $(VIM) -e -c "mkspell! $(SPELLDIR)/fr fr_FR" -c q
