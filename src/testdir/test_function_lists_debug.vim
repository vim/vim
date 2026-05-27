" Debug version of test_function_lists

func Test_function_lists_debug()

  " Create a file of the functions in evalfunc.c:global_functions[] that are
  " not obsolete, sorted in ASCII order.
  enew!
  read ../evalfunc.c
  1,/^static const funcentry_T global_functions\[\] =$/d
  call search('^};$')
  .,$d
  v/^    {/d
  g/\/\/ obsolete$/d
  %s/^    {"//
  %s/".*//
  sort u
  w! ++ff=unix Xsorted_current_global_functions

  " Create a file of the functions listed in ":help functions".
  enew!
  if filereadable('../../doc/builtin.txt')
    read ../../doc/builtin.txt
  else
    read ../../runtime/doc/builtin.txt
  endif
  call search('^USAGE')
  1,.d
  call search('^==========')
  .,$d
  v/^\S/d
  %s/(.*//
  let l:lines = getline(1, '$')
  call uniq(l:lines)
  call writefile(l:lines, "Xfunctions")

  " Print ch_log entries
  echo "=== C source ch_log entries ==="
  silent !grep "ch_log" Xsorted_current_global_functions
  echo "=== Help file ch_log entries ==="
  silent !grep "ch_log" Xfunctions
  echo "=== Diff ==="
  silent !diff -u Xsorted_current_global_functions Xfunctions

  call delete("Xsorted_current_global_functions")
  call delete("Xfunctions")
endfunc
