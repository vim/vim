" Vim syntax file
" Language:	Debian changelog files
" Maintainer:	Wichert Akkerman <wakkerma@debian.org>
" Last Change:	30 April 2001

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
syn match debchangelogUrgency	contained "; urgency=\(low\|medium\|high\|critical\)"
syn match debchangelogTarget	contained "\( stable\| frozen\| unstable\| experimental\)\+"
syn match debchangelogVersion	contained "(.\{-})"
syn match debchangelogCloses	contained "closes:\s*\(bug\)\=#\s\=\d\+\(,\s*\(bug\)\=#\s\=\d\+\)*"
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
