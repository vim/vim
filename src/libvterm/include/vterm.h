/*
 * NOTE: This is a MODIFIED version of libvterm, see the README file.
 */
#ifndef __VTERM_H__
#define __VTERM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#include "vterm_keycodes.h"

#define TRUE 1
#define FALSE 0

/* from stdint.h */
typedef unsigned char		uint8_t;
typedef unsigned int		uint32_t;

typedef struct VTerm VTerm;
typedef struct VTermState VTermState;
typedef struct VTermScreen VTermScreen;

/* Specifies a screen point. */
typedef struct {
  int row;
  int col;
} VTermPos;

/*
 * Some small utility functions; we can just keep these static here.
 */

/*
 * Order points by on-screen flow order:
 * Return < 0 if "a" is before "b"
 * Return  0  if "a" and "b" are equal
 * Return > 0 if "a" is after "b".
 */
int vterm_pos_cmp(VTermPos a, VTermPos b);

#if defined(DEFINE_INLINES) || USE_INLINE
INLINE int vterm_pos_cmp(VTermPos a, VTermPos b)
{
  return (a.row == b.row) ? a.col - b.col : a.row - b.row;
}
#endif

/* Specifies a rectangular screen area. */
typedef struct {
  int start_row;
  int end_row;
  int start_col;
  int end_col;
} VTermRect;

/* Return true if the rect "r" contains the point "p". */
int vterm_rect_contains(VTermRect r, VTermPos p);

#if defined(DEFINE_INLINES) || USE_INLINE
INLINE int vterm_rect_contains(VTermRect r, VTermPos p)
{
  return p.row >= r.start_row && p.row < r.end_row &&
         p.col >= r.start_col && p.col < r.end_col;
}
#endif

/* Move "rect" "row_delta" down and "col_delta" right.
 * Does not check boundaries. */
void vterm_rect_move(VTermRect *rect, int row_delta, int col_delta);

#if defined(DEFINE_INLINES) || USE_INLINE
INLINE void vterm_rect_move(VTermRect *rect, int row_delta, int col_delta)
{
  rect->start_row += row_delta; rect->end_row += row_delta;
  rect->start_col += col_delta; rect->end_col += col_delta;
}
#endif

typedef struct {
  uint8_t red, green, blue;
} VTermColor;

typedef enum {
  /* VTERM_VALUETYPE_NONE = 0 */
  VTERM_VALUETYPE_BOOL = 1,
  VTERM_VALUETYPE_INT,
  VTERM_VALUETYPE_STRING,
  VTERM_VALUETYPE_COLOR
} VTermValueType;

typedef union {
  int boolean;
  int number;
  char *string;
  VTermColor color;
} VTermValue;

typedef enum {
  /* VTERM_ATTR_NONE = 0 */
  VTERM_ATTR_BOLD = 1,   /* bool:   1, 22 */
  VTERM_ATTR_UNDERLINE,  /* number: 4, 21, 24 */
  VTERM_ATTR_ITALIC,     /* bool:   3, 23 */
  VTERM_ATTR_BLINK,      /* bool:   5, 25 */
  VTERM_ATTR_REVERSE,    /* bool:   7, 27 */
  VTERM_ATTR_STRIKE,     /* bool:   9, 29 */
  VTERM_ATTR_FONT,       /* number: 10-19 */
  VTERM_ATTR_FOREGROUND, /* color:  30-39 90-97 */
  VTERM_ATTR_BACKGROUND  /* color:  40-49 100-107 */
} VTermAttr;

typedef enum {
  /* VTERM_PROP_NONE = 0 */
  VTERM_PROP_CURSORVISIBLE = 1, /* bool */
  VTERM_PROP_CURSORBLINK,       /* bool */
  VTERM_PROP_ALTSCREEN,         /* bool */
  VTERM_PROP_TITLE,             /* string */
  VTERM_PROP_ICONNAME,          /* string */
  VTERM_PROP_REVERSE,           /* bool */
  VTERM_PROP_CURSORSHAPE,       /* number */
  VTERM_PROP_MOUSE              /* number */
} VTermProp;

enum {
  VTERM_PROP_CURSORSHAPE_BLOCK = 1,
  VTERM_PROP_CURSORSHAPE_UNDERLINE,
  VTERM_PROP_CURSORSHAPE_BAR_LEFT
};

enum {
  VTERM_PROP_MOUSE_NONE = 0,
  VTERM_PROP_MOUSE_CLICK,
  VTERM_PROP_MOUSE_DRAG,
  VTERM_PROP_MOUSE_MOVE
};

typedef struct {
  const uint32_t *chars;
  int             width;
  unsigned int    protected_cell:1;  /* DECSCA-protected against DECSEL/DECSED */
  unsigned int    dwl:1;             /* DECDWL or DECDHL double-width line */
  unsigned int    dhl:2;             /* DECDHL double-height line (1=top 2=bottom) */
} VTermGlyphInfo;

typedef struct {
  unsigned int    doublewidth:1;     /* DECDWL or DECDHL line */
  unsigned int    doubleheight:2;    /* DECDHL line (1=top 2=bottom) */
} VTermLineInfo;

typedef struct {
  /* libvterm relies on the allocated memory to be zeroed out before it is
   * returned by the allocator. */
  void *(*malloc)(size_t size, void *allocdata);
  void  (*free)(void *ptr, void *allocdata);
} VTermAllocatorFunctions;

/* Allocate and initialize a new terminal with default allocators. */
VTerm *vterm_new(int rows, int cols);

/* Allocate and initialize a new terminal with specified allocators. */
VTerm *vterm_new_with_allocator(int rows, int cols, VTermAllocatorFunctions *funcs, void *allocdata);

/* Free and cleanup a terminal and all its data. */
void   vterm_free(VTerm* vt);

/* Get the current size of the terminal and store in "rowsp" and "colsp". */
void vterm_get_size(const VTerm *vt, int *rowsp, int *colsp);

void vterm_set_size(VTerm *vt, int rows, int cols);

int  vterm_get_utf8(const VTerm *vt);
void vterm_set_utf8(VTerm *vt, int is_utf8);

size_t vterm_input_write(VTerm *vt, const char *bytes, size_t len);

size_t vterm_output_get_buffer_size(const VTerm *vt);
size_t vterm_output_get_buffer_current(const VTerm *vt);
size_t vterm_output_get_buffer_remaining(const VTerm *vt);

size_t vterm_output_read(VTerm *vt, char *buffer, size_t len);

void vterm_keyboard_unichar(VTerm *vt, uint32_t c, VTermModifier mod);
void vterm_keyboard_key(VTerm *vt, VTermKey key, VTermModifier mod);

void vterm_keyboard_start_paste(VTerm *vt);
void vterm_keyboard_end_paste(VTerm *vt);

void vterm_mouse_move(VTerm *vt, int row, int col, VTermModifier mod);
void vterm_mouse_button(VTerm *vt, int button, int pressed, VTermModifier mod);

/* ------------
 * Parser layer
 * ------------ */

/* Flag to indicate non-final subparameters in a single CSI parameter.
 * Consider
 *   CSI 1;2:3:4;5a
 * 1 4 and 5 are final.
 * 2 and 3 are non-final and will have this bit set
 *
 * Don't confuse this with the final byte of the CSI escape; 'a' in this case.
 */
#define CSI_ARG_FLAG_MORE (1<<30)
#define CSI_ARG_MASK      (~(1<<30))

#define CSI_ARG_HAS_MORE(a) ((a) & CSI_ARG_FLAG_MORE)
#define CSI_ARG(a)          ((a) & CSI_ARG_MASK)

/* Can't use -1 to indicate a missing argument; use this instead */
#define CSI_ARG_MISSING ((1<<30)-1)

#define CSI_ARG_IS_MISSING(a) (CSI_ARG(a) == CSI_ARG_MISSING)
#define CSI_ARG_OR(a,def)     (CSI_ARG(a) == CSI_ARG_MISSING ? (def) : CSI_ARG(a))
#define CSI_ARG_COUNT(a)      (CSI_ARG(a) == CSI_ARG_MISSING || CSI_ARG(a) == 0 ? 1 : CSI_ARG(a))

typedef struct {
  int (*text)(const char *bytes, size_t len, void *user);
  int (*control)(unsigned char control, void *user);
  int (*escape)(const char *bytes, size_t len, void *user);
  int (*csi)(const char *leader, const long args[], int argcount, const char *intermed, char command, void *user);
  int (*osc)(const char *command, size_t cmdlen, void *user);
  int (*dcs)(const char *command, size_t cmdlen, void *user);
  int (*resize)(int rows, int cols, void *user);
} VTermParserCallbacks;

void  vterm_parser_set_callbacks(VTerm *vt, const VTermParserCallbacks *callbacks, void *user);
void *vterm_parser_get_cbdata(VTerm *vt);

/* -----------
 * State layer
 * ----------- */

typedef struct {
  int (*putglyph)(VTermGlyphInfo *info, VTermPos pos, void *user);
  int (*movecursor)(VTermPos pos, VTermPos oldpos, int visible, void *user);
  int (*scrollrect)(VTermRect rect, int downward, int rightward, void *user);
  int (*moverect)(VTermRect dest, VTermRect src, void *user);
  int (*erase)(VTermRect rect, int selective, void *user);
  int (*initpen)(void *user);
  int (*setpenattr)(VTermAttr attr, VTermValue *val, void *user);
  /* Callback for setting various properties.  Must return 1 if the property
   * was accepted, 0 otherwise. */
  int (*settermprop)(VTermProp prop, VTermValue *val, void *user);
  int (*bell)(void *user);
  int (*resize)(int rows, int cols, VTermPos *delta, void *user);
  int (*setlineinfo)(int row, const VTermLineInfo *newinfo, const VTermLineInfo *oldinfo, void *user);
} VTermStateCallbacks;

VTermState *vterm_obtain_state(VTerm *vt);

void  vterm_state_set_callbacks(VTermState *state, const VTermStateCallbacks *callbacks, void *user);
void *vterm_state_get_cbdata(VTermState *state);

/* Only invokes control, csi, osc, dcs */
void  vterm_state_set_unrecognised_fallbacks(VTermState *state, const VTermParserCallbacks *fallbacks, void *user);
void *vterm_state_get_unrecognised_fbdata(VTermState *state);

/* Initialize the state. */
void vterm_state_reset(VTermState *state, int hard);

void vterm_state_get_cursorpos(const VTermState *state, VTermPos *cursorpos);
void vterm_state_get_default_colors(const VTermState *state, VTermColor *default_fg, VTermColor *default_bg);
void vterm_state_get_palette_color(const VTermState *state, int index, VTermColor *col);
void vterm_state_set_default_colors(VTermState *state, const VTermColor *default_fg, const VTermColor *default_bg);
void vterm_state_set_palette_color(VTermState *state, int index, const VTermColor *col);
void vterm_state_set_bold_highbright(VTermState *state, int bold_is_highbright);
int  vterm_state_get_penattr(const VTermState *state, VTermAttr attr, VTermValue *val);
int  vterm_state_set_termprop(VTermState *state, VTermProp prop, VTermValue *val);
const VTermLineInfo *vterm_state_get_lineinfo(const VTermState *state, int row);

/* ------------
 * Screen layer
 * ------------ */

typedef struct {
    unsigned int bold      : 1;
    unsigned int underline : 2;
    unsigned int italic    : 1;
    unsigned int blink     : 1;
    unsigned int reverse   : 1;
    unsigned int strike    : 1;
    unsigned int font      : 4; /* 0 to 9 */
    unsigned int dwl       : 1; /* On a DECDWL or DECDHL line */
    unsigned int dhl       : 2; /* On a DECDHL line (1=top 2=bottom) */
} VTermScreenCellAttrs;

typedef struct {
#define VTERM_MAX_CHARS_PER_CELL 6
  uint32_t chars[VTERM_MAX_CHARS_PER_CELL];
  char     width;
  VTermScreenCellAttrs attrs;
  VTermColor fg, bg;
} VTermScreenCell;

/* All fields are optional, NULL when not used. */
typedef struct {
  int (*damage)(VTermRect rect, void *user);
  int (*moverect)(VTermRect dest, VTermRect src, void *user);
  int (*movecursor)(VTermPos pos, VTermPos oldpos, int visible, void *user);
  int (*settermprop)(VTermProp prop, VTermValue *val, void *user);
  int (*bell)(void *user);
  int (*resize)(int rows, int cols, void *user);
  int (*sb_pushline)(int cols, const VTermScreenCell *cells, void *user);
  int (*sb_popline)(int cols, VTermScreenCell *cells, void *user);
} VTermScreenCallbacks;

/* Return the screen of the vterm. */
VTermScreen *vterm_obtain_screen(VTerm *vt);

/*
 * Install screen callbacks.  These are invoked when the screen contents is
 * changed.  "user" is passed into to the callback.
 */
void  vterm_screen_set_callbacks(VTermScreen *screen, const VTermScreenCallbacks *callbacks, void *user);
void *vterm_screen_get_cbdata(VTermScreen *screen);

/* Only invokes control, csi, osc, dcs */
void  vterm_screen_set_unrecognised_fallbacks(VTermScreen *screen, const VTermParserCallbacks *fallbacks, void *user);
void *vterm_screen_get_unrecognised_fbdata(VTermScreen *screen);

void vterm_screen_enable_altscreen(VTermScreen *screen, int altscreen);

typedef enum {
  VTERM_DAMAGE_CELL,    /* every cell */
  VTERM_DAMAGE_ROW,     /* entire rows */
  VTERM_DAMAGE_SCREEN,  /* entire screen */
  VTERM_DAMAGE_SCROLL   /* entire screen + scrollrect */
} VTermDamageSize;

/* Invoke the relevant callbacks to update the screen. */
void vterm_screen_flush_damage(VTermScreen *screen);

void vterm_screen_set_damage_merge(VTermScreen *screen, VTermDamageSize size);

/*
 * Reset the screen.  Also invokes vterm_state_reset().
 * Must be called before the terminal can be used.
 */
void   vterm_screen_reset(VTermScreen *screen, int hard);

/* Neither of these functions NUL-terminate the buffer */
size_t vterm_screen_get_chars(const VTermScreen *screen, uint32_t *chars, size_t len, const VTermRect rect);
size_t vterm_screen_get_text(const VTermScreen *screen, char *str, size_t len, const VTermRect rect);

typedef enum {
  VTERM_ATTR_BOLD_MASK       = 1 << 0,
  VTERM_ATTR_UNDERLINE_MASK  = 1 << 1,
  VTERM_ATTR_ITALIC_MASK     = 1 << 2,
  VTERM_ATTR_BLINK_MASK      = 1 << 3,
  VTERM_ATTR_REVERSE_MASK    = 1 << 4,
  VTERM_ATTR_STRIKE_MASK     = 1 << 5,
  VTERM_ATTR_FONT_MASK       = 1 << 6,
  VTERM_ATTR_FOREGROUND_MASK = 1 << 7,
  VTERM_ATTR_BACKGROUND_MASK = 1 << 8
} VTermAttrMask;

int vterm_screen_get_attrs_extent(const VTermScreen *screen, VTermRect *extent, VTermPos pos, VTermAttrMask attrs);

int vterm_screen_get_cell(const VTermScreen *screen, VTermPos pos, VTermScreenCell *cell);

int vterm_screen_is_eol(const VTermScreen *screen, VTermPos pos);

/* ---------
 * Utilities
 * --------- */

VTermValueType vterm_get_attr_type(VTermAttr attr);
VTermValueType vterm_get_prop_type(VTermProp prop);

void vterm_scroll_rect(VTermRect rect,
                       int downward,
                       int rightward,
                       int (*moverect)(VTermRect src, VTermRect dest, void *user),
                       int (*eraserect)(VTermRect rect, int selective, void *user),
                       void *user);

void vterm_copy_cells(VTermRect dest,
                      VTermRect src,
                      void (*copycell)(VTermPos dest, VTermPos src, void *user),
                      void *user);

#ifdef __cplusplus
}
#endif

#endif
