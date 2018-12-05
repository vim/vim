" Test for triming strings.
func Test_flatten()
    call assert_equal(flatten([], []))
    call assert_equal(flatten([1, 2, 3]), [1, 2, 3])
    call assert_equal(flatten([[1], [2], [3]]), [1, 2, 3])
    call assert_equal(flatten([[[[1]]], [2], [3]]), [1, 2, 3])
endfunc
