" Test signal handling.

if !has('unix')
  finish
endif

func Test_signal_WINCH()
  " Test signal WINCH (window resize signal)
  let signals = system('kill -l')
  if signals !~ 'WINCH'
    " Skip when signal WINCH is not available.
    return
  endif

  let old_lines = &lines
  let old_columns = &columns
  let new_lines = &lines - 2
  let new_columns = &columns - 2

  exe 'set lines=' . new_lines
  exe 'set columns=' . new_columns
  call assert_equal(new_lines, &lines)
  call assert_equal(new_columns, &columns)

  " Send signal and sleep enough time for
  " signal to be precessed.
  exe 'silent !kill -s WINCH ' . getpid()
  sleep 10m

  " lines and columns should have been restored
  " after handing signal WINCH.
  call assert_equal(old_lines, &lines)
  call assert_equal(old_columns, &columns)
endfunc

