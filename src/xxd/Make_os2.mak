# A very (if most the most) simplistic Makefile for OS/2

CC=gcc
CFLAGS=-O2 -fno-strength-reduce -DOS2

xxd.exe: xxd.c
	$(CC) $(CFLAGS) -s -o $@ $<

clean:
	- del xxd.o
	- del xxd.exe
