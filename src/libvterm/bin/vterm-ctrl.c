#define _XOPEN_SOURCE 500  /* strdup */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define streq(a,b) (strcmp(a,b)==0)
#define TRUE 1
#define FALSE 0

#include <termios.h>

static char *getvalue(int *argip, int argc, char *argv[])
{
  if(*argip >= argc) {
    fprintf(stderr, "Expected an option value\n");
    exit(1);
  }

  return argv[(*argip)++];
}

static int getchoice(int *argip, int argc, char *argv[], const char *options[])
{
  const char *arg = getvalue(argip, argc, argv);

  int value = -1;
  while(options[++value])
    if(streq(arg, options[value]))
      return value;

  fprintf(stderr, "Unrecognised option value %s\n", arg);
  exit(1);
}

typedef enum {
  OFF,
  ON,
  QUERY
} BoolQuery;

static BoolQuery getboolq(int *argip, int argc, char *argv[])
{
  const char *choices[] = {"off", "on", "query", NULL};
  return getchoice(argip, argc, argv, choices);
}

static char *helptext[] = {
  "reset",
  "s8c1t [off|on]",
  "keypad [app|num]",
  "screen [off|on|query]",
  "cursor [off|on|query]",
  "curblink [off|on|query]",
  "curshape [block|under|bar|query]",
  "mouse [off|click|clickdrag|motion]",
  "altscreen [off|on|query]",
  "bracketpaste [off|on|query]",
  "icontitle [STR]",
  "icon [STR]",
  "title [STR]",
  NULL
};

static int seticanon(int icanon, int echo)
{
  struct termios termios;
  int ret;

  tcgetattr(0, &termios);

  ret = (termios.c_lflag & ICANON);

  if(icanon) termios.c_lflag |=  ICANON;
  else       termios.c_lflag &= ~ICANON;

  if(echo) termios.c_lflag |=  ECHO;
  else     termios.c_lflag &= ~ECHO;

  tcsetattr(0, TCSANOW, &termios);

  return ret;
}

static void await_c1(int c1)
{
  int c;

  /* await CSI - 8bit or 2byte 7bit form */
  int in_esc = FALSE;
  while((c = getchar())) {
    if(c == c1)
      break;
    if(in_esc && c == (char)(c1 - 0x40))
      break;
    if(!in_esc && c == 0x1b)
      in_esc = TRUE;
    else
      in_esc = FALSE;
  }
}

static char *read_csi()
{
  unsigned char csi[32];
  int i = 0;

  await_c1(0x9B); /* CSI */

  /* TODO: This really should be a more robust CSI parser
   */
  for(; i < sizeof(csi)-1; i++) {
    int c = csi[i] = getchar();
    if(c >= 0x40 && c <= 0x7e)
      break;
  }
  csi[++i] = 0;

  /* TODO: returns longer than 32? */

  return strdup((char *)csi);
}

static char *read_dcs()
{
  unsigned char dcs[32];
  int in_esc = FALSE;
  int i;

  await_c1(0x90);

  for(i = 0; i < sizeof(dcs)-1; ) {
    char c = getchar();
    if(c == 0x9c) /* ST */
      break;
    if(in_esc && c == 0x5c)
      break;
    if(!in_esc && c == 0x1b)
      in_esc = TRUE;
    else {
      dcs[i++] = c;
      in_esc = FALSE;
    }
  }
  dcs[++i] = 0;

  return strdup((char *)dcs);
}

static void usage(int exitcode)
{
  char **p;

  fprintf(stderr, "Control a libvterm-based terminal\n"
      "\n"
      "Options:\n");

  for(p = helptext; *p; p++)
    fprintf(stderr, "  %s\n", *p);

  exit(exitcode);
}

static int query_dec_mode(int mode)
{
  char *s = NULL;

  printf("\x1b[?%d$p", mode);

  do {
    int reply_mode, reply_value;
    char reply_cmd;

    if(s)
      free(s);
    s = read_csi();

    /* expect "?" mode ";" value "$y" */

    /* If the sscanf format string ends in a literal, we can't tell from
     * its return value if it matches. Hence we'll %c the cmd and check it
     * explicitly
     */
    if(sscanf(s, "?%d;%d$%c", &reply_mode, &reply_value, &reply_cmd) < 3)
      continue;
    if(reply_cmd != 'y')
      continue;

    if(reply_mode != mode)
      continue;

    free(s);

    if(reply_value == 1 || reply_value == 3)
      return TRUE;
    if(reply_value == 2 || reply_value == 4)
      return FALSE;

    printf("Unrecognised reply to DECRQM: %d\n", reply_value);
    return FALSE;
  } while(1);
}

static void do_dec_mode(int mode, BoolQuery val, const char *name)
{
  switch(val) {
    case OFF:
    case ON:
      printf("\x1b[?%d%c", mode, val == ON ? 'h' : 'l');
      break;

    case QUERY:
      if(query_dec_mode(mode))
        printf("%s on\n", name);
      else
        printf("%s off\n", name);
      break;
  }
}

static int query_rqss_numeric(char *cmd)
{
  char *s = NULL;

  printf("\x1bP$q%s\x1b\\", cmd);

  do {
    int num;

    if(s)
      free(s);
    s = read_dcs();

    if(!s)
      return -1;
    if(strlen(s) < strlen(cmd))
      return -1;
    if(strcmp(s + strlen(s) - strlen(cmd), cmd) != 0) {
      printf("No match\n");
      continue;
    }

    if(s[0] != '1' || s[1] != '$' || s[2] != 'r')
      return -1;

    if(sscanf(s + 3, "%d", &num) != 1)
      return -1;

    return num;
  } while(1);
}

int wasicanon;

void restoreicanon(void)
{
  seticanon(wasicanon, TRUE);
}

int main(int argc, char *argv[])
{
  int argi = 1;

  if(argc == 1)
    usage(0);

  wasicanon = seticanon(FALSE, FALSE);
  atexit(restoreicanon);

  while(argi < argc) {
    const char *arg = argv[argi++];

    if(streq(arg, "reset")) {
      printf("\x1b" "c");
    }
    else if(streq(arg, "s8c1t")) {
      const char *choices[] = {"off", "on", NULL};
      switch(getchoice(&argi, argc, argv, choices)) {
      case 0:
        printf("\x1b F"); break;
      case 1:
        printf("\x1b G"); break;
      }
    }
    else if(streq(arg, "keypad")) {
      const char *choices[] = {"app", "num", NULL};
      switch(getchoice(&argi, argc, argv, choices)) {
      case 0:
        printf("\x1b="); break;
      case 1:
        printf("\x1b>"); break;
      }
    }
    else if(streq(arg, "screen")) {
      do_dec_mode(5, getboolq(&argi, argc, argv), "screen");
    }
    else if(streq(arg, "cursor")) {
      do_dec_mode(25, getboolq(&argi, argc, argv), "cursor");
    }
    else if(streq(arg, "curblink")) {
      do_dec_mode(12, getboolq(&argi, argc, argv), "curblink");
    }
    else if(streq(arg, "curshape")) {
      /* TODO: This ought to query the current value of DECSCUSR because it */
      /*   may need blinking on or off */
      const char *choices[] = {"block", "under", "bar", "query", NULL};
      int shape = getchoice(&argi, argc, argv, choices);
      switch(shape) {
        case 3: /* query */
          shape = query_rqss_numeric(" q");
          switch(shape) {
            case 1: case 2:
              printf("curshape block\n");
              break;
            case 3: case 4:
              printf("curshape under\n");
              break;
            case 5: case 6:
              printf("curshape bar\n");
              break;
          }
          break;

        case 0:
        case 1:
        case 2:
          printf("\x1b[%d q", 1 + (shape * 2));
          break;
      }
    }
    else if(streq(arg, "mouse")) {
      const char *choices[] = {"off", "click", "clickdrag", "motion", NULL};
      switch(getchoice(&argi, argc, argv, choices)) {
      case 0:
        printf("\x1b[?1000l"); break;
      case 1:
        printf("\x1b[?1000h"); break;
      case 2:
        printf("\x1b[?1002h"); break;
      case 3:
        printf("\x1b[?1003h"); break;
      }
    }
    else if(streq(arg, "altscreen")) {
      do_dec_mode(1049, getboolq(&argi, argc, argv), "altscreen");
    }
    else if(streq(arg, "bracketpaste")) {
      do_dec_mode(2004, getboolq(&argi, argc, argv), "bracketpaste");
    }
    else if(streq(arg, "icontitle")) {
      printf("\x1b]0;%s\a", getvalue(&argi, argc, argv));
    }
    else if(streq(arg, "icon")) {
      printf("\x1b]1;%s\a", getvalue(&argi, argc, argv));
    }
    else if(streq(arg, "title")) {
      printf("\x1b]2;%s\a", getvalue(&argi, argc, argv));
    }
    else {
      fprintf(stderr, "Unrecognised command %s\n", arg);
      exit(1);
    }
  }
  return 0;
}
