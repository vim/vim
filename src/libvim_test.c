#include "windows.h"
#include <assert.h>
#include <stdio.h>
#include <vim.h>

int main(int argc, char **argv) {
  vimInit(argc, argv);

  win_setwidth(5);
  win_setheight(100);

  buf_T *buf = vimBufferOpen("libvim_test.c", 1, 0);
  assert(vimGetMode() & NORMAL == NORMAL);

  char *line = vimBufferGetLine(buf, 1);
  printf("LINE: %s\n", line);
  int comp = strcmp(line, "#include \"windows.h\"");
  assert(comp == 0);

  assert(vimWindowGetCursorLine() == 1);

  vimInput("G");

  assert(vimWindowGetCursorLine() > 21);

  /* vimExecute("help tutor"); */
  /* assert(vimWindowGetCursorLine() == 32); */

  vimInput("g");
  vimInput("g");

  vimInput("v");
  assert(vimGetMode() & VISUAL == VISUAL);
  vimInput("l");
  vimInput("l");
  vimInput("x");
  printf("CURSOR LINE: %d\n", vimWindowGetCursorLine());
  assert(vimGetMode() & NORMAL == NORMAL);

  line = vimBufferGetLine(buf, 1);
  printf("LINE: %s\n", line);
  printf("Completed\n");
}
