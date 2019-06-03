#include <assert.h>
#include <stdio.h>
#include "windows.h"
#include <vim.h>

int main(int argc, char** argv) {
    vimInit(argc, argv);

    buf_T *buf = vimBufferOpen("libvim_test.c", 1, 0);

    char* line = vimBufferGetLine(buf, 1);
    int comp = strcmp(line, "#include <assert.h>");
    assert(comp == 0);

    assert(vimWindowGetCursorLine() == 1);

    vimInput("G");

    assert(vimWindowGetCursorLine() > 1);

    vimExecute("help tutor");
    line = vimBufferGetLine(curbuf, 1);
    printf("LINE: %s\n", line);

    printf("Completed\n");
}
