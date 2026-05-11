" Vim compiler file
" Compiler: Zig Compiler (zig cc)
" Last Change: 2025 Nov 16 by The Vim Project (set errorformat)

if exists('current_compiler')
  finish
endif
runtime compiler/zig.vim
let current_compiler = 'zig_cc'

let s:save_cpo = &cpo
set cpo&vim

CompilerSet makeprg=zig\ cc\ \%:S\ \$*

let &cpo = s:save_cpo
unlet s:save_cpo
" vim: tabstop=8 shiftwidth=4 softtabstop=4 expandtab
