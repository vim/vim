" Tests for the term_start() function.

if !exists('*term_start')
  finish
endif

function Test_terminal_basic()
  call term_start(&shell)

  let r = term_list()
  call assert_equal(1, len(r))

  let job = term_getjob(r[0])
  call assert_equal(v:t_job, type(job))

  call term_sendkeys(r[0], "exit\r\n")
  sleep 500m
  call assert_equal('dead', job_status(job))

  exe r[0] 'bwipe'
endfunction

function Test_terminal_scrape()
  let x = has('win32') ? 'cmd /c "cls && color 2 && echo 123"' : "sh -c 'echo echo -e \"\e[32m123\"'"
  call term_start(x)

  let r = term_list()
  call assert_equal(1, len(r))

  sleep 500m
  let l = term_scrape(r[0], 0)
  call assert_equal('1', l[0].char)
  call assert_false('#000000' == l[0].fg)
  call assert_true('#000000' == l[0].bg)

  let l = term_getline(r[0], 0)
  call assert_equal('123', l)

  exe r[0] 'bwipe'
endfunction
