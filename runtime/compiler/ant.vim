" Vim Compiler File
" Compiler:	ant
" Maintainer:	Johannes Zellner <johannes@zellner.org>
" Last Change:	Tue, 27 Apr 2004 15:01:45 CEST

if exists("current_compiler")
    finish
endif
let current_compiler = "ant"

if exists(":CompilerSet") != 2		" older Vim always used :setlocal
  command -nargs=* CompilerSet setlocal <args>
endif

let s:cpo_save = &cpo
set cpo&vim

CompilerSet makeprg=ant

" first  line:
"     ant with jikes +E, which assumes  the following
"     two property lines in your 'build.xml':
"
"	  <property name = "build.compiler"	  value = "jikes"/>
"	  <property name = "build.compiler.emacs" value = "true"/>
"
" second line:
"     ant with javac
"
" note that this will work also for tasks like [wtkbuild]
"
setlocal errorformat=\ %#[%.%#]\ %#%f:%l:%v:%*\\d:%*\\d:\ %t%[%^:]%#:%m,
    \%A\ %#[%.%#]\ %f:%l:\ %m,%-Z\ %#[%.%#]\ %p^,%C\ %#[%.%#]\ %#%m

" ,%-C%.%#

let &cpo = s:cpo_save
unlet s:cpo_save
