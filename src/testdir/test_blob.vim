" Tests for the Blob types

func TearDown()
  " Run garbage collection after every test
  call test_garbagecollect_now()
endfunc

" Tests for Blob type

" Blob creation
func Test_blob_create()
  " Creating Blob directly with different types
  let b = 0zDEADBEEF
  call assert_equal(4, len(b))
  call assert_equal(0xDE, b[0])
  call assert_equal(0xAD, b[1])
  call assert_equal(0xBE, b[2])
  call assert_equal(0xEF, b[3])
endfunc

" assignment to a blob
func Test_blob_assign()
  let b = 0zDEADBEEF
  let b2 = b[1:2]
  call assert_equal(0zADBE, b2)
endfunc

" test for range assign
func Test_blob_range_assign()
  let b = [0]
  let b[:] = [1, 2]
  call assert_equal([1, 2], b)
endfunc

" Test removing items in blob
func Test_blob_func_remove()
  " Test removing 1 element
  let b = 0zDEADBEEF
  call assert_equal(0xDE, remove(b, 0))
  call assert_equal(0zADBEEF, b)

  let b = 0zDEADBEEF
  call assert_equal(0xAD, remove(b, 1))
  call assert_equal(0zDEBEEF, b)

  " Test removing range of element(s)
  let b = 0zDEADBEEF
  "call assert_equal(0zBE, remove(b, 2, 2))
  "call assert_equal(0zDEADEF, b)
  return

  let b = 0zDEADBEEF
  call assert_equal([2, 3], remove(b, 1, 2))
  call assert_equal([1, 4], b)

  " Test invalid cases
  let b = 0zDEADBEEF
  call assert_fails("call remove(b, 5)", 'E684:')
  call assert_fails("call remove(b, 1, 5)", 'E684:')
  call assert_fails("call remove(b, 3, 2)", 'E16:')
  call assert_fails("call remove(1, 0)", 'E712:')
  call assert_fails("call remove(b, b)", 'E745:')
endfunc

" filter() item in blob
func Test_blob_filter()
  let b = 0zDEADBEEF
  call filter(b, 'v:val != 0xEF')
  call assert_equal(0zDEADBE, b)
endfunc

" map() item in blob
func Test_blob_map()
  let b = 0zDEADBEEF
  call map(b, 'v:val + 1')
  call assert_equal(0zDFAEBFF0, b)
endfunc
