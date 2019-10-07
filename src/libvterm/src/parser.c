#include "vterm_internal.h"

#include <stdio.h>
#include <string.h>

#undef DEBUG_PARSER

static int is_intermed(unsigned char c)
{
  return c >= 0x20 && c <= 0x2f;
}

static void do_control(VTerm *vt, unsigned char control)
{
  if(vt->parser.callbacks && vt->parser.callbacks->control)
    if((*vt->parser.callbacks->control)(control, vt->parser.cbdata))
      return;

  DEBUG_LOG1("libvterm: Unhandled control 0x%02x\n", control);
}

static void do_csi(VTerm *vt, char command)
{
#ifdef DEBUG_PARSER
  printf("Parsed CSI args as:\n", arglen, args);
  printf(" leader: %s\n", vt->parser.csi_leader);
  for(int argi = 0; argi < vt->parser.csi_argi; argi++) {
    printf(" %lu", CSI_ARG(vt->parser.csi_args[argi]));
    if(!CSI_ARG_HAS_MORE(vt->parser.csi_args[argi]))
      printf("\n");
  printf(" intermed: %s\n", vt->parser.intermed);
  }
#endif

  if(vt->parser.callbacks && vt->parser.callbacks->csi)
    if((*vt->parser.callbacks->csi)(
          vt->parser.csi_leaderlen ? vt->parser.csi_leader : NULL,
          vt->parser.csi_args,
          vt->parser.csi_argi,
          vt->parser.intermedlen ? vt->parser.intermed : NULL,
          command,
          vt->parser.cbdata))
      return;

  DEBUG_LOG1("libvterm: Unhandled CSI %c\n", command);
}

static void do_escape(VTerm *vt, char command)
{
  char seq[INTERMED_MAX+1];

  size_t len = vt->parser.intermedlen;
  strncpy(seq, vt->parser.intermed, len);
  seq[len++] = command;
  seq[len]   = 0;

  if(vt->parser.callbacks && vt->parser.callbacks->escape)
    if((*vt->parser.callbacks->escape)(seq, len, vt->parser.cbdata))
      return;

  DEBUG_LOG1("libvterm: Unhandled escape ESC 0x%02x\n", command);
}

static void append_strbuffer(VTerm *vt, const char *str, size_t len)
{
  if(len > vt->parser.strbuffer_len - vt->parser.strbuffer_cur) {
    len = vt->parser.strbuffer_len - vt->parser.strbuffer_cur;
    DEBUG_LOG1("Truncating strbuffer preserve to %zd bytes\n", len);
  }

  if(len > 0) {
    strncpy(vt->parser.strbuffer + vt->parser.strbuffer_cur, str, len);
    vt->parser.strbuffer_cur += len;
  }
}

static void start_string(VTerm *vt, VTermParserStringType type)
{
  vt->parser.stringtype = type;

  vt->parser.strbuffer_cur = 0;
}

static void more_string(VTerm *vt, const char *str, size_t len)
{
  append_strbuffer(vt, str, len);
}

static void done_string(VTerm *vt, const char *str, size_t len)
{
  if(vt->parser.strbuffer_cur) {
    if(str)
      append_strbuffer(vt, str, len);

    str = vt->parser.strbuffer;
    len = vt->parser.strbuffer_cur;
  }
  else if(!str) {
    DEBUG_LOG("parser.c: TODO: No strbuffer _and_ no final fragment???\n");
    len = 0;
  }

  switch(vt->parser.stringtype) {
  case VTERM_PARSER_OSC:
    if(vt->parser.callbacks && vt->parser.callbacks->osc)
      if((*vt->parser.callbacks->osc)(str, len, vt->parser.cbdata))
        return;

    DEBUG_LOG2("libvterm: Unhandled OSC %.*s\n", (int)len, str);
    return;

  case VTERM_PARSER_DCS:
    if(vt->parser.callbacks && vt->parser.callbacks->dcs)
      if((*vt->parser.callbacks->dcs)(str, len, vt->parser.cbdata))
        return;

    DEBUG_LOG2("libvterm: Unhandled DCS %.*s\n", (int)len, str);
    return;

  case VTERM_N_PARSER_TYPES:
    return;
  }
}

size_t vterm_input_write(VTerm *vt, const char *bytes, size_t len)
{
  size_t pos = 0;
  const char *string_start = NULL;  // init to avoid gcc warning

  vt->in_backspace = 0;		    // Count down with BS key and activate when
				    // it reaches 1

  switch(vt->parser.state) {
  case NORMAL:
  case CSI_LEADER:
  case CSI_ARGS:
  case CSI_INTERMED:
  case ESC:
    string_start = NULL;
    break;
  case STRING:
  case ESC_IN_STRING:
    string_start = bytes;
    break;
  }

#define ENTER_STRING_STATE()   do { vt->parser.state = STRING; string_start = bytes + pos + 1; } while(0)
#define ENTER_STATE(st)        do { vt->parser.state = st; string_start = NULL; } while(0)
#define ENTER_NORMAL_STATE()   ENTER_STATE(NORMAL)

  for( ; pos < len; pos++) {
    unsigned char c = bytes[pos];

    if(c == 0x00 || c == 0x7f) { // NUL, DEL
      if(vt->parser.state >= STRING) {
        more_string(vt, string_start, bytes + pos - string_start);
        string_start = bytes + pos + 1;
      }
      continue;
    }
    if(c == 0x18 || c == 0x1a) { // CAN, SUB
      ENTER_NORMAL_STATE();
      continue;
    }
    else if(c == 0x1b) { // ESC
      vt->parser.intermedlen = 0;
      if(vt->parser.state == STRING)
        vt->parser.state = ESC_IN_STRING;
      else
        ENTER_STATE(ESC);
      continue;
    }
    else if(c == 0x07 &&  // BEL, can stand for ST in OSC or DCS state
            vt->parser.state == STRING) {
      // fallthrough
    }
    else if(c < 0x20) { // other C0
      if(vterm_get_special_pty_type() == 2) {
        if(c == 0x08) // BS
          // Set the trick for BS output after a sequence, to delay backspace
          // activation
          if(pos + 2 < len && bytes[pos + 1] == 0x20 && bytes[pos + 2] == 0x08)
            vt->in_backspace = 2; // Trigger when count down to 1
      }
      if(vt->parser.state >= STRING)
        more_string(vt, string_start, bytes + pos - string_start);
      do_control(vt, c);
      if(vt->parser.state >= STRING)
        string_start = bytes + pos + 1;
      continue;
    }
    // else fallthrough

    switch(vt->parser.state) {
    case ESC_IN_STRING:
      if(c == 0x5c) { // ST
        vt->parser.state = STRING;
        done_string(vt, string_start, bytes + pos - string_start - 1);
        ENTER_NORMAL_STATE();
        break;
      }
      vt->parser.state = ESC;
      // else fallthrough

    case ESC:
      switch(c) {
      case 0x50: // DCS
        start_string(vt, VTERM_PARSER_DCS);
        ENTER_STRING_STATE();
        break;
      case 0x5b: // CSI
        vt->parser.csi_leaderlen = 0;
        ENTER_STATE(CSI_LEADER);
        break;
      case 0x5d: // OSC
        start_string(vt, VTERM_PARSER_OSC);
        ENTER_STRING_STATE();
        break;
      default:
        if(is_intermed(c)) {
          if(vt->parser.intermedlen < INTERMED_MAX-1)
            vt->parser.intermed[vt->parser.intermedlen++] = c;
        }
        else if(!vt->parser.intermedlen && c >= 0x40 && c < 0x60) {
          do_control(vt, c + 0x40);
          ENTER_NORMAL_STATE();
        }
        else if(c >= 0x30 && c < 0x7f) {
          do_escape(vt, c);
          ENTER_NORMAL_STATE();
        }
        else {
          DEBUG_LOG1("TODO: Unhandled byte %02x in Escape\n", c);
        }
      }
      break;

    case CSI_LEADER:
      // Extract leader bytes 0x3c to 0x3f
      if(c >= 0x3c && c <= 0x3f) {
        if(vt->parser.csi_leaderlen < CSI_LEADER_MAX-1)
          vt->parser.csi_leader[vt->parser.csi_leaderlen++] = c;
        break;
      }

      // else fallthrough
      vt->parser.csi_leader[vt->parser.csi_leaderlen] = 0;

      vt->parser.csi_argi = 0;
      vt->parser.csi_args[0] = CSI_ARG_MISSING;
      vt->parser.state = CSI_ARGS;

      // fallthrough
    case CSI_ARGS:
      // Numerical value of argument
      if(c >= '0' && c <= '9') {
        if(vt->parser.csi_args[vt->parser.csi_argi] == CSI_ARG_MISSING)
          vt->parser.csi_args[vt->parser.csi_argi] = 0;
        vt->parser.csi_args[vt->parser.csi_argi] *= 10;
        vt->parser.csi_args[vt->parser.csi_argi] += c - '0';
        break;
      }
      if(c == ':') {
        vt->parser.csi_args[vt->parser.csi_argi] |= CSI_ARG_FLAG_MORE;
        c = ';';
      }
      if(c == ';') {
        vt->parser.csi_argi++;
        vt->parser.csi_args[vt->parser.csi_argi] = CSI_ARG_MISSING;
        break;
      }

      // else fallthrough
      vt->parser.csi_argi++;
      vt->parser.intermedlen = 0;
      vt->parser.state = CSI_INTERMED;
      // fallthrough
    case CSI_INTERMED:
      if(is_intermed(c)) {
        if(vt->parser.intermedlen < INTERMED_MAX-1)
          vt->parser.intermed[vt->parser.intermedlen++] = c;
        break;
      }
      else if(c == 0x1b) {
        // ESC in CSI cancels
      }
      else if(c >= 0x40 && c <= 0x7e) {
        vt->parser.intermed[vt->parser.intermedlen] = 0;
        do_csi(vt, c);
      }
      // else was invalid CSI

      ENTER_NORMAL_STATE();
      break;

    case STRING:
      if(c == 0x07 || (c == 0x9c && !vt->mode.utf8)) {
        done_string(vt, string_start, bytes + pos - string_start);
        ENTER_NORMAL_STATE();
      }
      else if (pos + 1 == len) {
	// end of input but OSC string isn't finished yet, copy it to
	// vt->parser.strbuffer to continue it later
        more_string(vt, string_start, bytes + pos + 1 - string_start);
      }
      break;

    case NORMAL:
      if(c >= 0x80 && c < 0xa0 && !vt->mode.utf8) {
        switch(c) {
        case 0x90: // DCS
          start_string(vt, VTERM_PARSER_DCS);
          ENTER_STRING_STATE();
          break;
        case 0x9b: // CSI
          ENTER_STATE(CSI_LEADER);
          break;
        case 0x9d: // OSC
          start_string(vt, VTERM_PARSER_OSC);
          ENTER_STRING_STATE();
          break;
        default:
          do_control(vt, c);
          break;
        }
      }
      else {
        size_t eaten = 0;
        if(vt->parser.callbacks && vt->parser.callbacks->text)
          eaten = (*vt->parser.callbacks->text)(bytes + pos, len - pos, vt->parser.cbdata);

        if(!eaten) {
          DEBUG_LOG("libvterm: Text callback did not consume any input\n");
          // force it to make progress
          eaten = 1;
        }

        pos += (eaten - 1); // we'll ++ it again in a moment
      }
      break;
    }
  }

  return len;
}

void vterm_parser_set_callbacks(VTerm *vt, const VTermParserCallbacks *callbacks, void *user)
{
  vt->parser.callbacks = callbacks;
  vt->parser.cbdata = user;
}

void *vterm_parser_get_cbdata(VTerm *vt)
{
  return vt->parser.cbdata;
}
