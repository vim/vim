" Compiler: GNU Fortran Compiler
" Last Change: 2018 Mar 07
" License: Same as Vim

if exists('current_compiler')
    finish
endif
let current_compiler = 'gfortran'
let s:keepcpo= &cpo
set cpo&vim

if exists(":CompilerSet") != 2		" older Vim always used :setlocal
  command -nargs=* CompilerSet setlocal <args>
endif

CompilerSet errorformat=
            \%A%f:%l:%c:,
            \%A%f:%l.%c:,
            \%-Z%trror:\ %m,
            \%-Z%tarning:\ %m,
            \%-C%.%#,
            \%-G%.%#

let &cpo = s:keepcpo
unlet s:keepcpo
