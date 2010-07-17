/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * macros.h: macro definitions for often used code
 */

/*
 * pchar(lp, c) - put character 'c' at position 'lp'
 */
#define pchar(lp, c) (*(ml_get_buf(curbuf, (lp).lnum, TRUE) + (lp).col) = (c))

/*
 * Position comparisons
 */
#ifdef FEAT_VIRTUALEDIT
# define lt(a, b) (((a).lnum != (b).lnum) \
		   ? (a).lnum < (b).lnum \
		   : (a).col != (b).col \
		       ? (a).col < (b).col \
		       : (a).coladd < (b).coladd)
# define ltp(a, b) (((a)->lnum != (b)->lnum) \
		   ? (a)->lnum < (b)->lnum \
		   : (a)->col != (b)->col \
		       ? (a)->col < (b)->col \
		       : (a)->coladd < (b)->coladd)
# define equalpos(a, b) (((a).lnum == (b).lnum) && ((a).col == (b).col) && ((a).coladd == (b).coladd))
# define clearpos(a) {(a)->lnum = 0; (a)->col = 0; (a)->coladd = 0;}
#else
# define lt(a, b) (((a).lnum != (b).lnum) \
		   ? ((a).lnum < (b).lnum) : ((a).col < (b).col))
# define ltp(a, b) (((a)->lnum != (b)->lnum) \
		   ? ((a)->lnum < (b)->lnum) : ((a)->col < (b)->col))
# define equalpos(a, b) (((a).lnum == (b).lnum) && ((a).col == (b).col))
# define clearpos(a) {(a)->lnum = 0; (a)->col = 0;}
#endif

#define ltoreq(a, b) (lt(a, b) || equalpos(a, b))

/*
 * lineempty() - return TRUE if the line is empty
 */
#define lineempty(p) (*ml_get(p) == NUL)

/*
 * bufempty() - return TRUE if the current buffer is empty
 */
#define bufempty() (curbuf->b_ml.ml_line_count == 1 && *ml_get((linenr_T)1) == NUL)

/*
 * toupper() and tolower() that use the current locale.
 * On some systems toupper()/tolower() only work on lower/uppercase
 * characters, first use islower() or isupper() then.
 * Careful: Only call TOUPPER_LOC() and TOLOWER_LOC() with a character in the
 * range 0 - 255.  toupper()/tolower() on some systems can't handle others.
 * Note: It is often better to use MB_TOLOWER() and MB_TOUPPER(), because many
 * toupper() and tolower() implementations only work for ASCII.
 */
#ifdef MSWIN
#  define TOUPPER_LOC(c)	toupper_tab[(c) & 255]
#  define TOLOWER_LOC(c)	tolower_tab[(c) & 255]
#else
# ifdef BROKEN_TOUPPER
#  define TOUPPER_LOC(c)	(islower(c) ? toupper(c) : (c))
#  define TOLOWER_LOC(c)	(isupper(c) ? tolower(c) : (c))
# else
#  define TOUPPER_LOC		toupper
#  define TOLOWER_LOC		tolower
# endif
#endif

/* toupper() and tolower() for ASCII only and ignore the current locale. */
#ifdef EBCDIC
# define TOUPPER_ASC(c)	(islower(c) ? toupper(c) : (c))
# define TOLOWER_ASC(c)	(isupper(c) ? tolower(c) : (c))
#else
# define TOUPPER_ASC(c)	(((c) < 'a' || (c) > 'z') ? (c) : (c) - ('a' - 'A'))
# define TOLOWER_ASC(c)	(((c) < 'A' || (c) > 'Z') ? (c) : (c) + ('a' - 'A'))
#endif

/*
 * MB_ISLOWER() and MB_ISUPPER() are to be used on multi-byte characters.  But
 * don't use them for negative values!
 */
#ifdef FEAT_MBYTE
# define MB_ISLOWER(c)	vim_islower(c)
# define MB_ISUPPER(c)	vim_isupper(c)
# define MB_TOLOWER(c)	vim_tolower(c)
# define MB_TOUPPER(c)	vim_toupper(c)
#else
# define MB_ISLOWER(c)	islower(c)
# define MB_ISUPPER(c)	isupper(c)
# define MB_TOLOWER(c)	TOLOWER_LOC(c)
# define MB_TOUPPER(c)	TOUPPER_LOC(c)
#endif

