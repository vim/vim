/* xxd: my hexdump facility. jw
 *
 *  2.10.90 changed to word output
 *  3.03.93 new indent style, dumb bug inserted and fixed.
 *	    -c option, mls
 * 26.04.94 better option parser, -ps, -l, -s added.
 *  1.07.94 -r badly needs - as input file.  Per default autoskip over
 *	       consecutive lines of zeroes, as unix od does.
 *	    -a shows them too.
 *	    -i dump as c-style #include "file.h"
 *  1.11.95 if "xxd -i" knows the filename, an 'unsigned char filename_bits[]'
 *	    array is written in correct c-syntax.
 *	    -s improved, now defaults to absolute seek, relative requires a '+'.
 *	    -r improved, now -r -s -0x... is supported.
 *	       change/suppress leading '\0' bytes.
 *	    -l n improved: stops exactly after n bytes.
 *	    -r improved, better handling of partial lines with trailing garbage.
 *	    -r improved, now -r -p works again!
 *	    -r improved, less flushing, much faster now! (that was silly)
 *  3.04.96 Per repeated request of a single person: autoskip defaults to off.
 * 15.05.96 -v added. They want to know the version.
 *	    -a fixed, to show last line inf file ends in all zeros.
 *	    -u added: Print upper case hex-letters, as preferred by unix bc.
 *	    -h added to usage message. Usage message extended.
 *	    Now using outfile if specified even in normal mode, aehem.
 *	    No longer mixing of ints and longs. May help doze people.
 *	    Added binify ioctl for same reason. (Enough Doze stress for 1996!)
 * 16.05.96 -p improved, removed occasional superfluous linefeed.
 * 20.05.96 -l 0 fixed. tried to read anyway.
 * 21.05.96 -i fixed. now honours -u, and prepends __ to numeric filenames.
 *	    compile -DWIN32 for NT or W95. George V. Reilly, * -v improved :-)
 *	    support --gnuish-longhorn-options
 * 25.05.96 MAC support added: CodeWarrior already uses ``outline'' in Types.h
 *	    which is included by MacHeaders (Axel Kielhorn). Renamed to
 *	    xxdline().
 *  7.06.96 -i printed 'int' instead of 'char'. *blush*
 *	    added Bram's OS2 ifdefs...
 * 18.07.96 gcc -Wall @ SunOS4 is now slient.
 *	    Added osver for MSDOS/DJGPP/WIN32.
 * 29.08.96 Added size_t to strncmp() for Amiga.
 * 24.03.97 Windows NT support (Phil Hanna). Clean exit for Amiga WB (Bram)
 * 02.04.97 Added -E option, to have EBCDIC translation instead of ASCII
 *	    (azc10@yahoo.com)
 * 22.05.97 added -g (group octets) option (jcook@namerica.kla.com).
 * 23.09.98 nasty -p -r misfeature fixed: slightly wrong output, when -c was
 *	    missing or wrong.
 * 26.09.98 Fixed: 'xxd -i infile outfile' did not truncate outfile.
 * 27.10.98 Fixed: -g option parser required blank.
 *	    option -b added: 01000101 binary output in normal format.
 * 16.05.00 Added VAXC changes by Stephen P. Wall
 * 16.05.00 Improved MMS file and merge for VMS by Zoltan Arpadffy
 * 2011 March  Better error handling by Florian Zumbiehl.
 * 2011 April  Formatting by Bram Moolenaar
 * 08.06.2013  Little-endian hexdump (-e) and offset (-o) by Vadim Vygonets.
 *
 * (c) 1990-1998 by Juergen Weigert (jnweiger@informatik.uni-erlangen.de)
 *
 * I hereby grant permission to distribute and use xxd
 * under X11-MIT or GPL-2.0 (at the user's choice).
 *
 * Small changes made afterwards by Bram Moolenaar et al.
 *
 * Distribute freely and credit me,
 * make money and share with me,
 * lose money and don't ask me.
 */

/* Visual Studio 2005 has 'deprecated' many of the standard CRT functions */
#if _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif
#if !defined(CYGWIN) && (defined(CYGWIN32) || defined(__CYGWIN__) || defined(__CYGWIN32__))
# define CYGWIN
#endif

#define STR(X) #X

