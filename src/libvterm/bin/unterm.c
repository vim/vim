#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>

#include "vterm.h"

#define DEFINE_INLINES
#include "../src/utf8.h" /* fill_utf8 */

#define streq(a,b) (!strcmp(a,b))

static VTerm *vt;
static VTermScreen *vts;

static int cols;
static int rows;

static enum {
  FORMAT_PLAIN,
  FORMAT_SGR
} format = FORMAT_PLAIN;

static int col2index(VTermColor target)
{
  int index;

  for(index = 0; index < 256; index++) {
    VTermColor col;
    vterm_state_get_palette_color(NULL, index, &col);
    if(col.red == target.red && col.green == target.green && col.blue == target.blue)
      return index;
  }
  return -1;
}

static void dump_cell(const VTermScreenCell *cell, const VTermScreenCell *prevcell)
{
  switch(format) {
    case FORMAT_PLAIN:
      break;
    case FORMAT_SGR:
      {
        /* If all 7 attributes change, that means 7 SGRs max */
        /* Each colour could consume up to 3 */
        int sgr[7 + 2*3]; int sgri = 0;

        if(!prevcell->attrs.bold && cell->attrs.bold)
          sgr[sgri++] = 1;
        if(prevcell->attrs.bold && !cell->attrs.bold)
          sgr[sgri++] = 22;

        if(!prevcell->attrs.underline && cell->attrs.underline)
          sgr[sgri++] = 4;
        if(prevcell->attrs.underline && !cell->attrs.underline)
          sgr[sgri++] = 24;

        if(!prevcell->attrs.italic && cell->attrs.italic)
          sgr[sgri++] = 3;
        if(prevcell->attrs.italic && !cell->attrs.italic)
          sgr[sgri++] = 23;

        if(!prevcell->attrs.blink && cell->attrs.blink)
          sgr[sgri++] = 5;
        if(prevcell->attrs.blink && !cell->attrs.blink)
          sgr[sgri++] = 25;

        if(!prevcell->attrs.reverse && cell->attrs.reverse)
          sgr[sgri++] = 7;
        if(prevcell->attrs.reverse && !cell->attrs.reverse)
          sgr[sgri++] = 27;

        if(!prevcell->attrs.strike && cell->attrs.strike)
          sgr[sgri++] = 9;
        if(prevcell->attrs.strike && !cell->attrs.strike)
          sgr[sgri++] = 29;

        if(!prevcell->attrs.font && cell->attrs.font)
          sgr[sgri++] = 10 + cell->attrs.font;
        if(prevcell->attrs.font && !cell->attrs.font)
          sgr[sgri++] = 10;

        if(prevcell->fg.red   != cell->fg.red   ||
            prevcell->fg.green != cell->fg.green ||
            prevcell->fg.blue  != cell->fg.blue) {
          int index = col2index(cell->fg);
          if(index == -1)
            sgr[sgri++] = 39;
          else if(index < 8)
            sgr[sgri++] = 30 + index;
          else if(index < 16)
            sgr[sgri++] = 90 + (index - 8);
          else {
            sgr[sgri++] = 38;
            sgr[sgri++] = 5 | (1<<31);
            sgr[sgri++] = index | (1<<31);
          }
        }

        if(prevcell->bg.red   != cell->bg.red   ||
            prevcell->bg.green != cell->bg.green ||
            prevcell->bg.blue  != cell->bg.blue) {
          int index = col2index(cell->bg);
          if(index == -1)
            sgr[sgri++] = 49;
          else if(index < 8)
            sgr[sgri++] = 40 + index;
          else if(index < 16)
            sgr[sgri++] = 100 + (index - 8);
          else {
            sgr[sgri++] = 48;
            sgr[sgri++] = 5 | (1<<31);
            sgr[sgri++] = index | (1<<31);
          }
        }

        if(!sgri)
          break;

        printf("\x1b[");
	{
	  int i;
	  for(i = 0; i < sgri; i++)
	    printf(!i               ? "%d" :
		sgr[i] & (1<<31) ? ":%d" :
		";%d",
		sgr[i] & ~(1<<31));
	}
        printf("m");
      }
      break;
  }

  {
    int i;
    for(i = 0; i < VTERM_MAX_CHARS_PER_CELL && cell->chars[i]; i++) {
      char bytes[6];
      bytes[fill_utf8(cell->chars[i], bytes)] = 0;
      printf("%s", bytes);
    }
  }
}

