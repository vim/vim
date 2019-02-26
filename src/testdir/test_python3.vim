" Test for python 3 commands.
" TODO: move tests from test88.in here.

if !has('python3')
  finish
endif

func Test_py3do()
  " Check deleting lines does not trigger an ml_get error.
  py3 import vim
  new
  call setline(1, ['one', 'two', 'three'])
  py3do vim.command("%d_")
  bwipe!

  " Check switching to another buffer does not trigger an ml_get error.
  new
  let wincount = winnr('$')
  call setline(1, ['one', 'two', 'three'])
  py3do vim.command("new")
  call assert_equal(wincount + 1, winnr('$'))
  bwipe!
  bwipe!
endfunc

func Test_set_cursor()
  " Check that setting the cursor position works.
  py3 import vim
  new
  call setline(1, ['first line', 'second line'])
  normal gg
  py3do vim.current.window.cursor = (1, 5)
  call assert_equal([1, 6], [line('.'), col('.')])

  " Check that movement after setting cursor position keeps current column.
  normal j
  call assert_equal([2, 6], [line('.'), col('.')])
endfunc

func Test_vim_function()
  " Check creating vim.Function object
  py3 import vim

  func s:foo()
    return matchstr(expand('<sfile>'), '<SNR>\zs\d\+_foo$')
  endfunc
  let name = '<SNR>' . s:foo()

  try
    py3 f = vim.bindeval('function("s:foo")')
    call assert_equal(name, py3eval('f.name'))
  catch
    call assert_false(v:exception)
  endtry

  try
    py3 f = vim.Function(b'\x80\xfdR' + vim.eval('s:foo()').encode())
    call assert_equal(name, py3eval('f.name'))
  catch
    call assert_false(v:exception)
  endtry

  py3 del f
  delfunc s:foo
endfunc

func Test_skipped_python3_command_does_not_affect_pyxversion()
  set pyxversion=0
  if 0
    python3 import vim
  endif
  call assert_equal(0, &pyxversion)  " This assertion would have failed with Vim 8.0.0251. (pyxversion was introduced in 8.0.0251.)
endfunc
