" Vim syntax file
" Language:	    ALSA configuration file
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/
" Latest Revision:  2004-09-10
" arch-tag:	    3e06fe53-28d5-44a1-871d-279f22e7aed4

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" todo
syn keyword alsoconfTodo      contained FIXME TODO XXX NOTE

" comments
syn region  alsaconfComment   matchgroup=alsaconfComment start="#" end="$"
			      \ contains=alsaconfTodo

" special characters
syn match   alsaconfSpecialChar	contained "\\[ntvbrf]"
syn match   alsaconfSpecialChar	contained "\\\o\+"

" strings
syn region  alsaconfString    matchgroup=alsaconfString start=+"+ skip=+\\$+
			      \ end=+"+ end=+$+ contains=alsaconfSpecialChar

" preprocessor special
syn match   alsaconfSpecial   contained "confdir:"

" preprocessor
syn region  alsaconfPreProc   matchgroup=alsaconfPreProc start="<" end=">"
			      \ contains=alsaconfSpecial

" modes
syn match   alsaconfMode      "[+?!-]"

" keywords
syn keyword alsaconfKeyword   card default device errors files func strings
syn keyword alsaconfKeyword   subdevice type vars

" variables
syn match   alsaconfVariables "@\(hooks\|func\|args\)"

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_alsaconf_syn_inits")
  if version < 508
    let did_dircolors_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink alsoconfTodo	      Todo
  HiLink alsaconfComment      Comment
  HiLink alsaconfSpecialChar  SpecialChar
  HiLink alsaconfString	      String
  HiLink alsaconfSpecial      Special
  HiLink alsaconfPreProc      PreProc
  HiLink alsaconfMode	      Special
  HiLink alsaconfKeyword      Keyword
  HiLink alsaconfVariables    Identifier

  delcommand HiLink
endif

let b:current_syntax = "alsaconf"

" vim: set sts=2 sw=2:
