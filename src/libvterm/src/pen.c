#include "vterm_internal.h"

#include <stdio.h>

static const VTermColor ansi_colors[] = {
  /* R    G    B   index */
  {   0,   0,   0,  1 }, // black
  { 224,   0,   0,  2 }, // red
  {   0, 224,   0,  3 }, // green
  { 224, 224,   0,  4 }, // yellow
  {   0,   0, 224,  5 }, // blue
  { 224,   0, 224,  6 }, // magenta
  {   0, 224, 224,  7 }, // cyan
  { 224, 224, 224,  8 }, // white == light grey

  // high intensity
  { 128, 128, 128,  9 }, // black
  { 255,  64,  64, 10 }, // red
  {  64, 255,  64, 11 }, // green
  { 255, 255,  64, 12 }, // yellow
  {  64,  64, 255, 13 }, // blue
  { 255,  64, 255, 14 }, // magenta
  {  64, 255, 255, 15 }, // cyan
  { 255, 255, 255, 16 }, // white for real
};

static int ramp6[] = {
  0x00, 0x5F, 0x87, 0xAF, 0xD7, 0xFF,
};

/* Use 0x81 instead of 0x80 to be able to distinguish from ansi black */
static int ramp24[] = {
  0x08, 0x12, 0x1C, 0x26, 0x30, 0x3A, 0x44, 0x4E, 0x58, 0x62, 0x6C, 0x76,
  0x81, 0x8A, 0x94, 0x9E, 0xA8, 0xB2, 0xBC, 0xC6, 0xD0, 0xDA, 0xE4, 0xEE,
};

static int lookup_colour_ansi(const VTermState *state, long index, VTermColor *col)
{
  if(index >= 0 && index < 16) {
    *col = state->colors[index];
    return TRUE;
  }

  return FALSE;
}

static int lookup_colour_palette(const VTermState *state, long index, VTermColor *col)
{
  if(index >= 0 && index < 16) {
    // Normal 8 colours or high intensity - parse as palette 0
    return lookup_colour_ansi(state, index, col);
  }
  else if(index >= 16 && index < 232) {
    // 216-colour cube
    index -= 16;

    col->blue  = ramp6[index     % 6];
    col->green = ramp6[index/6   % 6];
    col->red   = ramp6[index/6/6 % 6];
    col->ansi_index = VTERM_ANSI_INDEX_NONE;

    return TRUE;
  }
  else if(index >= 232 && index < 256) {
    // 24 greyscales
    index -= 232;

    col->blue  = ramp24[index];
    col->green = ramp24[index];
    col->red   = ramp24[index];
    col->ansi_index = VTERM_ANSI_INDEX_NONE;

    return TRUE;
  }

  return FALSE;
}

static int lookup_colour(const VTermState *state, int palette, const long args[], int argcount, VTermColor *col, int *index)
{
  switch(palette) {
  case 2: // RGB mode - 3 args contain colour values directly
    if(argcount < 3)
      return argcount;

    col->red   = (uint8_t)CSI_ARG(args[0]);
    col->green = (uint8_t)CSI_ARG(args[1]);
    col->blue  = (uint8_t)CSI_ARG(args[2]);
    col->ansi_index = VTERM_ANSI_INDEX_NONE;

    return 3;

  case 5: // XTerm 256-colour mode
    if(index)
      *index = CSI_ARG_OR(args[0], -1);

    lookup_colour_palette(state, argcount ? CSI_ARG_OR(args[0], -1) : -1, col);

    return argcount ? 1 : 0;

  default:
    DEBUG_LOG1("Unrecognised colour palette %d\n", palette);
    return 0;
  }
}

// Some conveniences

static void setpenattr(VTermState *state, VTermAttr attr, VTermValueType type UNUSED, VTermValue *val)
{
#ifdef DEBUG
  if(type != vterm_get_attr_type(attr)) {
    DEBUG_LOG3("Cannot set attr %d as it has type %d, not type %d\n",
        attr, vterm_get_attr_type(attr), type);
    return;
  }
#endif
  if(state->callbacks && state->callbacks->setpenattr)
    (*state->callbacks->setpenattr)(attr, val, state->cbdata);
}

static void setpenattr_bool(VTermState *state, VTermAttr attr, int boolean)
{
  VTermValue val;
  val.boolean = boolean;
  setpenattr(state, attr, VTERM_VALUETYPE_BOOL, &val);
}

static void setpenattr_int(VTermState *state, VTermAttr attr, int number)
{
  VTermValue val;
  val.number = number;
  setpenattr(state, attr, VTERM_VALUETYPE_INT, &val);
}

static void setpenattr_col(VTermState *state, VTermAttr attr, VTermColor color)
{
  VTermValue val;
  val.color = color;
  setpenattr(state, attr, VTERM_VALUETYPE_COLOR, &val);
}

static void set_pen_col_ansi(VTermState *state, VTermAttr attr, long col)
{
  VTermColor *colp = (attr == VTERM_ATTR_BACKGROUND) ? &state->pen.bg : &state->pen.fg;

  lookup_colour_ansi(state, col, colp);

  setpenattr_col(state, attr, *colp);
}

INTERNAL void vterm_state_newpen(VTermState *state)
{
  int col;

  // 90% grey so that pure white is brighter
  state->default_fg.red = state->default_fg.green = state->default_fg.blue = 240;
  state->default_fg.ansi_index = VTERM_ANSI_INDEX_DEFAULT;
  state->default_bg.red = state->default_bg.green = state->default_bg.blue = 0;
  state->default_bg.ansi_index = VTERM_ANSI_INDEX_DEFAULT;

  for(col = 0; col < 16; col++)
    state->colors[col] = ansi_colors[col];
}

INTERNAL void vterm_state_resetpen(VTermState *state)
{
  state->pen.bold = 0;      setpenattr_bool(state, VTERM_ATTR_BOLD, 0);
  state->pen.underline = 0; setpenattr_int( state, VTERM_ATTR_UNDERLINE, 0);
  state->pen.italic = 0;    setpenattr_bool(state, VTERM_ATTR_ITALIC, 0);
  state->pen.blink = 0;     setpenattr_bool(state, VTERM_ATTR_BLINK, 0);
  state->pen.reverse = 0;   setpenattr_bool(state, VTERM_ATTR_REVERSE, 0);
  state->pen.strike = 0;    setpenattr_bool(state, VTERM_ATTR_STRIKE, 0);
  state->pen.font = 0;      setpenattr_int( state, VTERM_ATTR_FONT, 0);

  state->fg_index = -1;
  state->bg_index = -1;
  state->pen.fg = state->default_fg;  setpenattr_col(state, VTERM_ATTR_FOREGROUND, state->default_fg);
  state->pen.bg = state->default_bg;  setpenattr_col(state, VTERM_ATTR_BACKGROUND, state->default_bg);
}

INTERNAL void vterm_state_savepen(VTermState *state, int save)
{
  if(save) {
    state->saved.pen = state->pen;
  }
  else {
    state->pen = state->saved.pen;

    setpenattr_bool(state, VTERM_ATTR_BOLD,       state->pen.bold);
    setpenattr_int( state, VTERM_ATTR_UNDERLINE,  state->pen.underline);
    setpenattr_bool(state, VTERM_ATTR_ITALIC,     state->pen.italic);
    setpenattr_bool(state, VTERM_ATTR_BLINK,      state->pen.blink);
    setpenattr_bool(state, VTERM_ATTR_REVERSE,    state->pen.reverse);
    setpenattr_bool(state, VTERM_ATTR_STRIKE,     state->pen.strike);
    setpenattr_int( state, VTERM_ATTR_FONT,       state->pen.font);
    setpenattr_col( state, VTERM_ATTR_FOREGROUND, state->pen.fg);
    setpenattr_col( state, VTERM_ATTR_BACKGROUND, state->pen.bg);
  }
}

void vterm_state_get_default_colors(const VTermState *state, VTermColor *default_fg, VTermColor *default_bg)
{
  *default_fg = state->default_fg;
  *default_bg = state->default_bg;
}

void vterm_state_get_palette_color(const VTermState *state, int index, VTermColor *col)
{
  lookup_colour_palette(state, index, col);
}

void vterm_state_set_default_colors(VTermState *state, const VTermColor *default_fg, const VTermColor *default_bg)
{
  state->default_fg = *default_fg;
  state->default_bg = *default_bg;
}

void vterm_state_set_palette_color(VTermState *state, int index, const VTermColor *col)
{
  if(index >= 0 && index < 16)
  {
    state->colors[index] = *col;
    state->colors[index].ansi_index = index + VTERM_ANSI_INDEX_MIN;
  }
}

void vterm_state_set_bold_highbright(VTermState *state, int bold_is_highbright)
{
  state->bold_is_highbright = bold_is_highbright;
}

INTERNAL void vterm_state_setpen(VTermState *state, const long args[], int argcount)
{
  // SGR - ECMA-48 8.3.117

  int argi = 0;
  int value;

  while(argi < argcount) {
    // This logic is easier to do 'done' backwards; set it true, and make it
    // false again in the 'default' case
    int done = 1;

    long arg;
    switch(arg = CSI_ARG(args[argi])) {
    case CSI_ARG_MISSING:
    case 0: // Reset
      vterm_state_resetpen(state);
      break;

    case 1: // Bold on
      state->pen.bold = 1;
      setpenattr_bool(state, VTERM_ATTR_BOLD, 1);
      if(state->fg_index > -1 && state->fg_index < 8 && state->bold_is_highbright)
        set_pen_col_ansi(state, VTERM_ATTR_FOREGROUND, state->fg_index + (state->pen.bold ? 8 : 0));
      break;

    case 3: // Italic on
      state->pen.italic = 1;
      setpenattr_bool(state, VTERM_ATTR_ITALIC, 1);
      break;

    case 4: // Underline single
      state->pen.underline = 1;
      setpenattr_int(state, VTERM_ATTR_UNDERLINE, 1);
      break;

    case 5: // Blink
      state->pen.blink = 1;
      setpenattr_bool(state, VTERM_ATTR_BLINK, 1);
      break;

    case 7: // Reverse on
      state->pen.reverse = 1;
      setpenattr_bool(state, VTERM_ATTR_REVERSE, 1);
      break;

    case 9: // Strikethrough on
      state->pen.strike = 1;
      setpenattr_bool(state, VTERM_ATTR_STRIKE, 1);
      break;

    case 10: case 11: case 12: case 13: case 14:
    case 15: case 16: case 17: case 18: case 19: // Select font
      state->pen.font = CSI_ARG(args[argi]) - 10;
      setpenattr_int(state, VTERM_ATTR_FONT, state->pen.font);
      break;

    case 21: // Underline double
      state->pen.underline = 2;
      setpenattr_int(state, VTERM_ATTR_UNDERLINE, 2);
      break;

    case 22: // Bold off
      state->pen.bold = 0;
      setpenattr_bool(state, VTERM_ATTR_BOLD, 0);
      break;

    case 23: // Italic and Gothic (currently unsupported) off
      state->pen.italic = 0;
      setpenattr_bool(state, VTERM_ATTR_ITALIC, 0);
      break;

    case 24: // Underline off
      state->pen.underline = 0;
      setpenattr_int(state, VTERM_ATTR_UNDERLINE, 0);
      break;

    case 25: // Blink off
      state->pen.blink = 0;
      setpenattr_bool(state, VTERM_ATTR_BLINK, 0);
      break;

    case 27: // Reverse off
      state->pen.reverse = 0;
      setpenattr_bool(state, VTERM_ATTR_REVERSE, 0);
      break;

    case 29: // Strikethrough off
      state->pen.strike = 0;
      setpenattr_bool(state, VTERM_ATTR_STRIKE, 0);
      break;

    case 30: case 31: case 32: case 33:
    case 34: case 35: case 36: case 37: // Foreground colour palette
      value = CSI_ARG(args[argi]) - 30;
      state->fg_index = value;
      if(state->pen.bold && state->bold_is_highbright)
        value += 8;
      set_pen_col_ansi(state, VTERM_ATTR_FOREGROUND, value);
      break;

    case 38: // Foreground colour alternative palette
      state->fg_index = -1;
      if(argcount - argi < 1)
        return;
      argi += 1 + lookup_colour(state, CSI_ARG(args[argi+1]), args+argi+2, argcount-argi-2, &state->pen.fg, &state->fg_index);
      setpenattr_col(state, VTERM_ATTR_FOREGROUND, state->pen.fg);
      break;

    case 39: // Foreground colour default
      state->fg_index = -1;
      state->pen.fg = state->default_fg;
      setpenattr_col(state, VTERM_ATTR_FOREGROUND, state->pen.fg);
      break;

    case 40: case 41: case 42: case 43:
    case 44: case 45: case 46: case 47: // Background colour palette
      value = CSI_ARG(args[argi]) - 40;
      state->bg_index = value;
      set_pen_col_ansi(state, VTERM_ATTR_BACKGROUND, value);
      break;

    case 48: // Background colour alternative palette
      state->bg_index = -1;
      if(argcount - argi < 1)
        return;
      argi += 1 + lookup_colour(state, CSI_ARG(args[argi+1]), args+argi+2, argcount-argi-2, &state->pen.bg, &state->bg_index);
      setpenattr_col(state, VTERM_ATTR_BACKGROUND, state->pen.bg);
      break;

    case 49: // Default background
      state->bg_index = -1;
      state->pen.bg = state->default_bg;
      setpenattr_col(state, VTERM_ATTR_BACKGROUND, state->pen.bg);
      break;

    case 90: case 91: case 92: case 93:
    case 94: case 95: case 96: case 97: // Foreground colour high-intensity palette
      value = CSI_ARG(args[argi]) - 90 + 8;
      state->fg_index = value;
      set_pen_col_ansi(state, VTERM_ATTR_FOREGROUND, value);
      break;

    case 100: case 101: case 102: case 103:
    case 104: case 105: case 106: case 107: // Background colour high-intensity palette
      value = CSI_ARG(args[argi]) - 100 + 8;
      state->bg_index = value;
      set_pen_col_ansi(state, VTERM_ATTR_BACKGROUND, value);
      break;

    default:
      done = 0;
      break;
    }

    if (!done)
    {
      DEBUG_LOG1("libvterm: Unhandled CSI SGR %lu\n", arg);
    }

    while (CSI_ARG_HAS_MORE(args[argi++]))
      ;
  }
}

INTERNAL int vterm_state_getpen(VTermState *state, long args[], int argcount UNUSED)
{
  int argi = 0;

  if(state->pen.bold)
    args[argi++] = 1;

  if(state->pen.italic)
    args[argi++] = 3;

  if(state->pen.underline == 1)
    args[argi++] = 4;

  if(state->pen.blink)
    args[argi++] = 5;

  if(state->pen.reverse)
    args[argi++] = 7;

  if(state->pen.strike)
    args[argi++] = 9;

  if(state->pen.font)
    args[argi++] = 10 + state->pen.font;

  if(state->pen.underline == 2)
    args[argi++] = 21;

  if(state->fg_index >= 0 && state->fg_index < 8)
    args[argi++] = 30 + state->fg_index;
  else if(state->fg_index >= 8 && state->fg_index < 16)
    args[argi++] = 90 + state->fg_index - 8;
  else if(state->fg_index >= 16 && state->fg_index < 256) {
    args[argi++] = CSI_ARG_FLAG_MORE|38;
    args[argi++] = CSI_ARG_FLAG_MORE|5;
    args[argi++] = state->fg_index;
  }
  else if(state->fg_index == -1) {
    // Send palette 2 if the actual FG colour is not default
    if(state->pen.fg.red   != state->default_fg.red   ||
       state->pen.fg.green != state->default_fg.green ||
       state->pen.fg.blue  != state->default_fg.blue  ) {
      args[argi++] = CSI_ARG_FLAG_MORE|38;
      args[argi++] = CSI_ARG_FLAG_MORE|2;
      args[argi++] = CSI_ARG_FLAG_MORE | state->pen.fg.red;
      args[argi++] = CSI_ARG_FLAG_MORE | state->pen.fg.green;
      args[argi++] = state->pen.fg.blue;
    }
  }

  if(state->bg_index >= 0 && state->bg_index < 8)
    args[argi++] = 40 + state->bg_index;
  else if(state->bg_index >= 8 && state->bg_index < 16)
    args[argi++] = 100 + state->bg_index - 8;
  else if(state->bg_index >= 16 && state->bg_index < 256) {
    args[argi++] = CSI_ARG_FLAG_MORE|48;
    args[argi++] = CSI_ARG_FLAG_MORE|5;
    args[argi++] = state->bg_index;
  }
  else if(state->bg_index == -1) {
    // Send palette 2 if the actual BG colour is not default
    if(state->pen.bg.red   != state->default_bg.red   ||
       state->pen.bg.green != state->default_bg.green ||
       state->pen.bg.blue  != state->default_bg.blue  ) {
      args[argi++] = CSI_ARG_FLAG_MORE|48;
      args[argi++] = CSI_ARG_FLAG_MORE|2;
      args[argi++] = CSI_ARG_FLAG_MORE | state->pen.bg.red;
      args[argi++] = CSI_ARG_FLAG_MORE | state->pen.bg.green;
      args[argi++] = state->pen.bg.blue;
    }
  }

  return argi;
}

int vterm_state_get_penattr(const VTermState *state, VTermAttr attr, VTermValue *val)
{
  switch(attr) {
  case VTERM_ATTR_BOLD:
    val->boolean = state->pen.bold;
    return 1;

  case VTERM_ATTR_UNDERLINE:
    val->number = state->pen.underline;
    return 1;

  case VTERM_ATTR_ITALIC:
    val->boolean = state->pen.italic;
    return 1;

  case VTERM_ATTR_BLINK:
    val->boolean = state->pen.blink;
    return 1;

  case VTERM_ATTR_REVERSE:
    val->boolean = state->pen.reverse;
    return 1;

  case VTERM_ATTR_STRIKE:
    val->boolean = state->pen.strike;
    return 1;

  case VTERM_ATTR_FONT:
    val->number = state->pen.font;
    return 1;

  case VTERM_ATTR_FOREGROUND:
    val->color = state->pen.fg;
    return 1;

  case VTERM_ATTR_BACKGROUND:
    val->color = state->pen.bg;
    return 1;

  case VTERM_N_ATTRS:
    return 0;
  }

  return 0;
}
