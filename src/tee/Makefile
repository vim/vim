# A very (if not the most) simplistic Makefile for OS/2

CC=gcc
CFLAGS=-O2 -fno-strength-reduce

tee.exe: tee.o
	$(CC) $(CFLAGS) -s -o $@ $<

tee.o: tee.c
	$(CC) $(CFLAGS) -c $<

clean:
	- del tee.o
	- del tee.exe