static void dump_eol(const VTermScreenCell *prevcell)
{
  switch(format) {
    case FORMAT_PLAIN:
      break;
    case FORMAT_SGR:
      if(prevcell->attrs.bold || prevcell->attrs.underline || prevcell->attrs.italic ||
         prevcell->attrs.blink || prevcell->attrs.reverse || prevcell->attrs.strike ||
         prevcell->attrs.font)
        printf("\x1b[m");
      break;
  }

  printf("\n");
}

void dump_row(int row)
{
  VTermPos pos;
  VTermScreenCell prevcell;
  pos.row = row;
  pos.col = 0;
  memset(&prevcell, 0, sizeof(prevcell));
  vterm_state_get_default_colors(vterm_obtain_state(vt), &prevcell.fg, &prevcell.bg);

  while(pos.col < cols) {
    VTermScreenCell cell;
    vterm_screen_get_cell(vts, pos, &cell);

    dump_cell(&cell, &prevcell);

    pos.col += cell.width;
    prevcell = cell;
  }

  dump_eol(&prevcell);
}

static int screen_sb_pushline(int cols, const VTermScreenCell *cells, void *user)
{
  VTermScreenCell prevcell;
  int col;

  memset(&prevcell, 0, sizeof(prevcell));
  vterm_state_get_default_colors(vterm_obtain_state(vt), &prevcell.fg, &prevcell.bg);

  for(col = 0; col < cols; col++) {
    dump_cell(cells + col, &prevcell);
    prevcell = cells[col];
  }

  dump_eol(&prevcell);

  return 1;
}

static int screen_resize(int new_rows, int new_cols, void *user)
{
  rows = new_rows;
  cols = new_cols;
  return 1;
}

static VTermScreenCallbacks cb_screen = {
  NULL, /* damage */
  NULL, /* moverect */
  NULL, /* movecursor */
  NULL, /* settermprop */
  NULL, /* bell */
  &screen_resize, /* resize */
  &screen_sb_pushline, /* sb_pushline */
  NULL, /* popline */
};

int main(int argc, char *argv[])
{
  int opt;
  const char *file;
  int fd;
  int len;
  char buffer[1024];
  int row;

  rows = 25;
  cols = 80;

  while((opt = getopt(argc, argv, "f:l:c:")) != -1) {
    switch(opt) {
      case 'f':
        if(streq(optarg, "plain"))
          format = FORMAT_PLAIN;
        else if(streq(optarg, "sgr"))
          format = FORMAT_SGR;
        else {
          fprintf(stderr, "Unrecognised format '%s'\n", optarg);
          exit(1);
        }
        break;

      case 'l':
        rows = atoi(optarg);
        if(!rows)
          rows = 25;
        break;

      case 'c':
        cols = atoi(optarg);
        if(!cols)
          cols = 80;
        break;
    }
  }

  file = argv[optind++];
  fd = open(file, O_RDONLY);
  if(fd == -1) {
    fprintf(stderr, "Cannot open %s - %s\n", file, strerror(errno));
    exit(1);
  }

  vt = vterm_new(rows, cols);
  vterm_set_utf8(vt, TRUE);

  vts = vterm_obtain_screen(vt);
  vterm_screen_set_callbacks(vts, &cb_screen, NULL);

  vterm_screen_reset(vts, 1);

  while((len = read(fd, buffer, sizeof(buffer))) > 0) {
    vterm_input_write(vt, buffer, len);
  }

  for(row = 0; row < rows; row++) {
    dump_row(row);
  }

  close(fd);

  vterm_free(vt);
  return 0;
}
