" Tests for the Tcl interface.

if !has('tcl')
  finish
end

function Test_tcldo()
  " Check deleting lines does not trigger ml_get error.
  new
  call setline(1, ['one', 'two', 'three'])
  tcldo ::vim::command %d_
  bwipe!

  " Check switching to another buffer does not trigger ml_get error.
  new
  let wincount = winnr('$')
  call setline(1, ['one', 'two', 'three'])
  tcldo ::vim::command new
  call assert_equal(wincount + 1, winnr('$'))
  bwipe!
  bwipe!
endfunc

func Test_set_cursor()
  " Check that setting the cursor position works.
  new
  call setline(1, ['first line', 'second line'])
  normal gg
  tcldo $::vim::current(window) cursor 1 5
  call assert_equal([1, 5], [line('.'), col('.')])

  " Check that movement after setting cursor position keeps current column.
  normal j
  call assert_equal([2, 5], [line('.'), col('.')])
endfunc
