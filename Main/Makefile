CC=gcc
CFLAGS=-Wall -Wextra -O2
TARGET=main

all: $(TARGET)
&(TARGET): main.c 
	$(CC) $(CFLAGS) main.o -o $(TARGET)

main.o: main.c 
	$(CC) $(CFLAGS) -c main.c -o main.o

clean:
	rm -f main main.o 