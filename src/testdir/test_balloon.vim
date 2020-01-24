" Tests for 'balloonevalterm'.
" A few tests only work in the terminal.

source check.vim
CheckNotGui
CheckFeature balloon_eval_term

source screendump.vim
CheckScreendump

let s:common_script =<< trim [CODE]
  call setline(1, ["one one one", "two tXo two", "three three three"])
  set balloonevalterm balloonexpr=MyBalloonExpr() balloondelay=100
  func MyBalloonExpr()
    return "line " .. v:beval_lnum .. " column " .. v:beval_col .. ":\n" .. v:beval_text
  endfun
  redraw
[CODE]

func Test_balloon_eval_term()
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
  call writefile(s:common_script + xtra_lines, 'XTest_beval')

  " Check that the balloon shows up after a mouse move
  let buf = RunVimInTerminal('-S XTest_beval', {'rows': 10, 'cols': 50})
  call term_wait(buf, 100)
  call term_sendkeys(buf, 'll')
  call term_sendkeys(buf, ":call Trigger()\<CR>")
  call VerifyScreenDump(buf, 'Test_balloon_eval_term_01', {})

  " Make sure the balloon still shows after 'updatetime' passed and CursorHold
  " was triggered.
  call term_wait(buf, 300)
  call VerifyScreenDump(buf, 'Test_balloon_eval_term_01a', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XTest_beval')
endfunc

func Test_balloon_eval_term_visual()
  " Use <Ignore> after <MouseMove> to return from vgetc() without removing
  " the balloon.
  call writefile(s:common_script + [
	\ 'call test_setmouse(3, 6)',
	\ 'call feedkeys("3Gevfr\<MouseMove>\<Ignore>", "xt")',
	\ ], 'XTest_beval_visual')

  " Check that the balloon shows up after a mouse move
  let buf = RunVimInTerminal('-S XTest_beval_visual', {'rows': 10, 'cols': 50})
  call term_wait(buf, 100)
  call VerifyScreenDump(buf, 'Test_balloon_eval_term_02', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XTest_beval_visual')
endfunc
