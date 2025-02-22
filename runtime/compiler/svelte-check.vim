if exists("current_compiler") | finish | endif
let current_compiler = "svelte-check"

let s:cpo_save = &cpo
set cpo&vim

CompilerSet makeprg=npx\ svelte-check
CompilerSet errorformat=%E%f:%l:%c,
CompilerSet errorformat+=%+ZError\:\ %m,
CompilerSet errorformat+=%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save
