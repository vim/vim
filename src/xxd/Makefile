# The most simplistic Makefile

# SANITIZER_CFLAGS=-g -O0  -fsanitize-recover=all -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer

xxd: xxd.c
	$(CC) $(SANITIZER_CFLAGS) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -DUNIX -o xxd xxd.c $(LIBS)

clean:
	rm -f xxd xxd.o
	rm -rf xxd.dSYM
