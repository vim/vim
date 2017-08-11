#include "vterm_internal.h"

#include <stdio.h>
#include <string.h>

#define CSI_ARGS_MAX 16
#define CSI_LEADER_MAX 16
#define CSI_INTERMED_MAX 16

static void do_control(VTerm *vt, unsigned char control)
{
  if(vt->parser_callbacks && vt->parser_callbacks->control)
    if((*vt->parser_callbacks->control)(control, vt->cbdata))
      return;

  DEBUG_LOG1("libvterm: Unhandled control 0x%02x\n", control);
}

static void do_string_csi(VTerm *vt, const char *args, size_t arglen, char command)
{
  int i = 0;

  int leaderlen = 0;
  char leader[CSI_LEADER_MAX];
  int argcount = 1; /* Always at least 1 arg */
  long csi_args[CSI_ARGS_MAX];
  int argi;
  int intermedlen = 0;
  char intermed[CSI_INTERMED_MAX];

  /* Extract leader bytes 0x3c to 0x3f */
  for( ; i < (int)arglen; i++) {
    if(args[i] < 0x3c || args[i] > 0x3f)
      break;
    if(leaderlen < CSI_LEADER_MAX-1)
      leader[leaderlen++] = args[i];
  }

  leader[leaderlen] = 0;

  for( ; i < (int)arglen; i++)
    if(args[i] == 0x3b || args[i] == 0x3a) /* ; or : */
      argcount++;

  /* TODO: Consider if these buffers should live in the VTerm struct itself */
  if(argcount > CSI_ARGS_MAX)
    argcount = CSI_ARGS_MAX;

  for(argi = 0; argi < argcount; argi++)
    csi_args[argi] = CSI_ARG_MISSING;

  argi = 0;
  for(i = leaderlen; i < (int)arglen && argi < argcount; i++) {
    switch(args[i]) {
    case 0x30: case 0x31: case 0x32: case 0x33: case 0x34:
    case 0x35: case 0x36: case 0x37: case 0x38: case 0x39:
      if(csi_args[argi] == CSI_ARG_MISSING)
        csi_args[argi] = 0;
      csi_args[argi] *= 10;
      csi_args[argi] += args[i] - '0';
      break;
    case 0x3a:
      csi_args[argi] |= CSI_ARG_FLAG_MORE;
      /* FALLTHROUGH */
    case 0x3b:
      argi++;
      break;
    default:
      goto done_leader;
    }
  }
done_leader: ;

  for( ; i < (int)arglen; i++) {
    if((args[i] & 0xf0) != 0x20)
      break;

    if(intermedlen < CSI_INTERMED_MAX-1)
      intermed[intermedlen++] = args[i];
  }

  intermed[intermedlen] = 0;

  if(i < (int)arglen) {
    DEBUG_LOG2("libvterm: TODO unhandled CSI bytes \"%.*s\"\n", (int)(arglen - i), args + i);
  }

#if 0
  printf("Parsed CSI args %.*s as:\n", arglen, args);
  printf(" leader: %s\n", leader);
  for(argi = 0; argi < argcount; argi++) {
    printf(" %lu", CSI_ARG(csi_args[argi]));
    if(!CSI_ARG_HAS_MORE(csi_args[argi]))
      printf("\n");
  printf(" intermed: %s\n", intermed);
  }
#endif

  if(vt->parser_callbacks && vt->parser_callbacks->csi)
    if((*vt->parser_callbacks->csi)(leaderlen ? leader : NULL, csi_args, argcount, intermedlen ? intermed : NULL, command, vt->cbdata))
      return;

  DEBUG_LOG3("libvterm: Unhandled CSI %.*s %c\n", (int)arglen, args, command);
}

static void append_strbuffer(VTerm *vt, const char *str, size_t len)
{
  if(len > vt->strbuffer_len - vt->strbuffer_cur) {
    len = vt->strbuffer_len - vt->strbuffer_cur;
    DEBUG_LOG1("Truncating strbuffer preserve to %zd bytes\n", len);
  }

  if(len > 0) {
    strncpy(vt->strbuffer + vt->strbuffer_cur, str, len);
    vt->strbuffer_cur += len;
  }
}

static size_t do_string(VTerm *vt, const char *str_frag, size_t len)
{
  size_t eaten;

  if(vt->strbuffer_cur) {
    if(str_frag)
      append_strbuffer(vt, str_frag, len);

    str_frag = vt->strbuffer;
    len = vt->strbuffer_cur;
  }
  else if(!str_frag) {
    DEBUG_LOG("parser.c: TODO: No strbuffer _and_ no final fragment???\n");
    len = 0;
  }

  vt->strbuffer_cur = 0;

  switch(vt->parser_state) {
  case NORMAL:
    if(vt->parser_callbacks && vt->parser_callbacks->text)
      if((eaten = (*vt->parser_callbacks->text)(str_frag, len, vt->cbdata)))
        return eaten;

    DEBUG_LOG1("libvterm: Unhandled text (%zu chars)\n", len);
    return 0;

  case ESC:
    if(len == 1 && str_frag[0] >= 0x40 && str_frag[0] < 0x60) {
      /* C1 emulations using 7bit clean */
      /* ESC 0x40 == 0x80 */
      do_control(vt, str_frag[0] + 0x40);
      return 0;
    }

    if(vt->parser_callbacks && vt->parser_callbacks->escape)
      if((*vt->parser_callbacks->escape)(str_frag, len, vt->cbdata))
        return 0;

    DEBUG_LOG1("libvterm: Unhandled escape ESC 0x%02x\n", str_frag[len-1]);
    return 0;

  case CSI:
    do_string_csi(vt, str_frag, len - 1, str_frag[len - 1]);
    return 0;

  case OSC:
    if(vt->parser_callbacks && vt->parser_callbacks->osc)
      if((*vt->parser_callbacks->osc)(str_frag, len, vt->cbdata))
        return 0;

    DEBUG_LOG2("libvterm: Unhandled OSC %.*s\n", (int)len, str_frag);
    return 0;

  case DCS:
    if(vt->parser_callbacks && vt->parser_callbacks->dcs)
      if((*vt->parser_callbacks->dcs)(str_frag, len, vt->cbdata))
        return 0;

    DEBUG_LOG2("libvterm: Unhandled DCS %.*s\n", (int)len, str_frag);
    return 0;

  case ESC_IN_OSC:
  case ESC_IN_DCS:
    DEBUG_LOG("libvterm: ARGH! Should never do_string() in ESC_IN_{OSC,DCS}\n");
    return 0;
  }

  return 0;
}

size_t vterm_input_write(VTerm *vt, const char *bytes, size_t len)
{
  size_t pos = 0;
  const char *string_start = NULL;  /* init to avoid gcc warning */

  switch(vt->parser_state) {
  case NORMAL:
    string_start = NULL;
    break;
  case ESC:
  case ESC_IN_OSC:
  case ESC_IN_DCS:
  case CSI:
  case OSC:
  case DCS:
    string_start = bytes;
    break;
  }

#define ENTER_STRING_STATE(st) do { vt->parser_state = st; string_start = bytes + pos + 1; } while(0)
#define ENTER_NORMAL_STATE()   do { vt->parser_state = NORMAL; string_start = NULL; } while(0)

  for( ; pos < len; pos++) {
    unsigned char c = bytes[pos];

    if(c == 0x00 || c == 0x7f) { /* NUL, DEL */
      if(vt->parser_state != NORMAL) {
        append_strbuffer(vt, string_start, bytes + pos - string_start);
        string_start = bytes + pos + 1;
      }
      continue;
    }
    if(c == 0x18 || c == 0x1a) { /* CAN, SUB */
      ENTER_NORMAL_STATE();
      continue;
    }
    else if(c == 0x1b) { /* ESC */
      if(vt->parser_state == OSC)
        vt->parser_state = ESC_IN_OSC;
      else if(vt->parser_state == DCS)
        vt->parser_state = ESC_IN_DCS;
      else
        ENTER_STRING_STATE(ESC);
      continue;
    }
    else if(c == 0x07 &&  /* BEL, can stand for ST in OSC or DCS state */
            (vt->parser_state == OSC || vt->parser_state == DCS)) {
      /* fallthrough */
    }
    else if(c < 0x20) { /* other C0 */
      if(vt->parser_state != NORMAL)
        append_strbuffer(vt, string_start, bytes + pos - string_start);
      do_control(vt, c);
      if(vt->parser_state != NORMAL)
        string_start = bytes + pos + 1;
      continue;
    }
    /* else fallthrough */

    switch(vt->parser_state) {
    case ESC_IN_OSC:
    case ESC_IN_DCS:
      if(c == 0x5c) { /* ST */
        switch(vt->parser_state) {
          case ESC_IN_OSC: vt->parser_state = OSC; break;
          case ESC_IN_DCS: vt->parser_state = DCS; break;
          default: break;
        }
        do_string(vt, string_start, bytes + pos - string_start - 1);
        ENTER_NORMAL_STATE();
        break;
      }
      vt->parser_state = ESC;
      string_start = bytes + pos;
      /* else fallthrough */

    case ESC:
      switch(c) {
      case 0x50: /* DCS */
        ENTER_STRING_STATE(DCS);
        break;
      case 0x5b: /* CSI */
        ENTER_STRING_STATE(CSI);
        break;
      case 0x5d: /* OSC */
        ENTER_STRING_STATE(OSC);
        break;
      default:
        if(c >= 0x30 && c < 0x7f) {
          /* +1 to pos because we want to include this command byte as well */
          do_string(vt, string_start, bytes + pos - string_start + 1);
          ENTER_NORMAL_STATE();
        }
        else if(c >= 0x20 && c < 0x30) {
          /* intermediate byte */
        }
        else {
          DEBUG_LOG1("TODO: Unhandled byte %02x in Escape\n", c);
        }
      }
      break;

    case CSI:
      if(c >= 0x40 && c <= 0x7f) {
        /* +1 to pos because we want to include this command byte as well */
        do_string(vt, string_start, bytes + pos - string_start + 1);
        ENTER_NORMAL_STATE();
      }
      break;

    case OSC:
    case DCS:
      if(c == 0x07 || (c == 0x9c && !vt->mode.utf8)) {
        do_string(vt, string_start, bytes + pos - string_start);
        ENTER_NORMAL_STATE();
      }
      break;

    case NORMAL:
      if(c >= 0x80 && c < 0xa0 && !vt->mode.utf8) {
        switch(c) {
        case 0x90: /* DCS */
          ENTER_STRING_STATE(DCS);
          break;
        case 0x9b: /* CSI */
          ENTER_STRING_STATE(CSI);
          break;
        case 0x9d: /* OSC */
          ENTER_STRING_STATE(OSC);
          break;
        default:
          do_control(vt, c);
          break;
        }
      }
      else {
        size_t text_eaten = do_string(vt, bytes + pos, len - pos);

        if(text_eaten == 0) {
          string_start = bytes + pos;
          goto pause;
        }

        pos += (text_eaten - 1); /* we'll ++ it again in a moment */
      }
      break;
    }
  }

pause:
  if(string_start && string_start < len + bytes) {
    size_t remaining = len - (string_start - bytes);
    append_strbuffer(vt, string_start, remaining);
  }

  return len;
}
