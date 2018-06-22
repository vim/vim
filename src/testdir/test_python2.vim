" Test for python 2 commands.
" TODO: move tests from test87.in here.

if !has('python')
  finish
endif

func Test_pydo()
  " Check deleting lines does not trigger ml_get error.
  py import vim
  new
  call setline(1, ['one', 'two', 'three'])
  pydo vim.command("%d_")
  bwipe!

  " Check switching to another buffer does not trigger ml_get error.
  new
  let wincount = winnr('$')
  call setline(1, ['one', 'two', 'three'])
  pydo vim.command("new")
  call assert_equal(wincount + 1, winnr('$'))
  bwipe!
  bwipe!
endfunc

func Test_set_cursor()
  " Check that setting the cursor position works.
  py import vim
  new
  call setline(1, ['first line', 'second line'])
  normal gg
  pydo vim.current.window.cursor = (1, 5)
  call assert_equal([1, 6], [line('.'), col('.')])

  " Check that movement after setting cursor position keeps current column.
  normal j
  call assert_equal([2, 6], [line('.'), col('.')])
endfunc
