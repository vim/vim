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

func Test_helptags()
  let docdir1 = &packpath . '/pack/mine/start/foo/doc'
  let docdir2 = &packpath . '/pack/mine/start/bar/doc'
  call mkdir(docdir1, 'p')
  call mkdir(docdir2, 'p')
  call writefile(['look here: *look-here*'], docdir1 . '/bar.txt')
  call writefile(['look away: *look-away*'], docdir2 . '/foo.txt')
  exe 'set rtp=' . &packpath . '/pack/mine/start/foo,' . &packpath . '/pack/mine/start/bar'

  helptags ALL

  let tags1 = readfile(docdir1 . '/tags') 
  call assert_true(tags1[0] =~ 'look-here')
  let tags2 = readfile(docdir2 . '/tags') 
  call assert_true(tags2[0] =~ 'look-away')
endfunc

func Test_colorscheme()
  let colordirrun = &packpath . '/runtime/colors'
  let colordirstart = &packpath . '/pack/mine/start/foo/colors'
  let colordiropt = &packpath . '/pack/mine/opt/bar/colors'
  call mkdir(colordirrun, 'p')
  call mkdir(colordirstart, 'p')
  call mkdir(colordiropt, 'p')
  call writefile(['let g:found_one = 1'], colordirrun . '/one.vim')
  call writefile(['let g:found_two = 1'], colordirstart . '/two.vim')
  call writefile(['let g:found_three = 1'], colordiropt . '/three.vim')
  exe 'set rtp=' . &packpath . '/runtime'

  colorscheme one
  call assert_equal(1, g:found_one)
  colorscheme two
  call assert_equal(1, g:found_two)
  colorscheme three
  call assert_equal(1, g:found_three)
endfunc

func Test_runtime()
  let rundir = &packpath . '/runtime/extra'
  let startdir = &packpath . '/pack/mine/start/foo/extra'
  let optdir = &packpath . '/pack/mine/opt/bar/extra'
  call mkdir(rundir, 'p')
  call mkdir(startdir, 'p')
  call mkdir(optdir, 'p')
  call writefile(['let g:sequence .= "run"'], rundir . '/bar.vim')
  call writefile(['let g:sequence .= "start"'], startdir . '/bar.vim')
  call writefile(['let g:sequence .= "foostart"'], startdir . '/foo.vim')
  call writefile(['let g:sequence .= "opt"'], optdir . '/bar.vim')
  call writefile(['let g:sequence .= "xxxopt"'], optdir . '/xxx.vim')
  exe 'set rtp=' . &packpath . '/runtime'

  let g:sequence = ''
  runtime extra/bar.vim
  call assert_equal('run', g:sequence)
  let g:sequence = ''
  runtime START extra/bar.vim
  call assert_equal('start', g:sequence)
  let g:sequence = ''
  runtime OPT extra/bar.vim
  call assert_equal('opt', g:sequence)
  let g:sequence = ''
  runtime PACK extra/bar.vim
  call assert_equal('start', g:sequence)
  let g:sequence = ''
  runtime! PACK extra/bar.vim
  call assert_equal('startopt', g:sequence)
  let g:sequence = ''
  runtime PACK extra/xxx.vim
  call assert_equal('xxxopt', g:sequence)

  let g:sequence = ''
  runtime ALL extra/bar.vim
  call assert_equal('run', g:sequence)
  let g:sequence = ''
  runtime ALL extra/foo.vim
  call assert_equal('foostart', g:sequence)
  let g:sequence = ''
  runtime! ALL extra/xxx.vim
  call assert_equal('xxxopt', g:sequence)
  let g:sequence = ''
  runtime! ALL extra/bar.vim
  call assert_equal('runstartopt', g:sequence)
endfunc
