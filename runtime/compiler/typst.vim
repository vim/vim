" Vim compiler file
" Language:    Typst
" Previous Maintainer:  Gregory Anders
" Maintainer:  Luca Saccarola <github.e41mv@aleeas.com>
" Last Change: 2025 Mar 11
" Based on:    https://github.com/kaarmu/typst.vim

if exists('current_compiler')
    finish
endif
let current_compiler = get(g:, 'typst_cmd', 'typst')

" With `--diagnostic-format` we can use the default errorformat
let s:makeprg = [current_compiler, 'compile', '--diagnostic-format', 'short', '%:S']

" CompilerSet makeprg=typst
execute 'CompilerSet makeprg=' . join(s:makeprg, '\ ')
