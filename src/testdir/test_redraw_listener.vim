" Tests for redraw_listener_add() and redraw_listener_remove()

CheckRunVimInTerminal
CheckFeature eval

" Test if redraws are correctly picked up
func Test_redraw_listening()
  let lines =<< trim END
    let g:redrawtick = 0
    let g:redrawtickend = 0

    func OnRedrawStart()
      let g:redrawtick += 1
      call writefile([g:redrawtick, g:redrawtickend], 'XRedrawStartResult')
    endfunc

    func OnRedrawEnd()
      let g:redrawtickend += 1
      call writefile([g:redrawtick, g:redrawtickend], 'XRedrawEndResult')
    endfunc

    let g:listenerid = redraw_listener_add(#{
          \ on_start: function("OnRedrawStart"),
          \ on_end: function("OnRedrawEnd")
          \ })
  END
  call writefile(lines, 'XTest_redrawlistener', 'D')
  defer delete('XRedrawStartResult')
  defer delete('XRedrawEndResult')

  let buf = RunVimInTerminal('-S XTest_redrawlistener', {'rows': 10, 'cols': 78})

  " We do it in separate chunks so they dont get bunched up into one redraw
  call term_sendkeys(buf, "i") " 1 on startup
  call TermWait(buf)
  call term_sendkeys(buf, "one\<CR>") " 2
  call TermWait(buf)
  call term_sendkeys(buf, "two\<CR>") " 3
  call TermWait(buf)
  call term_sendkeys(buf, "three\<Esc>") " 4
  call TermWait(buf)

  call WaitForAssert({-> assert_equal(["4", "3"], readfile('XRedrawStartResult'))})
  call WaitForAssert({-> assert_equal(["4", "4"], readfile('XRedrawEndResult'))})

  call term_sendkeys(buf, "\<Esc>:vsplit\<CR>:enew\<CR>") " 5 and 6
  call TermWait(buf)

  call WaitForAssert({-> assert_equal(["6", "5"], readfile('XRedrawStartResult'))})
  call WaitForAssert({-> assert_equal(["6", "6"], readfile('XRedrawEndResult'))})

  call term_sendkeys(buf, "\<Esc>:redraw!\<CR>") " 7
  call TermWait(buf)

  call WaitForAssert({-> assert_equal(["7", "6"], readfile('XRedrawStartResult'))})
  call WaitForAssert({-> assert_equal(["7", "7"], readfile('XRedrawEndResult'))})

  " Test if removing listener works
  call term_sendkeys(buf, "\<Esc>:call redraw_listener_remove(g:listenerid)\<CR>")
  call TermWait(buf)
  call term_sendkeys(buf, "\<Esc>:redraw!\<CR>")
  call TermWait(buf)
  call term_sendkeys(buf, "\<Esc>:split\<CR>")
  call TermWait(buf)
  call WaitForAssert({-> assert_equal(["7", "6"], readfile('XRedrawStartResult'))})
  call WaitForAssert({-> assert_equal(["7", "7"], readfile('XRedrawEndResult'))})

  call StopVimInTerminal(buf)
endfunc

" Test if another redraw isn't caused right after if on_start callback causes one.
func Test_redraw_no_redraw()
  let lines =<< trim END
    let g:redrawtick = 0

    func OnRedrawStart()
      call setline(1, "hello")

      let g:redrawtick += 1
      call writefile([g:redrawtick], 'XRedrawStartResult')
    endfunc

    let g:listenerid = redraw_listener_add(#{
          \ on_start: function("OnRedrawStart"),
          \ })
  END
  call writefile(lines, 'XTest_redrawlistener', 'D')
  defer delete('XRedrawStartResult')

  let buf = RunVimInTerminal('-S XTest_redrawlistener', {'rows': 10, 'cols': 78})

  call term_sendkeys(buf, "ione\<Esc>")
  call TermWait(buf)

  call WaitForAssert({-> assert_equal(["2"], readfile('XRedrawStartResult'))})

  call StopVimInTerminal(buf)
endfunc

" Test if listener can be removed in the callback
func Test_redraw_remove_in_callback()
  let lines =<< trim END
    let g:redrawtick = 0

    func OnRedrawStart()
      let g:redrawtick += 1
      call writefile([g:redrawtick], 'XRedrawStartResult')
      call redraw_listener_remove(g:listenerid)
    endfunc

    let g:listenerid = redraw_listener_add(#{
          \ on_start: function("OnRedrawStart"),
          \ })
  END
  call writefile(lines, 'XTest_redrawlistener', 'D')
  defer delete('XRedrawStartResult')

  let buf = RunVimInTerminal('-S XTest_redrawlistener', {'rows': 10, 'cols': 78})

  call term_sendkeys(buf, "i")
  call TermWait(buf)
  call term_sendkeys(buf, "one\<CR>")
  call TermWait(buf)
  call term_sendkeys(buf, "two\<CR>")
  call TermWait(buf)
  call term_sendkeys(buf, "three\<Esc>")
  call TermWait(buf)

  call WaitForAssert({-> assert_equal(["1"], readfile('XRedrawStartResult'))})

  call StopVimInTerminal(buf)
endfunc

" vim: shiftwidth=2 sts=2 expandtab
