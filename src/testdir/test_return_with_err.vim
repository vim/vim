" Test for :return with error

function! s:foo() abort
  try
    return [] == 0
  catch
    return 1
  endtry
endfunction

function Test_return_with_error()
  let v = s:foo()
  call assert_equal(1, v)
endfunction
