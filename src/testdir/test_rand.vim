
function! Test_Rand()
  let r = srand()
  call assert_equal([123456789, 362436069, 521288629, 88675123], r)
  call assert_equal(597902826, rand(r))
  call assert_equal(458295558, rand(r))
  call assert_equal(1779455562, rand(r))
  call assert_equal(663552176, rand(r))
  call assert_equal(507026878, rand(r))
endfunction
