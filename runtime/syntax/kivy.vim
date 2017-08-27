" Vim syntax file
" ~/.vim/syntax/kivy.vim
"
" Vim syntax file
" Language:	Kivy
" Maintainer:	Gabriel Pettier <gabriel.pettier@gmail.com>
" Last Change:	2017 august 26
" Version:     2
" URL:         http://kivy.org/

if exists("b:current_syntax")
    finish
endif

syn include @pyth $VIMRUNTIME/syntax/python.vim

syn match kivyComment       /#.*\n/ display contains=pythonTodo,Spell
syn match kivyPreProc       /^#:.*/
syn match kivyDefine        /^#:set .*/
syn match kivyInclude       /^#:include .*/
syn match kivyImport        /^#:import .*/
syn match kivyAttribute     /\I\i*/ nextgroup=kivyValue
syn match kivyBind          /on_\I\i*:/ nextgroup=kivyValue
syn match kivyRule          /<-*\I\i*\([,@]s*\I\i*\)*>:/
syn match kivyRootRule      /^\I\i*:$/
syn match kivyInstruction   /^\s\+\u\i*:/ nextgroup=kivyValue
syn match kivyWidget        /^\s\+\u\i*:/

syn region kivyAttribute start=/^\z(\s\+\)\l\+:\n\1\s\{4}/ skip=/^\z1\s\{4}.*$/ end=/^$/ contains=@pyth

syn region kivyBind start=/^\z(\s\+\)on_\i\+:\n\1\s\{4}/ skip=/^\z1\s\{4}.*$/ end=/^$/ contains=@pyth
syn region kivyBind start=/^\z(\s\+\)on_\i\+:\n\1\s\{4}/ skip="^$\|^\z1\s{4}" end="^\z1\I"me=e-9999 contains=@pyth
syn region kivyBind start=/on_\i\+:\s/ end=/$/ contains=@pyth

syn match kivyValue /\(id\s*\)\@<!:\s*.*$/ contains=@pyth skipwhite
syn match kivyId   /\(id:\s*\)\@<=\w\+/

syn match kivyCanvas         /^\s*canvas.*:$/ nextgroup=kivyInstruction
syn region kivyCanvas        start=/^\z(\s*\)canvas.*:$/ skip="^$\|^\z1\s{4}" end="^\z1\I"me=e-9999 contains=kivyInstruction,kivyValue

hi def link kivyPreproc      PreProc
hi def link kivyComment      Comment
hi def link kivyRule         Typedef
hi def link kivyRootRule     Identifier
hi def link kivyAttribute    Label
hi def link kivyBind         Keyword
hi def link kivyWidget       Type
hi def link kivyCanvas       Function
hi def link kivyInstruction  Statement
hi def link KivyId           Define
hi def link kivyDefine       Define
hi def link kivyImport       Macro
hi def link kivyInclude      Include

let b:current_syntax = "kivy"

" vim: ts=8
