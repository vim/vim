" Tests for Lua.
" TODO: move tests from test85.in here.

if !has('lua')
  finish
endif

func Test_luado()
  new
  call setline(1, ['one', 'two', 'three'])
  luado vim.command("%d_")
  bwipe!

  " Check switching to another buffer does not trigger ml_get error.
  new
  let wincount = winnr('$')
  call setline(1, ['one', 'two', 'three'])
  luado vim.command("new")
  call assert_equal(wincount + 1, winnr('$'))
  bwipe!
  bwipe!
endfunc

func Test_set_cursor()
  " Check that setting the cursor position works.
  new
  call setline(1, ['first line', 'second line'])
  normal gg
  lua << EOF
w = vim.window()
w.line = 1
w.col = 5
EOF
  call assert_equal([1, 5], [line('.'), col('.')])

  " Check that movement after setting cursor position keeps current column.
  normal j
  call assert_equal([2, 5], [line('.'), col('.')])
endfunc
