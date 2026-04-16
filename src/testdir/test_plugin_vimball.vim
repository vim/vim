" Test for the vimball plugin

let s:testdir = expand("<script>:h")
let s:default_vimball = ['" Vimball Archiver by Charles E. Campbell',
                       \ 'UseVimball',
                       \ 'finish',
                       \ 'XVimball/Xtest.txt	[[[1',
                       \ '2',
                       \ 'Hello Vimball',
                       \ '123']

func SetUp()
  ru plugin/vimballPlugin.vim
  let g:vimball_home = s:testdir
endfunc

func TearDown()
  call delete('Xtest.vmb')
  call delete('.VimballRecord')
endfunc

func s:setup()
  call mkdir('XVimball', 'p')
  call writefile(['Hello Vimball', '123'], 'XVimball/Xtest.txt')
endfunc

func s:teardown()
  call delete('XVimball', 'rf')
  call delete('Xtest.vmb')
  bw! Xtest.vmb
  bw! XVimball/Xtest.txt
  if bufloaded('.VimballRecord')
    bw! .VimballRecord
  endif
endfunc

func s:Mkvimball()
  call s:setup()
  new
  0put ='XVimball/Xtest.txt'
  $d
  1,1MkVimball! Xtest
  bw!
endfunc

func Test_vimball_basic()
  call s:Mkvimball()
  call assert_true(filereadable('Xtest.vmb'), 'vimball file should be created')
  call assert_equal(s:default_vimball, readfile('Xtest.vmb'))

  call delete('XVimball', 'rf')
  sp Xtest.vmb
  let mess = execute(':mess')
  call assert_match('\*\*\*vimball\*\*\* Source this file to extract it!', mess)
  so %
  call feedkeys("\<cr>", "t")
  unlet mess
  let mess = execute(':mess')->split('\n')
  call assert_equal('extracted <XVimball/Xtest.txt>: 2 lines', mess[-2])

  call assert_true(filereadable('XVimball/Xtest.txt'), 'extracted file should exist')
  call assert_equal(['Hello Vimball', '123'], readfile('XVimball/Xtest.txt'))
 
  " Vimball extraction has been recorded
  call assert_true(filereadable('.VimballRecord'))
  let record = readfile('.VimballRecord')
  call assert_equal(1, record->len())
  call assert_match('^Xtest.vmb: rmdir.*call delete(', record[0])
  call s:teardown()
endfunc

func Test_vimball_path_traversal()
  call s:Mkvimball()
  call delete('XVimball', 'rf')
  sp Xtest.vmb
  " try to write into upper dir
  4s#XVimball#../&#
  so %
  call feedkeys("\<cr>", "it")

  let mess = execute(':mess')->split('\n')[-1]
  call assert_match('(Vimball) Path Traversal Attack detected, aborting\.\.\.', mess)
  call assert_false(filereadable('../XVimball/Xtest.txt'))
  call s:teardown()
endfunc

func Test_vimball_path_traversal_drive_letter()
  call s:Mkvimball()
  call delete('XVimball', 'rf')
  sp Xtest.vmb
  " try to write to a Windows-style absolute path with a drive letter
  4s#XVimball#C:/&#
  so %
  call feedkeys("\<cr>", "it")

  let mess = execute(':mess')->split('\n')[-1]
  call assert_match('(Vimball) Path Traversal Attack detected, aborting\.\.\.', mess)
  call s:teardown()
endfunc
