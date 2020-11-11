" Test for Vim9 script with failures, causing memory leaks to be reported.
" The leaks happen after a fork() and can be ignored.

def Test_assignment()
  if has('channel')
    var chan1: channel
    var job1: job
    var job2: job = job_start('willfail')
  endif
enddef
