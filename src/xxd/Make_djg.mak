# The most simplistic Makefile, for DJGPP on MS-DOS

CFLAGS = -O2 -Wall

xxd.exe: xxd.c
	gcc $(CFLAGS) -s -o xxd.exe xxd.c -lpc

clean:
	del xxd.exe
