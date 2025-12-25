" Vim compiler file
" Compiler:     Pyrefly (Python Type Checker)
" Maintainer:   @konfekt
" Last Change:  2025 Dec 24

if exists("current_compiler") | finish | endif
let current_compiler = "pyrefly"

let s:cpo_save = &cpo
set cpo&vim

" CompilerSet makeprg=pyrefly
exe 'CompilerSet makeprg=' ..  escape(
        \ get(b:, 'pyrefly_makeprg', get(g:, 'pyrefly_makeprg', 'pyrefly')) .. ' --color=never check --summary=none --output-format=full-text',
        \ ' \|"')
" Parse Pyrefly output shaped like:
"   ERROR ... [code]
"      --> path/to/file.py:line:col
"   WARN  ... [code]
"      --> ...
CompilerSet errorformat=
      \%E%\\s%#ERROR\ %m,
      \%W%\\s%#WARN\ %m,
      \%N%\\s%#NOTE\ %m,
      \%C%\\s%#-->\ %f:%l:%c,
      \%-G%*\\d%\\s%#\|%.%#,
      \%-G%\\s%#\|%.%#,
      \%-G%\\s%#,
      \%C[ \t]\ %.%#,
      \%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save
