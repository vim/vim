" Tests for :loadplugin

func SetUp()
  let s:topdir = expand('%:h') . '/Xdir'
  exe 'set packpath=' . s:topdir
  let s:plugdir = s:topdir . '/pack/mine/opt/mytest'
endfunc

func TearDown()
  call delete(s:topdir, 'rf')
endfunc

func Test_loadplugin()
  call mkdir(s:plugdir . '/plugin', 'p')
  call mkdir(s:plugdir . '/ftdetect', 'p')
  set rtp&
  let rtp = &rtp
  filetype on

  exe 'split ' . s:plugdir . '/plugin/test.vim'
  call setline(1, 'let g:plugin_works = 42')
  wq

  exe 'split ' . s:plugdir . '/ftdetect/test.vim'
  call setline(1, 'let g:ftdetect_works = 17')
  wq

  loadplugin mytest

  call assert_equal(42, g:plugin_works)
  call assert_equal(17, g:ftdetect_works)
  call assert_true(len(&rtp) > len(rtp))
  call assert_true(&rtp =~ 'testdir/Xdir/pack/mine/opt/mytest\($\|,\)')
endfunc

func Test_packadd()
  call mkdir(s:plugdir . '/syntax', 'p')
  set rtp&
  let rtp = &rtp
  packadd mytest
  call assert_true(len(&rtp) > len(rtp))
  call assert_true(&rtp =~ 'testdir/Xdir/pack/mine/opt/mytest\($\|,\)')

  " check the path is not added twice
  let new_rtp = &rtp
  packadd mytest
  call assert_equal(new_rtp, &rtp)
endfunc
