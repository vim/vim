" Vim syntax file
" Language:    Debian changelog files
" Maintainer:  Debian Vim Maintainers <pkg-vim-maintainers@lists.alioth.debian.org>
" Former Maintainer: Wichert Akkerman <wakkerma@debian.org>
" Last Change: $LastChangedDate: 2006-04-16 21:50:31 -0400 (dom, 16 apr 2006) $
" URL: http://svn.debian.org/wsvn/pkg-vim/trunk/runtime/syntax/debchangelog.vim?op=file&rev=0&sc=0

" Standard syntax initialization
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Case doesn't matter for us
syn case ignore

" Define some common expressions we can use later on
syn match debchangelogName	contained "^[[:alpha:]][[:alnum:].+-]\+ "
syn match debchangelogUrgency	contained "; urgency=\(low\|medium\|high\|critical\|emergency\)\( \S.*\)\="
syn match debchangelogTarget	contained "\( stable\| frozen\| unstable\| testing-proposed-updates\| experimental\| sarge-backports\| sarge-volatile\| stable-security\| testing-security\)\+"
syn match debchangelogVersion	contained "(.\{-})"
syn match debchangelogCloses	contained "closes:\s*\(bug\)\=#\=\s\=\d\+\(,\s*\(bug\)\=#\=\s\=\d\+\)*"
syn match debchangelogEmail	contained "[_=[:alnum:].+-]\+@[[:alnum:]./\-]\+"
syn match debchangelogEmail	contained "<.\{-}>"

" Define the entries that make up the changelog
syn region debchangelogHeader start="^[^ ]" end="$" contains=debchangelogName,debchangelogUrgency,debchangelogTarget,debchangelogVersion oneline
syn region debchangelogFooter start="^ [^ ]" end="$" contains=debchangelogEmail oneline
syn region debchangelogEntry start="^  " end="$" contains=debchangelogCloses oneline

" Associate our matches and regions with pretty colours
if version >= 508 || !exists("did_debchangelog_syn_inits")
  if version < 508
    let did_debchangelog_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink debchangelogHeader		Error
  HiLink debchangelogFooter		Identifier
  HiLink debchangelogEntry		Normal
  HiLink debchangelogCloses		Statement
  HiLink debchangelogUrgency		Identifier
  HiLink debchangelogName		Comment
  HiLink debchangelogVersion		Identifier
  HiLink debchangelogTarget		Identifier
  HiLink debchangelogEmail		Special

  delcommand HiLink
endif

let b:current_syntax = "debchangelog"

" vim: ts=8 sw=2
