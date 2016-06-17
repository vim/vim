runtime ftplugin/man.vim

function Test_g_ft_man_open_mode()
  let l:w = winwidth(1)
  vnew
  let l:h = winheight(1)
  q

  " split horizontally
  Man 'vim'
  call assert_equal(l:w, winwidth(1))
  call assert_true(l:h > winheight(1))
  call assert_equal(1, tabpagenr('$'))
  call assert_equal(1, tabpagenr())
  q

  " split horizontally
  let g:ft_man_open_mode = "horz"
  Man 'vim'
  call assert_equal(l:w, winwidth(1))
  call assert_true(l:h > winheight(1))
  call assert_equal(1, tabpagenr('$'))
  call assert_equal(1, tabpagenr())
  q

  " split vertically
  let g:ft_man_open_mode = "vert"
  Man 'vim'
  call assert_true(l:w > winwidth(1))
  call assert_equal(l:h, winheight(1))
  call assert_equal(1, tabpagenr('$'))
  call assert_equal(1, tabpagenr())
  q

  " separate tab
  let g:ft_man_open_mode = "tab"
  Man 'vim'
  call assert_equal(l:w, winwidth(1))
  call assert_equal(l:h, winheight(1))
  call assert_equal(2, tabpagenr('$'))
  call assert_equal(2, tabpagenr())
  q
endfunction

function Test_nomodifiable()
  Man 'vim'
  call assert_false(&l:modifiable)
  q
endfunction
