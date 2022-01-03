/^CFLAGS[[:blank:]]*=/s/$/ -Wall -Wextra -Wshadow -Werror/
/^PERL_CFLAGS_EXTRA[[:blank:]]*=/s/$/ -Wno-error=unused-function/
/^RUBY_CFLAGS_EXTRA[[:blank:]]*=/s/$/ -Wno-error=unused-parameter/
