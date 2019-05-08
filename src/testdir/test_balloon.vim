" Tests for 'balloonevalterm'.

if !has('balloon_eval_term') || has('gui_running')
  finish
endif

source screendump.vim
if !CanRunVimInTerminal()
  finish
endif

func Test_balloon_eval_term()
  call writefile([
	\ 'call setline(1, ["one one one", "two tXo two", "three three three"])',
	\ 'set balloonevalterm balloonexpr=MyBalloonExpr() balloondelay=100',
	\ 'func MyBalloonExpr()',
	\ ' return "line " . v:beval_lnum . " column " . v:beval_col',
	\ 'endfun',
	\ 'redraw',
	\ 'call test_setmouse(2, 6)',
	\ 'call feedkeys("\<MouseMove>\<Ignore>", "xt")',
	\ ], 'XTest_beval')

  " Check that the balloon shows up
  let buf = RunVimInTerminal('-S XTest_beval', {'rows': 10, 'cols': 50})
  call term_wait(buf, 100)
  call VerifyScreenDump(buf, 'Test_balloon_eval_term_01', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XTest_beval')
endfunc
