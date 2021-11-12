" Vim syntax file
" Language:        RouterOS scripts
" Maintainer:      zainin
" Original Author: ndbjorne @ MikroTik forums
" Last Change:     2017-07-26

" quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

syn case ignore

syn iskeyword @,48-57,-

" comments
syn match     rscComment      /^\s*#.*/

" options submenus: /interface ether1 etc
syn match     rscSubMenu      "\([a-z]\)\@<!/[a-zA-Z0-9-]*"

" variables are matched by looking at strings ending with "=", e.g. var=
syn match     rscVariable     "[a-zA-Z0-9-/]*\(=\)\@="
syn match     rscVariable     "$[a-zA-Z0-9-]*"

" colored for clarity
syn match     rscDelimiter    "[,=]"
" match slash in CIDR notation (1.2.3.4/24, 2001:db8::/48, ::1/128)
syn match     rscDelimiter    "\(\x\|:\)\@<=\/\(\d\)\@="
" dash in IP ranges
syn match     rscDelimiter    "\(\x\|:\)\@<=-\(\x\|:\)\@="

" match service names after "set", like in original routeros syntax
syn match     rscService      "\(set\)\@<=\s\(api-ssl\|api\|dns\|ftp\|http\|https\|pim\|ntp\|smb\|ssh\|telnet\|winbox\|www\|www-ssl\)"

" colors various interfaces
syn match     rscInterface    "bridge\d\+\|ether\d\+\|wlan\d\+\|pppoe-\(out\|in\)\d\+"

syn keyword   rscBoolean      yes no true false

syn keyword   rscConditional  if

" operators
syn match     rscOperator     " [\+\-\*\<\>\=\!\~\^\&\.\,] "
syn match     rscOperator     "[\<\>\!]="
syn match     rscOperator     "\(<<\|>>\)"
syn match     rscOperator     "[\+\-]\(\d\)\@="

" commands
syn keyword   rscCommands     beep delay put len typeof pick log time set find environment
syn keyword   rscCommands     terminal error parse resolve toarray tobool toid toip toip6
syn keyword   rscCommands     tonum tostr totime add remove enable disable where get print
syn keyword   rscCommands     export edit find append as-value brief detail count-only file
syn keyword   rscCommands     follow follow-only from interval terse value-list without-paging
syn keyword   rscCommands     return

" variable types
syn keyword   rscType         global local

" loop keywords
syn keyword   rscRepeat       do while for foreach

syn match     rscSpecial      "[():\[\]{|}]"

syn region    rscString       start=+L\="+ skip=+\\\\\|\\"+ end=+"+ contains=rscSpecial

hi link rscComment              Comment
hi link rscSubMenu              Function
hi link rscVariable             Identifier
hi link rscDelimiter            Operator
hi link rscService              Type
hi link rscInterface            Type
hi link rscBoolean              Boolean
hi link rscConditional          Conditional
hi link rscOperator             Operator
hi link rscCommands             Operator
hi link rscType                 Type
hi link rscRepeat               Repeat
hi link rscSpecial              Delimiter
hi link rscString               String

let b:current_syntax = "rsc"
