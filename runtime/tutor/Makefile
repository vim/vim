# Makefile for the Vim tutor.
#
# The Japanese tutor exists in three encodings.  Use the UTF-8 version as the
# original and create the others with conversion.

all: tutor.ja.sjis tutor.ja.euc tutor.ko.euc

tutor.ja.sjis: tutor.ja.utf-8
	nkf -WXs tutor.ja.utf-8 > tutor.ja.sjis

tutor.ja.euc: tutor.ja.utf-8
	nkf -WXe tutor.ja.utf-8 > tutor.ja.euc

tutor.ko.euc: tutor.ko.utf-8
	iconv -f UTF-8 -t EUC-KR tutor.ko.utf-8 > tutor.ko.euc
