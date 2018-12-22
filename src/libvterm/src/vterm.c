#define DEFINE_INLINES

#include "vterm_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "utf8.h"

/*****************
 * API functions *
 *****************/

static void *default_malloc(size_t size, void *allocdata UNUSED)
{
  void *ptr = malloc(size);
  if(ptr)
    memset(ptr, 0, size);
  return ptr;
}

static void default_free(void *ptr, void *allocdata UNUSED)
{
  free(ptr);
}

static VTermAllocatorFunctions default_allocator = {
  &default_malloc, // malloc
  &default_free // free
};

VTerm *vterm_new(int rows, int cols)
{
  return vterm_new_with_allocator(rows, cols, &default_allocator, NULL);
}

VTerm *vterm_new_with_allocator(int rows, int cols, VTermAllocatorFunctions *funcs, void *allocdata)
{
  /* Need to bootstrap using the allocator function directly */
  VTerm *vt = (*funcs->malloc)(sizeof(VTerm), allocdata);

  vt->allocator = funcs;
  vt->allocdata = allocdata;

  vt->rows = rows;
  vt->cols = cols;

  vt->parser.state = NORMAL;

  vt->parser.callbacks = NULL;
  vt->parser.cbdata    = NULL;

  vt->parser.strbuffer_len = 500; /* should be able to hold an OSC string */
  vt->parser.strbuffer_cur = 0;
  vt->parser.strbuffer = vterm_allocator_malloc(vt, vt->parser.strbuffer_len);

  vt->outbuffer_len = 200;
  vt->outbuffer_cur = 0;
  vt->outbuffer = vterm_allocator_malloc(vt, vt->outbuffer_len);

  return vt;
}

void vterm_free(VTerm *vt)
{
  if(vt->screen)
    vterm_screen_free(vt->screen);

  if(vt->state)
    vterm_state_free(vt->state);

  vterm_allocator_free(vt, vt->parser.strbuffer);
  vterm_allocator_free(vt, vt->outbuffer);

  vterm_allocator_free(vt, vt);
}

INTERNAL void *vterm_allocator_malloc(VTerm *vt, size_t size)
{
  return (*vt->allocator->malloc)(size, vt->allocdata);
}

INTERNAL void vterm_allocator_free(VTerm *vt, void *ptr)
{
  (*vt->allocator->free)(ptr, vt->allocdata);
}

void vterm_get_size(const VTerm *vt, int *rowsp, int *colsp)
{
  if(rowsp)
    *rowsp = vt->rows;
  if(colsp)
    *colsp = vt->cols;
}

void vterm_set_size(VTerm *vt, int rows, int cols)
{
  vt->rows = rows;
  vt->cols = cols;

  if(vt->parser.callbacks && vt->parser.callbacks->resize)
    (*vt->parser.callbacks->resize)(rows, cols, vt->parser.cbdata);
}

int vterm_get_utf8(const VTerm *vt)
{
  return vt->mode.utf8;
}

void vterm_set_utf8(VTerm *vt, int is_utf8)
{
  vt->mode.utf8 = is_utf8;
}

INTERNAL void vterm_push_output_bytes(VTerm *vt, const char *bytes, size_t len)
{
  if(len > vt->outbuffer_len - vt->outbuffer_cur) {
    DEBUG_LOG("vterm_push_output(): buffer overflow; truncating output\n");
    len = vt->outbuffer_len - vt->outbuffer_cur;
  }

  memcpy(vt->outbuffer + vt->outbuffer_cur, bytes, len);
  vt->outbuffer_cur += len;
}

static int outbuffer_is_full(VTerm *vt)
{
  return vt->outbuffer_cur >= vt->outbuffer_len - 1;
}

#if (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= 500) \
	|| defined(_ISOC99_SOURCE) || defined(_BSD_SOURCE)
# undef VSNPRINTF
# define VSNPRINTF vsnprintf
#else
# ifdef VSNPRINTF
/* Use a provided vsnprintf() function. */
int VSNPRINTF(char *str, size_t str_m, const char *fmt, va_list ap);
# endif
#endif


