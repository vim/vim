" Tests for 'balloonevalterm'.
" A few tests only work in the terminal.

CheckNotGui
CheckFeature balloon_eval_term

source util/screendump.vim

let s:common_script =<< trim [CODE]
  call setline(1, ["one one one", "two tXo two", "three three three"])
  set balloonevalterm balloonexpr=MyBalloonExpr()..s:trailing balloondelay=100
  let s:trailing = '<'  " check that script context is set
  func MyBalloonExpr()
    return "line " .. v:beval_lnum .. " column " .. v:beval_col .. ":\n" .. v:beval_text
  endfun
  redraw
[CODE]

func Test_balloon_eval_term()
  CheckScreendump
  " Use <Ignore> after <MouseMove> to return from vgetc() without removing
  " the balloon.
  let xtra_lines =<< trim [CODE]
    set updatetime=300
    au CursorHold * echo 'hold fired'
    func Trigger()
      call test_setmouse(2, 6)
      call feedkeys("\<MouseMove>\<Ignore>", "xt")
    endfunc
  [CODE]
  call writefile(s:common_script + xtra_lines, 'XTest_beval', 'D')

  " Check that the balloon shows up after a mouse move
  let buf = RunVimInTerminal('-S XTest_beval', {'rows': 10, 'cols': 50})
  call TermWait(buf, 50)
  call term_sendkeys(buf, 'll')
  call term_sendkeys(buf, ":call Trigger()\<CR>")
  sleep 150m " Wait for balloon to show up (100ms balloondelay time)
  call VerifyScreenDump(buf, 'Test_balloon_eval_term_01', {})

  " Make sure the balloon still shows after 'updatetime' passed and CursorHold
  " was triggered.
  call TermWait(buf, 150)
  call VerifyScreenDump(buf, 'Test_balloon_eval_term_01a', {})

  " clean up
  call StopVimInTerminal(buf)
endfunc

func Test_balloon_eval_term_visual()
  CheckScreendump
  " Use <Ignore> after <MouseMove> to return from vgetc() without removing
  " the balloon.
  call writefile(s:common_script + [
	\ 'call test_setmouse(3, 6)',
	\ 'call feedkeys("3Gevfr", "xt")',
	\ 'redraw!',
	\ 'call feedkeys("\<MouseMove>\<Ignore>", "xt")',
	\ ], 'XTest_beval_visual', 'D')

  " Check that the balloon shows up after a mouse move
  let buf = RunVimInTerminal('-S XTest_beval_visual', {'rows': 10, 'cols': 50})
  call TermWait(buf, 50)
  call WaitForAssert({-> assert_match('-- VISUAL --', term_getline(buf, 10))})
  call VerifyScreenDump(buf, 'Test_balloon_eval_term_02', {})

  " clean up
  call StopVimInTerminal(buf)
endfunc

func Test_balloon_eval_term_rightleft()
  CheckScreendump
  CheckFeature rightleft

  " Use <Ignore> after <MouseMove> to return from vgetc() without removing
  " the balloon.
  let xtra_lines =<< trim [CODE]
    set rightleft
    func Trigger()
      call test_setmouse(2, 50 + 1 - 6)
      call feedkeys("\<MouseMove>\<Ignore>", "xt")
    endfunc
  [CODE]
  call writefile(s:common_script + xtra_lines, 'XTest_beval_rl', 'D')

  " Check that the balloon shows up after a mouse move
  let buf = RunVimInTerminal('-S XTest_beval_rl', {'rows': 10, 'cols': 50})
  call TermWait(buf, 50)
  call term_sendkeys(buf, 'll')
  call term_sendkeys(buf, ":call Trigger()\<CR>")
  call VerifyScreenDump(buf, 'Test_balloon_eval_term_03', {})

  " clean up
  call StopVimInTerminal(buf)
endfunc

func Test_balloon_eval_term_conceallevel_three_col()
  CheckRunVimInTerminal

  let line = repeat('a', 24)
        \ .. ' HIDDEN target words after hidden text to force wrapping'
        \ .. ' and mapping checks'
  let target_col = stridx(line, 'target') + 1
  let xtra_lines =<< trim [CODE]
    setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
          \ signcolumn=no nonumber
    syntax match Hidden /HIDDEN / conceal
    set balloonevalterm balloonexpr=MyBalloonExpr() balloondelay=100
    func MyBalloonExpr()
      call writefile([string(v:beval_lnum), string(v:beval_col),
            \ v:beval_text], 'Xbeval_conceal_result')
      return ''
    endfunc
    func Trigger()
      let target_col = stridx(getline(1), 'target') + 1
      let pos = screenpos(0, 1, target_col)
      call writefile([string(pos.row), string(pos.curscol),
            \ string(target_col)], 'Xbeval_conceal_pos')
      call test_setmouse(pos.row, pos.curscol)
      call feedkeys("\<MouseMove>\<Ignore>", "xt")
    endfunc
  [CODE]
  call writefile(['call setline(1, ' .. string(line) .. ')'] + xtra_lines,
        \ 'XTest_beval_conceal', 'D')
  call delete('Xbeval_conceal_result')
  call delete('Xbeval_conceal_pos')

  let buf = RunVimInTerminal('-S XTest_beval_conceal',
        \ {'rows': 10, 'cols': 40})
  call TermWait(buf, 50)
  call term_sendkeys(buf, ":call Trigger()\<CR>")
  call WaitFor({-> filereadable('Xbeval_conceal_pos')
        \ && len(readfile('Xbeval_conceal_pos')) > 0})
  let pos = readfile('Xbeval_conceal_pos')
  call assert_equal('1', pos[0])
  call assert_true(str2nr(pos[1]) < target_col)
  call WaitFor({-> filereadable('Xbeval_conceal_result')
        \ && len(readfile('Xbeval_conceal_result')) > 0})
  call assert_equal(['1', string(target_col), 'target'],
        \ readfile('Xbeval_conceal_result'))

  call StopVimInTerminal(buf)
  call delete('Xbeval_conceal_result')
  call delete('Xbeval_conceal_pos')
endfunc

" vim: shiftwidth=2 sts=2 expandtab
