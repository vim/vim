" Vim syntax file
" Author: Trevor Hemsley <themsley@voiceflex.com>
" Author: Dan Frincu <df.cluster@gmail.com>
" Language:     pcmk
" Filenames:    *.pcmk

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

set modeline

" setlocal iskeyword+=-

" Errors
syn match    pcmkParErr     ")"
syn match    pcmkBrackErr   "]"
syn match    pcmkBraceErr   "}"

" Enclosing delimiters
syn region   pcmkEncl transparent matchgroup=pcmkParEncl   start="("  matchgroup=pcmkParEncl   end=")"  contains=ALLBUT,pcmkParErr
syn region   pcmkEncl transparent matchgroup=pcmkBrackEncl start="\[" matchgroup=pcmkBrackEncl end="\]" contains=ALLBUT,pcmkBrackErr
syn region   pcmkEncl transparent matchgroup=pcmkBraceEncl start="{"  matchgroup=pcmkBraceEncl end="}"  contains=ALLBUT,pcmkBraceErr

" Comments
syn region   pcmkComment start="//"  end="$"   contains=pcmkComment,pcmkTodo
syn region   pcmkComment start="/\*" end="\*/" contains=pcmkComment,pcmkTodo
syn keyword  pcmkTodo    contained TODO FIXME XXX

" Strings
syn region   pcmkString    start=+"+ skip=+\\\\\|\\"+ end=+"+

" General keywords
syn keyword  pcmkKeyword  node primitive property rsc_defaults op_defaults group clone nextgroup=pcmkName skipwhite
syn keyword  pcmkKey2     location nextgroup=pcmkResource skipwhite
syn keyword  pcmkKey3     colocation order nextgroup=pcmkName3 skipwhite
syn match    pcmkResource /\<\f\+\>/ nextgroup=pcmkName2 skipwhite
syn match    pcmkName     /\<\f\+\>/
syn match    pcmkName2    /\<\f\+\>/ nextgroup=pcmkPrio skipwhite
syn match    pcmkName3    /\<\f\+\>/ nextgroup=pcmkPrio skipwhite
syn match    pcmkPrio     /\<\w\+\>/
syn match    pcmkNumbers  /[[:digit:]]\+\:/
syn match    pcmkInf      /inf\:/

" Graph attributes
syn keyword  pcmkType attributes params op meta
syn keyword  pcmkTag monitor start stop migrate_from migrate_to notify demote promote Master Slave

" Special chars
"syn match    pcmkKeyChar  "="
syn match    pcmkKeyChar  ";"
syn match    pcmkKeyChar  "->"
syn match    pcmkKeyChar  "\$"
"syn match    pcmkKeyChar  "\\"
syn match    pcmkKeyChar  ":"
syn match    pcmkKeyChar  "-"
syn match    pcmkKeyChar  "+"

" Identifier
syn match    pcmkIdentifier /\<\w\+\>/
syn match    pcmkKeyword    "^ms\s*" nextgroup=pcmkName skipwhite

" Synchronization
syn sync minlines=50
syn sync maxlines=500

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_pcmk_syntax_inits")
  if version < 508
    let did_pcmk_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink pcmkParErr      Error
  HiLink pcmkBraceErr    Error
  HiLink pcmkBrackErr    Error

  HiLink pcmkComment     Comment
  HiLink pcmkTodo        Todo

  HiLink pcmkParEncl     Keyword
  HiLink pcmkBrackEncl   Keyword
  HiLink pcmkBraceEncl   Keyword

  HiLink pcmkKeyword     Keyword
  HiLink pcmkKey2        Keyword
  HiLink pcmkKey3        Keyword
  HiLink pcmkType        Keyword
  HiLink pcmkKeyChar     Keyword

"  hi Normal ctermfg=yellow ctermbg=NONE cterm=NONE
  HiLink pcmkString     String
  HiLink pcmkIdentifier Identifier
  HiLink pcmkTag        Tag
  HiLink pcmkName       Type
  HiLink pcmkName2      Tag
  HiLink pcmkName3      Type
  HiLink pcmkResource   Type
  HiLink pcmkPrio       Number
  HiLink pcmkNumbers    String
  HiLink pcmkInf        String

  delcommand HiLink
endif

let b:current_syntax = "pcmk"
