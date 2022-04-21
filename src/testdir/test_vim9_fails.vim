" Test for Vim9 script with failures, causing memory leaks to be reported.
" The leaks happen after a fork() and can be ignored.

source check.vim

def Test_assignment()
  if !has('channel')
    CheckFeature channel
  else
    var chan1: channel
    var job1: job
    var job2: job = job_start('willfail')
  endif
enddef

" Unclear why this test causes valgrind to report problems.
def Test_job_info_return_type()
  if !has('job')
    CheckFeature job
  else
    var job: job = job_start(&shell)
    var jobs = job_info()
    assert_equal('list<job>', typename(jobs))
    assert_equal('dict<any>', typename(job_info(jobs[0])))
    job_stop(job)
  endif
enddef

