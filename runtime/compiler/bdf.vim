" Vim compiler file
" Compiler:	    BDF to PCF Conversion
" Maintainer:	    Nikolai Weibull <sourc@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/compiler/pcp/bdf/
" Latest Revision:  2004-05-22
" arch-tag:	    2e2f3a55-199b-468c-aa2e-d6b1a7b87806

if exists("current_compiler")
  finish
endif
let current_compiler = "bdf"

if exists(":CompilerSet") != 2          " older Vim always used :setlocal
  command -nargs=* CompilerSet setlocal <args>
endif

let s:cpo_save = &cpo
set cpo-=C

CompilerSet makeprg=bdftopcf\ $*

CompilerSet errorformat=%ABDF\ %trror\ on\ line\ %l:\ %m,
      \%-Z%p^,
      \%Cbdftopcf:\ bdf\ input\\,\ %f\\,\ corrupt,
      \%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save

" vim: set sts=2 sw=2:
