" Vim syntax file
" Language:	    CVS RC File
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/syntax/pcp/cvsrc/
" Latest Revision:  2004-05-06
" arch-tag:	    1910f2a8-66f4-4dde-9d1a-297566934535

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" strings
syn region  cvsrcString	    start=+"+ skip=+\\\\\|\\\\"+ end=+"\|$+
syn region  cvsrcString	    start=+'+ skip=+\\\\\|\\\\'+ end=+'\|$+

" numbers
syn match   cvsrcNumber	    "\<\d\+\>"

" commands
syn match   cvsrcBegin	    "^" nextgroup=cvsrcCommand skipwhite

syn region  cvsrcCommand    contained transparent matchgroup=cvsrcCommand start="add\|admin\|checkout\|commit\|cvs\|diff\|export\|history\|import\|init\|log\|rdiff\|release\|remove\|rtag\|status\|tag\|update" end="$" contains=cvsrcOption,cvsrcString,cvsrcNumber keepend

" options
syn match   cvsrcOption	    "-\a\+"

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_cvsrc_syn_inits")
  if version < 508
    let did_cvsrc_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink cvsrcString	String
  HiLink cvsrcNumber	Number
  HiLink cvsrcCommand	Keyword
  HiLink cvsrcOption	Identifier
  delcommand HiLink
endif

let b:current_syntax = "cvsrc"

" vim: set sts=2 sw=2:
