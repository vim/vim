" Vim syntax file
" Language:	    CRM114
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/syntax/pcp/crm/
" Latest Revision:  2004-05-22
" arch-tag:	    a3d3eaaf-4700-44ff-b332-f6c42c036883

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Todo
syn keyword crmTodo	    contained TODO FIXME XXX NOTE

" Comments
syn region  crmComment	    matchgroup=crmComment start='#' end='$' end='\\#' contains=crmTodo

" Variables
syn match   crmVariable    ':[*#@]:[^:]\{-1,}:'

" Special Characters
syn match   crmSpecial	    '\\\%(x\x\x\|o\o\o\o\|[]nrtabvf0>)};/\\]\)'

" Statements
syn keyword crmStatement    insert noop accept alius alter classify eval exit
syn keyword crmStatement    fail fault goto hash intersect isolate input learn
syn keyword crmStatement    liaf match output syscall trap union window

" Regexes
syn region   crmRegex	    matchgroup=crmRegex start='/' skip='\\/' end='/' contains=crmVariable

" Labels
syn match   crmLabel	    '^\s*:[[:graph:]]\+:'

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_crm_syn_inits")
  if version < 508
    let did_crm_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink crmTodo	Todo
  HiLink crmComment	Comment
  HiLink crmVariable	Identifier
  HiLink crmSpecial	SpecialChar
  HiLink crmStatement	Statement
  HiLink crmRegex	String
  HiLink crmLabel	Label

  delcommand HiLink
endif

let b:current_syntax = "crm"

" vim: set sts=2 sw=2:
