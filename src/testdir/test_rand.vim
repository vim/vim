
function! Test_Rand()
  let r = srand(123456789)
  call assert_equal([123456789, 362436069, 521288629, 88675123], r)
  call assert_equal(3701687786, rand(r))
  call assert_equal(458299110, rand(r))
  call assert_equal(2500872618, rand(r))
  call assert_equal(3633119408, rand(r))
  call assert_equal(516391518, rand(r))

  let s = srand()
  sleep 1
  call assert_notequal(s, srand())

  call srand()
  let v = rand()
  call assert_notequal(v, rand())
endfunction
