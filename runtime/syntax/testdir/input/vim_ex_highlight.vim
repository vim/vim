" Vim :highlight command
" VIM_TEST_SETUP hi link vimHiGroup Todo


" List

highlight
highlight Comment
highlight Conceal
highlight ErrorMsg
highlight Foo


" Reset

highlight clear


" Disable

highlight clear Comment
highlight clear Conceal
highlight clear ErrorMsg
highlight clear Foo

highlight Comment NONE
highlight Conceal NONE
highlight ErrorMsg NONE
highlight Foo NONE


" Add/modify

highlight Comment cterm=underline
highlight Conceal cterm=underline
highlight ErrorMsg cterm=underline
highlight Foo cterm=underline

highlight default Comment term=bold
highlight default Conceal term=bold
highlight default ErrorMsg term=bold
highlight Foo ErrorMsg term=bold


" Link

highlight link Foo Comment
highlight! link Foo Comment
highlight link Foo NONE
highlight! link Foo NONE
highlight link Foo ErrorMsg
highlight! link Foo ErrorMsg


" Default link

highlight default link Foo Comment
highlight! default link Foo Comment
highlight default link Foo NONE
highlight! default link Foo NONE
highlight default link Foo ErrorMsg
highlight! default link Foo ErrorMsg


" Line continuation and command separator

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


" 'statusline' user groups

highlight User1 ctermfg=black
highlight User2 ctermfg=darkblue
highlight User3 ctermfg=darkgreen
highlight User4 ctermfg=darkcyan
highlight User5 ctermfg=darkred
highlight User6 ctermfg=darkmagenta
highlight User7 ctermfg=darkyellow
highlight User8 ctermfg=lightgray
highlight User9 ctermfg=darkgray


" :terminal group

hi Terminal ctermbg=red ctermfg=blue

