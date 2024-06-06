" Vim comment strings
" VIM_TEST_SETUP let g:vimsyn_comment_strings = v:true

" pre "string" post

function Foo()
  " pre "string" post
endfunction

def Bar()
  # pre "string" post
enddef

command Foo {
  # pre "string" post
}

autocmd BufNewFile * {
  # pre "string" post
}
