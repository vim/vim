" ninja build file syntax.
" Language: ninja build file as described at
"           http://martine.github.com/ninja/manual.html
" Version: 1.2
" Last Change: 2012/06/01
" Maintainer: Nicolas Weber <nicolasweber@gmx.de>

" ninja lexer and parser are at
" https://github.com/martine/ninja/blob/master/src/lexer.in.cc
" https://github.com/martine/ninja/blob/master/src/parsers.cc

if exists("b:current_syntax")
  finish
endif

syn case match

syn match ninjaComment /#.*/  contains=@Spell

" Toplevel statements are the ones listed here and
" toplevel variable assignments (ident '=' value).
" lexer.in.cc, ReadToken() and parsers.cc, Parse()
syn match ninjaKeyword "^build\>"
syn match ninjaKeyword "^rule\>"
syn match ninjaKeyword "^default\>"
syn match ninjaKeyword "^include\>"
syn match ninjaKeyword "^subninja\>"

" Both 'build' and 'rule' begin a variable scope that ends
" on the first line without indent. 'rule' allows only a
" limited set of magic variables, 'build' allows general
" let assignments.
" parsers.cc, ParseRule()
syn region ninjaRule start="^rule" end="^\ze\S" contains=ALL transparent
syn keyword ninjaRuleCommand contained command depfile description generator restat

" Strings are parsed as follows:
" lexer.in.cc, ReadEvalString()
" simple_varname = [a-zA-Z0-9_-]+;
" varname = [a-zA-Z0-9_.-]+;
" $$ -> $
" $\n -> line continuation
" '$ ' -> escaped space
" $simple_varname -> variable
" ${varname} -> variable

syn match   ninjaWrapLineOperator "\$$"
syn match   ninjaSimpleVar "\$[a-zA-Z0-9_-]\+"
syn match   ninjaVar       "\${[a-zA-Z0-9_.-]\+}"

" operators are:
" variable assignment =
" rule definition :
" implicit dependency |
" order-only dependency ||
syn match ninjaOperator "\(=\|:\||\|||\)\ze\s"

hi def link ninjaComment Comment
hi def link ninjaKeyword Keyword
hi def link ninjaRuleCommand Statement
hi def link ninjaWrapLineOperator ninjaOperator
hi def link ninjaOperator Operator
hi def link ninjaSimpleVar ninjaVar
hi def link ninjaVar Identifier

let b:current_syntax = "ninja"