#include <stdio.h>
#include "xxd.h"
#ifdef VAXC
# include <file.h>
#else
# include <fcntl.h>
#endif
#if defined(WIN32) || defined(__BORLANDC__) || defined(CYGWIN)
# include <io.h>	/* for setmode() */
#else
# ifdef UNIX
#  include <unistd.h>
# endif
#endif
#include <stdlib.h>
#include <errno.h>	/* for errno */
#include <ctype.h>	/* for isalnum() */
#if __MWERKS__ && !defined(BEBOX)
# include <unix.h>	/* for fdopen() on MAC */
#endif

#if defined(__BORLANDC__) && __BORLANDC__ <= 0x0410 && !defined(fileno)
/* Missing define and prototype grabbed from the BC 4.0 <stdio.h> */
# define fileno(f)       ((f)->fd)
FILE   _FAR *_Cdecl _FARFUNC fdopen(int __handle, char _FAR *__type);
#endif


/*  This corrects the problem of missing prototypes for certain functions
 *  in some GNU installations (e.g. SunOS 4.1.x).
 *  Darren Hiebert <darren@hmi.com> (sparc-sun-sunos4.1.3_U1/2.7.2.2)
 */
#if defined(__GNUC__) && defined(__STDC__)
# ifndef __USE_FIXED_PROTOTYPES__
#  define __USE_FIXED_PROTOTYPES__
# endif
#endif

#ifndef __USE_FIXED_PROTOTYPES__
/*
 * This is historic and works only if the compiler really has no prototypes:
 *
 * Include prototypes for Sun OS 4.x, when using an ANSI compiler.
 * FILE is defined on OS 4.x, not on 5.x (Solaris).
 * if __SVR4 is defined (some Solaris versions), don't include this.
 */
#if defined(sun) && defined(FILE) && !defined(__SVR4) && defined(__STDC__)
#  define __P(a) a
/* excerpt from my sun_stdlib.h */
extern int fprintf __P((FILE *, char *, ...));
extern int fputs   __P((char *, FILE *));
extern int _flsbuf __P((unsigned char, FILE *));
extern int _filbuf __P((FILE *));
extern int fflush  __P((FILE *));
extern int fclose  __P((FILE *));
extern int fseek   __P((FILE *, long, int));
extern int rewind  __P((FILE *));

extern void perror __P((char *));
# endif
#endif

extern long int strtol();
extern long int ftell();

#ifdef WIN32
char version[] = "xxd V1.10 27oct98 by Juergen Weigert (Win32)";
#else
char version[] = "xxd V1.10 27oct98 by Juergen Weigert";
#endif

#if defined(WIN32)
# define BIN_READ(yes)  ((yes) ? "rb" : "rt")
# define BIN_WRITE(yes) ((yes) ? "wb" : "wt")
# define BIN_CREAT(yes) ((yes) ? (O_CREAT|O_BINARY) : O_CREAT)
# define BIN_ASSIGN(fp, yes) setmode(fileno(fp), (yes) ? O_BINARY : O_TEXT)
# define PATH_SEP '\\'
#elif defined(CYGWIN)
# define BIN_READ(yes)  ((yes) ? "rb" : "rt")
# define BIN_WRITE(yes) ((yes) ? "wb" : "w")
# define BIN_CREAT(yes) ((yes) ? (O_CREAT|O_BINARY) : O_CREAT)
# define BIN_ASSIGN(fp, yes) ((yes) ? (void) setmode(fileno(fp), O_BINARY) : (void) (fp))
# define PATH_SEP '/'
#else
# ifdef VMS
#  define BIN_READ(dummy)  "r"
#  define BIN_WRITE(dummy) "w"
#  define BIN_CREAT(dummy) O_CREAT
#  define BIN_ASSIGN(fp, dummy) fp
#  define PATH_SEP ']'
#  define FILE_SEP '.'
# else
#  define BIN_READ(dummy)  "r"
#  define BIN_WRITE(dummy) "w"
#  define BIN_CREAT(dummy) O_CREAT
#  define BIN_ASSIGN(fp, dummy) fp
#  define PATH_SEP '/'
# endif
#endif

/* open has only to arguments on the Mac */
#if __MWERKS__
# define OPEN(name, mode, umask) open(name, mode)
#else
# define OPEN(name, mode, umask) open(name, mode, umask)
#endif

#ifndef __P
# if defined(__STDC__) || defined(WIN32) || defined(__BORLANDC__)
#  define __P(a) a
# else
#  define __P(a) ()
# endif
#endif

/* Let's collect some prototypes */
/* CodeWarrior is really picky about missing prototypes */
static void exit_with_usage __P((char *));

#define TRY_SEEK	/* attempt to use lseek, or skip forward by reading */

  static void
exit_with_usage(char *pname)
{
  fprintf(stderr, "Usage:\n       %s [options] [infile [outfile]]\n", pname);
  fprintf(stderr, "    or\n       %s -r [-s [-]offset] [-c cols] [-ps] [infile [outfile]]\n", pname);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "    -a          toggle autoskip: A single '*' replaces nul-lines. Default off.\n");
  fprintf(stderr, "    -b          binary digit dump (incompatible with -ps,-i,-r). Default hex.\n");
  fprintf(stderr, "    -c cols     format <cols> octets per line. Default 16 (-i: 12, -ps: 30).\n");
  fprintf(stderr, "    -E          show characters in EBCDIC. Default ASCII.\n");
  fprintf(stderr, "    -e          little-endian dump (incompatible with -ps,-i,-r).\n");
  fprintf(stderr, "    -g          number of octets per group in normal output. Default 2 (-e: 4).\n");
  fprintf(stderr, "    -h          print this summary.\n");
  fprintf(stderr, "    -i          output in C include file style.\n");
  fprintf(stderr, "    -l len      stop after <len> octets.\n");
  fprintf(stderr, "    -o off      add <off> to the displayed file position.\n");
  fprintf(stderr, "    -ps         output in postscript plain hexdump style.\n");
  fprintf(stderr, "    -r          reverse operation: convert (or patch) hexdump into binary.\n");
  fprintf(stderr, "    -r -s off   revert with <off> added to file positions found in hexdump.\n");
  fprintf(stderr, "    -s %sseek  start at <seek> bytes abs. %sinfile offset.\n",
#ifdef TRY_SEEK
	  "[+][-]", "(or +: rel.) ");
#else
	  "", "");
#endif
  fprintf(stderr, "    -u          use upper case hex letters.\n");
  fprintf(stderr, "    -v          show version: \"%s\".\n", version);
  exit(1);
}

static char *pname(char **argv) {
  char *pname, *pp;

  pname = argv[0];
  for (pp = pname; *pp; )
    if (*pp++ == PATH_SEP)
      pname = pp;
#ifdef FILE_SEP
  for (pp = pname; *pp; pp++)
    if (*pp == FILE_SEP)
      {
	*pp = '\0';
	break;
      }
#endif

  return pname;
}

static int handle_error(xxd_ctx *ctx, char **argv, xxd_rc rc) {
  if (rc == XXD_USAGE_ERROR) {
    exit_with_usage(pname(argv));
  } else if (rc != XXD_OK) {
    fprintf(stderr, "%s: %s\n", pname(argv), ctx->error);
    exit(ctx->exit_code);
  }
  return EXIT_SUCCESS;
}

  int
main(int argc, char *argv[])
{
  xxd_ctx ctx;

#ifdef AMIGA
  /* This program doesn't work when started from the Workbench */
  if (argc == 0)
    exit(1);
#endif

  xxd_init(&ctx);

  handle_error(&ctx, argv, xxd_parse_cmd_line(&ctx, argc, argv));

  handle_error(&ctx, argv, xxd_validate(&ctx));

  if (argc > 3)
    exit_with_usage(pname(argv));

  if (argc == 1 || (argv[1][0] == '-' && !argv[1][1]))
    BIN_ASSIGN(ctx.fp = stdin, !ctx.revert);
  else
    {
      ctx.input_filename = argv[1];
      if ((ctx.fp = fopen(ctx.input_filename, BIN_READ(!ctx.revert))) == NULL)
	{
	  fprintf(stderr,"%s: ", pname(argv));
	  perror(ctx.input_filename);
	  return 2;
	}
    }

  if (argc < 3 || (argv[2][0] == '-' && !argv[2][1]))
    BIN_ASSIGN(ctx.fpo = stdout, ctx.revert);
  else
    {
      int fd;
      int mode = ctx.revert ? O_WRONLY : (O_TRUNC|O_WRONLY);

      if (((fd = OPEN(argv[2], mode | BIN_CREAT(ctx.revert), 0666)) < 0) ||
	  (ctx.fpo = fdopen(fd, BIN_WRITE(ctx.revert))) == NULL)
	{
	  fprintf(stderr, "%s: ", pname(argv));
	  perror(argv[2]);
	  return 3;
	}
      rewind(ctx.fpo);
    }

  return handle_error(&ctx, argv, xxd(&ctx));
}

/* vi:set ts=8 sw=4 sts=2 cino+={2 cino+=n-2 : */
