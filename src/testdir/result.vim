if 1 " eval feature
  set nocp
  func Count(match, type)
    if a:type ==# 'executed'
      let g:executed += (a:match+0)
    elseif a:type ==# 'failed'
      let g:failed = a:match+0
    elseif a:type ==# 'skipped'
      let g:skipped += 1
      call extend(g:skipped_output, ["\t".a:match])
    endif
  endfunc

  let g:executed=0
  let g:skipped=0
  let g:failed=0
  let g:skipped_output=[]
  let g:failed_output=[]
  let output = [ "",
        \ "Test results: " . strftime('%Y%m%d %H:%M:%S'),
        \ "-------------------------------"]

  try
    %s/^Executed\s\+\zs\d\+\ze\s\+tests/\=Count(submatch(0),'executed')/egn
    %s/^SKIPPED \zs.*/\=Count(submatch(0), 'skipped')/egn
    %s/^\(\d\+\)\s\+FAILED:/\=Count(submatch(1), 'failed')/egn

    call extend(output, ["Executed: ". printf("%5d", g:executed). " Tests"])
    call extend(output, [" Skipped: ". printf("%5d", g:skipped).  " Tests"])
    call extend(output, ["  Failed: ". printf("%5d", g:failed).   " Tests", ""])
    call extend(output, [" Skipped:"])
    call extend(output, skipped_output)
    if filereadable('test.log')
      call extend(output, ["", "Failures: "])
      let failed_output = filter(readfile('test.log'), { v,k -> !empty(k)})
      call extend(output, map(failed_output, { v,k -> "\t".k}) + [""])
    endif

  catch  " Catch-all
  finally
    call writefile(output, 'test_result.log')  " overwrites an existing file
    q!
  endtry
endif  " end eval
%d
r test.log
w test_result.log
q!
