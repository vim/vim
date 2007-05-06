# Makefile for the Vim tutor.
#
# The Japanese tutor exists in three encodings.  Use the UTF-8 version as the
# original and create the others with conversion.
#
# Similarly for Russian and Korean

all: tutor.ja.sjis tutor.ja.euc \
	tutor.ko.euc \
	tutor.ru tutor.ru.cp1251 \
	tutor.gr tutor.gr.cp737 \
	tutor.hu

tutor.ja.sjis: tutor.ja.utf-8
	nkf -WXs tutor.ja.utf-8 > tutor.ja.sjis

tutor.ja.euc: tutor.ja.utf-8
	nkf -WXe tutor.ja.utf-8 > tutor.ja.euc

tutor.ko.euc: tutor.ko.utf-8
	iconv -f UTF-8 -t EUC-KR tutor.ko.utf-8 > tutor.ko.euc

tutor.ru: tutor.ru.utf-8
	iconv -f UTF-8 -t KOI8-R tutor.ru.utf-8 > tutor.ru

tutor.ru.cp1251: tutor.ru.utf-8
	iconv -f UTF-8 -t cp1251 tutor.ru.utf-8 > tutor.ru.cp1251

tutor.gr: tutor.gr.utf-8
	iconv -f UTF-8 -t ISO-8859-7 tutor.gr.utf-8 > tutor.gr

tutor.gr.cp737: tutor.gr.utf-8
	iconv -f UTF-8 -t cp737 tutor.gr.utf-8 > tutor.gr.cp737

tutor.hu: tutor.hu.utf-8
	iconv -f UTF-8 -t ISO-8859-2 tutor.hu.utf-8 > tutor.hu
