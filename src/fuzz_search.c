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
    // The first line is regex pattern.
    // The rest is the text to search. Could be empty.

    if (!fuzzer_load_file(data, size))
	abort();

    if (curbuf->b_ml.ml_line_count == 1)
	do_cmdline_cmd((char_u*)"call setline(2, \"\")");

    do_cmdline_cmd((char_u*)"2");
    do_cmdline_cmd((char_u*)"echo search(getline(1), \"W\")");

    do_cmdline_cmd((char_u*)"bwipe!");
    got_int = FALSE;

    return 0;
}

#endif