INTERNAL void vterm_push_output_vsprintf(VTerm *vt, const char *format, va_list args)
{
  int written;
#ifndef VSNPRINTF
  /* When vsnprintf() is not available (C90) fall back to vsprintf(). */
  char buffer[1024]; /* 1Kbyte is enough for everybody, right? */
#endif

  if(outbuffer_is_full(vt)) {
    DEBUG_LOG("vterm_push_output(): buffer overflow; truncating output\n");
    return;
  }

#ifdef VSNPRINTF
  written = VSNPRINTF(vt->outbuffer + vt->outbuffer_cur,
      vt->outbuffer_len - vt->outbuffer_cur,
      format, args);

  if(written == (int)(vt->outbuffer_len - vt->outbuffer_cur)) {
    /* output was truncated */
    vt->outbuffer_cur = vt->outbuffer_len - 1;
  }
  else
    vt->outbuffer_cur += written;
#else
  written = vsprintf(buffer, format, args);

  if(written >= (int)(vt->outbuffer_len - vt->outbuffer_cur - 1)) {
    /* output was truncated */
    written = vt->outbuffer_len - vt->outbuffer_cur - 1;
  }
  if (written > 0)
  {
    strncpy(vt->outbuffer + vt->outbuffer_cur, buffer, written + 1);
    vt->outbuffer_cur += written;
  }
#endif
}

INTERNAL void vterm_push_output_sprintf(VTerm *vt, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vterm_push_output_vsprintf(vt, format, args);
  va_end(args);
}

INTERNAL void vterm_push_output_sprintf_ctrl(VTerm *vt, unsigned char ctrl, const char *fmt, ...)
{
  size_t orig_cur = vt->outbuffer_cur;
  va_list args;

  if(ctrl >= 0x80 && !vt->mode.ctrl8bit)
    vterm_push_output_sprintf(vt, ESC_S "%c", ctrl - 0x40);
  else
    vterm_push_output_sprintf(vt, "%c", ctrl);

  va_start(args, fmt);
  vterm_push_output_vsprintf(vt, fmt, args);
  va_end(args);

  if(outbuffer_is_full(vt))
    vt->outbuffer_cur = orig_cur;
}

INTERNAL void vterm_push_output_sprintf_dcs(VTerm *vt, const char *fmt, ...)
{
  size_t orig_cur = vt->outbuffer_cur;
  va_list args;

  if(!vt->mode.ctrl8bit)
    vterm_push_output_sprintf(vt, ESC_S "%c", C1_DCS - 0x40);
  else
    vterm_push_output_sprintf(vt, "%c", C1_DCS);

  va_start(args, fmt);
  vterm_push_output_vsprintf(vt, fmt, args);
  va_end(args);

  vterm_push_output_sprintf_ctrl(vt, C1_ST, "");

  if(outbuffer_is_full(vt))
    vt->outbuffer_cur = orig_cur;
}

size_t vterm_output_get_buffer_size(const VTerm *vt)
{
  return vt->outbuffer_len;
}

size_t vterm_output_get_buffer_current(const VTerm *vt)
{
  return vt->outbuffer_cur;
}

size_t vterm_output_get_buffer_remaining(const VTerm *vt)
{
  return vt->outbuffer_len - vt->outbuffer_cur;
}

size_t vterm_output_read(VTerm *vt, char *buffer, size_t len)
{
  if(len > vt->outbuffer_cur)
    len = vt->outbuffer_cur;

  memcpy(buffer, vt->outbuffer, len);

  if(len < vt->outbuffer_cur)
    memmove(vt->outbuffer, vt->outbuffer + len, vt->outbuffer_cur - len);

  vt->outbuffer_cur -= len;

  return len;
}

VTermValueType vterm_get_attr_type(VTermAttr attr)
{
  switch(attr) {
    case VTERM_ATTR_BOLD:       return VTERM_VALUETYPE_BOOL;
    case VTERM_ATTR_UNDERLINE:  return VTERM_VALUETYPE_INT;
    case VTERM_ATTR_ITALIC:     return VTERM_VALUETYPE_BOOL;
    case VTERM_ATTR_BLINK:      return VTERM_VALUETYPE_BOOL;
    case VTERM_ATTR_REVERSE:    return VTERM_VALUETYPE_BOOL;
    case VTERM_ATTR_STRIKE:     return VTERM_VALUETYPE_BOOL;
    case VTERM_ATTR_FONT:       return VTERM_VALUETYPE_INT;
    case VTERM_ATTR_FOREGROUND: return VTERM_VALUETYPE_COLOR;
    case VTERM_ATTR_BACKGROUND: return VTERM_VALUETYPE_COLOR;

    case VTERM_N_ATTRS: return 0;
  }
  return 0; /* UNREACHABLE */
}

