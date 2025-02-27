" Vim :set command

set
set!
set all
set! all
set termcap
set! termcap

set aleph?
set aleph

set noallowrevins

set allowrevins!
set invallowrevins

set aleph&
set aleph&vi
set aleph&vim

set all&

set aleph=128
set aleph:128

set aleph =128
set aleph :128

set aleph+=96
set aleph^=2
set aleph-=96

set backspace+=nostop
set backspace^=nostop
set backspace-=nostop

set ai nosi sw=3 tw=3

set <t_#4>=^[Ot " FIXME
set <M-b>=^[b   " FIXME

setlocal autoread
setglobal noautoread
set autoread<


" :help option-backslash

" When setting options using |:let| and |literal-string|, you need to use one
" fewer layer of backslash. A few examples:
set makeprg=make\ file	    " results in "make file"
let &makeprg='make file'    " (same as above)
set makeprg=make\\\ file    " results in "make\ file"
set tags=tags\ /usr/tags    " results in "tags" and "/usr/tags"
set tags=tags\\\ file	    " results in "tags file"
let &tags='tags\ file'	    " (same as above)

set makeprg=make,file	    " results in "make,file"
set makeprg=make\\,file	    " results in "make\,file"
set tags=tags,file	    " results in "tags" and "file"
set tags=tags\\,file	    " results in "tags,file"
let &tags='tags\,file'	    " (same as above)

" This example sets the 'titlestring' option to "hi|there":
set titlestring=hi\|there
" This sets the 'titlestring' option to "hi" and 'iconstring' to "there":
set titlestring=hi|set iconstring=there

set dir=\\machine\path	    " results in "\\machine\path"
set dir=\\\\machine\\path   " results in "\\machine\path"
set dir=\\path\\file	    " results in "\\path\file" (wrong!)


" :help :set_env

set term=$TERM.new
set path=/usr/$INCLUDE,$HOME/include,.


" Multiline :set and option values

set path=abc,def,ghi
      "\ def is the 'define' option
      \ def=abc,def,ghi
 
set path=abc,
      "\ def is a 'path' directory value
      \def,ghi

set path=
      "\ def is a 'path' directory value
      \abc,def


" CompilerSet

CompilerSet makeprg=ant
CompilerSet errorformat=\ %#[%.%#]\ %#%f:%l:%v:%*\\d:%*\\d:\ %t%[%^:]%#:%m,
    \%A\ %#[%.%#]\ %f:%l:\ %m,%-Z\ %#[%.%#]\ %p^,%C\ %#[%.%#]\ %#%m


" Unreported issue (double backslash)

setlocal com=s1:/*,mb:*,ex:*/,b:--,be:\\
echo "Foo"
setlocal include=^\\s*\\%(so\\%[urce]\\\|ru\\%[ntime]\\)[!\ ]\ *\\zs[^\\|]*
echo "Foo"
set quoteescape=\\
echo "Foo"
set quoteescape=\
echo "Foo"

