" Vim predefined user commands


" :CompilerSet - runtime/compiler/*.vim

CompilerSet makeprg=ant
CompilerSet errorformat=\ %#[%.%#]\ %#%f:%l:%v:%*\\d:%*\\d:\ %t%[%^:]%#:%m,
    \%A\ %#[%.%#]\ %f:%l:\ %m,%-Z\ %#[%.%#]\ %p^,%C\ %#[%.%#]\ %#%m

" :SynColor - runtime/synmenu.vim

SynColor Comment term=bold cterm=NONE ctermfg=Cyan ctermbg=NONE gui=NONE guifg=#80a0ff guibg=NONE

" :SynMenu - runtime/makemenu.vim

SynMenu AB.A2ps\ config:a2ps
SynMenu AB.Aap:aap
SynMenu AB.ABAP/4:abap
SynMenu AB.Abaqus:abaqus
SynMenu AB.ABC\ music\ notation:abc
SynMenu AB.ABEL:abel
SynMenu AB.AceDB\ model:acedb
SynMenu AB.Ada:ada

" :VimFold\a, :Vim9, :VimL - runtime/syntax/vim.vim

VimFolda syntax region vimFoo start="foo" end="bar"

Vim9 syntax keyword vim9Foo foo nextgroup=vim9Comment
VimL syntax keyword vimFoo  foo nextgroup=vimComment

