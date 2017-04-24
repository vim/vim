" Test argument list commands

func Test_argidx()
  args a b c
  last
  call assert_equal(2, argidx())
  %argdelete
  call assert_equal(0, argidx())
  " doing it again doesn't result in an error
  %argdelete
  call assert_equal(0, argidx())
  call assert_fails('2argdelete', 'E16:')

  args a b c
  call assert_equal(0, argidx())
  next
  call assert_equal(1, argidx())
  next
  call assert_equal(2, argidx())
  1argdelete
  call assert_equal(1, argidx())
  1argdelete
  call assert_equal(0, argidx())
  1argdelete
  call assert_equal(0, argidx())
endfunc

func Test_argadd()
  %argdelete
  argadd a b c
  call assert_equal(0, argidx())

  %argdelete
  argadd a
  call assert_equal(0, argidx())
  argadd b c d
  call assert_equal(0, argidx())

  call Init_abc()
  argadd x
  call Assert_argc(['a', 'b', 'x', 'c'])
  call assert_equal(1, argidx())

  call Init_abc()
  0argadd x
  call Assert_argc(['x', 'a', 'b', 'c'])
  call assert_equal(2, argidx())

  call Init_abc()
  1argadd x
  call Assert_argc(['a', 'x', 'b', 'c'])
  call assert_equal(2, argidx())

  call Init_abc()
  $argadd x
  call Assert_argc(['a', 'b', 'c', 'x'])
  call assert_equal(1, argidx())

  call Init_abc()
  $argadd x
  +2argadd y
  call Assert_argc(['a', 'b', 'c', 'x', 'y'])
  call assert_equal(1, argidx())

  %argd
  edit d
  arga
  call assert_equal(len(argv()), 1)
  call assert_equal(get(argv(), 0, ''), 'd')

  %argd
  new
  arga
  call assert_equal(len(argv()), 0)
endfunc

func Init_abc()
  args a b c
  next
endfunc

func Assert_argc(l)
  call assert_equal(len(a:l), argc())
  let i = 0
  while i < len(a:l) && i < argc()
    call assert_equal(a:l[i], argv(i))
    let i += 1
  endwhile
endfunc

