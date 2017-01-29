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
