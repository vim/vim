" Vim syntax file
" Language:	    indent RC File
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/syntax/pcp/indent/
" Latest Revision:  2004-05-22
" arch-tag:	    23c11190-79fa-4493-9fc5-36435402a20d
" TODO: is the deny-all (a la lilo.vim nice or no?)...
"	irritating to be wrong to the last char...
"	would be sweet if right until one char fails

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Set iskeyword since we need `-' (and potentially others) in keywords.
" For version 5.x: Set it globally
" For version 6.x: Set it locally
if version >= 600
  command -nargs=1 SetIsk setlocal iskeyword=<args>
else
  command -nargs=1 SetIsk set iskeyword=<args>
endif
SetIsk 48-57,65-90,97-122,-,_
delcommand SetIsk

" errors
syn match   indentError "\S\+"

" todo
syn keyword indentTodo contained TODO FIXME XXX NOTE

" comments
syn region  indentComment matchgroup=indentComment start="/\*" end="\*/" contains=indentTodo

" keywords (command-line switches)
syn match   indentOptions "\<--\(no-\)\=blank-\(before-sizeof\|Bill-Shannon\|lines-\(after-\(commas\|declarations\|procedures\)\|before-block-comments\)\)\>"
syn match   indentOptions "\<--brace-indent\s*\d\+\>"
syn match   indentOptions "\<--braces-\(after\|on\)-\(if\|struct-decl\)-line\>"
syn match   indentOptions "\<--break-\(\(after\|before\)-boolean-operator\|function-decl-args\)\>"
syn match   indentOptions "\<--\(case\(-brace\)\=\|comment\|continuation\|declaration\|line-comments\|parameter\|paren\|struct-brace\)-indentation\s*\d\+\>"
syn match   indentOptions "\<--\(no-\)\=comment-delimiters-on-blank-lines\>"
syn match   indentOptions "\<--\(dont-\)\=cuddle-\(do-while\|else\)\>"
syn match   indentOptions "\<--\(declaration-comment\|else-endif\)-column\s*\d\+\>"
syn match   indentOptions "\<--dont-break-\(function-decl-args\|procedure-type\)\>"
syn match   indentOptions "\<--\(dont-\)\=\(format\(-first-column\)\=\|star\)-comments\>"
syn match   indentOptions "\<--\(honour\|ignore\)-newlines\>"
syn match   indentOptions "\<--\(indent-level\|\(comment-\)\=line-length\)\s*\d\+\>"
syn match   indentOptions "\<--\(leave\|remove\)-preprocessor-space\>"
"not 100%, since casts\= should always be cast if no- isn't given
syn match   indentOptions "\<--\(no-\)\=space-after-\(parentheses\|casts\=\|for\|if\|while\)\>"
syn match   indentOptions "\<--\(dont-\)\=space-special-semicolon\>"
syn match   indentOptions "\<--\(leave\|swallow\)-optional-blank-lines\>"
syn match   indentOptions "\<--tab-size\s*\d\+\>"
syn match   indentOptions "\<--\(no\|use\)-tabs\>"
syn keyword indentOptions --gnu-style --ignore-profile --k-and-r-style --original
syn keyword indentOptions --preserve-mtime --no-verbosity --verbose --output-file
syn keyword indentOptions --no-parameter-indentation --procnames-start-lines
syn keyword indentOptions --standard-output --start-left-side-of-comments
syn keyword indentOptions --space-after-procedure-calls
" this also here since the gnu indent fellas aren't consistent. (ever read
" the code to `indent'? you'll know what i mean if you have)
syn match   indentOptions "\<-\(bli\|cbi\|cd\|ci\|cli\|c\|cp\|di\|d\|i\|ip\|l\|lc\|pi\|sbi\|ts\)\s*\d\+\>"
syn match   indentOptions "\<-T\s\+\w\+\>"
syn keyword indentOptions --format-all-comments --continue-at-parentheses --dont-line-up-parentheses
syn keyword indentOptions --no-space-after-function-call-names
syn keyword indentOptions -bad -bap -bbb -bbo -bc -bfda -bl -br -bs -nbs -cdb -cdw -ce -cs -dce -fc1 -fca
syn keyword indentOptions -gnu -hnl -kr -lp -lps -nbad -nbap -nbbb -nbbo -nbc -nbfda -ncdb -ncdw -nprs
syn keyword indentOptions -nce -ncs -nfc1 -nfca -nhnl -nip -nlp -nlps -npcs -npmt -npro -npsl -nsaf -nsai
syn keyword indentOptions -nsaw -nsc -nsob -nss -nv -o -orig -pcs -pmt -prs -psl -saf -sai -saw -sc
syn keyword indentOptions -sob -ss -st -v -version -bls -brs -ut -nut

if exists("indent_minlines")
    let b:indent_minlines = indent_minlines
else
    let b:indent_minlines = 50
endif
exec "syn sync minlines=" . b:indent_minlines

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_indent_syn_inits")
  if version < 508
    let did_indent_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink indentError	Error
  HiLink indentComment	Comment
  HiLink indentTodo	Todo
  HiLink indentOptions	Keyword
  delcommand HiLink
endif

let b:current_syntax = "indent"

" vim: set sts=2 sw=2:
