" Vim syntax file
" Language:	JavaScript
" Maintainer:	Claudio Fleiner <claudio@fleiner.com>
" URL:		http://www.fleiner.com/vim/syntax/javascript.vim
" Last Change:	2004 May 16

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
" tuning parameters:
" unlet javaScript_fold

if !exists("main_syntax")
  if version < 600
    syntax clear
  elseif exists("b:current_syntax")
    finish
  endif
  let main_syntax = 'javascript'
endif

" Drop fold if it set but vim doesn't support it.
if version < 600 && exists("javaScript_fold")
  unlet javaScript_fold
endif

syn case ignore


syn keyword javaScriptCommentTodo      TODO FIXME XXX TBD contained
syn match   javaScriptLineComment      "\/\/.*$" contains=javaScriptCommentTodo
syn match   javaScriptCommentSkip      "^[ \t]*\*\($\|[ \t]\+\)"
syn region  javaScriptComment	       start="/\*"  end="\*/" contains=javaScriptCommentTodo
syn match   javaScriptSpecial	       "\\\d\d\d\|\\."
syn region  javaScriptStringD	       start=+"+  skip=+\\\\\|\\"+  end=+"+  contains=javaScriptSpecial,@htmlPreproc
syn region  javaScriptStringS	       start=+'+  skip=+\\\\\|\\'+  end=+'+  contains=javaScriptSpecial,@htmlPreproc
syn match   javaScriptSpecialCharacter "'\\.'"
syn match   javaScriptNumber	       "-\=\<\d\+L\=\>\|0[xX][0-9a-fA-F]\+\>"
syn region  javaScriptRegexpString     start=+/+ skip=+\\\\\|\\/+ end=+/[gi]\?\s*$+ end=+/[gi]\?\s*[;,)]+me=e-1 contains=@htmlPreproc oneline
syn keyword javaScriptConditional      if else
syn keyword javaScriptRepeat	       while for
syn keyword javaScriptBranch	       break continue switch case default
syn keyword javaScriptOperator	       new in
syn keyword javaScriptType	       this var const
syn keyword javaScriptStatement        return with
syn keyword javaScriptBoolean	       true false

if exists("javaScript_fold")
    syn match	javaScriptFunction      "\<function\>"
    syn region	javaScriptFunctionFold	start="\<function\>.*[^};]$" end="^\z1}.*$" transparent fold keepend

    syn sync match javaScriptSync	grouphere javaScriptFunctionFold "\<function\>"
    syn sync match javaScriptSync	grouphere NONE "^}"

    setlocal foldmethod=syntax
    setlocal foldtext=getline(v:foldstart)
else
    syn keyword	javaScriptFunction      function
    syn match	javaScriptBraces	   "[{}]"
endif

syn sync fromstart
syn sync maxlines=100

" catch errors caused by wrong parenthesis
syn region  javaScriptParen       transparent start="(" end=")" contains=javaScriptParen,javaScriptComment,javaScriptSpecial,javaScriptStringD,javaScriptStringS,javaScriptSpecialCharacter,javaScriptNumber,javaScriptRegexpString,javaScriptBoolean,javaScriptBraces
syn match   javaScrParenError  ")"

if main_syntax == "javascript"
  syn sync ccomment javaScriptComment
endif

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_javascript_syn_inits")
  if version < 508
    let did_javascript_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif
  HiLink javaScriptComment	     Comment
  HiLink javaScriptLineComment	     Comment
  HiLink javaScriptCommentTodo	     Todo
  HiLink javaScriptSpecial	     Special
  HiLink javaScriptStringS	     String
  HiLink javaScriptStringD	     String
  HiLink javaScriptCharacter	     Character
  HiLink javaScriptSpecialCharacter  javaScriptSpecial
  HiLink javaScriptNumber	     javaScriptValue
  HiLink javaScriptConditional	     Conditional
  HiLink javaScriptRepeat	     Repeat
  HiLink javaScriptBranch	     Conditional
  HiLink javaScriptOperator	     Operator
  HiLink javaScriptType		     Type
  HiLink javaScriptStatement	     Statement
  HiLink javaScriptFunction	     Function
  HiLink javaScriptBraces	     Function
  HiLink javaScriptError	     Error
  HiLink javaScrParenError	     javaScriptError
  HiLink javaScriptBoolean	     Boolean
  HiLink javaScriptRegexpString      String
  delcommand HiLink
endif

let b:current_syntax = "javascript"
if main_syntax == 'javascript'
  unlet main_syntax
endif

" vim: ts=8
