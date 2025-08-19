" Test for clipboard provider feature

CheckFeature clipboard_provider

" Test if 'available' callback works properly
func Test_clipboard_provider_cb_available()
  let g:testval = v:true

  let v:clipproviders["test"] = {
        \   "available": {-> g:testval}
        \ }

  set clipmethod=test
  call assert_equal("test", v:clipmethod)

  call assert_fails("put", "E353:")

  let g:testval = v:false
  clipreset
  call assert_equal("none", v:clipmethod)

  set clipmethod&
endfunc

func Copy(reg)
  let g:test_reg = a:reg
  let g:test_result = getreg(a:reg)

  call assert_fails("call setline(1, 'test')", "E565:")
endfunc

" Test if 'copy' callback works properly
func Test_clipboard_provider_cb_copy()
  new
  let v:clipproviders["test"] = {
        \   "available": {-> v:true},
        \   "copy": function("Copy")
        \ }
  set clipmethod=test

  if has('unnamedplus')
    call setreg('+', "plus")

    call assert_equal("plus", g:test_result)
    call assert_equal("+", g:test_reg)
  endif

  call setreg('*', "star")

  call assert_equal("star", g:test_result)
  call assert_equal("*", g:test_reg)

  bw!
  set clipmethod&
endfunc

func Paste(reg)
  call setreg(a:reg, "reg: " .. a:reg)

  " Check if texlock is working
  call assert_fails("call setline(1, 'test')", "E565:")
endfunc

" Test if 'paste' callback works properly
func Test_clipboard_provider_cb_paste()
  new
  let v:clipproviders["test"] = {
        \   "available": {-> v:true},
        \   "paste": function("Paste")
        \ }
  set clipmethod=test

  if has('unnamedplus')
      call assert_equal("reg: +", getreg('+'))
  endif

  call assert_equal("reg: *", getreg('*'))

  set clipmethod&
  bw!
endfunc
