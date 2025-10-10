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
    return ("b40", ["tuple", "of", "strings"])
endfunc

func! Copy(reg, type, lines)
    call echoraw("\<Esc>]52;c;" .. base64_encode(str2blob(a:lines)) .. "\<Esc>\\")
endfunc

" Test if "available" function works properly for provider
func Test_clipboard_provider_available()
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
endfunc
