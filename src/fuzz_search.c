#include "vim.h"

#ifndef PROTO

    int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static int initialized = 0;
    if (!initialized) {
	initialized = fuzzer_init();
        if (!initialized)
            abort();
    }

    // Input format:
    // The first line is regex pattern
    // The rest is the text to search.

    void *p = memchr(data, '\n', size);
    // At least two lines.
    if (!p || p == data)
	return 0;
    // The second line has at least one character (or \n).
    if (p == data + size - 1)
	return 0;

    if (!fuzzer_load_file(data, size))
	abort();

    do_cmdline_cmd((char_u*)"2");
    do_cmdline_cmd((char_u*)"echo search(getline(1), \"W\")");

    do_cmdline_cmd((char_u*)"bwipe!");
    got_int = FALSE;

    return 0;
}

#endif
