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

