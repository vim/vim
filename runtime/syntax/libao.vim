" Vim syntax file
" Language:	    libao configuration file
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/syntax/pcp/libao/
" Latest Revision:  2004-05-22
" arch-tag:	    4ddef0a8-6817-4555-a5a1-0be82094053d

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Todo
syn keyword libaoTodo	    contained TODO FIXME XXX NOTE

" Comments
syn region  libaoComment    matchgroup=libaoComment start='^\s*#' end='$' contains=libaoTodo

" Keywords
syn keyword libaoKeyword    default_driver

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_libao_syn_inits")
  if version < 508
    let did_libao_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
    command -nargs=+ HiDef hi <args>
  else
    command -nargs=+ HiLink hi def link <args>
    command -nargs=+ HiDef hi def <args>
  endif

  HiLink libaoTodo	Todo
  HiLink libaoComment	Comment
  HiLink libaoKeyword	Keyword

  delcommand HiLink
  delcommand HiDef
endif

let b:current_syntax = "libao"

" vim: set sts=2 sw=2:
