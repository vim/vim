" Vim compiler file
" Language:	jq
" Maintainer:	Vito C <vito.blog@gmail.com>
" Upstream:	https://github.com/vito-c/jq.vim
" Last Change:	2015 Nov 28

if exists('b:current_compiler')
  finish
endif
let b:current_compiler = 'jq'

let s:save_cpoptions = &cpoptions
set cpoptions&vim

if exists(':CompilerSet') != 2
  command -nargs=* CompilerSet setlocal <args>
endif
if has('unix')
  CompilerSet makeprg=jq\ -f\ %:S\ /dev/null
else
  CompilerSet makeprg=jq\ -f\ %:S\ nul
endif
CompilerSet errorformat=%E%m\ at\ \\<%o\\>\\,\ line\ %l:,
            \%Z,
            \%-G%.%#

let &cpoptions = s:save_cpoptions
unlet s:save_cpoptions
