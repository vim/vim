" Tests for startup.

source shared.vim

" Check that loading startup.vim works.
func Test_startup_script()
  set compatible
  source $VIMRUNTIME/defaults.vim

  call assert_equal(0, &compatible)
endfunc

" Verify the order in which plugins are loaded:
" 1. plugins in non-after directories
" 2. packages
" 3. plugins in after directories
func Test_after_comes_later()
  if !has('packages')
    return
  endif
  let before = [
	\ 'let $HOME = "/does/not/exist"',
	\ 'set loadplugins',
	\ 'set rtp=Xhere,Xafter',
	\ 'set packpath=Xhere,Xafter',
	\ 'set nomore',
	\ ]
  let after = [
	\ 'redir! > Xtestout',
	\ 'scriptnames',
	\ 'redir END',
	\ 'quit',
	\ ]
  call mkdir('Xhere/plugin', 'p')
  call writefile(['let done = 1'], 'Xhere/plugin/here.vim')
  call mkdir('Xhere/pack/foo/start/foobar/plugin', 'p')
  call writefile(['let done = 1'], 'Xhere/pack/foo/start/foobar/plugin/foo.vim')

  call mkdir('Xafter/plugin', 'p')
  call writefile(['let done = 1'], 'Xafter/plugin/later.vim')

  if RunVim(before, after)

    let lines = readfile('Xtestout')
    let expected = ['Xbefore.vim', 'here.vim', 'foo.vim', 'later.vim', 'Xafter.vim']
    let found = []
    for line in lines
      for one in expected
	if line =~ one
	  call add(found, one)
	endif
      endfor
    endfor
    call assert_equal(expected, found)
  endif

  call delete('Xtestout')
  call delete('Xhere', 'rf')
  call delete('Xafter', 'rf')
endfunc
