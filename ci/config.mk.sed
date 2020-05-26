/^CFLAGS[[:blank:]]*=/s/$/ -Wall -Wextra -Wshadow -Werror/
/^PERL_CFLAGS[[:blank:]]*=/s/$/ -Wno-error=unused-function/