VTermValueType vterm_get_prop_type(VTermProp prop)
{
  switch(prop) {
    case VTERM_PROP_CURSORVISIBLE: return VTERM_VALUETYPE_BOOL;
    case VTERM_PROP_CURSORBLINK:   return VTERM_VALUETYPE_BOOL;
    case VTERM_PROP_ALTSCREEN:     return VTERM_VALUETYPE_BOOL;
    case VTERM_PROP_TITLE:         return VTERM_VALUETYPE_STRING;
    case VTERM_PROP_ICONNAME:      return VTERM_VALUETYPE_STRING;
    case VTERM_PROP_REVERSE:       return VTERM_VALUETYPE_BOOL;
    case VTERM_PROP_CURSORSHAPE:   return VTERM_VALUETYPE_INT;
    case VTERM_PROP_MOUSE:         return VTERM_VALUETYPE_INT;
    case VTERM_PROP_CURSORCOLOR:   return VTERM_VALUETYPE_STRING;

    case VTERM_N_PROPS: return 0;
  }
  return 0; /* UNREACHABLE */
}

void vterm_scroll_rect(VTermRect rect,
    int downward,
    int rightward,
    int (*moverect)(VTermRect src, VTermRect dest, void *user),
    int (*eraserect)(VTermRect rect, int selective, void *user),
    void *user)
{
  VTermRect src;
  VTermRect dest;

  if(abs(downward)  >= rect.end_row - rect.start_row ||
     abs(rightward) >= rect.end_col - rect.start_col) {
    /* Scroll more than area; just erase the lot */
    (*eraserect)(rect, 0, user);
    return;
  }

  if(rightward >= 0) {
    /* rect: [XXX................]
     * src:     [----------------]
     * dest: [----------------]
     */
    dest.start_col = rect.start_col;
    dest.end_col   = rect.end_col   - rightward;
    src.start_col  = rect.start_col + rightward;
    src.end_col    = rect.end_col;
  }
  else {
    /* rect: [................XXX]
     * src:  [----------------]
     * dest:    [----------------]
     */
    int leftward = -rightward;
    dest.start_col = rect.start_col + leftward;
    dest.end_col   = rect.end_col;
    src.start_col  = rect.start_col;
    src.end_col    = rect.end_col - leftward;
  }

  if(downward >= 0) {
    dest.start_row = rect.start_row;
    dest.end_row   = rect.end_row   - downward;
    src.start_row  = rect.start_row + downward;
    src.end_row    = rect.end_row;
  }
  else {
    int upward = -downward;
    dest.start_row = rect.start_row + upward;
    dest.end_row   = rect.end_row;
    src.start_row  = rect.start_row;
    src.end_row    = rect.end_row - upward;
  }

  if(moverect)
    (*moverect)(dest, src, user);

  if(downward > 0)
    rect.start_row = rect.end_row - downward;
  else if(downward < 0)
    rect.end_row = rect.start_row - downward;

  if(rightward > 0)
    rect.start_col = rect.end_col - rightward;
  else if(rightward < 0)
    rect.end_col = rect.start_col - rightward;

  (*eraserect)(rect, 0, user);
}

void vterm_copy_cells(VTermRect dest,
    VTermRect src,
    void (*copycell)(VTermPos dest, VTermPos src, void *user),
    void *user)
{
  int downward  = src.start_row - dest.start_row;
  int rightward = src.start_col - dest.start_col;

  int init_row, test_row, init_col, test_col;
  int inc_row, inc_col;

  VTermPos pos;

  if(downward < 0) {
    init_row = dest.end_row - 1;
    test_row = dest.start_row - 1;
    inc_row = -1;
  }
  else /* downward >= 0 */ {
    init_row = dest.start_row;
    test_row = dest.end_row;
    inc_row = +1;
  }

  if(rightward < 0) {
    init_col = dest.end_col - 1;
    test_col = dest.start_col - 1;
    inc_col = -1;
  }
  else /* rightward >= 0 */ {
    init_col = dest.start_col;
    test_col = dest.end_col;
    inc_col = +1;
  }

  for(pos.row = init_row; pos.row != test_row; pos.row += inc_row)
    for(pos.col = init_col; pos.col != test_col; pos.col += inc_col) {
      VTermPos srcpos;
      srcpos.row = pos.row + downward;
      srcpos.col = pos.col + rightward;
      (*copycell)(pos, srcpos, user);
    }
}
