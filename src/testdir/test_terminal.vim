" Tests for the term_open() function.

if !exists('*term_open')
  finish
endif

function Test_terminal()
  call term_open(&shell)

  let r = term_list()
  call assert_equal(1, len(r))

  let job = term_getjob(r[0])
  call assert_equal(v:t_job, type(job))

  call term_sendkeys(r[0], "exit\r\n")
  sleep 500m
  call assert_equal('dead', job_status(job))

  exe r[0] 'bwipe'

  let x = has('win32') ? 'cmd /c "echo 123"' : "sh -c 'echo 123'"
  call term_open(x)

  let r = term_list()
  call assert_equal(1, len(r))

  sleep 500m
  let l = term_scrape(r[0], 0)
  call assert_equal('123', l)

  exe r[0] 'bwipe'
endfunction
