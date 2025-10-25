" Test for clipboard provider feature

CheckFeature clipboard_provider

func! AvailableBoth()
    return "+*"
endfunc

func! AvailablePlus()
    return "+"
endfunc

func! PasteList(reg, type)
    return ["c", ["list"]]
endfunc

func! PasteTuple(reg, type)
    return ("", ["tuple", "of", "strings"])
endfunc

func! PasteType(reg, type)
    let g:vim_test_type = a:type
    return ("c", [a:type])
endfunc

func! PasteRegType(reg, type)
    return (g:vim_test_reg_type, ["7 chars"])
endfunc

func! Copy(reg, type, lines)
    let g:vim_test_stuff = {
                \ "type": a:type,
                \ "lines": a:lines
                \ }
endfunc

" Test if "available" function works properly for provider
func Test_clipboard_provider_available()
    CheckUnix
    CheckFeature clipboard_plus_avail

    let v:clipproviders["test"] = {
                \ "available": function("AvailablePlus"),
                \ "paste": {
                \       '+': function("PasteList"),
                \       '*': function("PasteList")
                \   }
                \ }

    set clipmethod=test
    call assert_equal("test", v:clipmethod)

    call assert_equal("list", getreg("+"))
    " Test if star register is unavailable
    call assert_equal("", getreg("*"))

    let v:clipproviders["test"] = {
                \ "available": function("AvailableBoth"),
                \ "paste": {
                \       '+': function("PasteList"),
                \       '*': function("PasteList")
                \   }
                \ }

    clipreset

    call assert_equal("list", getreg("+"))
    call assert_equal("list", getreg("*"))

    let v:clipproviders["test"] = {
                \ "paste": {
                \       '+': function("PasteList"),
                \       '*': function("PasteList")
                \   }
                \ }

    " Should default to TRUE
    call assert_equal("list", getreg("+"))
    call assert_equal("list", getreg("*"))

    set clipmethod&
endfunc

" Test if "paste" functions work properly for provider
func Test_clipboard_provider_paste()
    " Test if tuples and lists work the same
    let v:clipproviders["test"] = {
                \ "paste": {
                \       '*': function("PasteList")
                \   }
                \ }

    set clipmethod=test
    call assert_equal("test", v:clipmethod)

    call assert_equal("list", getreg("*"))

    let v:clipproviders["test"] = {
                \ "paste": {
                \       '*': function("PasteTuple")
                \   }
                \ }

    call assert_equal("tuple\nof\nstrings\n", getreg("*"))

    " Test if "implicit" and "explicit" arguments are correctly used
    let v:clipproviders["test"] = {
                \ "paste": {
                \       '*': function("PasteType")
                \   }
                \ }

    call assert_equal("explicit", getreg("*"))

    :registers

    call assert_equal("implicit", g:vim_test_type)
    unlet g:vim_test_type

    " Test if correct register type is used
    let v:clipproviders["test"] = {
                \ "paste": {
                \       '*': function("PasteRegType")
                \   }
                \ }

    let g:vim_test_reg_type = "v"
    call assert_equal("v", getregtype("*"))
    let g:vim_test_reg_type = "c"
    call assert_equal("v", getregtype("*"))

    let g:vim_test_reg_type = "l"
    call assert_equal("V", getregtype("*"))
    let g:vim_test_reg_type = "l"
    call assert_equal("V", getregtype("*"))

    let g:vim_test_reg_type = "b"
    call assert_equal("7", getregtype("*"))
    let g:vim_test_reg_type = ""
    call assert_equal("7", getregtype("*"))

    let g:vim_test_reg_type = "b40"
    call assert_equal("40", getregtype("*"))

    set clipmethod&
endfunc

" Test if "copy" functions work properly for provider
func Test_clipboard_provider_copy()
    let v:clipproviders["test"] = {
                \ "copy": {
                \       '*': function("Copy")
                \   }
                \ }

    set clipmethod=test
    call assert_equal("test", v:clipmethod)

    call setreg("*", ["hello", "world", "!"], "c")
    call assert_equal(["hello", "world", "!"], g:vim_test_stuff.lines)
    call assert_equal("v", g:vim_test_stuff.type)

    call setreg("*", ["hello", "world", "!"], "l")
    call assert_equal(["hello", "world", "!"], g:vim_test_stuff.lines)
    call assert_equal("V", g:vim_test_stuff.type)

    call setreg("*", ["hello", "world", "!"], "b40")
    call assert_equal(["hello", "world", "!"], g:vim_test_stuff.lines)
    call assert_equal("40", g:vim_test_stuff.type)

    set clipmethod&
endfunc
