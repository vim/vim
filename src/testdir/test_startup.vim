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
	\ 'set nocp viminfo+=nviminfo',
	\ 'set guioptions+=M',
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

  if RunVim(before, after, '')

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

func Test_help_arg()
  if RunVim([], [], '--help >Xtestout')
    let lines = readfile('Xtestout')
    call assert_true(len(lines) > 20)
    call assert_true(lines[0] =~ 'Vi IMproved')

    " check if  couple of lines are there
    let found = 0
    for line in lines
      if line =~ '-R.*Readonly mode'
	let found += 1
      endif
      if line =~ '--version'
	let found += 1
      endif
    endfor
    call assert_equal(2, found)
  endif
  call delete('Xtestout')
endfunc
