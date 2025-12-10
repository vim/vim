" Tests for redraw_listener_add() and redraw_listener_remove()

CheckRunVimInTerminal
CheckFeature eval

" Test if redraws are correctly picked up
func Test_redraw_listening()
  let lines =<< trim END
    let g:redrawtick = 0
    let g:redrawret = 1

    func OnRedrawStart()
      let g:redrawtick += 1
      call writefile([g:redrawtick], 'XRedrawStartResult')
      return g:redrawret
    endfunc

    func OnRedrawWin(winid, bufnr, topline, botline)
      call writefile([a:winid, a:bufnr, a:topline, a:botline], 'XRedrawWinResult')
    endfunc

    let g:listenerid = redraw_listener_add(#{
          \ on_start: function("OnRedrawStart"),
          \ on_win: function("OnRedrawWin")
          \ })
  END
  call writefile(lines, 'XTest_redrawlistener', 'D')

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

  call WaitForAssert({-> assert_equal(["4"], readfile('XRedrawStartResult'))})
  call WaitForAssert({-> assert_equal(["1000", "1", "1", "3"],
        \ readfile('XRedrawWinResult'))})

  call term_sendkeys(buf, "\<Esc>:vsplit\<CR>:enew\<CR>") " 5 and 6
  call TermWait(buf)

  call WaitForAssert({-> assert_equal(["6"], readfile('XRedrawStartResult'))})
  call WaitForAssert({-> assert_equal(["1001", "2", "1", "1"],
        \ readfile('XRedrawWinResult'))})

  call term_sendkeys(buf, "\<Esc>:redraw!\<CR>") " 7
  call TermWait(buf)

  call WaitForAssert({-> assert_equal(["7"], readfile('XRedrawStartResult'))})

  " Test if removing listener works
  call term_sendkeys(buf, "\<Esc>:call redraw_listener_remove(g:listenerid)\<CR>")
  call TermWait(buf)
  call term_sendkeys(buf, "\<Esc>:redraw!\<CR>")
  call TermWait(buf)
  call term_sendkeys(buf, "\<Esc>:split\<CR>")
  call TermWait(buf)
  call WaitForAssert({-> assert_equal(["7"], readfile('XRedrawStartResult'))})

  call StopVimInTerminal(buf)
endfunc

" vim: shiftwidth=2 sts=2 expandtab
