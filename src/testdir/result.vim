if 1 " eval feature
  set nocp
  func CountExecuted(match)
    let g:executed += (a:match+0)
  endfunc

  func! CountSkipped(match)
    let g:skipped += 1
    call extend(g:skipped_output, ["\t".a:match])
  endfunc

  let g:executed=0
  let g:skipped=0
  let g:skipped_output=[]
  let g:failed_output=[]
  let output = [ "",
        \ "Test results: " . strftime('%Y%m%d %H:%M:%S'),
        \ "-------------------------------"]

  %s/^Executed\s\+\zs\d\+\ze\s\+tests/\=CountExecuted(submatch(0))/gn
  %s/^SKIPPED \zs.*/\=CountSkipped(submatch(0))/gn

  call extend(output, [" Skipped: ". g:skipped. " Tests"])
  call extend(output, ["Executed: ". g:executed. " Tests", ""])
  call extend(output, [" Skipped:"])
  call extend(output, skipped_output)
  if filereadable('test.log')
    call extend(output, ["", "Failures: "])
    let failed_output = filter(readfile('test.log'), { v,k -> !empty(k)})
    call extend(output, map(failed_output, { v,k -> "\t".k}))
  endif

  call writefile(output, 'test_result.log')
  q!
endif
r test.log
w test_result.log
q!
