" Vim syntax file
" Language:	    calendar(1) file.
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/syntax/pcp/calendar/
" Latest Revision:  2004-05-06
" arch-tag:	    d714127d-469d-43bd-9c79-c2a46ec54535

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Todo
syn keyword calendarTodo	contained TODO FIXME XXX NOTE

" Comments
syn region  calendarComment	matchgroup=calendarComment start='/\*' end='\*/' contains=calendarTodo

" Strings
syn region  calendarCppString	start=+L\="+ skip=+\\\\\|\\"\|\\$+ excludenl end=+"+ end='$' contains=calendarSpecial
syn match   calendarSpecial	display contained '\\\%(x\x\+\|\o\{1,3}\|.\|$\)'
syn match   calendarSpecial	display contained "\\\(u\x\{4}\|U\x\{8}\)"

" cpp(1) Preprocessor directives (adapted from syntax/c.vim)

syn region  calendarPreCondit	start='^\s*#\s*\%(if\|ifdef\|ifndef\|elif\)\>' skip='\\$' end='$' contains=calendarComment,calendarCppString
syn match   calendarPreCondit	display '^\s*#\s*\%(else\|endif\)\>'
syn region  calendarCppOut	start='^\s*#\s*if\s\+0\+' end='.\@=\|$' contains=calendarCppOut2
syn region  calendarCppOut2	contained start='0' end='^\s*#\s*\%(endif\|else\|elif\)\>' contains=calendarSpaceError,calendarCppSkip
syn region  calendarCppSkip	contained start='^\s*#\s*\%(if\|ifdef\|ifndef\)\>' skip='\\$' end='^\s*#\s*endif\>' contains=calendarSpaceError,calendarCppSkip
syn region  calendarIncluded	display contained start=+"+ skip=+\\\\\|\\"+ end=+"+
syn match   calendarIncluded	display contained '<[^>]*>'
syn match   calendarInclude	display '^\s*#\s*include\>\s*["<]' contains=calendarIncluded
syn cluster calendarPreProcGroup    contains=calendarPreCondit,calendarIncluded,calendarInclude,calendarDefine,calendarCppOut,calendarCppOut2,calendarCppSkip,calendarString,calendarSpecial,calendarTodo
syn region  calendarDefine	start='^\s*#\s*\%(define\|undef\)\>' skip='\\$' end='$' contains=ALLBUT,@calendarPreProcGroup
syn region  calendarPreProc	start='^\s*#\s*\%(pragma\|line\|warning\|warn\|error\)\>' skip='\\$' end='$' keepend contains=ALLBUT,@calendarPreProcGroup

" Keywords
syn keyword calendarKeyword	CHARSET BODUN LANG
syn case ignore
syn keyword calendarKeyword	Easter Pashka
syn case match

" Dates
syn case ignore
syn match   calendarNumber	'\<\d\+\>'
syn keyword calendarMonth	Jan[uary] Feb[ruary] Mar[ch] Apr[il] May Jun[e]
syn keyword calendarMonth	Jul[y] Aug[ust] Sep[tember] Oct[ober]
syn keyword calendarMonth	Nov[ember] Dec[ember]
syn match   calendarMonth	'\<\%(Jan\|Feb\|Mar\|Apr\|May\|Jun\|Jul\|Aug\|Sep\|Oct\|Nov\|Dec\)\.'
syn keyword calendarWeekday	Mon[day] Tue[sday] Wed[nesday] Thu[rsday]
syn keyword calendarWeekday	Fri[day] Sat[urday] Sun[day]
syn match   calendarWeekday	'\<\%(Mon\|Tue\|Wed\|Thu\|Fri\|Sat\|Sun\)\.' nextgroup=calendarWeekdayMod
syn match   calendarWeekdayMod	'[+-]\d\+\>'
syn case match

" Times
syn match   calendarTime	'\<\%([01]\=\d\|2[0-3]\):[0-5]\d\%(:[0-5]\d\)\='
syn match   calendarTime	'\<\%(0\=[1-9]\|1[0-2]\):[0-5]\d\%(:[0-5]\d\)\=\s*[AaPp][Mm]'

" Variables
syn match calendarVariable	'\*'

let b:c_minlines = 50		" #if 0 constructs can be long
exec "syn sync ccomment calendarComment minlines=" . b:c_minlines

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_calendar_syn_inits")
  if version < 508
    let did_calendar_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink calendarTodo		Todo
  HiLink calendarComment	Comment
  HiLink calendarCppString	String
  HiLink calendarSpecial	SpecialChar
  HiLink calendarPreCondit	PreCondit
  HiLink calendarCppOut	Comment
  HiLink calendarCppOut2	calendarCppOut
  HiLink calendarCppSkip	calendarCppOut
  HiLink calendarIncluded	String
  HiLink calendarInclude	Include
  HiLink calendarDefine	Macro
  HiLink calendarPreProc	PreProc
  HiLink calendarKeyword	Keyword
  HiLink calendarNumber	Number
  HiLink calendarMonth	String
  HiLink calendarWeekday	String
  HiLink calendarWeekdayMod	Special
  HiLink calendarTime		Number
  HiLink calendarVariable	Identifier

  delcommand HiLink
endif

let b:current_syntax = "calendar"

" vim: set sts=2 sw=2:
