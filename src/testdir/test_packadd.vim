" Tests for 'packpath' and :packadd

func SetUp()
  let s:topdir = expand('%:h') . '/Xdir'
  exe 'set packpath=' . s:topdir
  let s:plugdir = s:topdir . '/pack/mine/opt/mytest'
endfunc

func TearDown()
  call delete(s:topdir, 'rf')
endfunc

func Test_packadd()
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

  packadd mytest

  call assert_equal(42, g:plugin_works)
  call assert_equal(17, g:ftdetect_works)
  call assert_true(len(&rtp) > len(rtp))
  call assert_true(&rtp =~ 'testdir/Xdir/pack/mine/opt/mytest\($\|,\)')

  " Check exception
  call assert_fails("packadd directorynotfound", 'E919:')
  call assert_fails("packadd", 'E471:')
endfunc

func Test_packadd_noload()
  call mkdir(s:plugdir . '/plugin', 'p')
  call mkdir(s:plugdir . '/syntax', 'p')
  set rtp&
  let rtp = &rtp

  exe 'split ' . s:plugdir . '/plugin/test.vim'
  call setline(1, 'let g:plugin_works = 42')
  wq
  let g:plugin_works = 0

  packadd! mytest

  call assert_true(len(&rtp) > len(rtp))
  call assert_true(&rtp =~ 'testdir/Xdir/pack/mine/opt/mytest\($\|,\)')
  call assert_equal(0, g:plugin_works)

  " check the path is not added twice
  let new_rtp = &rtp
  packadd! mytest
  call assert_equal(new_rtp, &rtp)
endfunc

" Check command-line completion for 'packadd'
func Test_packadd_completion()
  let optdir1 = &packpath . '/pack/mine/opt'
  let optdir2 = &packpath . '/pack/candidate/opt'

  call mkdir(optdir1 . '/pluginA', 'p')
  call mkdir(optdir1 . '/pluginC', 'p')
  call mkdir(optdir2 . '/pluginB', 'p')
  call mkdir(optdir2 . '/pluginC', 'p')

  let li = []
  call feedkeys(":packadd \<Tab>')\<C-B>call add(li, '\<CR>", 't')
  call feedkeys(":packadd " . repeat("\<Tab>", 2) . "')\<C-B>call add(li, '\<CR>", 't')
  call feedkeys(":packadd " . repeat("\<Tab>", 3) . "')\<C-B>call add(li, '\<CR>", 't')
  call feedkeys(":packadd " . repeat("\<Tab>", 4) . "')\<C-B>call add(li, '\<CR>", 'tx')
  call assert_equal("packadd pluginA", li[0])
  call assert_equal("packadd pluginB", li[1])
  call assert_equal("packadd pluginC", li[2])
  call assert_equal("packadd ", li[3])
endfunc

func Test_packloadall()
  let plugindir = &packpath . '/pack/mine/start/foo/plugin'
  call mkdir(plugindir, 'p')
  call writefile(['let g:plugin_foo_number = 1234'], plugindir . '/bar.vim')
  packloadall
  call assert_equal(1234, g:plugin_foo_number)

  " only works once
  call writefile(['let g:plugin_bar_number = 4321'], plugindir . '/bar2.vim')
  packloadall
  call assert_false(exists('g:plugin_bar_number'))

  " works when ! used
  packloadall!
  call assert_equal(4321, g:plugin_bar_number)
endfunc
