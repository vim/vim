" Vim syntax file
" Language:	Asterisk config file
" Maintainer:	brc007
" Last Change:	2005 Sep 19
" version 0.2
"
" Additional changes made 2005 Mar 7 by Corydon76
" * CVS priority, including n and s, and new label definitions
" * ENV( and LEN( support
" * Class patterns in extensions now match only the class pattern (instead of to a following expression)
" * anthm's functions are matched
" * Variables now appear in their own colors inside expressions

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

"testing only
syn sync clear
syn sync fromstart


syn keyword	asteriskTodo	TODO contained
syn match	asteriskComment		";.*" contains=asteriskTodo
syn match       asteriskContext         "\[.\{-}\]"
syn match	asteriskExten		"^\s*exten\s*=>"
syn match       asteriskApp             "\s*,\s*\zs[a-zA-Z]*\ze"
" Digits plus oldlabel (newlabel)
syn match       asteriskPriority        ",[[:digit:]]\+\(+[[:alpha:]][[:alnum:]_]*\)\?\(([[:alpha:]][[:alnum:]_]*)\)\?,"
" s or n plus digits (newlabel)
syn match       asteriskPriority        ",[sn]\(+[[:digit:]]\+\)\?\(([[:alpha:]][[:alnum:]_]*)\)\?,"
syn match       asteriskIncludeBad "^\s*#\s*[[:alnum:]]*"
syn match       asteriskInclude		"^\s#\sinclude\s.*"
syn match       asteriskVar             "\${_\{0,2}[[:alpha:]][[:alnum:]_]*\(:[[:digit:]]\+\)\{0,2}}"
syn match       asteriskVarLen          "\${_\{0,2}[[:alpha:]][[:alnum:]_]*(.\{-})}" contains=asteriskVar,asteriskVarLen,asteriskExp
syn match       asteriskExp             "\$\[.\{-}\]" contains=asteriskVar,asteriskVarLen,asteriskExp
syn match       asteriskFunc            "\$([[:alpha:]][[:alnum:]_]*.*)" contains=asteriskVar,asteriskVarLen,asteriskExp

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
:if version >= 508 || !exists("did_conf_syntax_inits")
  if version < 508
    let did_conf_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif


  HiLink	asteriskComment	Comment
" not sure what type this should be, using String for testing.
  HiLink	asteriskExten	String
" same here
  HiLink	asteriskContext		Identifier 
  HiLink        asteriskApplication     Statement
  HiLink        asteriskInclude		Preproc 
  HiLink        asteriskIncludeBad  Error
  HiLink	asteriskPriority	Preproc	
  HiLink        asteriskVar             String
  HiLink        asteriskVarLen          Function
  HiLink        asteriskExp             Type
 delcommand HiLink
endif


let b:current_syntax = "asterisk" 

" vim: ts=8 sw=2
