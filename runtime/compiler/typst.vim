" Vim compiler file
" Compiler: typst
" Maintainer: Kaj Munhoz Arfvidsson <kajarfvidsson@gmail.com>
" Upstream: https://github.com/kaarmu/typst.vim

if exists("current_compiler")
    finish
endif
let current_compiler = g:typst_cmd

let s:save_cpo = &cpo
set cpo&vim

if exists(":CompilerSet") != 2
    command -nargs=* CompilerSet setlocal <args>
endif

" With `--diagnostic-format` we can use the default errorformat
let s:makeprg = [current_compiler, 'compile',
              \  '--diagnostic-format', 'short']

if has('patch-7.4.191')
    call add(s:makeprg, '%:S')
else
    call add(s:makeprg, '%')
endif

" This style of `CompilerSet makeprg` is non-typical.  The reason is that I
" want to avoid a long string of escaped spaces and we can very succinctly
" build makeprg.  You cannot write something like this `CompilerSet
" makeprg=s:makeprg`.
execute 'CompilerSet makeprg=' . join(s:makeprg, '\ ')

let &cpo = s:save_cpo
unlet s:save_cpo
" vim: tabstop=8 shiftwidth=4 softtabstop=4 expandtab