/* Like isalpha() but reject non-ASCII characters.  Can't be used with a
 * special key (negative value). */
#ifdef EBCDIC
# define ASCII_ISALPHA(c) isalpha(c)
# define ASCII_ISALNUM(c) isalnum(c)
# define ASCII_ISLOWER(c) islower(c)
# define ASCII_ISUPPER(c) isupper(c)
#else
# define ASCII_ISALPHA(c) ((c) < 0x7f && isalpha(c))
# define ASCII_ISALNUM(c) ((c) < 0x7f && isalnum(c))
# define ASCII_ISLOWER(c) ((c) < 0x7f && islower(c))
# define ASCII_ISUPPER(c) ((c) < 0x7f && isupper(c))
#endif

/* Use our own isdigit() replacement, because on MS-Windows isdigit() returns
 * non-zero for superscript 1.  Also avoids that isdigit() crashes for numbers
 * below 0 and above 255.  For complicated arguments and in/decrement use
 * vim_isdigit() instead. */
#define VIM_ISDIGIT(c) ((c) >= '0' && (c) <= '9')

/* macro version of chartab().
 * Only works with values 0-255!
 * Doesn't work for UTF-8 mode with chars >= 0x80. */
#define CHARSIZE(c)	(chartab[c] & CT_CELL_MASK)

#ifdef FEAT_LANGMAP
/*
 * Adjust chars in a language according to 'langmap' option.
 * NOTE that there is no noticeable overhead if 'langmap' is not set.
 * When set the overhead for characters < 256 is small.
 * Don't apply 'langmap' if the character comes from the Stuff buffer.
 * The do-while is just to ignore a ';' after the macro.
 */
# ifdef FEAT_MBYTE
#  define LANGMAP_ADJUST(c, condition) \
    do { \
	if (*p_langmap && (condition) && !KeyStuffed && (c) >= 0) \
	{ \
	    if ((c) < 256) \
		c = langmap_mapchar[c]; \
	    else \
		c = langmap_adjust_mb(c); \
	} \
    } while (0)
# else
#  define LANGMAP_ADJUST(c, condition) \
    do { \
	if (*p_langmap && (condition) && !KeyStuffed && (c) >= 0 && (c) < 256) \
	    c = langmap_mapchar[c]; \
    } while (0)
# endif
#else
# define LANGMAP_ADJUST(c, condition) /* nop */
#endif

/*
 * vim_isbreak() is used very often if 'linebreak' is set, use a macro to make
 * it work fast.
 */
#define vim_isbreak(c) (breakat_flags[(char_u)(c)])

/*
 * On VMS file names are different and require a translation.
 * On the Mac open() has only two arguments.
 */
#ifdef VMS
# define mch_access(n, p)	access(vms_fixfilename(n), (p))
				/* see mch_open() comment */
# define mch_fopen(n, p)	fopen(vms_fixfilename(n), (p))
# define mch_fstat(n, p)	fstat(vms_fixfilename(n), (p))
	/* VMS does not have lstat() */
# define mch_stat(n, p)		stat(vms_fixfilename(n), (p))
#else
# ifndef WIN32
#   define mch_access(n, p)	access((n), (p))
# endif
# if !(defined(FEAT_MBYTE) && defined(WIN3264))
#  define mch_fopen(n, p)	fopen((n), (p))
# endif
# define mch_fstat(n, p)	fstat((n), (p))
# ifdef MSWIN	/* has it's own mch_stat() function */
#  define mch_stat(n, p)	vim_stat((n), (p))
# else
#  ifdef STAT_IGNORES_SLASH
    /* On Solaris stat() accepts "file/" as if it was "file".  Return -1 if
     * the name ends in "/" and it's not a directory. */
#   define mch_stat(n, p)	(illegal_slash(n) ? -1 : stat((n), (p)))
#  else
#   define mch_stat(n, p)	stat((n), (p))
#  endif
# endif
#endif

#ifdef HAVE_LSTAT
# define mch_lstat(n, p)	lstat((n), (p))
#else
# define mch_lstat(n, p)	mch_stat((n), (p))
#endif

