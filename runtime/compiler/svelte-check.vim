if exists("current_compiler") | finish | endif
let current_compiler = "svelte-check"

let s:cpo_save = &cpo
set cpo&vim

CompilerSet makeprg=npx\ svelte-check\ --output\ machine
CompilerSet errorformat=%*\\d\ %t%*\\a\ \"%f\"\ %l:%c\ %m
CompilerSet errorformat+=%-G%.%#

" " Fall-back for versions of svelte-check that don't support --output machine
" CompilerSet makeprg=npx\ svelte-check
" CompilerSet errorformat=%E%f:%l:%c,
" CompilerSet errorformat+=%+ZError\:\ %m,
" CompilerSet errorformat+=%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save
