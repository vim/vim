/^CFLAGS\b/s/$/ -Wall -Wextra -Wshadow -Werror/
/^PERL_CFLAGS\b/s/$/ -Wno-error=unused-function/
