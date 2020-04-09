/^CFLAGS\b/s/$/ -Wall -Wextra -Wshadow -Werror/
/^PERL_CFLAGS\b/s/$/ -Wno-error=unused-function/
/^RUBY_CFLAGS\b/s/$/ -Wno-error=unknown-attributes/
