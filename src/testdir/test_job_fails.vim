" This test is in a separate file, because it usually causes reports for memory
" leaks under valgrind.  That is because when fork/exec fails memory is not
" freed.  Since the process exists right away it's not a real leak.

source shared.vim

func Test_job_start_fails()
  if has('job')
    let job = job_start('axdfxsdf')
    if has('unix')
      call WaitFor({-> job_status(job) == "dead"})
    else
      call WaitFor({-> job_status(job) == "fail"})
    endif
  endif
endfunc
