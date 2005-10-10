" Vim syntax file
" Language:	HMTL Tidy configuration file ( /etc/tidyrc ~/.tidyrc )
" Maintainer:	Doug Kearns <djkea2@gus.gscit.monash.edu.au>
" URL:		http://gus.gscit.monash.edu.au/~djkea2/vim/syntax/tidy.vim
" Last Change:	2005 Oct 06

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

if version < 600
  set iskeyword=@,48-57,-
else
  setlocal iskeyword=@,48-57,-
endif

syn match	tidyComment		"^\s*//.*$" contains=tidyTodo
syn match	tidyComment		"^\s*#.*$"  contains=tidyTodo
syn keyword	tidyTodo		TODO NOTE FIXME XXX contained

syn match	tidyAssignment		"^[a-z0-9-]\+:\s*.*$" contains=tidyOption,@tidyValue,tidyDelimiter
syn match	tidyDelimiter		":" contained

syn match	tidyNewTagAssignment	"^new-\l\+-tags:\s*.*$" contains=tidyNewTagOption,tidyNewTagDelimiter,tidyNewTagValue,tidyDelimiter
syn match	tidyNewTagDelimiter	"," contained
syn match	tidyNewTagValue		"\<\w\+\>" contained

syn case ignore
syn keyword	tidyBoolean		t[rue] f[alse] y[es] n[o] contained
syn case match
syn match	tidyDoctype		"\<omit\|auto\|strict\|loose\|transitional\|user\>" contained
" NOTE: use match rather than keyword here so that tidyEncoding 'raw' does not
"       always have precedence over tidyOption 'raw'
syn match	tidyEncoding		"\<\(ascii\|latin0\|latin1\|raw\|utf8\|iso2022\|mac\|utf16le\|utf16be\|utf16\|win1252\|ibm858\|big5\|shiftjis\)\>" contained
syn match	tidyNewline		"\<\(LF\|CRLF\|CR\)\>"
syn match	tidyNumber		"\<\d\+\>" contained
syn match	tidyRepeat		"\<keep-first\|keep-last\>" contained
syn region	tidyString		start=+"+ skip=+\\\\\|\\"+ end=+"+ contained oneline
syn region	tidyString		start=+'+ skip=+\\\\\|\\'+ end=+'+ contained oneline
syn cluster	tidyValue		contains=tidyBoolean,tidyDoctype,tidyEncoding,tidyNewline,tidyNumber,tidyRepeat,tidyString

syn match	tidyOption		"^accessibility-check"		contained
syn match	tidyOption		"^add-xml-decl"			contained
syn match	tidyOption		"^add-xml-pi"			contained
syn match	tidyOption		"^add-xml-space"		contained
syn match	tidyOption		"^alt-text"			contained
syn match	tidyOption		"^ascii-chars"			contained
syn match	tidyOption		"^assume-xml-procins"		contained
syn match	tidyOption		"^bare"				contained
syn match	tidyOption		"^break-before-br"		contained
syn match	tidyOption		"^char-encoding"		contained
syn match	tidyOption		"^clean"			contained
syn match	tidyOption		"^css-prefix"			contained
syn match	tidyOption		"^doctype"			contained
syn match	tidyOption		"^doctype-mode"			contained
syn match	tidyOption		"^drop-empty-paras"		contained
syn match	tidyOption		"^drop-font-tags"		contained
syn match	tidyOption		"^drop-proprietary-attributes"	contained
syn match	tidyOption		"^enclose-block-text"		contained
syn match	tidyOption		"^enclose-text"			contained
syn match	tidyOption		"^error-file"			contained
syn match	tidyOption		"^escape-cdata"			contained
syn match	tidyOption		"^fix-backslash"		contained
syn match	tidyOption		"^fix-bad-comments"		contained
syn match	tidyOption		"^fix-uri"			contained
syn match	tidyOption		"^force-output"			contained
syn match	tidyOption		"^gnu-emacs"			contained
syn match	tidyOption		"^gnu-emacs-file"		contained
syn match	tidyOption		"^hide-comments"		contained
syn match	tidyOption		"^hide-endtags"			contained
syn match	tidyOption		"^indent"			contained
syn match	tidyOption		"^indent-attributes"		contained
syn match	tidyOption		"^indent-cdata"			contained
syn match	tidyOption		"^indent-spaces"		contained
syn match	tidyOption		"^input-encoding"		contained
syn match	tidyOption		"^input-xml"			contained
syn match	tidyOption		"^join-classes"			contained
syn match	tidyOption		"^join-styles"			contained
syn match	tidyOption		"^keep-time"			contained
syn match	tidyOption		"^language"			contained
syn match	tidyOption		"^literal-attributes"		contained
syn match	tidyOption		"^logical-emphasis"		contained
syn match	tidyOption		"^lower-literals"		contained
syn match	tidyOption		"^markup"			contained
syn match	tidyOption		"^merge-divs"			contained
syn match	tidyOption		"^ncr"				contained
syn match	tidyOption		"^newline"			contained
syn match	tidyOption		"^numeric-entities"		contained
syn match	tidyOption		"^output-bom"			contained
syn match	tidyOption		"^output-encoding"		contained
syn match	tidyOption		"^output-file"			contained
syn match	tidyOption		"^output-html"			contained
syn match	tidyOption		"^output-xhtml"			contained
syn match	tidyOption		"^output-xml"			contained
syn match	tidyOption		"^punctuation-wrap"		contained
syn match	tidyOption		"^quiet"			contained
syn match	tidyOption		"^quote-ampersand"		contained
syn match	tidyOption		"^quote-marks"			contained
syn match	tidyOption		"^quote-nbsp"			contained
syn match	tidyOption		"^raw"				contained
syn match	tidyOption		"^repeated-attributes"		contained
syn match	tidyOption		"^replace-color"		contained
syn match	tidyOption		"^show-body-only"		contained
syn match	tidyOption		"^show-errors"			contained
syn match	tidyOption		"^show-warnings"		contained
syn match	tidyOption		"^slide-style"			contained
syn match	tidyOption		"^split"			contained
syn match	tidyOption		"^tab-size"			contained
syn match	tidyOption		"^tidy-mark"			contained
syn match	tidyOption		"^uppercase-attributes"		contained
syn match	tidyOption		"^uppercase-tags"		contained
syn match	tidyOption		"^word-2000"			contained
syn match	tidyOption		"^wrap"				contained
syn match	tidyOption		"^wrap-asp"			contained
syn match	tidyOption		"^wrap-attributes"		contained
syn match	tidyOption		"^wrap-jste"			contained
syn match	tidyOption		"^wrap-php"			contained
syn match	tidyOption		"^wrap-script-literals"		contained
syn match	tidyOption		"^wrap-sections"		contained
syn match	tidyOption		"^write-back"			contained
syn match	tidyOption		"^vertical-space"		contained
syn match	tidyNewTagOption	"^new-blocklevel-tags"		contained
syn match	tidyNewTagOption	"^new-empty-tags"		contained
syn match	tidyNewTagOption	"^new-inline-tags"		contained
syn match	tidyNewTagOption	"^new-pre-tags"			contained

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_tidy_syn_inits")
  if version < 508
    let did_tidy_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink tidyBoolean		Boolean
  HiLink tidyComment		Comment
  HiLink tidyDelimiter		Special
  HiLink tidyDoctype		Constant
  HiLink tidyEncoding		Constant
  HiLink tidyNewline		Constant
  HiLink tidyNewTagDelimiter	Special
  HiLink tidyNewTagOption	Identifier
  HiLink tidyNewTagValue	Constant
  HiLink tidyNumber		Number
  HiLink tidyOption		Identifier
  HiLink tidyRepeat		Constant
  HiLink tidyString		String
  HiLink tidyTodo		Todo

  delcommand HiLink
endif

let b:current_syntax = "tidy"

" vim: ts=8