" Test for [count]argument and [count]argdelete commands
" Ported from the test_argument_count.in test script
func Test_argument()
  " Clean the argument list
  arga a | %argd

  let save_hidden = &hidden
  set hidden

  let g:buffers = []
  augroup TEST
    au BufEnter * call add(buffers, expand('%:t'))
  augroup END

  argadd a b c d
  $argu
  $-argu
  -argu
  1argu
  +2argu

  augroup TEST
    au!
  augroup END

  call assert_equal(['d', 'c', 'b', 'a', 'c'], g:buffers)

  redir => result
  ar
  redir END
  call assert_true(result =~# 'a b \[c] d')

  .argd
  call assert_equal(['a', 'b', 'd'], argv())

  -argd
  call assert_equal(['a', 'd'], argv())

  $argd
  call assert_equal(['a'], argv())

  1arga c
  1arga b
  $argu
  $arga x
  call assert_equal(['a', 'b', 'c', 'x'], argv())

  0arga y
  call assert_equal(['y', 'a', 'b', 'c', 'x'], argv())

  %argd
  call assert_equal([], argv())

  arga a b c d e f
  2,$-argd
  call assert_equal(['a', 'f'], argv())

  let &hidden = save_hidden

  " Setting argument list should fail when the current buffer has unsaved
  " changes
  %argd
  enew!
  set modified
  call assert_fails('args x y z', 'E37:')
  args! x y z
  call assert_equal(['x', 'y', 'z'], argv())
  call assert_equal('x', expand('%:t'))

  last | enew | argu
  call assert_equal('z', expand('%:t'))

  %argdelete
  call assert_fails('argument', 'E163:')
endfunc

" Test for 0argadd and 0argedit
" Ported from the test_argument_0count.in test script
func Test_zero_argadd()
  " Clean the argument list
  arga a | %argd

  arga a b c d
  2argu
  0arga added
  call assert_equal(['added', 'a', 'b', 'c', 'd'], argv())

  2argu
  arga third
  call assert_equal(['added', 'a', 'third', 'b', 'c', 'd'], argv())

  %argd
  arga a b c d
  2argu
  0arge edited
  call assert_equal(['edited', 'a', 'b', 'c', 'd'], argv())

  2argu
  arga third
  call assert_equal(['edited', 'a', 'third', 'b', 'c', 'd'], argv())
endfunc

func Reset_arglist()
  args a | %argd
endfunc

" Test for argc()
func Test_argc()
  call Reset_arglist()
  call assert_equal(0, argc())
  argadd a b
  call assert_equal(2, argc())
endfunc

" Test for arglistid()
func Test_arglistid()
  call Reset_arglist()
  arga a b
  call assert_equal(0, arglistid())
  split
  arglocal
  call assert_equal(1, arglistid())
  tabnew | tabfirst
  call assert_equal(0, arglistid(2))
  call assert_equal(1, arglistid(1, 1))
  call assert_equal(0, arglistid(2, 1))
  call assert_equal(1, arglistid(1, 2))
  tabonly | only | enew!
  argglobal
  call assert_equal(0, arglistid())
endfunc

" Test for argv()
func Test_argv()
  call Reset_arglist()
  call assert_equal([], argv())
  call assert_equal("", argv(2))
  argadd a b c d
  call assert_equal('c', argv(2))
endfunc

" Test for the :argedit command
func Test_argedit()
  call Reset_arglist()
  argedit a
  call assert_equal(['a'], argv())
  call assert_equal('a', expand('%:t'))
  argedit b
  call assert_equal(['a', 'b'], argv())
  call assert_equal('b', expand('%:t'))
  argedit a
  call assert_equal(['a', 'b'], argv())
  call assert_equal('a', expand('%:t'))
  if has('unix')
    " on MS-Windows this would edit file "a b"
    call assert_fails('argedit a b', 'E172:')
  endif
  argedit c
  call assert_equal(['a', 'c', 'b'], argv())
  0argedit x
  call assert_equal(['x', 'a', 'c', 'b'], argv())
  enew! | set modified
  call assert_fails('argedit y', 'E37:')
  argedit! y
  call assert_equal(['x', 'y', 'a', 'c', 'b'], argv())
  %argd
endfunc

" Test for the :argdelete command
func Test_argdelete()
  call Reset_arglist()
  args aa a aaa b bb
  argdelete a*
  call assert_equal(['b', 'bb'], argv())
  call assert_equal('aa', expand('%:t'))
  last
  argdelete %
  call assert_equal(['b'], argv())
  call assert_fails('argdelete', 'E471:')
  call assert_fails('1,100argdelete', 'E16:')
  %argd
endfunc

" Tests for the :next, :prev, :first, :last, :rewind commands
func Test_argpos()
  call Reset_arglist()
  args a b c d
  last
  call assert_equal(3, argidx())
  call assert_fails('next', 'E165:')
  prev
  call assert_equal(2, argidx())
  Next
  call assert_equal(1, argidx())
  first
  call assert_equal(0, argidx())
  call assert_fails('prev', 'E164:')
  3next
  call assert_equal(3, argidx())
  rewind
  call assert_equal(0, argidx())
  %argd
endfunc

" Test for autocommand that redefines the argument list, when doing ":all".
func Test_arglist_autocmd()
  autocmd BufReadPost Xxx2 next Xxx2 Xxx1
  call writefile(['test file Xxx1'], 'Xxx1')
  call writefile(['test file Xxx2'], 'Xxx2')
  call writefile(['test file Xxx3'], 'Xxx3')

  new
  " redefine arglist; go to Xxx1
  next! Xxx1 Xxx2 Xxx3
  " open window for all args
  all
  call assert_equal('test file Xxx1', getline(1))
  wincmd w
  wincmd w
  call assert_equal('test file Xxx1', getline(1))
  " should now be in Xxx2
  rewind
  call assert_equal('test file Xxx2', getline(1))

  autocmd! BufReadPost Xxx2
  enew! | only
  call delete('Xxx1')
  call delete('Xxx2')
  call delete('Xxx3')
  argdelete Xxx*
  bwipe! Xxx1 Xxx2 Xxx3
endfunc

func Test_arg_all_expand()
  call writefile(['test file Xxx1'], 'Xx x')
  next notexist Xx\ x runtest.vim
  call assert_equal('notexist Xx\ x runtest.vim', expand('##'))
  call delete('Xx x')
endfunc
