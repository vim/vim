" Tests for various eval things.

function s:foo() abort
  try
    return [] == 0
  catch
    return 1
  endtry
endfunction

func Test_catch_return_with_error()
  call assert_equal(1, s:foo())
endfunc

func Test_nocatch_restore_silent_emsg()
  silent! try
    throw 1
  catch
  endtry
  echoerr 'wrong'
  let c1 = nr2char(screenchar(&lines, 1))
  let c2 = nr2char(screenchar(&lines, 2))
  let c3 = nr2char(screenchar(&lines, 3))
  let c4 = nr2char(screenchar(&lines, 4))
  let c5 = nr2char(screenchar(&lines, 5))
  call assert_equal('wrong', c1 . c2 . c3 . c4 . c5)
endfunc

func Test_mkdir_p()
  call mkdir('Xmkdir/nested', 'p')
  call assert_true(isdirectory('Xmkdir/nested'))
  try
    " Trying to make existing directories doesn't error
    call mkdir('Xmkdir', 'p')
    call mkdir('Xmkdir/nested', 'p')
  catch /E739:/
    call assert_report('mkdir(..., "p") failed for an existing directory')
  endtry
  " 'p' doesn't suppress real errors
  call writefile([], 'Xfile')
  call assert_fails('call mkdir("Xfile", "p")', 'E739')
  call delete('Xfile')
  call delete('Xmkdir', 'rf')
endfunc

func Test_line_continuation()
  let array = [5,
	"\ ignore this
	\ 6,
	"\ more to ignore
	"\ more moreto ignore
	\ ]
	"\ and some more
  call assert_equal([5, 6], array)
endfunc

func Test_E963()
  " These commands used to cause an internal error prior to vim 8.1.0563
  let v_e = v:errors
  let v_o = v:oldfiles
  call assert_fails("let v:errors=''", 'E963:')
  call assert_equal(v_e, v:errors)
  call assert_fails("let v:oldfiles=''", 'E963:')
  call assert_equal(v_o, v:oldfiles)
endfunc

func Test_for_invalid()
  call assert_fails("for x in 99", 'E714:')
  call assert_fails("for x in 'asdf'", 'E714:')
  call assert_fails("for x in {'a': 9}", 'E714:')
endfunc

func Test_readfile_binary()
  new
  call setline(1, ['one', 'two', 'three'])
  setlocal ff=dos
  write XReadfile
  let lines = readfile('XReadfile')
  call assert_equal(['one', 'two', 'three'], lines)
  let lines = readfile('XReadfile', '', 2)
  call assert_equal(['one', 'two'], lines)
  let lines = readfile('XReadfile', 'b')
  call assert_equal(["one\r", "two\r", "three\r", ""], lines)
  let lines = readfile('XReadfile', 'b', 2)
  call assert_equal(["one\r", "two\r"], lines)

  bwipe!
  call delete('XReadfile')
endfunc

func Test_let_errmsg()
  call assert_fails('let v:errmsg = []', 'E730:')
  let v:errmsg = ''
  call assert_fails('let v:errmsg = []', 'E730:')
  let v:errmsg = ''
endfunc
