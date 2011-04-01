" Compiler: Intel Fortran Compiler
" Maintainer: H Xu <xuhdev@gmail.com>
" Version: 0.1.1
" Last Change: 19 March 2011
" Homepage: http://www.vim.org/scripts/script.php?script_id=3497
"           https://bitbucket.org/xuhdev/compiler-ifort.vim
" License: Same as Vim

if exists('current_compiler')
    finish
endif
let current_compiler = 'ifort'

if exists(":CompilerSet") != 2		" older Vim always used :setlocal
  command -nargs=* CompilerSet setlocal <args>
endif

CompilerSet errorformat=
            \%A%f(%l):\ %trror\ \#%n:\ %m,
            \%A%f(%l):\ %tarning\ \#%n:\ %m,
            \%-Z%p^,
            \%-G%.%#
