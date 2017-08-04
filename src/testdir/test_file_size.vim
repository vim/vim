" Inserts 2 million lines with consecutive integers starting from 1
" (essentially, the output of GNU's seq 1 2000000), writes them to Xtest
" and writes its cksum to test.out.
"
" We need 2 million lines to trigger a call to mf_hash_grow().  If it would mess
" up the lines the checksum would differ.
"
" cksum is part of POSIX and so should be available on most Unixes.
" If it isn't available then the test will be skipped.
func Test_File_Size()
  if !executable('cksum')
      return
  endif

  new
  set belloff=all fileformat=unix undolevels=-1
  for i in range(1, 2000000, 100)
      call append(i, range(i, i + 99))
  endfor

  1delete
  w! Xtest
  let res = systemlist('cksum Xtest')[0]
  let res = substitute(res, "\r", "", "")
  call assert_equal('3678979763 14888896 Xtest', res)

  enew!
  call delete('Xtest')
  set belloff& fileformat& undolevels&
endfunc
