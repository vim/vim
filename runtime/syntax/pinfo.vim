" Vim syntax file
" Language:	    pinfo(1) configuration file
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/
" Latest Revision:  2004-05-22
" arch-tag:	    da2cfa1c-0350-45dc-b2d2-2bf3915bd0a2

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Set iskeyword since we need `-' (and potentially others) in keywords.
" For version 5.x: Set it globally
" For version 6.x: Set it locally
if version >= 600
  command -nargs=1 SetIsk setlocal iskeyword=<args>
else
  command -nargs=1 SetIsk set iskeyword=<args>
endif
SetIsk @,48-57,_,-
delcommand SetIsk

" Ignore Case
syn case ignore

" Todo
syn keyword pinfoTodo	contained FIXME TODO XXX NOTE

" Comments
syn region  pinfoComment    start='^#' end='$' contains=pinfoTodo

" Keywords
syn keyword pinfoOptions    MANUAL CUT-MAN-HEADERS CUT-EMPTY-MAN-LINES
syn keyword pinfoOptions    RAW-FILENAME APROPOS DONT-HANDLE-WITHOUT-TAG-TABLE
syn keyword pinfoOptions    HTTPVIEWER FTPVIEWER MAILEDITOR PRINTUTILITY
syn keyword pinfoOptions    MANLINKS INFOPATH MAN-OPTIONS STDERR-REDIRECTION
syn keyword pinfoOptions    LONG-MANUAL-LINKS FILTER-0xB7 QUIT-CONFIRMATION
syn keyword pinfoOptions    QUIT-CONFIRM-DEFAULT CLEAR-SCREEN-AT-EXIT
syn keyword pinfoOptions    CALL-READLINE-HISTORY HIGHLIGHTREGEXP SAFE-USER
syn keyword pinfoOptions    SAFE-GROUP

" Colors
syn keyword pinfoColors	    COL_NORMAL COL_TOPLINE COL_BOTTOMLINE COL_MENU
syn keyword pinfoColors	    COL_MENUSELECTED COL_NOTE COL_NOTESELECTED COL_URL
syn keyword pinfoColors	    COL_URLSELECTED COL_INFOHIGHLIGHT COL_MANUALBOLD
syn keyword pinfoColors	    COL_MANUALITALIC
syn keyword pinfoColorDefault	COLOR_DEFAULT
syn keyword pinfoColorBold	BOLD
syn keyword pinfoColorNoBold	NO_BOLD
syn keyword pinfoColorBlink	BLINK
syn keyword pinfoColorNoBlink	NO_BLINK
syn keyword pinfoColorBlack	COLOR_BLACK
syn keyword pinfoColorRed	COLOR_RED
syn keyword pinfoColorGreen	COLOR_GREEN
syn keyword pinfoColorYellow	COLOR_YELLOW
syn keyword pinfoColorBlue	COLOR_BLUE
syn keyword pinfoColorMagenta	COLOR_MAGENTA
syn keyword pinfoColorCyan	COLOR_CYAN
syn keyword pinfoColorWhite	COLOR_WHITE

" Keybindings
syn keyword pinfoKeys	KEY_TOTALSEARCH_1 KEY_TOTALSEARCH_2 KEY_SEARCH_1
syn keyword pinfoKeys	KEY_SEARCH_2 KEY_SEARCH_AGAIN_1 KEY_SEARCH_AGAIN_2
syn keyword pinfoKeys	KEY_GOTO_1 KEY_GOTO_2 KEY_PREVNODE_1 KEY_PREVNODE_2
syn keyword pinfoKeys	KEY_NEXTNODE_1 KEY_NEXTNODE_2 KEY_UP_1 KEY_UP_2
syn keyword pinfoKeys	KEY_END_1 KEY_END_2 KEY_PGDN_1 KEY_PGDN_2
syn keyword pinfoKeys	KEY_PGDN_AUTO_1 KEY_PGDN_AUTO_2 KEY_HOME_1 KEY_HOME_2
syn keyword pinfoKeys	KEY_PGUP_1 KEY_PGUP_2 KEY_PGUP_AUTO_1 KEY_PGUP_AUTO_2
syn keyword pinfoKeys	KEY_DOWN_1 KEY_DOWN_2 KEY_TOP_1 KEY_TOP_2 KEY_BACK_1
syn keyword pinfoKeys	KEY_BACK_2 KEY_FOLLOWLINK_1 KEY_FOLLOWLINK_2
syn keyword pinfoKeys	KEY_REFRESH_1 KEY_REFRESH_2 KEY_SHELLFEED_1
syn keyword pinfoKeys	KEY_SHELLFEED_2 KEY_QUIT_1 KEY_QUIT_2 KEY_GOLINE_1
syn keyword pinfoKeys	KEY_GOLINE_2 KEY_PRINT_1 KEY_PRINT_2
syn keyword pinfoKeys	KEY_DIRPAGE_1 KEY_DIRPAGE_2

" Special Keys
syn keyword pinfoSpecialKeys	KEY_BREAK KEY_DOWN KEY_UP KEY_LEFT KEY_RIGHT
syn keyword pinfoSpecialKeys	KEY_DOWN KEY_HOME KEY_BACKSPACE KEY_NPAGE
syn keyword pinfoSpecialKeys	KEY_PPAGE KEY_END KEY_IC KEY_DC
syn region  pinfoSpecialKeys	matchgroup=pinfoSpecialKeys transparent start=+KEY_\%(F\|CTRL\|ALT\)(+ end=+)+
syn region  pinfoSimpleKey	matchgroup=pinfoSimpleKey start=+'+ skip=+\\'+ end=+'+ contains=pinfoSimpleKeyEscape
syn match   pinfoSimpleKeyEscape    +\\[\\nt']+
syn match   pinfoKeycode    '\<\d\+\>'

" Constants
syn keyword pinfoConstants  TRUE FALSE YES NO

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_pinfo_syn_inits")
  if version < 508
    let did_pinfo_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
    command -nargs=+ HiDef hi <args>
  else
    command -nargs=+ HiLink hi def link <args>
    command -nargs=+ HiDef hi def <args>
  endif

  HiLink pinfoTodo		Todo
  HiLink pinfoComment		Comment
  HiLink pinfoOptions		Keyword
  HiLink pinfoColors		Keyword
  HiLink pinfoColorDefault	Normal
  HiDef pinfoColorBold		cterm=bold
  HiDef pinfoColorNoBold	cterm=none
  " we can't access the blink attribute from Vim atm
  HiDef pinfoColorBlink		cterm=inverse
  HiDef pinfoColorNoBlink	cterm=none
  HiDef pinfoColorBlack		ctermfg=Black	    guifg=Black
  HiDef pinfoColorRed		ctermfg=DarkRed	    guifg=DarkRed
  HiDef pinfoColorGreen		ctermfg=DarkGreen   guifg=DarkGreen
  HiDef pinfoColorYellow	ctermfg=DarkYellow  guifg=DarkYellow
  HiDef pinfoColorBlue		ctermfg=DarkBlue    guifg=DarkBlue
  HiDef pinfoColorMagenta	ctermfg=DarkMagenta guifg=DarkMagenta
  HiDef pinfoColorCyan		ctermfg=DarkCyan    guifg=DarkCyan
  HiDef pinfoColorWhite		ctermfg=LightGray   guifg=LightGray
  HiLink pinfoKeys		Keyword
  HiLink pinfoSpecialKeys	SpecialChar
  HiLink pinfoSimpleKey		String
  HiLink pinfoSimpleKeyEscape	SpecialChar
  HiLink pinfoKeycode		Number
  HiLink pinfoConstants	Constant

  delcommand HiLink
  delcommand HiDef
endif

let b:current_syntax = "pinfo"

" vim: set sts=2 sw=2:
