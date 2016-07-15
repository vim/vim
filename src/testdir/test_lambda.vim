function! Test_lambda_with_filter()
  let s:x = 2
  call assert_equal([2, 3], filter([1, 2, 3], {i, v -> v >= s:x}))
endfunction

function! Test_lambda_with_map()
  let s:x = 1
  call assert_equal([2, 3, 4], map([1, 2, 3], {i, v -> v + s:x}))
endfunction

function! Test_lambda_with_sort()
  call assert_equal([1, 2, 3, 4, 7], sort([3,7,2,1,4], {a, b -> a - b}))
endfunction

function! Test_lambda_with_timer()
  if !has('timers')
    return
  endif

  let s:n = 0
  let s:timer_id = 0
  function! s:Foo()
    "let n = 0
    let s:timer_id = timer_start(50, {-> execute("let s:n += 1 | echo s:n")}, {"repeat": -1})
  endfunction

  call s:Foo()
  sleep 200ms
  " do not collect lambda
  call test_garbagecollect_now()
  let m = s:n
  sleep 200ms
  call timer_stop(s:timer_id)
  call assert_true(m > 1)
  call assert_true(s:n > m + 1)
  call assert_true(s:n < 9)
endfunction

function! Test_lambda_with_partial()
  let l:Cb = function({... -> ['zero', a:1, a:2, a:3]}, ['one', 'two'])
  call assert_equal(['zero', 'one', 'two', 'three'], l:Cb('three'))
endfunction

function Test_lambda_fails()
  call assert_equal(3, {a, b -> a + b}(1, 2))
  call assert_fails('echo {a, a -> a + a}(1, 2)', 'E15:')
  call assert_fails('echo {a, b -> a + b)}(1, 2)', 'E15:')
endfunc
