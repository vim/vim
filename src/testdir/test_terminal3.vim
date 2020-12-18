" Tests for the terminal window.
" This is split in two, because it can take a lot of time.
" See test_terminal.vim and test_terminal2.vim for further tests.

source check.vim
CheckFeature terminal

source shared.vim
source screendump.vim
source mouse.vim
source term_util.vim

let $PROMPT_COMMAND=''

func Test_terminal_altscreen()
  " somehow doesn't work on MS-Windows
  CheckUnix
  let cmd = "cat Xtext\<CR>"

  let buf = term_start(&shell, {})
  call writefile(["\<Esc>[?1047h"], 'Xtext')
  call term_sendkeys(buf, cmd)
  call WaitForAssert({-> assert_equal(1, term_getaltscreen(buf))})

  call writefile(["\<Esc>[?1047l"], 'Xtext')
  call term_sendkeys(buf, cmd)
  call WaitForAssert({-> assert_equal(0, term_getaltscreen(buf))})

  call term_sendkeys(buf, "exit\r")
  exe buf . "bwipe!"
  call delete('Xtext')
endfunc

func Test_terminal_shell_option()
  if has('unix')
    " exec is a shell builtin command, should fail without a shell.
    term exec ls runtest.vim
    call WaitForAssert({-> assert_match('job failed', term_getline(bufnr(), 1))})
    bwipe!

    term ++shell exec ls runtest.vim
    call WaitForAssert({-> assert_match('runtest.vim', term_getline(bufnr(), 1))})
    bwipe!
  elseif has('win32')
    " dir is a shell builtin command, should fail without a shell.
    " However, if dir.exe (which might be provided by Cygwin/MSYS2) exists in
    " the %PATH%, "term dir" succeeds unintentionally.  Use dir.com instead.
    try
      term dir.com /b runtest.vim
      call WaitForAssert({-> assert_match('job failed', term_getline(bufnr(), 1))})
    catch /CreateProcess/
      " ignore
    endtry
    bwipe!

    " This should execute the dir builtin command even with ".com".
    term ++shell dir.com /b runtest.vim
    call WaitForAssert({-> assert_match('runtest.vim', term_getline(bufnr(), 1))})
    bwipe!
  else
    throw 'Skipped: does not work on this platform'
  endif
endfunc

func Test_terminal_invalid_arg()
  call assert_fails('terminal ++xyz', 'E181:')
endfunc

func Test_terminal_in_popup()
  CheckRunVimInTerminal

  let text =<< trim END
    some text
    to edit
    in a popup window
  END
  call writefile(text, 'Xtext')
  let cmd = GetVimCommandCleanTerm()
  let lines = [
	\ 'call setline(1, range(20))',
	\ 'hi PopTerm ctermbg=grey',
	\ 'func OpenTerm(setColor)',
	\ "  set noruler",
	\ "  let s:buf = term_start('" .. cmd .. " Xtext', #{hidden: 1, term_finish: 'close'})",
	\ '  let g:winid = popup_create(s:buf, #{minwidth: 45, minheight: 7, border: [], drag: 1, resize: 1})',
	\ '  if a:setColor',
	\ '    call win_execute(g:winid, "set wincolor=PopTerm")',
	\ '  endif',
	\ 'endfunc',
	\ 'func HidePopup()',
	\ '  call popup_hide(g:winid)',
	\ 'endfunc',
	\ 'func ClosePopup()',
	\ '  call popup_close(g:winid)',
	\ 'endfunc',
	\ 'func ReopenPopup()',
	\ '  call popup_create(s:buf, #{minwidth: 40, minheight: 6, border: []})',
	\ 'endfunc',
	\ ]
  call writefile(lines, 'XtermPopup')
  let buf = RunVimInTerminal('-S XtermPopup', #{rows: 15})
  call TermWait(buf, 100)
  call term_sendkeys(buf, ":call OpenTerm(0)\<CR>")
  call TermWait(buf, 100)
  call term_sendkeys(buf, ":\<CR>")
  call TermWait(buf, 100)
  call term_sendkeys(buf, "\<C-W>:echo getwinvar(g:winid, \"&buftype\") win_gettype(g:winid)\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_1', {})

  call term_sendkeys(buf, ":q\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_2', {})
 
  call term_sendkeys(buf, ":call OpenTerm(1)\<CR>")
  call TermWait(buf, 150)
  call term_sendkeys(buf, ":set hlsearch\<CR>")
  call TermWait(buf, 100)
  call term_sendkeys(buf, "/edit\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_3', {})
 
  call term_sendkeys(buf, "\<C-W>:call HidePopup()\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_4', {})
  call term_sendkeys(buf, "\<CR>")
  call TermWait(buf, 50)

  call term_sendkeys(buf, "\<C-W>:call ClosePopup()\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_5', {})

  call term_sendkeys(buf, "\<C-W>:call ReopenPopup()\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_6', {})

  " Go to terminal-Normal mode and visually select text.
  call term_sendkeys(buf, "\<C-W>Ngg/in\<CR>vww")
  call VerifyScreenDump(buf, 'Test_terminal_popup_7', {})

  " Back to job mode, redraws
  call term_sendkeys(buf, "A")
  call VerifyScreenDump(buf, 'Test_terminal_popup_8', {})

  call TermWait(buf, 50)
  call term_sendkeys(buf, ":q\<CR>")
  call TermWait(buf, 150)  " wait for terminal to vanish

  call StopVimInTerminal(buf)
  call delete('Xtext')
  call delete('XtermPopup')
endfunc

" Check a terminal in popup window uses the default minimum size.
func Test_terminal_in_popup_min_size()
  CheckRunVimInTerminal

  let text =<< trim END
    another text
    to show
    in a popup window
  END
  call writefile(text, 'Xtext')
  let lines = [
	\ 'call setline(1, range(20))',
	\ 'func OpenTerm()',
	\ "  let s:buf = term_start('cat Xtext', #{hidden: 1})",
	\ '  let g:winid = popup_create(s:buf, #{ border: []})',
	\ 'endfunc',
	\ ]
  call writefile(lines, 'XtermPopup')
  let buf = RunVimInTerminal('-S XtermPopup', #{rows: 15})
  call TermWait(buf, 100)
  call term_sendkeys(buf, ":set noruler\<CR>")
  call term_sendkeys(buf, ":call OpenTerm()\<CR>")
  call TermWait(buf, 50)
  call term_sendkeys(buf, ":\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_m1', {})

  call TermWait(buf, 50)
  call term_sendkeys(buf, ":q\<CR>")
  call TermWait(buf, 50)  " wait for terminal to vanish
  call StopVimInTerminal(buf)
  call delete('Xtext')
  call delete('XtermPopup')
endfunc

" Check a terminal in popup window with different colors
func Terminal_in_popup_colored(group_name, highlight_cmd, highlight_opt)
  CheckRunVimInTerminal
  CheckUnix

  let lines = [
	\ 'call setline(1, range(20))',
	\ 'func OpenTerm()',
	\ "  let s:buf = term_start('cat', #{hidden: 1, "
	\ .. a:highlight_opt .. "})",
	\ '  let g:winid = popup_create(s:buf, #{ border: []})',
	\ 'endfunc',
	\ a:highlight_cmd,
	\ ]
  call writefile(lines, 'XtermPopup')
  let buf = RunVimInTerminal('-S XtermPopup', #{rows: 15})
  call TermWait(buf, 100)
  call term_sendkeys(buf, ":set noruler\<CR>")
  call term_sendkeys(buf, ":call OpenTerm()\<CR>")
  call TermWait(buf, 50)
  call term_sendkeys(buf, "hello\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_' .. a:group_name, {})

  call term_sendkeys(buf, "\<C-D>")
  call TermWait(buf, 50)
  call term_sendkeys(buf, ":q\<CR>")
  call TermWait(buf, 50)  " wait for terminal to vanish
  call StopVimInTerminal(buf)
  call delete('XtermPopup')
endfunc

func Test_terminal_in_popup_colored_Terminal()
  call Terminal_in_popup_colored("Terminal", "highlight Terminal ctermfg=blue ctermbg=yellow", "")
endfunc

func Test_terminal_in_popup_colored_group()
  call Terminal_in_popup_colored("MyTermCol", "highlight MyTermCol ctermfg=darkgreen ctermbg=lightblue", "term_highlight: 'MyTermCol',")
endfunc

func Test_double_popup_terminal()
  let buf1 = term_start(&shell, #{hidden: 1})
  let win1 = popup_create(buf1, {})
  let buf2 = term_start(&shell, #{hidden: 1})
  call assert_fails('call popup_create(buf2, {})', 'E861:')
  call popup_close(win1)
  exe buf1 .. 'bwipe!'
  exe buf2 .. 'bwipe!'
endfunc

func Test_issue_5607()
  let wincount = winnr('$')
  exe 'terminal' &shell &shellcmdflag 'exit'
  let job = term_getjob(bufnr())
  call WaitForAssert({-> assert_equal("dead", job_status(job))})

  let old_wincolor = &wincolor
  try
    set wincolor=
  finally
    let &wincolor = old_wincolor
    bw!
  endtry
endfunc

func Test_hidden_terminal()
  let buf = term_start(&shell, #{hidden: 1})
  call assert_equal('', bufname('^$'))
  call StopShellInTerminal(buf)
endfunc

func Test_term_nasty_callback()
  CheckExecutable sh

  set hidden
  let g:buf0 = term_start('sh', #{hidden: 1, term_finish: 'close'})
  call popup_create(g:buf0, {})
  call assert_fails("call term_start(['sh', '-c'], #{curwin: 1})", 'E863:')

  call popup_clear(1)
  set hidden&
endfunc

func Test_term_and_startinsert()
  CheckRunVimInTerminal
  CheckUnix

  let lines =<< trim EOL
     put='some text'
     term
     startinsert
  EOL
  call writefile(lines, 'XTest_startinsert')
  let buf = RunVimInTerminal('-S XTest_startinsert', {})

  call term_sendkeys(buf, "exit\r")
  call WaitForAssert({-> assert_equal("some text", term_getline(buf, 1))})
  call term_sendkeys(buf, "0l")
  call term_sendkeys(buf, "A<\<Esc>")
  call WaitForAssert({-> assert_equal("some text<", term_getline(buf, 1))})

  call StopVimInTerminal(buf)
  call delete('XTest_startinsert')
endfunc

" Test for passing invalid arguments to terminal functions
func Test_term_func_invalid_arg()
  call assert_fails('let b = term_getaltscreen([])', 'E745:')
  call assert_fails('let a = term_getattr(1, [])', 'E730:')
  call assert_fails('let c = term_getcursor([])', 'E745:')
  call assert_fails('let l = term_getline([], 1)', 'E745:')
  call assert_fails('let l = term_getscrolled([])', 'E745:')
  call assert_fails('let s = term_getsize([])', 'E745:')
  call assert_fails('let s = term_getstatus([])', 'E745:')
  call assert_fails('let s = term_scrape([], 1)', 'E745:')
  call assert_fails('call term_sendkeys([], "a")', 'E745:')
  call assert_fails('call term_setapi([], "")', 'E745:')
  call assert_fails('call term_setrestore([], "")', 'E745:')
  call assert_fails('call term_setkill([], "")', 'E745:')
  if has('gui') || has('termguicolors')
    call assert_fails('let p = term_getansicolors([])', 'E745:')
    call assert_fails('call term_setansicolors([], [])', 'E745:')
  endif
endfunc

" Test for sending various special keycodes to a terminal
func Test_term_keycode_translation()
  CheckRunVimInTerminal

  let buf = RunVimInTerminal('', {})
  call term_sendkeys(buf, ":set nocompatible\<CR>")

  let keys = ["\<F1>", "\<F2>", "\<F3>", "\<F4>", "\<F5>", "\<F6>", "\<F7>",
        \ "\<F8>", "\<F9>", "\<F10>", "\<F11>", "\<F12>", "\<Home>",
        \ "\<S-Home>", "\<C-Home>", "\<End>", "\<S-End>", "\<C-End>",
	\ "\<Ins>", "\<Del>", "\<Left>", "\<S-Left>", "\<C-Left>", "\<Right>",
        \ "\<S-Right>", "\<C-Right>", "\<Up>", "\<S-Up>", "\<Down>",
        \ "\<S-Down>"]
  let output = ['<F1>', '<F2>', '<F3>', '<F4>', '<F5>', '<F6>', '<F7>',
        \ '<F8>', '<F9>', '<F10>', '<F11>', '<F12>', '<Home>', '<S-Home>',
        \ '<C-Home>', '<End>', '<S-End>', '<C-End>', '<Insert>', '<Del>',
        \ '<Left>', '<S-Left>', '<C-Left>', '<Right>', '<S-Right>',
        \ '<C-Right>', '<Up>', '<S-Up>', '<Down>', '<S-Down>']

  call term_sendkeys(buf, "i")
  for i in range(len(keys))
    call term_sendkeys(buf, "\<C-U>\<C-K>" .. keys[i])
    call WaitForAssert({-> assert_equal(output[i], term_getline(buf, 1))})
  endfor

  let keypad_keys = ["\<k0>", "\<k1>", "\<k2>", "\<k3>", "\<k4>", "\<k5>",
        \ "\<k6>", "\<k7>", "\<k8>", "\<k9>", "\<kPoint>", "\<kPlus>",
        \ "\<kMinus>", "\<kMultiply>", "\<kDivide>"]
  let keypad_output = ['0', '1', '2', '3', '4', '5',
        \ '6', '7', '8', '9', '.', '+',
        \ '-', '*', '/']
  for i in range(len(keypad_keys))
    " TODO: Mysteriously keypad 3 and 9 do not work on some systems.
    if keypad_output[i] == '3' || keypad_output[i] == '9'
      continue
    endif
    call term_sendkeys(buf, "\<C-U>" .. keypad_keys[i])
    call WaitForAssert({-> assert_equal(keypad_output[i], term_getline(buf, 1))})
  endfor

  call feedkeys("\<C-U>\<kEnter>\<BS>one\<C-W>.two", 'xt')
  call WaitForAssert({-> assert_equal('two', term_getline(buf, 1))})

  call StopVimInTerminal(buf)
endfunc

" Test for using the mouse in a terminal
func Test_term_mouse()
  CheckNotGui
  CheckRunVimInTerminal

  let save_mouse = &mouse
  let save_term = &term
  let save_ttymouse = &ttymouse
  let save_clipboard = &clipboard
  set mouse=a term=xterm ttymouse=sgr mousetime=200 clipboard=

  let lines =<< trim END
    one two three four five
    red green yellow red blue
    vim emacs sublime nano
  END
  call writefile(lines, 'Xtest_mouse')

  " Create a terminal window running Vim for the test with mouse enabled
  let prev_win = win_getid()
  let buf = RunVimInTerminal('Xtest_mouse -n', {})
  call term_sendkeys(buf, ":set nocompatible\<CR>")
  call term_sendkeys(buf, ":set mouse=a term=xterm ttymouse=sgr\<CR>")
  call term_sendkeys(buf, ":set clipboard=\<CR>")
  call term_sendkeys(buf, ":set mousemodel=extend\<CR>")
  call TermWait(buf)
  redraw!

  " Use the mouse to enter the terminal window
  call win_gotoid(prev_win)
  call feedkeys(MouseLeftClickCode(1, 1), 'x')
  call feedkeys(MouseLeftReleaseCode(1, 1), 'x')
  call assert_equal(1, getwininfo(win_getid())[0].terminal)

  " Test for <LeftMouse> click/release
  call test_setmouse(2, 5)
  call feedkeys("\<LeftMouse>\<LeftRelease>", 'xt')
  call test_setmouse(3, 8)
  call term_sendkeys(buf, "\<LeftMouse>\<LeftRelease>")
  call TermWait(buf, 50)
  call term_sendkeys(buf, ":call writefile([json_encode(getpos('.'))], 'Xbuf')\<CR>")
  call TermWait(buf, 50)
  let pos = json_decode(readfile('Xbuf')[0])
  call assert_equal([3, 8], pos[1:2])

  " Test for selecting text using mouse
  call delete('Xbuf')
  call test_setmouse(2, 11)
  call term_sendkeys(buf, "\<LeftMouse>")
  call test_setmouse(2, 16)
  call term_sendkeys(buf, "\<LeftRelease>y")
  call TermWait(buf, 50)
  call term_sendkeys(buf, ":call writefile([@\"], 'Xbuf')\<CR>")
  call TermWait(buf, 50)
  call assert_equal('yellow', readfile('Xbuf')[0])

  " Test for selecting text using doubleclick
  call delete('Xbuf')
  call test_setmouse(1, 11)
  call term_sendkeys(buf, "\<LeftMouse>\<LeftRelease>\<LeftMouse>")
  call test_setmouse(1, 17)
  call term_sendkeys(buf, "\<LeftRelease>y")
  call TermWait(buf, 50)
  call term_sendkeys(buf, ":call writefile([@\"], 'Xbuf')\<CR>")
  call TermWait(buf, 50)
  call assert_equal('three four', readfile('Xbuf')[0])

  " Test for selecting a line using triple click
  call delete('Xbuf')
  call test_setmouse(3, 2)
  call term_sendkeys(buf, "\<LeftMouse>\<LeftRelease>\<LeftMouse>\<LeftRelease>\<LeftMouse>\<LeftRelease>y")
  call TermWait(buf, 50)
  call term_sendkeys(buf, ":call writefile([@\"], 'Xbuf')\<CR>")
  call TermWait(buf, 50)
  call assert_equal("vim emacs sublime nano\n", readfile('Xbuf')[0])

  " Test for selecting a block using qudraple click
  call delete('Xbuf')
  call test_setmouse(1, 11)
  call term_sendkeys(buf, "\<LeftMouse>\<LeftRelease>\<LeftMouse>\<LeftRelease>\<LeftMouse>\<LeftRelease>\<LeftMouse>")
  call test_setmouse(3, 13)
  call term_sendkeys(buf, "\<LeftRelease>y")
  call TermWait(buf, 50)
  call term_sendkeys(buf, ":call writefile([@\"], 'Xbuf')\<CR>")
  call TermWait(buf, 50)
  call assert_equal("ree\nyel\nsub", readfile('Xbuf')[0])

  " Test for extending a selection using right click
  call delete('Xbuf')
  call test_setmouse(2, 9)
  call term_sendkeys(buf, "\<LeftMouse>\<LeftRelease>")
  call test_setmouse(2, 16)
  call term_sendkeys(buf, "\<RightMouse>\<RightRelease>y")
  call TermWait(buf, 50)
  call term_sendkeys(buf, ":call writefile([@\"], 'Xbuf')\<CR>")
  call TermWait(buf, 50)
  call assert_equal("n yellow", readfile('Xbuf')[0])

  " Test for pasting text using middle click
  call delete('Xbuf')
  call term_sendkeys(buf, ":let @r='bright '\<CR>")
  call test_setmouse(2, 22)
  call term_sendkeys(buf, "\"r\<MiddleMouse>\<MiddleRelease>")
  call TermWait(buf, 50)
  call term_sendkeys(buf, ":call writefile([getline(2)], 'Xbuf')\<CR>")
  call TermWait(buf, 50)
  call assert_equal("red bright blue", readfile('Xbuf')[0][-15:])

  " cleanup
  call TermWait(buf)
  call StopVimInTerminal(buf)
  let &mouse = save_mouse
  let &term = save_term
  let &ttymouse = save_ttymouse
  let &clipboard = save_clipboard
  set mousetime&
  call delete('Xtest_mouse')
  call delete('Xbuf')
endfunc

" Test for modeless selection in a terminal
func Test_term_modeless_selection()
  CheckUnix
  CheckNotGui
  CheckRunVimInTerminal
  CheckFeature clipboard_working

  let save_mouse = &mouse
  let save_term = &term
  let save_ttymouse = &ttymouse
  set mouse=a term=xterm ttymouse=sgr mousetime=200
  set clipboard=autoselectml

  let lines =<< trim END
    one two three four five
    red green yellow red blue
    vim emacs sublime nano
  END
  call writefile(lines, 'Xtest_modeless')

  " Create a terminal window running Vim for the test with mouse disabled
  let prev_win = win_getid()
  let buf = RunVimInTerminal('Xtest_modeless -n', {})
  call term_sendkeys(buf, ":set nocompatible\<CR>")
  call term_sendkeys(buf, ":set mouse=\<CR>")
  call TermWait(buf)
  redraw!

  " Use the mouse to enter the terminal window
  call win_gotoid(prev_win)
  call feedkeys(MouseLeftClickCode(1, 1), 'x')
  call feedkeys(MouseLeftReleaseCode(1, 1), 'x')
  call TermWait(buf)
  call assert_equal(1, getwininfo(win_getid())[0].terminal)

  " Test for copying a modeless selection to clipboard
  let @* = 'clean'
  " communicating with X server may take a little time
  sleep 100m
  call feedkeys(MouseLeftClickCode(2, 3), 'x')
  call feedkeys(MouseLeftDragCode(2, 11), 'x')
  call feedkeys(MouseLeftReleaseCode(2, 11), 'x')
  call assert_equal("d green y", @*)

  " cleanup
  call TermWait(buf)
  call StopVimInTerminal(buf)
  let &mouse = save_mouse
  let &term = save_term
  let &ttymouse = save_ttymouse
  set mousetime& clipboard&
  call delete('Xtest_modeless')
  new | only!
endfunc

func Test_terminal_getwinpos()
  CheckRunVimInTerminal

  " split, go to the bottom-right window
  split
  wincmd j
  set splitright

  let buf = RunVimInTerminal('', {'cols': 60})
  call TermWait(buf, 100)
  call term_sendkeys(buf, ":echo getwinpos(500)\<CR>")

  " Find the output of getwinpos() in the bottom line.
  let rows = term_getsize(buf)[0]
  call WaitForAssert({-> assert_match('\[\d\+, \d\+\]', term_getline(buf, rows))})
  let line = term_getline(buf, rows)
  let xpos = str2nr(substitute(line, '\[\(\d\+\), \d\+\]', '\1', ''))
  let ypos = str2nr(substitute(line, '\[\d\+, \(\d\+\)\]', '\1', ''))

  " Position must be bigger than the getwinpos() result of Vim itself.
  " The calculation in the console assumes a 10 x 7 character cell.
  " In the GUI it can be more, let's assume a 20 x 14 cell.
  " And then add 100 / 200 tolerance.
  let [xroot, yroot] = getwinpos()
  let winpos = 50->getwinpos()
  call assert_equal(xroot, winpos[0])
  call assert_equal(yroot, winpos[1])
  let [winrow, wincol] = win_screenpos(0)
  let xoff = wincol * (has('gui_running') ? 14 : 7) + 100
  let yoff = winrow * (has('gui_running') ? 20 : 10) + 200
  call assert_inrange(xroot + 2, xroot + xoff, xpos)
  call assert_inrange(yroot + 2, yroot + yoff, ypos)

  call TermWait(buf)
  call term_sendkeys(buf, ":q\<CR>")
  call StopVimInTerminal(buf)
  set splitright&
  only!
endfunc


" vim: shiftwidth=2 sts=2 expandtab