#ifdef MACOS_CLASSIC
/* MacOS classic doesn't support perm but MacOS X does. */
# define mch_open(n, m, p)	open((n), (m))
#else
# ifdef VMS
/*
 * It is possible to force some record format with:
 * #  define mch_open(n, m, p) open(vms_fixfilename(n), (m), (p)), "rat=cr", "rfm=stmlf", "mrs=0")
 * but it is not recommended, because it can destroy indexes etc.
 */
#  define mch_open(n, m, p)	open(vms_fixfilename(n), (m), (p))
# else
#  if !(defined(FEAT_MBYTE) && defined(WIN3264))
#   define mch_open(n, m, p)	open((n), (m), (p))
#  endif
# endif
#endif

/* mch_open_rw(): invoke mch_open() with third argument for user R/W. */
#if defined(UNIX) || defined(VMS)  /* open in rw------- mode */
# define mch_open_rw(n, f)	mch_open((n), (f), (mode_t)0600)
#else
# if defined(MSDOS) || defined(MSWIN) || defined(OS2)  /* open read/write */
#  define mch_open_rw(n, f)	mch_open((n), (f), S_IREAD | S_IWRITE)
# else
#  define mch_open_rw(n, f)	mch_open((n), (f), 0)
# endif
#endif

#ifdef STARTUPTIME
# define TIME_MSG(s) { if (time_fd != NULL) time_msg(s, NULL); }
#else
# define TIME_MSG(s)
#endif

#ifdef FEAT_VREPLACE
# define REPLACE_NORMAL(s) (((s) & REPLACE_FLAG) && !((s) & VREPLACE_FLAG))
#else
# define REPLACE_NORMAL(s) ((s) & REPLACE_FLAG)
#endif

#ifdef FEAT_ARABIC
# define UTF_COMPOSINGLIKE(p1, p2)  utf_composinglike((p1), (p2))
#else
# define UTF_COMPOSINGLIKE(p1, p2)  utf_iscomposing(utf_ptr2char(p2))
#endif

#ifdef FEAT_RIGHTLEFT
    /* Whether to draw the vertical bar on the right side of the cell. */
# define CURSOR_BAR_RIGHT (curwin->w_p_rl && (!(State & CMDLINE) || cmdmsg_rl))
#endif

/*
 * mb_ptr_adv(): advance a pointer to the next character, taking care of
 * multi-byte characters if needed.
 * mb_ptr_back(): backup a pointer to the previous character, taking care of
 * multi-byte characters if needed.
 * MB_COPY_CHAR(f, t): copy one char from "f" to "t" and advance the pointers.
 * PTR2CHAR(): get character from pointer.
 */
#ifdef FEAT_MBYTE
/* Advance multi-byte pointer, skip over composing chars. */
# define mb_ptr_adv(p)	    p += has_mbyte ? (*mb_ptr2len)(p) : 1
/* Advance multi-byte pointer, do not skip over composing chars. */
# define mb_cptr_adv(p)	    p += enc_utf8 ? utf_ptr2len(p) : has_mbyte ? (*mb_ptr2len)(p) : 1
/* Backup multi-byte pointer. */
# define mb_ptr_back(s, p)  p -= has_mbyte ? ((*mb_head_off)(s, p - 1) + 1) : 1
/* get length of multi-byte char, not including composing chars */
# define mb_cptr2len(p)	    (enc_utf8 ? utf_ptr2len(p) : (*mb_ptr2len)(p))

# define MB_COPY_CHAR(f, t) if (has_mbyte) mb_copy_char(&f, &t); else *t++ = *f++
# define MB_CHARLEN(p)	    (has_mbyte ? mb_charlen(p) : (int)STRLEN(p))
# define PTR2CHAR(p)	    (has_mbyte ? mb_ptr2char(p) : (int)*(p))
#else
# define mb_ptr_adv(p)		++p
# define mb_cptr_adv(p)		++p
# define mb_ptr_back(s, p)	--p
# define MB_COPY_CHAR(f, t)	*t++ = *f++
# define MB_CHARLEN(p)		STRLEN(p)
# define PTR2CHAR(p)		((int)*(p))
#endif

#ifdef FEAT_AUTOCHDIR
# define DO_AUTOCHDIR if (p_acd) do_autochdir();
#else
# define DO_AUTOCHDIR
#endif
