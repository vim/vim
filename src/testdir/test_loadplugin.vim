" Tests for :loadplugin

func Test_loadplugin()
  let topdir = expand('%:h') . '/Xdir'
  exe 'set packpath=' . topdir
  let plugdir = topdir . '/pack/mine/opt/mytest'
  call mkdir(plugdir . '/plugin', 'p')
  call mkdir(plugdir . '/ftdetect', 'p')
  filetype on
  try
    exe 'split ' . plugdir . '/plugin/test.vim'
    call setline(1, 'let g:plugin_works = 42')
    wq

    exe 'split ' . plugdir . '/ftdetect/test.vim'
    call setline(1, 'let g:ftdetect_works = 17')
    wq

    loadplugin mytest
    call assert_true(42, g:plugin_works)
    call assert_true(17, g:ftdetect_works)
  finally
    call delete(topdir, 'rf')
  endtry
endfunc
