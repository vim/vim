" Vim syntax file
" Language:	    GNU Arch inventory file.
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/syntax/pcp/arch/
" Latest Revision:  2004-05-22
" arch-tag:	    529d60c4-53d8-4d3a-80d6-54ada86d9932

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

" Todo
syn keyword archTodo	  TODO FIXME XXX NOTE

" Comment
syn region  archComment  matchgroup=archComment start='^\%(#\|\s\)' end='$' contains=archTodo

" Keywords
syn keyword archKeyword  implicit tagline explicit names
syn keyword archKeyword  untagged-source
syn keyword archKeyword  exclude junk backup precious unrecognized source skipwhite nextgroup=archRegex

" Regexes
syn match   archRegex    contained '\s*\zs.*'

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_arch_syn_inits")
  if version < 508
    let did_arch_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink archTodo     Todo
  HiLink archComment  Comment
  HiLink archKeyword  Keyword
  HiLink archRegex    String

  delcommand HiLink
endif

let b:current_syntax = "arch"

" vim: set sts=2 sw=2:
