" Tests for 'balloonevalterm'.

" Tests that only work in the terminal.
if has('balloon_eval_term') && !has('gui_running')

source screendump.vim
if !CanRunVimInTerminal()
  finish
endif

let s:common_script = [
	\ 'call setline(1, ["one one one", "two tXo two", "three three three"])',
	\ 'set balloonevalterm balloonexpr=MyBalloonExpr() balloondelay=100',
	\ 'func MyBalloonExpr()',
	\ ' return "line " .. v:beval_lnum .. " column " .. v:beval_col .. ": " .. v:beval_text',
	\ 'endfun',
	\ 'redraw',
	\ ]

func Test_balloon_eval_term()
  " Use <Ignore> after <MouseMove> to return from vgetc() without removing
  " the balloon.
  call writefile(s:common_script + [
	\ 'call test_setmouse(2, 6)',
	\ 'call feedkeys("\<MouseMove>\<Ignore>", "xt")',
	\ ], 'XTest_beval')

  " Check that the balloon shows up after a mouse move
  let buf = RunVimInTerminal('-S XTest_beval', {'rows': 10, 'cols': 50})
  call term_wait(buf, 100)
  call VerifyScreenDump(buf, 'Test_balloon_eval_term_01', {})

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

endif

" Tests that only work in the GUI
if has('gui_running')

func Test_balloon_show_gui()
  let msg = 'this this this this'
  call balloon_show(msg)
  call assert_equal(msg, balloon_gettext())
  sleep 10m
  call balloon_show('')

  let msg = 'that that'
  call balloon_show(msg)
  call assert_equal(msg, balloon_gettext())
  sleep 10m
  call balloon_show('')
endfunc

endif
