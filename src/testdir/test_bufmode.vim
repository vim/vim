" Test getting file mode of current buffer.

func Test_bufmode()
    " The mode of an unwritten buffer should be 0.
    call assert_equal(0, bufmode())

    " Using an invalid type for bufname ought to throw an exception.
    try
        call bufmode([])
        call assert_report("should have thrown E745")
    catch
        call assert_exception("E745:")
    endtry

    " The mode of a file buffer should match its file on disk.
    call writefile(["line"], "Xtest1")
    call writefile(["line"], "Xtest2")
    call setfperm("Xtest1", "rw-------")
    call setfperm("Xtest2", "rwx--xr--")
    e Xtest1
    call assert_equal(0600, bufmode())
    e Xtest2
    call assert_equal(0714, bufmode())

    " Mode of a different buffer.
    call assert_equal(0600, bufmode("#"))

    " Mode of the current buffer without masking.
    call assert_equal(0100714, bufmode("%", 0, 1))

    " Mode of a buffer formatted.
    call assert_equal("0714", bufmode("%", 1, 0))

    " Mode of a buffer unmasked and formatted.
    call assert_equal("0100714", bufmode("%", 1, 1))

    %bwipe!
    call delete("Xtest1")
    call delete("Xtest2")
endfunc
