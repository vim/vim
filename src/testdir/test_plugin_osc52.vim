" Test for the OSC 52 plugin

CheckRunVimInTerminal
" Does not run on BSD CI test runner
CheckNotBSD

source util/screendump.vim

" Check if plugin correctly detects OSC 52 support if possible
func Test_osc52_detect()
  let lines =<< trim END
    packadd osc52
    set clipmethod=osc52
  END
  call writefile(lines, "Xosc52.vim", "D")
  defer delete("Xosc52result")

  let buf = RunVimInTerminal("-S Xosc52.vim", {})

  " The plugin creates an autocmd listening for DA1 responses

  " No support
  call term_sendkeys(buf, "\<Esc>[?62;22;c")
  call TermWait(buf)

  call term_sendkeys(buf,
        \ "\<Esc>:call writefile([v:termda1, v:clipmethod], 'Xosc52result')\<CR>")
  call TermWait(buf)
  call WaitForAssert({->
        \ assert_equal(["\<Esc>[?62;22;c", "none"], readfile('Xosc52result'))})

  " Yes support
  call term_sendkeys(buf, "\<Esc>[?62;2;3;4;1;52;c")
  call TermWait(buf)
  call term_sendkeys(buf,
        \ "\<Esc>:call writefile([v:termda1, v:clipmethod], 'Xosc52result')\<CR>")
  call TermWait(buf)
  call WaitForAssert({-> assert_equal(["\<Esc>[?62;2;3;4;1;52;c", "osc52"],
        \ readfile('Xosc52result'))})

  call StopVimInTerminal(buf)
endfunc

" Test if pasting works
func Test_osc52_paste()
  CheckScreendump

  let lines =<< trim END
    packadd osc52
    set clipmethod=osc52
    redraw!
  END
  call writefile(lines, "Xosc52.vim", "D")

  let buf = RunVimInTerminal("-S Xosc52.vim", {})

  call term_sendkeys(buf, "\<Esc>[?52;c")
  call TermWait(buf)

  call term_sendkeys(buf, "\"+p")
  call TermWait(buf)

  " Check to see if message is shown after a second of waiting for a response
  sleep 1500m
  call VerifyScreenDump(buf, 'Test_osc52_paste_01', {})

  call term_sendkeys(buf, "\<Esc>]52;c;" ..
        \ base64_encode(str2blob(["hello", "world!"])) .. "\<Esc>\\")
  call TermWait(buf)

  " Check if message is gone
  call VerifyScreenDump(buf, 'Test_osc52_paste_02', {})

  " Test when invalid base64 content received (should emit a message)
  call term_sendkeys(buf, "\"+p")
  call TermWait(buf)

  call term_sendkeys(buf, "\<Esc>]52;c;abc\<Esc>\\")
  call TermWait(buf)

  call VerifyScreenDump(buf, 'Test_osc52_paste_03', {})

  " Test if interrupt is handled and message is outputted
  call term_sendkeys(buf, "\"+p")
  call TermWait(buf)

  call term_sendkeys(buf, "\<C-c>")
  call TermWait(buf)

  call VerifyScreenDump(buf, 'Test_osc52_paste_04', {})

  call StopVimInTerminal(buf)
endfunc

" vim: shiftwidth=2 sts=2 expandtab
