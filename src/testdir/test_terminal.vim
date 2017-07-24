" Tests for the term_open() function.

if !exists('*term_open')
  finish
endif

function Test_terminal()
  call term_open(&shell)

  let r = term_list()
  call assert_equal(len(r), 1)

  let job = term_getjob(r[0])
  call assert_equal(type(job), v:t_job)

  call term_sendkeys(r[0], "exit\n")
  call assert_equal(job_status(job), 'exit')

  let r = term_list()
  call assert_equal(len(r), 0)
endfunction
