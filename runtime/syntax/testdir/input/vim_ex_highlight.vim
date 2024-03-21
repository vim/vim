" Vim :highlight command

" list
highlight
highlight Comment

" reset
highlight clear

" disable
highlight clear Comment
highlight Comment NONE

" add/modify
highlight Comment cterm=underline
highlight default Comment term=bold

" link
highlight link Foo Comment
highlight! link Foo Comment
highlight link Foo NONE
highlight! link Foo NONE

" default link
highlight default link Foo Comment
highlight! default link Foo Comment
highlight default link Foo NONE
highlight! default link Foo NONE


" line continuation and command separator

hi Comment
      "\ comment
      \ term=bold
      "\ comment
      \ ctermfg=Cyan
      \ guifg=#80a0ff
      \ gui=bold

hi Comment
      \ term=bold
      \ ctermfg=Cyan
      \ guifg=#80a0ff
      \ gui=bold | echo "Foo"

hi Comment term=bold ctermfg=Cyan guifg=#80a0ff gui=bold | echo "Foo"

hi default link
      \ Foo
      \ Comment

hi default link
      \ Foo
      \ Comment | echo "Foo"
