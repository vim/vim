" Vim syntax file
" Language:	crontab 2.3.3
" Maintainer:	John Hoelzel johnh51@users.sourceforge.net
" Last change:	Mon Jun  9 2003
" Filenames:    /tmp/crontab.* used by "crontab -e"
" URL:		http://johnh51.get.to/vim/syntax/crontab.vim
"
" crontab line format:
" Minutes   Hours   Days   Months   Days_of_Week   Commands # comments

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syntax match  crontabMin     "\_^[0-9\-\/\,\.]\{}\>\|\*"  nextgroup=crontabHr   skipwhite
syntax match  crontabHr       "\<[0-9\-\/\,\.]\{}\>\|\*"  nextgroup=crontabDay  skipwhite contained
syntax match  crontabDay      "\<[0-9\-\/\,\.]\{}\>\|\*"  nextgroup=crontabMnth skipwhite contained

syntax match  crontabMnth  "\<[a-z0-9\-\/\,\.]\{}\>\|\*"  nextgroup=crontabDow  skipwhite contained
syntax keyword crontabMnth12 contained   jan feb mar apr may jun jul aug sep oct nov dec

syntax match  crontabDow   "\<[a-z0-9\-\/\,\.]\{}\>\|\*"  nextgroup=crontabCmd  skipwhite contained
syntax keyword crontabDow7   contained    sun mon tue wed thu fri sat

"  syntax region crontabCmd  start="\<[a-z0-9\/\(]" end="$" nextgroup=crontabCmnt skipwhite contained contains=crontabCmnt keepend

syntax region crontabCmd  start="\S" end="$" nextgroup=crontabCmnt skipwhite contained contains=crontabCmnt keepend
syntax match  crontabCmnt /#.*/

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_crontab_syn_inits")
  if version < 508
    let did_crontab_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink crontabMin		Number
  HiLink crontabHr		PreProc
  HiLink crontabDay		Type

  HiLink crontabMnth		Number
  HiLink crontabMnth12		Number
  HiLink crontabMnthS		Number
  HiLink crontabMnthN		Number

  HiLink crontabDow		PreProc
  HiLink crontabDow7		PreProc
  HiLink crontabDowS		PreProc
  HiLink crontabDowN		PreProc

" comment out next line for to suppress unix commands coloring.
  HiLink crontabCmd		Type

  HiLink crontabCmnt		Comment

  delcommand HiLink
endif

let b:current_syntax = "crontab"

" vim: ts=8
