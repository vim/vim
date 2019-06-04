#include <assert.h>
#include <stdio.h>
#include "vim.h"

void test_setup(void) {
  vimInput("g");
  vimInput("g");
}

void test_G_gg(void) {
  assert(vimWindowGetCursorLine() == 1);

  vimInput("G");

  assert(vimWindowGetCursorLine() == 9);

  vimInput("g");
  vimInput("g");

  assert(vimWindowGetCursorLine() == 1);
}

void test_j_k(void) {
  assert(vimWindowGetCursorLine() == 1);

  vimInput("j");

  assert(vimWindowGetCursorLine() == 2);

  vimInput("k");

  assert(vimWindowGetCursorLine() == 1);
}

int main(int argc, char **argv) {
  vimInit(argc, argv);

  win_setwidth(5);
  win_setheight(100);

  buf_T *buf = vimBufferOpen("testfile.txt", 1, 0);

  test_G_gg();
  test_j_k();
}
