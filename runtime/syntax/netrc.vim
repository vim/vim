" Vim syntax file
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/
" Latest Revision:  2004-12-16
" arch-tag:	    4f6ecb37-d10c-4eca-add0-77991559414a

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Keywords
syn keyword netrcKeyword      machine password nextgroup=netrcName skipwhite skipnl
syn keyword netrcKeyword      login nextgroup=netrcName,netrcSpecial skipwhite skipnl
syn keyword netrcKeyword      default
syn keyword netrcKeyword      macdef nextgroup=netrcInit,netrcMacroName skipwhite skipnl
syn region  netrcMacro	      contained start='.' end='^$'

" Names
syn match   netrcName	      contained display '\S\+'
syn match   netrcName	      contained display '"[^\\"]*\(\\.[^\\"]*\)*'
syn match   netrcMacroName    contained display '\S\+' nextgroup=netrcMacro skipwhite skipnl
syn match   netrcMacroName    contained display '"[^\\"]*\(\\.[^\\"]*\)*' nextgroup=netrcMacro skipwhite skipnl

" Special
syn keyword netrcSpecial      contained anonymous
syn match   netrcInit	      contained '\<init$' nextgroup=netrcMacro skipwhite skipnl

syn sync fromstart

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_netrc_syn_inits")
  if version < 508
    let did_netrc_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink netrcKeyword	Keyword
  HiLink netrcMacro	PreProc
  HiLink netrcName	String
  HiLink netrcMacroName	String
  HiLink netrcSpecial	Special
  HiLink netrcInit	Special

  delcommand HiLink
endif

let b:current_syntax = "netrc"

" vim: set sts=2 sw=2:
