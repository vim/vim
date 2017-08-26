/* Require getopt(3) */
#define _XOPEN_SOURCE

#include <stdio.h>
#include <string.h>
#define streq(a,b) (strcmp(a,b)==0)

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "vterm.h"

static const char *special_begin = "{";
static const char *special_end   = "}";

static int parser_text(const char bytes[], size_t len, void *user)
{
  unsigned char *b = (unsigned char *)bytes;

  int i;
  for(i = 0; i < len; /* none */) {
    if(b[i] < 0x20)        /* C0 */
      break;
    else if(b[i] < 0x80)   /* ASCII */
      i++;
    else if(b[i] < 0xa0)   /* C1 */
      break;
    else if(b[i] < 0xc0)   /* UTF-8 continuation */
      break;
    else if(b[i] < 0xe0) { /* UTF-8 2-byte */
      /* 2-byte UTF-8 */
      if(len < i+2) break;
      i += 2;
    }
    else if(b[i] < 0xf0) { /* UTF-8 3-byte */
      if(len < i+3) break;
      i += 3;
    }
    else if(b[i] < 0xf8) { /* UTF-8 4-byte */
      if(len < i+4) break;
      i += 4;
    }
    else                   /* otherwise invalid */
      break;
  }

  printf("%.*s", i, b);
  return i;
}

/* 0     1      2      3       4     5      6      7      8      9      A      B      C      D      E      F    */
static const char *name_c0[] = {
  "NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL", "BS",  "HT",  "LF",  "VT",  "FF",  "CR",  "LS0", "LS1",
  "DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB", "CAN", "EM",  "SUB", "ESC", "FS",  "GS",  "RS",  "US",
};
static const char *name_c1[] = {
  NULL,  NULL,  "BPH", "NBH", NULL,  "NEL", "SSA", "ESA", "HTS", "HTJ", "VTS", "PLD", "PLU", "RI",  "SS2", "SS3",
  "DCS", "PU1", "PU2", "STS", "CCH", "MW",  "SPA", "EPA", "SOS", NULL,  "SCI", "CSI", "ST",  "OSC", "PM",  "APC",
};

static int parser_control(unsigned char control, void *user)
{
  if(control < 0x20)
    printf("%s%s%s", special_begin, name_c0[control], special_end);
  else if(control >= 0x80 && control < 0xa0 && name_c1[control - 0x80])
    printf("%s%s%s", special_begin, name_c1[control - 0x80], special_end);
  else
    printf("%sCONTROL 0x%02x%s", special_begin, control, special_end);

  if(control == 0x0a)
    printf("\n");
  return 1;
}

static int parser_escape(const char bytes[], size_t len, void *user)
{
  if(bytes[0] >= 0x20 && bytes[0] < 0x30) {
    if(len < 2)
      return -1;
    len = 2;
  }
  else {
    len = 1;
  }

  printf("%sESC %.*s%s", special_begin, (int)len, bytes, special_end);

  return len;
}

/* 0     1      2      3       4     5      6      7      8      9      A      B      C      D      E      F    */
static const char *name_csi_plain[] = {
  "ICH", "CUU", "CUD", "CUF", "CUB", "CNL", "CPL", "CHA", "CUP", "CHT", "ED",  "EL",  "IL",  "DL",  "EF",  "EA",
  "DCH", "SSE", "CPR", "SU",  "SD",  "NP",  "PP",  "CTC", "ECH", "CVT", "CBT", "SRS", "PTX", "SDS", "SIMD",NULL,
  "HPA", "HPR", "REP", "DA",  "VPA", "VPR", "HVP", "TBC", "SM",  "MC",  "HPB", "VPB", "RM",  "SGR", "DSR", "DAQ",
};

/*0           4           8           B         */
static const int newline_csi_plain[] = {
  0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0,
};

static int parser_csi(const char *leader, const long args[], int argcount, const char *intermed, char command, void *user)
{
  const char *name = NULL;
  if(!leader && !intermed && command < 0x70)
    name = name_csi_plain[command - 0x40];
  else if(leader && streq(leader, "?") && !intermed) {
    /* DEC */
    switch(command) {
      case 'h': name = "DECSM"; break;
      case 'l': name = "DECRM"; break;
    }
    if(name)
      leader = NULL;
  }

  if(!leader && !intermed && command < 0x70 && newline_csi_plain[command - 0x40])
    printf("\n");

  if(name)
    printf("%s%s", special_begin, name);
  else
    printf("%sCSI", special_begin);

  if(leader && leader[0])
    printf(" %s", leader);

  {
    int i;
    for(i = 0; i < argcount; i++) {
      printf(i ? "," : " ");
  }

    if(args[i] == CSI_ARG_MISSING)
      printf("*");
    else {
      while(CSI_ARG_HAS_MORE(args[i]))
        printf("%ld+", CSI_ARG(args[i++]));
      printf("%ld", CSI_ARG(args[i]));
    }
  }

  if(intermed && intermed[0])
    printf(" %s", intermed);

  if(name)
    printf("%s", special_end);
  else
    printf(" %c%s", command, special_end);

  return 1;
}

static int parser_osc(const char *command, size_t cmdlen, void *user)
{
  printf("%sOSC %.*s%s", special_begin, (int)cmdlen, command, special_end);

  return 1;
}

static int parser_dcs(const char *command, size_t cmdlen, void *user)
{
  printf("%sDCS %.*s%s", special_begin, (int)cmdlen, command, special_end);

  return 1;
}

static VTermParserCallbacks parser_cbs = {
  &parser_text, /* text */
  &parser_control, /* control */
  &parser_escape, /* escape */
  &parser_csi, /* csi */
  &parser_osc, /* osc */
  &parser_dcs, /* dcs */
  NULL /* resize */
};

int main(int argc, char *argv[])
{
  int use_colour = isatty(1);
  const char *file;
  int fd;
  VTerm *vt;
  int len;
  char buffer[1024];

  int opt;
  while((opt = getopt(argc, argv, "c")) != -1) {
    switch(opt) {
      case 'c': use_colour = 1; break;
    }
  }

  file = argv[optind++];

  if(!file || streq(file, "-"))
    fd = 0; /* stdin */
  else {
    fd = open(file, O_RDONLY);
    if(fd == -1) {
      fprintf(stderr, "Cannot open %s - %s\n", file, strerror(errno));
      exit(1);
    }
  }

  if(use_colour) {
    special_begin = "\x1b[7m{";
    special_end   = "}\x1b[m";
  }

  /* Size matters not for the parser */
  vt = vterm_new(25, 80);
  vterm_set_utf8(vt, 1);
  vterm_parser_set_callbacks(vt, &parser_cbs, NULL);

  while((len = read(fd, buffer, sizeof(buffer))) > 0) {
    vterm_input_write(vt, buffer, len);
  }

  printf("\n");

  close(fd);
  vterm_free(vt);
  return 0;
}
