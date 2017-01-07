function! Test_System()
  if !executable('echo') || !executable('cat') || !executable('wc')
    return
  endif
  call assert_equal("123\n", system('echo 123'))
  call assert_equal(['123'], systemlist('echo 123'))
  call assert_equal('123',   system('cat', '123'))
  call assert_equal(['123'], systemlist('cat', '123'))
  call assert_equal("11\n",  system('wc -l', bufnr('.')))
  call assert_equal(['11'],  systemlist('wc -l', bufnr('.')))
endfunction
