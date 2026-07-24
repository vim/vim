" Tests for 'conceal'.

CheckFeature conceal

source util/screendump.vim
source util/mouse.vim

func Test_conceal_two_windows()
  CheckScreendump

  let code =<< trim [CODE]
    let lines = ["one one one one one", "two |hidden| here", "three |hidden| three"]
    call setline(1, lines)
    syntax match test /|hidden|/ conceal
    set conceallevel=2
    set concealcursor=
    exe "normal /here\r"
    new
    call setline(1, lines)
    call setline(4, "Second window")
    syntax match test /|hidden|/ conceal
    set conceallevel=2
    set concealcursor=nc
    exe "normal /here\r"
  [CODE]

  call writefile(code, 'XTest_conceal', 'D')
  " Check that cursor line is concealed
  let buf = RunVimInTerminal('-S XTest_conceal', {})
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_01', {})

  " Check that with concealed text vertical cursor movement is correct.
  call term_sendkeys(buf, "k")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_02', {})

  " Check that with cursor line is not concealed
  call term_sendkeys(buf, "j")
  call term_sendkeys(buf, ":set concealcursor=\r")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_03', {})

  " Check that with cursor line is not concealed when moving cursor down
  call term_sendkeys(buf, "j")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_04', {})

  " Check that with cursor line is not concealed when switching windows
  call term_sendkeys(buf, "\<C-W>\<C-W>")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_05', {})

  " Check that with cursor line is only concealed in Normal mode
  call term_sendkeys(buf, ":set concealcursor=n\r")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_06n', {})
  call term_sendkeys(buf, "a")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_06i', {})
  call term_sendkeys(buf, "\<Esc>/e")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_06c', {})
  call term_sendkeys(buf, "\<Esc>v")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_06v', {})
  call term_sendkeys(buf, "\<Esc>")

  " Check that with cursor line is only concealed in Insert mode
  call term_sendkeys(buf, ":set concealcursor=i\r")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_07n', {})
  call term_sendkeys(buf, "14|a")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_07i', {})
  call term_sendkeys(buf, "\<Esc>")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_07in', {})
  call term_sendkeys(buf, "/e")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_07c', {})
  call term_sendkeys(buf, "\<Esc>v")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_07v', {})
  call term_sendkeys(buf, "\<Esc>")

  " Check that with cursor line is only concealed in Command mode
  call term_sendkeys(buf, ":set concealcursor=c\r")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_08n', {})
  call term_sendkeys(buf, "a")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_08i', {})
  call term_sendkeys(buf, "\<Esc>/e")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_08c', {})
  call term_sendkeys(buf, "\<Esc>v")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_08v', {})
  call term_sendkeys(buf, "\<Esc>")

  " Check that with cursor line is only concealed in Visual mode
  call term_sendkeys(buf, ":set concealcursor=v\r")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_09n', {})
  call term_sendkeys(buf, "a")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_09i', {})
  call term_sendkeys(buf, "\<Esc>/e")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_09c', {})
  call term_sendkeys(buf, "\<Esc>v")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_09v', {})
  call term_sendkeys(buf, "\<Esc>")

  " Check moving the cursor while in insert mode.
  call term_sendkeys(buf, ":set concealcursor=\r")
  call term_sendkeys(buf, "a")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_10', {})
  call term_sendkeys(buf, "\<Down>")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_11', {})
  call term_sendkeys(buf, "\<Esc>")

  " Check the "o" command
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_12', {})
  call term_sendkeys(buf, "o")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_13', {})
  call term_sendkeys(buf, "\<Esc>")

  " clean up
  call StopVimInTerminal(buf)
endfunc

func Test_conceal_with_cursorline()
  CheckScreendump

  " Opens a help window, where 'conceal' is set, switches to the other window
  " where 'cursorline' needs to be updated when the cursor moves.
  let code =<< trim [CODE]
    set cursorline
    normal othis is a test
    new
    call setline(1, ["one", "two", "three", "four", "five"])
    set ft=help
    normal M
  [CODE]

  call writefile(code, 'XTest_conceal_cul', 'D')
  let buf = RunVimInTerminal('-S XTest_conceal_cul', {})
  call VerifyScreenDump(buf, 'Test_conceal_cul_01', {})

  call term_sendkeys(buf, ":wincmd w\r")
  call VerifyScreenDump(buf, 'Test_conceal_cul_02', {})

  call term_sendkeys(buf, "k")
  call VerifyScreenDump(buf, 'Test_conceal_cul_03', {})

  " clean up
  call StopVimInTerminal(buf)
endfunc

func Test_conceal_with_cursorcolumn()
  CheckScreendump

  " Check that cursorcolumn and colorcolumn don't get broken in presence of
  " wrapped lines containing concealed text
  let code =<< trim [CODE]
    let lines = ["one one one |hidden| one one one one one one one one",
          \ "two two two two |hidden| here two two",
          \ "three |hidden| three three three three three three three three"]
    call setline(1, lines)
    set wrap linebreak
    let &showbreak = ' >>> '
    syntax match test /|hidden|/ conceal
    set conceallevel=2
    set concealcursor=
    exe "normal /here\r"
    set cursorcolumn
    set colorcolumn=50
  [CODE]

  call writefile(code, 'XTest_conceal_cuc', 'D')
  let buf = RunVimInTerminal('-S XTest_conceal_cuc', {'rows': 10, 'cols': 40})
  call VerifyScreenDump(buf, 'Test_conceal_cuc_01', {})

  " move cursor to the end of line (the cursor jumps to the next screen line)
  call term_sendkeys(buf, "$")
  call VerifyScreenDump(buf, 'Test_conceal_cuc_02', {})

  " clean up
  call StopVimInTerminal(buf)
endfunc

" Check that 'cursorline' and 'wincolor' apply to the whole line in presence
" of wrapped lines containing concealed text.
func Test_conceal_wrapped_cursorline_wincolor()
  CheckScreendump

  let code =<< trim [CODE]
    call setline(1, 'one one one |hidden| one one one one one one one one')
    syntax match test /|hidden|/ conceal
    set conceallevel=2 concealcursor=n cursorline
    normal! g$
  [CODE]

  call writefile(code, 'XTest_conceal_cul_wcr', 'D')
  let buf = RunVimInTerminal('-S XTest_conceal_cul_wcr', {'rows': 4, 'cols': 40})
  call VerifyScreenDump(buf, 'Test_conceal_cul_wcr_01', {})

  call term_sendkeys(buf, ":set wincolor=ErrorMsg\n")
  call VerifyScreenDump(buf, 'Test_conceal_cul_wcr_02', {})

  call term_sendkeys(buf, ":set nocursorline\n")
  call VerifyScreenDump(buf, 'Test_conceal_cul_wcr_03', {})

  " clean up
  call StopVimInTerminal(buf)
endfunc

" Same as Test_conceal_wrapped_cursorline_wincolor(), but with 'rightleft'.
func Test_conceal_wrapped_cursorline_wincolor_rightleft()
  CheckFeature rightleft
  CheckScreendump

  let code =<< trim [CODE]
    call setline(1, 'one one one |hidden| one one one one one one one one')
    syntax match test /|hidden|/ conceal
    set conceallevel=2 concealcursor=n cursorline rightleft
    normal! g$
  [CODE]

  call writefile(code, 'XTest_conceal_cul_wcr_rl', 'D')
  let buf = RunVimInTerminal('-S XTest_conceal_cul_wcr_rl', {'rows': 4, 'cols': 40})
  call VerifyScreenDump(buf, 'Test_conceal_cul_wcr_rl_01', {})

  call term_sendkeys(buf, ":set wincolor=ErrorMsg\n")
  call VerifyScreenDump(buf, 'Test_conceal_cul_wcr_rl_02', {})

  call term_sendkeys(buf, ":set nocursorline\n")
  call VerifyScreenDump(buf, 'Test_conceal_cul_wcr_rl_03', {})

  " clean up
  call StopVimInTerminal(buf)
endfunc

func Test_conceal_resize_term()
  CheckScreendump

  let code =<< trim [CODE]
    call setline(1, '`one` `two` `three` `four` `five`, the backticks should be concealed')
    setl cocu=n cole=3
    syn region CommentCodeSpan matchgroup=Comment start=/`/ end=/`/ concealends
    normal fb
  [CODE]
  call writefile(code, 'XTest_conceal_resize', 'D')
  let buf = RunVimInTerminal('-S XTest_conceal_resize', {'rows': 6})
  call VerifyScreenDump(buf, 'Test_conceal_resize_01', {})

  call win_execute(buf->win_findbuf()[0], 'wincmd +')
  call VerifyScreenDump(buf, 'Test_conceal_resize_02', {})

  " clean up
  call StopVimInTerminal(buf)
endfunc

func Test_conceal_linebreak()
  CheckScreendump

  let code =<< trim [CODE]
      vim9script
      &wrap = true
      &conceallevel = 2
      &concealcursor = 'nc'
      &linebreak = true
      &showbreak = '+ '
      var line: string = 'a`a`a`a`'
          .. 'a'->repeat(&columns - 15)
          .. ' b`b`'
          .. 'b'->repeat(&columns - 10)
          .. ' cccccc'
      ['x'->repeat(&columns), '', line]->setline(1)
      syntax region CodeSpan matchgroup=Delimiter start=/\z(`\+\)/ end=/\z1/ concealends
  [CODE]
  call writefile(code, 'XTest_conceal_linebreak', 'D')
  let buf = RunVimInTerminal('-S XTest_conceal_linebreak', {'rows': 8})
  call VerifyScreenDump(buf, 'Test_conceal_linebreak_1', {})

  " clean up
  call StopVimInTerminal(buf)
endfunc

" Tests for correct display (cursor column position) with +conceal and
" tabulators.  Need to run this test in a separate Vim instance. Otherwise the
" screen is not updated (lazy redraw) and the cursor position is wrong.
func Test_conceal_cursor_pos()
  let code =<< trim [CODE]
    :let l = ['start:', '.concealed.     text', "|concealed|\ttext"]
    :let l += ['', "\t.concealed.\ttext", "\t|concealed|\ttext", '']
    :let l += [".a.\t.b.\t.c.\t.d.", "|a|\t|b|\t|c|\t|d|"]
    :call append(0, l)
    :call cursor(1, 1)
    :" Conceal settings.
    :set conceallevel=2
    :set concealcursor=nc
    :syntax match test /|/ conceal
    :" Save current cursor position. Only works in <expr> mode, can't be used
    :" with :normal because it moves the cursor to the command line. Thanks
    :" to ZyX <zyx.vim@gmail.com> for the idea to use an <expr> mapping.
    :let curpos = []
    :nnoremap <expr> GG ":let curpos += ['".screenrow().":".screencol()."']\n"
    :normal ztj
    GGk
    :" We should end up in the same column when running these commands on the
    :" two lines.
    :normal ft
    GGk
    :normal $
    GGk
    :normal 0j
    GGk
    :normal ft
    GGk
    :normal $
    GGk
    :normal 0j0j
    GGk
    :" Same for next test block.
    :normal ft
    GGk
    :normal $
    GGk
    :normal 0j
    GGk
    :normal ft
    GGk
    :normal $
    GGk
    :normal 0j0j
    GGk
    :" And check W with multiple tabs and conceals in a line.
    :normal W
    GGk
    :normal W
    GGk
    :normal W
    GGk
    :normal $
    GGk
    :normal 0j
    GGk
    :normal W
    GGk
    :normal W
    GGk
    :normal W
    GGk
    :normal $
    GGk
    :set lbr
    :normal $
    GGk
    :set list listchars=tab:>-
    :normal 0
    GGk
    :normal W
    GGk
    :normal W
    GGk
    :normal W
    GGk
    :normal $
    GGk
    :call writefile(curpos, 'Xconceal_curpos.out')
    :q!

  [CODE]
  call writefile(code, 'XTest_conceal_curpos', 'D')

  if RunVim([], [], '-s XTest_conceal_curpos')
    call assert_equal([
          \ '2:1', '2:17', '2:20', '3:1', '3:17', '3:20', '5:8', '5:25',
          \ '5:28', '6:8', '6:25', '6:28', '8:1', '8:9', '8:17', '8:25',
          \ '8:27', '9:1', '9:9', '9:17', '9:25', '9:26', '9:26', '9:1',
          \ '9:9', '9:17', '9:25', '9:26'], readfile('Xconceal_curpos.out'))
  endif

  call delete('Xconceal_curpos.out')
endfunc

func Test_conceal_eol()
  new!
  setlocal concealcursor=n conceallevel=1
  call setline(1, ["x", ""])
  call matchaddpos('Conceal', [[2, 1, 1]], 2, -1, {'conceal': 1})
  redraw!

  call assert_notequal(screenchar(1, 1), screenchar(2, 2))
  call assert_equal(screenattr(1, 1), screenattr(1, 2))
  call assert_equal(screenattr(1, 2), screenattr(2, 2))
  call assert_equal(screenattr(2, 1), screenattr(2, 2))

  set list
  redraw!

  call assert_equal(screenattr(1, 1), screenattr(2, 2))
  call assert_notequal(screenattr(1, 1), screenattr(1, 2))
  call assert_notequal(screenattr(1, 2), screenattr(2, 1))

  set nolist
endfunc

func Test_conceal_mouse_click()
  call NewWindow(10, 40)
  set mouse=a
  setlocal conceallevel=2 concealcursor=nc
  syn match Concealed "this" conceal
  hi link Concealed Search

  " Test with both 'nocursorline' and 'cursorline', as they use two different
  " code paths to set virtual columns for the cells to clear.
  for cul in [v:false, v:true]
    let &l:cursorline = cul

    call setline(1, 'conceal this click here')
    call assert_equal([
          \ 'conceal  click here                     ',
          \ ], ScreenLines(1, 40))

    " Click on the space between "this" and "click" puts cursor there.
    call test_setmouse(1, 9)
    call feedkeys("\<LeftMouse>", "tx")
    call assert_equal([0, 1, 13, 0, 13], getcurpos())
    " Click on 'h' of "here" puts cursor there.
    call test_setmouse(1, 16)
    call feedkeys("\<LeftMouse>", "tx")
    call assert_equal([0, 1, 20, 0, 20], getcurpos())
    " Click on 'e' of "here" puts cursor there.
    call test_setmouse(1, 19)
    call feedkeys("\<LeftMouse>", "tx")
    call assert_equal([0, 1, 23, 0, 23], getcurpos())
    " Click after end of line puts cursor on 'e' without 'virtualedit'.
    call test_setmouse(1, 20)
    call feedkeys("\<LeftMouse>", "tx")
    call assert_equal([0, 1, 23, 0, 24], getcurpos())
    call test_setmouse(1, 21)
    call feedkeys("\<LeftMouse>", "tx")
    call assert_equal([0, 1, 23, 0, 25], getcurpos())
    call test_setmouse(1, 22)
    call feedkeys("\<LeftMouse>", "tx")
    call assert_equal([0, 1, 23, 0, 26], getcurpos())
    call test_setmouse(1, 31)
    call feedkeys("\<LeftMouse>", "tx")
    call assert_equal([0, 1, 23, 0, 35], getcurpos())
    call test_setmouse(1, 32)
    call feedkeys("\<LeftMouse>", "tx")
    call assert_equal([0, 1, 23, 0, 36], getcurpos())

    set virtualedit=all
    redraw
    " Click on the space between "this" and "click" puts cursor there.
    call test_setmouse(1, 9)
    call feedkeys("\<LeftMouse>", "tx")
    call assert_equal([0, 1, 13, 0, 13], getcurpos())
    " Click on 'h' of "here" puts cursor there.
    call test_setmouse(1, 16)
    call feedkeys("\<LeftMouse>", "tx")
    call assert_equal([0, 1, 20, 0, 20], getcurpos())
    " Click on 'e' of "here" puts cursor there.
    call test_setmouse(1, 19)
    call feedkeys("\<LeftMouse>", "tx")
    call assert_equal([0, 1, 23, 0, 23], getcurpos())
    " Click after end of line puts cursor there with 'virtualedit'.
    call test_setmouse(1, 20)
    call feedkeys("\<LeftMouse>", "tx")
    call assert_equal([0, 1, 24, 0, 24], getcurpos())
    call test_setmouse(1, 21)
    call feedkeys("\<LeftMouse>", "tx")
    call assert_equal([0, 1, 24, 1, 25], getcurpos())
    call test_setmouse(1, 22)
    call feedkeys("\<LeftMouse>", "tx")
    call assert_equal([0, 1, 24, 2, 26], getcurpos())
    call test_setmouse(1, 31)
    call feedkeys("\<LeftMouse>", "tx")
    call assert_equal([0, 1, 24, 11, 35], getcurpos())
    call test_setmouse(1, 32)
    call feedkeys("\<LeftMouse>", "tx")
    call assert_equal([0, 1, 24, 12, 36], getcurpos())
    " Behavior should also be the same with 'colorcolumn'.
    setlocal colorcolumn=30
    redraw
    call test_setmouse(1, 31)
    call feedkeys("\<LeftMouse>", "tx")
    call assert_equal([0, 1, 24, 11, 35], getcurpos())
    call test_setmouse(1, 32)
    call feedkeys("\<LeftMouse>", "tx")
    call assert_equal([0, 1, 24, 12, 36], getcurpos())
    setlocal colorcolumn&

    if has('rightleft')
      setlocal rightleft
      call assert_equal([
            \ '                     ereh kcilc  laecnoc',
            \ ], ScreenLines(1, 40))
      " Click on the space between "this" and "click" puts cursor there.
      call test_setmouse(1, 41 - 9)
      call feedkeys("\<LeftMouse>", "tx")
      call assert_equal([0, 1, 13, 0, 13], getcurpos())
      " Click on 'h' of "here" puts cursor there.
      call test_setmouse(1, 41 - 16)
      call feedkeys("\<LeftMouse>", "tx")
      call assert_equal([0, 1, 20, 0, 20], getcurpos())
      " Click on 'e' of "here" puts cursor there.
      call test_setmouse(1, 41 - 19)
      call feedkeys("\<LeftMouse>", "tx")
      call assert_equal([0, 1, 23, 0, 23], getcurpos())
      " Click after end of line puts cursor there with 'virtualedit'.
      call test_setmouse(1, 41 - 20)
      call feedkeys("\<LeftMouse>", "tx")
      call assert_equal([0, 1, 24, 0, 24], getcurpos())
      call test_setmouse(1, 41 - 21)
      call feedkeys("\<LeftMouse>", "tx")
      call assert_equal([0, 1, 24, 1, 25], getcurpos())
      call test_setmouse(1, 41 - 22)
      call feedkeys("\<LeftMouse>", "tx")
      call assert_equal([0, 1, 24, 2, 26], getcurpos())
      call test_setmouse(1, 41 - 31)
      call feedkeys("\<LeftMouse>", "tx")
      call assert_equal([0, 1, 24, 11, 35], getcurpos())
      call test_setmouse(1, 41 - 32)
      call feedkeys("\<LeftMouse>", "tx")
      call assert_equal([0, 1, 24, 12, 36], getcurpos())
      setlocal rightleft&
    endif

    set virtualedit&

    " Test with a wrapped line.
    call setline(1, ['conceal this click here']->repeat(3)->join())
    call assert_equal([
          \ 'conceal  click here conceal  cli        ',
          \ 'ck here conceal  click here             ',
          \ ], ScreenLines([1, 2], 40))
    " Click on boguscols puts cursor on the last char of a screen line.
    for col in range(33, 40)
      call test_setmouse(1, col)
      call feedkeys("\<LeftMouse>", "tx")
      call assert_equal([0, 1, 40, 0, 40], getcurpos())
    endfor

    " Also test with the last char of a screen line concealed.
    setlocal number signcolumn=yes
    call assert_equal([
          \ '    1 conceal  click here conceal       ',
          \ '       click here conceal  click h      ',
          \ '      ere                               ',
          \ ], ScreenLines([1, 3], 40))
    call test_setmouse(1, 34)
    call feedkeys("\<LeftMouse>", "tx")
    call assert_equal([0, 1, 32, 0, 32], getcurpos())
    call test_setmouse(2, 7)
    call feedkeys("\<LeftMouse>", "tx")
    call assert_equal([0, 1, 37, 0, 37], getcurpos())
    " Click on boguscols puts cursor on the last char of a screen line.
    for col in range(35, 40)
      call test_setmouse(1, col)
      call feedkeys("\<LeftMouse>", "tx")
      call assert_equal([0, 1, 34, 0, 34], getcurpos())
      call test_setmouse(2, col)
      call feedkeys("\<LeftMouse>", "tx")
      call assert_equal([0, 1, 68, 0, 68], getcurpos())
    endfor
    setlocal number& signcolumn&
  endfor

  call CloseWindow()
  set mouse&
endfunc

func Test_conceallevel_three_wrapped_mouse_click()
  call NewWindow(10, 80)
  set mouse=a
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nc
  syntax region testCode matchgroup=testTick start=/`/ end=/`/ concealends

  let line = 'The inline code `printf("日本語 %s", value)` should hide its backticks when Markdown conceal is active, while this escaped marker \# should display as a literal hash and not make following wrapped text appear one cell early or late.'
  call setline(1, line)
  redraw

  let target_col = stridx(line, 'its') + 1
  let pos = screenpos(0, 1, target_col)
  call assert_true(pos.row > 0)

  setlocal cursorline
  call test_setmouse(pos.row, pos.curscol)
  call feedkeys("\<LeftMouse>", "tx")
  call assert_equal([0, 1, target_col, 0, pos.col], getcurpos())

  syntax clear testCode
  call CloseWindow()
  set mouse&
endfunc

func Test_conceallevel_three_wrapped_mouse_click_after_concealends()
  call NewWindow(7, 59)
  set mouse=a
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nc
        \ signcolumn=no nonumber showbreak= scrolloff=0
  syntax region testHtml matchgroup=test start=/<code>/ end=/<\/code>/ concealends
  syntax region testPre matchgroup=test start=/<pre>/ end=/<\/pre>/ concealends

  let line = 'Here is an HTML code tag: <code>wide_value = "日本語コンシール"</code> and a pre tag: <pre>alpha beta 日本語 gamma delta</pre> followed by enough plain text to make the line wrap after the concealed tag boundaries.'
  call setline(1, line)
  redraw

  let target_col = stridx(line, 'alpha beta') + 1
  let pos = screenpos(0, 1, target_col)
  call assert_true(pos.row > 0)

  call test_setmouse(pos.row, pos.curscol)
  call feedkeys("\<LeftMouse>", "tx")
  call assert_equal(target_col, col('.'))

  syntax clear testHtml
  syntax clear testPre
  call CloseWindow()
  set mouse&
endfunc

func Test_conceallevel_three_terminal_mouse_protocols()
  CheckNotGui
  CheckUnix

  let save_mouse = &mouse
  let save_mousetime = &mousetime
  let save_term = &term
  let save_ttymouse = &ttymouse

  call test_override('no_query_mouse', 1)
  call NewWindow(10, 40)
  try
    set mouse=a mousetime=0 term=xterm
    call WaitForResponses()
    setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
          \ signcolumn=no nonumber
    syntax match Hidden /HIDDEN / conceal

    let line = repeat('a', 42)
          \ .. ' HIDDEN target words after hidden text to force wrapping'
          \ .. ' and mapping checks'
    call setline(1, line)
    redraw!

    let target_col = stridx(line, 'target') + 1
    let target = screenpos(0, 1, target_col)
    call assert_true(target.row > 0)

    for ttymouse_val in g:Ttymouse_values + g:Ttymouse_dec + g:Ttymouse_netterm
      let msg = 'ttymouse=' .. ttymouse_val
      exe 'set ttymouse=' .. ttymouse_val
      call cursor(1, 1)
      redraw!

      call MouseLeftClick(target.row, target.curscol)
      call MouseLeftRelease(target.row, target.curscol)
      call assert_equal(target_col, col('.'), msg)
    endfor
  finally
    syntax clear Hidden
    call CloseWindow()
    let &ttymouse = save_ttymouse
    let &term = save_term
    let &mousetime = save_mousetime
    let &mouse = save_mouse
    call test_override('no_query_mouse', 0)
  endtry
endfunc

func Test_conceallevel_three_getmousepos_after_conceal()
  call NewWindow(10, 40)
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
        \ signcolumn=no nonumber
  syntax match Hidden /HIDDEN / conceal

  let line = repeat('a', 24)
        \ .. ' HIDDEN target words after hidden text to force wrapping'
        \ .. ' and mapping checks'
  call setline(1, line)
  redraw!

  let target_col = stridx(line, 'target') + 1
  let pos = screenpos(0, 1, target_col)
  let [winrow, wincol] = win_screenpos(0)
  call assert_equal([1, 26],
        \ [pos.row - winrow + 1, pos.curscol - wincol + 1])

  " Use the independently expected cell, not the forward screenpos() result,
  " to verify the reverse mouse mapping.
  call test_setmouse(winrow, wincol + 25)
  let mousepos = getmousepos()
  call assert_equal(1, mousepos.line)
  call assert_equal(target_col, mousepos.column)
  call assert_equal(0, mousepos.coladd)

  syntax clear Hidden
  call CloseWindow()
endfunc

func Test_conceallevel_three_getmousepos_rightleft_after_conceal()
  CheckFeature rightleft

  call NewWindow(10, 40)
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
        \ signcolumn=no nonumber rightleft
  syntax match Hidden /HIDDEN / conceal

  let line = repeat('a', 42)
        \ .. ' HIDDEN target words after hidden text to force wrapping'
        \ .. ' and mapping checks'
  call setline(1, line)
  redraw!

  let target_col = stridx(line, 'target') + 1
  let pos = screenpos(0, 1, target_col)
  let [winrow, wincol] = win_screenpos(0)
  call assert_equal(2, pos.row - winrow + 1)
  call assert_equal(4, pos.col - wincol + 1)
  call assert_equal(37, pos.curscol - wincol + 1)
  call assert_equal(4, pos.endcol - wincol + 1)

  call test_setmouse(pos.row, pos.curscol)
  let mousepos = getmousepos()
  call assert_equal(1, mousepos.line)
  call assert_equal(target_col, mousepos.column)
  call assert_equal(0, mousepos.coladd)
  call assert_equal(2, mousepos.winrow)
  call assert_equal(37, mousepos.wincol)

  syntax clear Hidden
  call CloseWindow()
endfunc

func Test_conceallevel_three_popup_getmousepos_after_conceal()
  call NewWindow(10, 60)

  let line = repeat('a', 24)
        \ .. ' HIDDEN ' .. repeat('b', 20)
        \ .. ' target words after hidden text to force wrapping'
        \ .. ' and mapping checks'
  call setline(1, line)
  let winid = popup_create(bufnr(), #{
        \ line: 2, col: 5, minwidth: 32, maxwidth: 32, minheight: 5,
        \ border: [], padding: [1, 2, 1, 2], wrap: v:true,
        \ })
  call win_execute(winid, 'setlocal wrap linebreak breakindent conceallevel=3'
        \ .. ' concealcursor=nvic signcolumn=no nonumber')
  call win_execute(winid, 'syntax match Hidden /HIDDEN / conceal')
  redraw!

  let target_col = stridx(line, 'target') + 1
  let pos = screenpos(winid, 1, target_col)
  call assert_true(pos.row > 0)

  " Add border/padding offsets to click the popup's drawn text cell.
  let poppos = popup_getpos(winid)
  let mouse_row = pos.row + poppos.core_line - poppos.line
  let mouse_col = pos.curscol + poppos.core_col - poppos.col
  call assert_true(mouse_row > poppos.core_line)
  call assert_true(mouse_col >= poppos.core_col)
  call assert_true(mouse_col < poppos.core_col + poppos.core_width)

  call test_setmouse(mouse_row, mouse_col)
  let mousepos = getmousepos()
  call assert_equal(winid, mousepos.winid)
  call assert_equal(mouse_row, mousepos.screenrow)
  call assert_equal(mouse_col, mousepos.screencol)
  call assert_equal(mouse_row - poppos.line + 1, mousepos.winrow)
  call assert_equal(mouse_col - poppos.col + 1, mousepos.wincol)
  call assert_equal(1, mousepos.line)
  call assert_equal(target_col, mousepos.column)
  call assert_equal(0, mousepos.coladd)

  call popup_close(winid)
  syntax clear Hidden
  call CloseWindow()
endfunc

func Test_conceallevel_three_popup_scrollbar_getmousepos()
  call NewWindow(10, 70)

  let target_line = repeat('a', 28)
        \ .. ' HIDDEN ' .. repeat('b', 24)
        \ .. ' target words after hidden text to force wrapping'
        \ .. ' and mapping checks'
  let lines = [
        \ 'top filler',
        \ target_line,
        \ 'middle filler one',
        \ 'middle filler two',
        \ 'middle filler three',
        \ 'middle filler four',
        \ 'middle filler five',
        \ 'bottom filler',
        \]
  call setline(1, lines)
  let winid = popup_create(bufnr(), #{
        \ line: 2, col: 5, minwidth: 36, maxwidth: 36,
        \ minheight: 5, maxheight: 5, scrollbar: v:true,
        \ border: [], padding: [1, 2, 1, 2], wrap: v:true,
        \ })
  call win_execute(winid, 'setlocal wrap linebreak breakindent conceallevel=3'
        \ .. ' concealcursor=nvic signcolumn=no nonumber')
  call win_execute(winid, 'syntax match Hidden /HIDDEN / conceal')
  redraw!

  let target_col = stridx(target_line, 'target') + 1
  let pos = screenpos(winid, 2, target_col)
  call assert_true(pos.row > 0)

  let poppos = popup_getpos(winid)
  call assert_equal(1, poppos.scrollbar)
  call assert_true(poppos.core_height < len(lines))

  " Add border/padding offsets to click the popup's drawn text cell.
  let mouse_row = pos.row + poppos.core_line - poppos.line
  let mouse_col = pos.curscol + poppos.core_col - poppos.col
  call assert_true(mouse_row >= poppos.core_line)
  call assert_true(mouse_row < poppos.core_line + poppos.core_height)
  call assert_true(mouse_col >= poppos.core_col)
  call assert_true(mouse_col < poppos.core_col + poppos.core_width)

  call test_setmouse(mouse_row, mouse_col)
  let mousepos = getmousepos()
  call assert_equal(winid, mousepos.winid)
  call assert_equal(mouse_row, mousepos.screenrow)
  call assert_equal(mouse_col, mousepos.screencol)
  call assert_equal(mouse_row - poppos.line + 1, mousepos.winrow)
  call assert_equal(mouse_col - poppos.col + 1, mousepos.wincol)
  call assert_equal(2, mousepos.line)
  call assert_equal(target_col, mousepos.column)
  call assert_equal(0, mousepos.coladd)

  call popup_close(winid)
  syntax clear Hidden
  call CloseWindow()
endfunc

func Test_conceallevel_three_popup_rightleft_narrow_getmousepos()
  CheckFeature rightleft

  call NewWindow(10, 70)

  let line = repeat('a', 22)
        \ .. ' HIDDEN target words after hidden text to force wrapping'
        \ .. ' and mapping checks'
  call setline(1, line)
  let winid = popup_create(bufnr(), #{
        \ line: 2, col: 8, minwidth: 26, maxwidth: 26, minheight: 5,
        \ border: [], padding: [1, 2, 1, 2], wrap: v:true,
        \ })
  call win_execute(winid, 'setlocal rightleft wrap linebreak breakindent'
        \ .. ' conceallevel=3 concealcursor=nvic signcolumn=no nonumber')
  call win_execute(winid, 'syntax match Hidden /HIDDEN / conceal')
  redraw!

  let target_col = stridx(line, 'target') + 1
  let pos = screenpos(winid, 1, target_col)
  call assert_true(pos.row > 0)

  let poppos = popup_getpos(winid)
  call assert_true(poppos.core_width <= 26)

  " Add border/padding offsets to click the popup's drawn text cell.
  let mouse_row = pos.row + poppos.core_line - poppos.line
  let mouse_col = pos.curscol + poppos.core_col - poppos.col
  call assert_true(mouse_row > poppos.core_line)
  call assert_true(mouse_col >= poppos.core_col)
  call assert_true(mouse_col < poppos.core_col + poppos.core_width)

  call test_setmouse(mouse_row, mouse_col)
  let mousepos = getmousepos()
  call assert_equal(winid, mousepos.winid)
  call assert_equal(mouse_row, mousepos.screenrow)
  call assert_equal(mouse_col, mousepos.screencol)
  call assert_equal(mouse_row - poppos.line + 1, mousepos.winrow)
  call assert_equal(mouse_col - poppos.col + 1, mousepos.wincol)
  call assert_equal(1, mousepos.line)
  call assert_equal(target_col, mousepos.column)
  call assert_equal(0, mousepos.coladd)

  call popup_close(winid)
  syntax clear Hidden
  call CloseWindow()
endfunc

func Test_conceallevel_three_wrapped_mouse_click_rightleft()
  CheckFeature rightleft

  call NewWindow(10, 40)
  set mouse=a
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
        \ signcolumn=no nonumber rightleft
  syntax match Hidden /HIDDEN / conceal

  let line = repeat('a', 24)
        \ .. ' HIDDEN target words after hidden text to force wrapping'
        \ .. ' and mapping checks'
  call setline(1, line)
  redraw!

  let target_col = stridx(line, 'target') + 1
  let pos = screenpos(0, 1, target_col)
  call assert_true(pos.row > 0)

  call test_setmouse(pos.row, pos.curscol)
  call feedkeys("\<LeftMouse>", "tx")
  call assert_equal(target_col, col('.'))

  syntax clear Hidden
  call CloseWindow()
  set mouse&
endfunc

func Test_conceallevel_three_visual_drag_rightleft()
  CheckFeature rightleft

  call NewWindow(10, 40)
  set mouse=a
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
        \ signcolumn=no nonumber rightleft
  syntax match Hidden /HIDDEN / conceal

  let line = repeat('a', 42)
        \ .. ' HIDDEN target words after hidden text to force wrapping'
        \ .. ' and mapping checks'
  call setline(1, line)
  redraw!

  let start_col = 10
  let target_col = stridx(line, 'target') + 4
  let start = screenpos(0, 1, start_col)
  let target = screenpos(0, 1, target_col)
  let [winrow, wincol] = win_screenpos(0)
  call assert_equal(1, start.row - winrow + 1)
  call assert_equal(31, start.curscol - wincol + 1)
  call assert_equal(2, target.row - winrow + 1)
  call assert_equal(34, target.curscol - wincol + 1)

  call test_setmouse(start.row, start.curscol)
  call feedkeys("\<LeftMouse>", "tx")
  call test_setmouse(target.row, target.curscol)
  call feedkeys("\<LeftDrag>", "tx")
  redraw

  call assert_equal(target_col, col('.'))
  let selected_attr = screenattr(target.row, target.curscol + 1)
  call assert_notequal(0, selected_attr)
  call assert_equal(selected_attr, screenattr(target.row, target.curscol + 2))

  syntax clear Hidden
  call CloseWindow()
  set mouse&
endfunc

func Test_conceallevel_three_visual_linewise_concealcursor_v()
  call NewWindow(10, 40)
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
        \ signcolumn=no nonumber
  syntax match Hidden /HIDDEN / conceal

  let line = repeat('a', 42)
        \ .. ' HIDDEN target words after hidden text to force wrapping'
        \ .. ' and mapping checks'
  call setline(1, [line, 'second visible line', 'after'])
  redraw!

  let target_col = stridx(line, 'target') + 1
  call cursor(1, 1)
  call feedkeys("Vj", 'tx')
  redraw

  let [winrow, wincol] = win_screenpos(0)
  let pos = screenpos(0, 1, target_col)
  call assert_equal(2, pos.row - winrow + 1)
  call assert_equal(4, pos.curscol - wincol + 1)

  let selected_attr = screenattr(pos.row, pos.curscol)
  call assert_notequal(0, selected_attr)
  call assert_equal(selected_attr, screenattr(pos.row + 1, 1))

  call feedkeys("\<Esc>", 'tx')
  syntax clear Hidden
  call CloseWindow()
endfunc

func Test_conceallevel_three_visual_block_after_conceal()
  call NewWindow(10, 40)
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
        \ signcolumn=no nonumber showbreak=
  syntax match Hidden /HIDDEN / conceal

  let line1 = repeat('a', 42)
        \ .. ' HIDDEN target words after hidden text to force wrapping'
  let line2 = repeat('a', 42)
        \ .. ' target words after visible text to force wrapping'
  call setline(1, [line1, line2])
  redraw!

  let start_col = stridx(line1, 'target') + 1
  let target_col = stridx(line2, 'target') + 1
  call cursor(1, start_col)
  call feedkeys("\<C-V>lj", 'tx')
  redraw

  call assert_equal(target_col + 1, col('.'))
  let start = screenpos(0, 1, start_col)
  let target = screenpos(0, 2, target_col)
  call assert_true(start.row > 0)
  call assert_true(target.row > 0)

  let selected_attr = screenattr(start.row, start.curscol)
  call assert_notequal(0, selected_attr)
  call assert_equal(selected_attr, screenattr(start.row, start.curscol + 1))
  call assert_equal(selected_attr, screenattr(target.row, target.curscol))
  call assert_notequal(selected_attr, screenattr(target.row, target.curscol + 2))

  call feedkeys("\<Esc>", 'tx')
  syntax clear Hidden
  call CloseWindow()
endfunc

func Test_conceallevel_three_visual_block_to_concealed_line()
  call NewWindow(6, 40)
  setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
  syntax match Hidden /HIDDEN / conceal

  call setline(1, ['abcdX', 'HIDDEN abcdX'])
  call cursor(1, 5)
  redraw!
  execute "normal! \<C-V>j"
  call assert_equal([2, 12, 0], getcurpos()[1:3])
  execute "normal! \<Esc>"

  call setline(1, ['0123456789X', 'abcde', 'HIDDEN 0123456789X'])
  call cursor(1, 11)
  redraw!
  execute "normal! \<C-V>jj"
  call assert_equal([3, 18, 0], getcurpos()[1:3])
  execute "normal! \<Esc>"

  call setline(1, ['HIDDEN abcdX', 'abcdX'])
  call cursor(2, 5)
  redraw!
  execute "normal! \<C-V>k"
  call assert_equal([1, 12, 0], getcurpos()[1:3])

  execute "normal! \<Esc>"
  syntax clear Hidden
  call CloseWindow()
endfunc

func Test_conceallevel_three_visual_block_multicell_target()
  let save_display = &display
  let save_virtualedit = &virtualedit
  call NewWindow(6, 40)
  try
    setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
          \ tabstop=8
    syntax match Hidden /HIDDEN / conceal

    set display=
    set virtualedit=block
    call setline(1, ['HIDDEN abcdX', "\tY"])
    call cursor(1, 12)
    redraw!
    execute "normal! \<C-V>j"
    call assert_equal([2, 1, 4], getcurpos()[1:3])
    call assert_equal(5, virtcol('.'))
    execute "normal! \<Esc>"

    set virtualedit=
    call cursor(1, 12)
    redraw!
    execute "normal! \<C-V>j"
    call assert_equal([2, 1, 0], getcurpos()[1:3])
    execute "normal! \<Esc>"

    set virtualedit=block
    call setline(1, ['HIDDEN bX', nr2char(1) .. 'Z'])
    call cursor(1, 9)
    redraw!
    execute "normal! \<C-V>j"
    call assert_equal([2, 1, 1], getcurpos()[1:3])
    call assert_equal(2, virtcol('.'))
    execute "normal! \<Esc>"

    set virtualedit=
    call cursor(1, 9)
    redraw!
    execute "normal! \<C-V>j"
    call assert_equal([2, 1, 0], getcurpos()[1:3])
    execute "normal! \<Esc>"

    if has('multi_byte') && &encoding ==# 'utf-8'
      set virtualedit=block
      call setline(1, ['HIDDEN abcX', 'ab日Z'])
      call cursor(1, 11)
      redraw!
      execute "normal! \<C-V>j"
      call assert_equal([2, 3, 0], getcurpos()[1:3])
      call assert_equal(4, virtcol('.'))
      execute "normal! \<Esc>"

      call setline(1, ['HIDDEN aX', '日Z'])
      call cursor(1, 9)
      redraw!
      execute "normal! \<C-V>j"
      call assert_equal([2, 1, 0], getcurpos()[1:3])
      call assert_equal(2, virtcol('.'))
    endif
  finally
    execute "normal! \<Esc>"
    syntax clear Hidden
    let &display = save_display
    let &virtualedit = save_virtualedit
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_visual_block_stops_after_conceal()
  call NewWindow(10, 59)
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
        \ signcolumn=no nonumber showbreak=
  syntax region Hidden matchgroup=Hidden start=/`/ end=/`/ concealends

  let line = 'The inline code `printf("日本語 %s", value)` should hide its'
        \ .. ' backticks when Markdown conceal is active, while trailing'
        \ .. ' text wraps.'
  call setline(1, [
        \ repeat('x', 30),
        \ line,
        \ repeat('x', 30),
        \ ])
  redraw!

  call cursor(1, 1)
  execute "normal! \<C-V>24l2j"
  redraw

  call assert_equal([3, 25], [line('.'), col('.')])
  let selected_attr = screenattr(1, 1)
  call assert_notequal(0, selected_attr)
  call assert_equal(selected_attr, screenattr(2, 1))
  call assert_equal(selected_attr, screenattr(2, 24))
  call assert_equal(selected_attr, screenattr(2, 26))
  call assert_notequal(selected_attr, screenattr(2, 27))
  call assert_notequal(selected_attr, screenattr(2, 59))

  call feedkeys("\<Esc>", 'tx')
  syntax clear Hidden
  call CloseWindow()
endfunc

func Test_conceallevel_three_visual_block_boundary_redraw()
  for width in [52, 60, 80, 120]
    call NewWindow(10, width)
    setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
          \ signcolumn=no number showbreak=
    syntax match Hidden /\[/ conceal
    syntax match Hidden /\](https:[^)]*)/ conceal
    syntax match Hidden /\*\*/ conceal

    let line = '0123456789 0123456789 0123456789 **日本語**'
          \ .. ' 0123456789 0123456789'
          \ .. ' [link](https://example.invalid/hidden)'
          \ .. ' 0123456789 0123456789'
    call setline(1, repeat(['short filler'], 4) + [line, 'after'])
    redraw!

    call cursor(1, 1)
    execute "normal! \<C-V>4j"
    for _ in range(1, 70)
      redraw
      let startrow = screenpos(0, 5, 1).row
      let afterrow = screenpos(0, 6, 1).row
      let rows = []
      for row in range(startrow, afterrow - 1)
        call add(rows, join(map(range(1, winwidth(0)),
              \ 'screenstring(row, v:val)'), ''))
      endfor
      let text = join(rows, ' ')
      call assert_match('0123456789.*日本語.*link.*0123456789', text)
      normal! l
    endfor

    call feedkeys("\<Esc>", 'tx')
    syntax clear Hidden
    call CloseWindow()
  endfor
endfunc

func Test_conceallevel_three_visual_block_restarts_after_concealed_cursor()
  call NewWindow(6, 80)
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
        \ signcolumn=no nonumber showbreak=
  syntax match Hidden /\[/ conceal
  syntax match Hidden /\](https:[^)]*)/ conceal

  let line = 'prefix [link](https://example.invalid/hidden)'
        \ .. ' suffix words after concealed target'
  call setline(1, [repeat('x', 70), line])
  redraw!

  call cursor(1, 45)
  execute "normal! \<C-V>j"
  let hidden_col = stridx(line, 'https://') + 1
  call assert_true(col('.') > hidden_col)
  execute 'normal! ' .. (col('.') - hidden_col) .. 'h'
  redraw

  call assert_match('https://', strpart(line, col('.') - 1))
  let anchor = screenpos(0, 1, 45)
  let suffix = screenpos(0, 2, stridx(line, 'suffix') + 1)
  call assert_true(anchor.row > 0)
  call assert_true(suffix.row > 0)

  let selected_attr = screenattr(anchor.row, anchor.curscol)
  call assert_notequal(0, selected_attr)
  call assert_equal(selected_attr, screenattr(suffix.row, suffix.curscol))

  call feedkeys("\<Esc>", 'tx')
  syntax clear Hidden
  call CloseWindow()
endfunc

func Test_conceallevel_three_visual_block_boundary_line_33()
  let save_lines = &lines
  let save_columns = &columns
  try
    set lines=45 columns=160
    call NewWindow(40, 140)
    setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
          \ signcolumn=no number showbreak=
    syntax match Hidden /\[/ conceal
    syntax match Hidden /\](https:[^)]*)/ conceal
    syntax match Hidden /\*\*/ conceal

    let line = '0123456789 0123456789 0123456789 **日本語**'
          \ .. ' 0123456789 0123456789'
          \ .. ' [link](https://example.invalid/hidden)'
          \ .. ' 0123456789 0123456789'
    call setline(1, repeat(['short filler'], 32) + [line, 'after'])
    redraw!

    call cursor(1, 1)
    execute "normal! \<C-V>32j"
    for _ in range(1, 90)
      redraw
      let startrow = screenpos(0, 33, 1).row
      let afterrow = screenpos(0, 34, 1).row
      let lastrow = afterrow > startrow ? afterrow - 1
            \ : win_screenpos(0)[0] + winheight(0) - 1
      let rows = []
      call assert_true(startrow > 0)
      call assert_true(lastrow >= startrow)
      for row in range(startrow, lastrow)
        call add(rows, join(map(range(1, winwidth(0)),
              \ 'screenstring(row, v:val)'), ''))
      endfor
      let text = join(rows, ' ')
      call assert_match('0123456789.*日本語.*link.*0123456789', text)
      normal! l
    endfor
  finally
    call feedkeys("\<Esc>", 'tx')
    syntax clear Hidden
    call CloseWindow()
    let &columns = save_columns
    let &lines = save_lines
  endtry
endfunc

func Test_conceallevel_three_mouse_extend_after_conceal()
  call NewWindow(10, 40)
  set mouse=a mousemodel=extend
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
        \ signcolumn=no nonumber
  syntax match Hidden /HIDDEN / conceal

  let line = repeat('a', 42)
        \ .. ' HIDDEN target words after hidden text to force wrapping'
        \ .. ' and mapping checks'
  call setline(1, line)
  redraw!

  let start_col = 10
  let target_col = stridx(line, 'target') + 4
  let start = screenpos(0, 1, start_col)
  let target = screenpos(0, 1, target_col)
  call assert_true(start.row > 0)
  call assert_true(target.row > start.row)

  call test_setmouse(start.row, start.curscol)
  call feedkeys("\<LeftMouse>", 'tx')
  call test_setmouse(target.row, target.curscol)
  call feedkeys("\<RightMouse>", 'tx')
  redraw

  call assert_equal(target_col, col('.'))
  normal! y
  call assert_equal(strpart(line, start_col - 1, target_col - start_col + 1),
        \ @")

  syntax clear Hidden
  call CloseWindow()
  set mouse& mousemodel&
endfunc

func Test_conceallevel_three_mouse_extend_rightleft()
  CheckFeature rightleft

  call NewWindow(10, 40)
  set mouse=a mousemodel=extend
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
        \ signcolumn=no nonumber rightleft
  syntax match Hidden /HIDDEN / conceal

  let line = repeat('a', 42)
        \ .. ' HIDDEN target words after hidden text to force wrapping'
        \ .. ' and mapping checks'
  call setline(1, line)
  redraw!

  let start_col = 10
  let target_col = stridx(line, 'target') + 4
  let start = screenpos(0, 1, start_col)
  let target = screenpos(0, 1, target_col)
  let [winrow, wincol] = win_screenpos(0)
  call assert_equal(1, start.row - winrow + 1)
  call assert_equal(31, start.curscol - wincol + 1)
  call assert_equal(2, target.row - winrow + 1)
  call assert_equal(34, target.curscol - wincol + 1)

  call test_setmouse(start.row, start.curscol)
  call feedkeys("\<LeftMouse>", 'tx')
  call test_setmouse(target.row, target.curscol)
  call feedkeys("\<RightMouse>", 'tx')
  redraw

  call assert_equal(target_col, col('.'))
  normal! y
  call assert_equal(strpart(line, start_col - 1, target_col - start_col + 1),
        \ @")

  syntax clear Hidden
  call CloseWindow()
  set mouse& mousemodel&
endfunc

func Test_conceallevel_three_mouse_shift_left_extend_after_conceal()
  call NewWindow(10, 40)
  set mouse=a mousemodel=popup
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
        \ signcolumn=no nonumber
  syntax match Hidden /HIDDEN / conceal

  let line = repeat('a', 42)
        \ .. ' HIDDEN target words after hidden text to force wrapping'
        \ .. ' and mapping checks'
  call setline(1, line)
  redraw!

  let start_col = 10
  let target_col = stridx(line, 'target') + 4
  let start = screenpos(0, 1, start_col)
  let target = screenpos(0, 1, target_col)
  call assert_true(start.row > 0)
  call assert_true(target.row > start.row)

  call test_setmouse(start.row, start.curscol)
  call feedkeys("\<LeftMouse>", 'tx')
  call test_setmouse(target.row, target.curscol)
  call feedkeys("\<S-LeftMouse>", 'tx')
  redraw

  call assert_equal(target_col, col('.'))
  normal! y
  call assert_equal(strpart(line, start_col - 1, target_col - start_col + 1),
        \ @")

  syntax clear Hidden
  call CloseWindow()
  set mouse& mousemodel&
endfunc

func Test_conceallevel_three_mouse_selectmode_after_conceal()
  call NewWindow(10, 40)
  set mouse=a selectmode=mouse
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
        \ signcolumn=no nonumber
  syntax match Hidden /HIDDEN / conceal

  let line = repeat('a', 42)
        \ .. ' HIDDEN target words after hidden text to force wrapping'
        \ .. ' and mapping checks'
  call setline(1, line)
  redraw!

  let start_col = 10
  let target_col = stridx(line, 'target') + 4
  let start = screenpos(0, 1, start_col)
  let target = screenpos(0, 1, target_col)
  call assert_true(start.row > 0)
  call assert_true(target.row > start.row)

  call test_setmouse(start.row, start.curscol)
  call feedkeys("\<LeftMouse>", 'tx')
  call test_setmouse(target.row, target.curscol)
  call feedkeys("\<LeftDrag>", 'tx')
  redraw

  call assert_equal('s', mode())
  call assert_equal(target_col, col('.'))
  call feedkeys("\<C-G>", 'tx')
  normal! y
  call assert_equal(strpart(line, start_col - 1, target_col - start_col + 1),
        \ @")

  syntax clear Hidden
  call CloseWindow()
  set mouse& selectmode&
endfunc

func Test_conceallevel_three_multiple_clicks_after_conceal()
  call NewWindow(10, 40)
  set mouse=a
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
        \ signcolumn=no nonumber
  syntax region testCode matchgroup=testTick start=/`/ end=/`/ concealends

  let line = repeat('a', 42)
        \ .. ' `target` words after hidden delimiters to force wrapping'
        \ .. ' and mapping checks'
  call setline(1, line)
  redraw!

  let target_col = stridx(line, 'target') + 1
  let target = screenpos(0, 1, target_col)
  call assert_true(target.row > 0)

  call test_setmouse(target.row, target.curscol)
  call feedkeys("\<2-LeftMouse>", 'tx')
  normal! y
  call assert_equal('v', getregtype('"'))
  call assert_equal(['target'], getreg('"', 1, 1))

  call test_setmouse(target.row, target.curscol)
  call feedkeys("\<3-LeftMouse>", 'tx')
  normal! y
  call assert_equal('V', getregtype('"'))
  call assert_equal([line], getreg('"', 1, 1))

  syntax clear testCode
  call CloseWindow()
  set mouse&
endfunc

func Test_conceallevel_three_operator_pending_screenline()
  call NewWindow(10, 40)
  setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
  syntax match Hidden /HIDDEN / conceal

  let line = repeat('a', 42)
        \ .. ' HIDDEN target words after hidden text to force wrapping'
        \ .. ' and mapping checks'
  let target_col = stridx(line, 'target') + 1

  call setline(1, line)
  redraw!
  normal! ygj
  call assert_equal('v', getregtype('"'))
  call assert_equal([strpart(line, 0, 40)], getreg('"', 1, 1))

  call setline(1, line)
  call cursor(1, 1)
  redraw!
  normal! dgj
  call assert_equal([strpart(line, 0, 40)], getreg('"', 1, 1))
  call assert_equal(strpart(line, 40), getline(1))

  call setline(1, line)
  call cursor(1, target_col)
  redraw!
  normal! ygk
  call assert_equal(4, col('.'))
  call assert_equal([strpart(line, 3, target_col - 4)], getreg('"', 1, 1))

  call setline(1, line)
  call cursor(1, 1)
  redraw!
  normal! yg$
  call assert_equal([strpart(line, 0, 40)], getreg('"', 1, 1))

  call setline(1, line)
  call cursor(1, target_col)
  redraw!
  normal! dg$
  call assert_equal([strpart(line, target_col - 1, 37)], getreg('"', 1, 1))
  call assert_equal(strpart(line, 0, target_col - 1)
        \ .. strpart(line, target_col + 36), getline(1))

  call setline(1, line)
  call cursor(1, target_col)
  redraw!
  normal! yg0
  call assert_equal(41, col('.'))
  call assert_equal([strpart(line, 40, target_col - 41)], getreg('"', 1, 1))

  let g:conceallevel_three_op_types = []
  func! ConceallevelThreeScreenlineOp(type) abort
    call add(g:conceallevel_three_op_types, a:type)
    normal! `[v`]y
  endfunc
  set operatorfunc=ConceallevelThreeScreenlineOp
  call setline(1, line)
  call cursor(1, 1)
  redraw!
  normal! g@gj
  call assert_equal(['char'], g:conceallevel_three_op_types)
  call assert_equal([strpart(line, 0, 40)], getreg('"', 1, 1))
  set operatorfunc&
  delfunc ConceallevelThreeScreenlineOp
  unlet g:conceallevel_three_op_types

  call setline(1, line)
  call cursor(1, 1)
  redraw!
  normal! dgj
  normal! .
  call assert_equal([strpart(line, 40, 47)], getreg('"', 1, 1))
  call assert_equal(strpart(line, 87), getline(1))

  syntax clear Hidden
  call CloseWindow()
endfunc

func Test_conceallevel_three_screenline_counts()
  call NewWindow(10, 40)
  setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
  syntax match Hidden /HIDDEN / conceal

  let line = repeat('a', 42)
        \ .. ' HIDDEN target words after hidden text to force wrapping'
        \ .. ' and mapping checks'
  call setline(1, [line, 'second line', 'third line'])
  redraw!
  let [winrow, wincol] = win_screenpos(0)

  normal! 2gj
  let pos = screenpos(0, line('.'), col('.'))
  call assert_equal([3, 1], [pos.row - winrow + 1, pos.curscol - wincol + 1])

  call cursor(3, 1)
  redraw!
  normal! 3gk
  let pos = screenpos(0, line('.'), col('.'))
  call assert_equal(1, line('.'))
  call assert_equal([2, 1], [pos.row - winrow + 1, pos.curscol - wincol + 1])

  call cursor(1, 1)
  redraw!
  normal! 2g$
  let pos = screenpos(0, line('.'), col('.'))
  call assert_equal([2, 1], [pos.row - winrow + 1, pos.curscol - wincol + 1])

  normal! g0
  let pos = screenpos(0, line('.'), col('.'))
  call assert_equal([2, 1], [pos.row - winrow + 1, pos.curscol - wincol + 1])

  normal! g$
  let pos = screenpos(0, line('.'), col('.'))
  call assert_equal([2, 40], [pos.row - winrow + 1, pos.curscol - wincol + 1])

  call cursor(1, stridx(line, 'target') + 1)
  redraw!
  normal! g^
  let pos = screenpos(0, line('.'), col('.'))
  call assert_equal([2, 1], [pos.row - winrow + 1, pos.curscol - wincol + 1])

  call cursor(1, 1)
  redraw!
  normal! gm
  let pos = screenpos(0, line('.'), col('.'))
  call assert_equal([1, 21], [pos.row - winrow + 1, pos.curscol - wincol + 1])

  syntax clear Hidden
  call CloseWindow()
endfunc

func Test_conceallevel_three_screenline_to_concealed_line()
  call NewWindow(6, 40)
  setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
  syntax match Hidden /HIDDEN / conceal

  call setline(1, ['abcdX', 'HIDDEN abcdX'])
  call cursor(1, 5)
  redraw!
  normal! gj
  call assert_equal([2, 12, 2, 5],
        \ [line('.'), col('.'), winline(), wincol()])

  call setline(1, ['HIDDEN abcdX', 'abcdX'])
  call cursor(2, 5)
  redraw!
  normal! gk
  call assert_equal([1, 12, 1, 5],
        \ [line('.'), col('.'), winline(), wincol()])

  syntax clear Hidden
  call CloseWindow()
endfunc

func Test_conceallevel_three_counted_gj_keeps_cursor_visible()
  call NewWindow(5, 40)
  setlocal wrap nolinebreak nobreakindent nosmoothscroll conceallevel=3
        \ concealcursor=nvic signcolumn=no nonumber showbreak= scrolloff=0

  let matchid = matchadd('Conceal', 'HIDDEN\d\+ ', 10, -1,
        \ #{conceal: ''})
  let segment = 'aa HIDDEN123 ' .. "\t" .. 'あ bb '
  try
    let longline = repeat(segment, 2400)
    call setline(1, longline)
    call append(1, longline)
    redraw!

    normal! 30gj
    call assert_equal([winheight(0), 1], [winline(), wincol()])
    call assert_true(winsaveview().skipcol > 0)
    let scrolled_view = winsaveview()
    let scrolled_pos = getpos('.')
    call setline(1, getline(1) .. 'x')
    call setline(1, longline)
    call setpos('.', scrolled_pos)
    call winrestview(scrolled_view)
    redraw!
    normal! 30gk
    call assert_equal([1, 1, 0],
          \ [winline(), col('.'), winsaveview().skipcol])

    execute 'normal! ' .. repeat('gj', 30)
    call assert_equal([winheight(0), 1], [winline(), wincol()])
    call assert_true(winsaveview().skipcol > 0)
    execute 'normal! ' .. repeat('gk', 30)
    call assert_equal([1, 1, 0],
          \ [winline(), col('.'), winsaveview().skipcol])

    normal! 300gj
    normal! 300gj
    let target_col = col('.')
    call assert_equal([winheight(0), 1], [winline(), wincol()])
    call assert_true(winsaveview().skipcol > 0)
    normal! g$
    call assert_true(col('.') > target_col)
    normal! g0
    call assert_equal([target_col, winheight(0), 1],
          \ [col('.'), winline(), wincol()])
    normal! 600gk
    call assert_equal([1, 1, 0],
          \ [winline(), col('.'), winsaveview().skipcol])

    execute 'normal! ' .. repeat('gj', 30)
    normal! g0
    let g0_col = col('.')
    normal! j
    call assert_equal([2, g0_col], [line('.'), col('.')])
    normal! k
    call assert_equal([1, g0_col], [line('.'), col('.')])

    normal! gg0
    redraw!
    execute 'normal! ' .. repeat('gj', 30)
    normal! g$
    let gdollar_col = col('.')
    normal! j
    call assert_equal([2, gdollar_col], [line('.'), col('.')])
    normal! k
    call assert_equal([1, gdollar_col], [line('.'), col('.')])

  finally
    call matchdelete(matchid)
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_smoothscroll_cache_invalidation()
  call NewWindow(5, 40)
  setlocal wrap nolinebreak nobreakindent nosmoothscroll conceallevel=3
        \ concealcursor=nvic signcolumn=no nonumber showbreak= scrolloff=0

  let matchid = matchadd('Conceal', 'HIDDEN\d\+ ', 10, -1,
        \ #{conceal: ''})
  let segment = 'aa HIDDEN123 ' .. "\t" .. 'あ bb '
  let longline = repeat(segment, 2400)
  try
    call setline(1, longline)
    redraw!
    normal! 30gj
    call setline(1, longline .. 'x')
    call setline(1, longline)
    setlocal smoothscroll
    normal! gj

    setlocal nosmoothscroll
    let start_pos = getpos('.')
    let start_view = winsaveview()
    normal! gj
    let cached_result = [col('.'), virtcol('.'), winline(), wincol(),
          \ winsaveview().skipcol]

    call setpos('.', start_pos)
    call winrestview(start_view)
    call setline(1, longline .. 'x')
    call setline(1, longline)
    call setpos('.', start_pos)
    call winrestview(start_view)
    redraw!
    normal! gj
    call assert_equal(cached_result,
          \ [col('.'), virtcol('.'), winline(), wincol(),
          \ winsaveview().skipcol])
  finally
    call matchdelete(matchid)
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_plain_wrapped_gk_after_blank()
  call NewWindow(10, 64)
  setlocal wrap linebreak breakindent smoothscroll conceallevel=3
        \ concealcursor=n signcolumn=no number showbreak= scrolloff=0

  call setline(1, [
        \ 'before',
        \ 'Onboarding does not end at 90 days. Continue learning, contribute to our standards, mentor future hires, and build systems that move Kepler forward. Welcome to the team!',
        \ '',
        \ 'after',
        \ ])
  redraw!

  let long_row = screenpos(0, 2, 1).row
  let blank_row = screenpos(0, 3, 1).row
  call assert_true(blank_row - long_row >= 3)

  call cursor(4, 1)
  redraw!
  normal! gkgkgk
  redraw!
  call assert_equal(2, line('.'))
  call assert_equal(blank_row - 2, screenpos(0, line('.'), col('.')).row)

  call CloseWindow()
endfunc

func Test_conceallevel_three_scroll_commands()
  call NewWindow(5, 40)
  setlocal wrap smoothscroll conceallevel=3 concealcursor=nvic
        \ signcolumn=no nonumber scrolloff=0 scroll=3
  syntax match Hidden /HIDDEN / conceal

  let line = repeat('a', 42) .. ' HIDDEN ' .. repeat('b', 240)
  call setline(1, [line, 'second line', 'third line'])
  call cursor(1, 1)
  redraw!

  execute "normal! \<C-E>"
  call assert_equal([1, 81, 1, 40],
        \ [line('.'), col('.'), winsaveview().topline, winsaveview().skipcol])
  execute "normal! \<C-E>"
  call assert_equal([1, 121, 1, 80],
        \ [line('.'), col('.'), winsaveview().topline, winsaveview().skipcol])
  execute "normal! \<C-Y>"
  call assert_equal([1, 121, 1, 40],
        \ [line('.'), col('.'), winsaveview().topline, winsaveview().skipcol])

  call winrestview({'lnum': 1, 'col': 0, 'topline': 1, 'leftcol': 0,
        \ 'skipcol': 0, 'curswant': 0})
  call cursor(1, 1)
  execute "normal! \<C-D>"
  call assert_equal([1, 121, 1, 80],
        \ [line('.'), col('.'), winsaveview().topline, winsaveview().skipcol])
  execute "normal! \<C-D>"
  call assert_equal([1, 241, 1, 200],
        \ [line('.'), col('.'), winsaveview().topline, winsaveview().skipcol])
  execute "normal! \<C-U>"
  call assert_equal([1, 121, 1, 80],
        \ [line('.'), col('.'), winsaveview().topline, winsaveview().skipcol])

  call winrestview({'lnum': 1, 'col': 0, 'topline': 1, 'leftcol': 0,
        \ 'skipcol': 0, 'curswant': 0})
  call cursor(1, 1)
  execute "normal! \<C-F>"
  call assert_equal([1, 241, 1, 200],
        \ [line('.'), col('.'), winsaveview().topline, winsaveview().skipcol])
  execute "normal! \<C-B>"
  call assert_equal([1, 128, 1, 0],
        \ [line('.'), col('.'), winsaveview().topline, winsaveview().skipcol])

  let target_col = stridx(line, 'bbbb') + 120
  call winrestview({'lnum': 1, 'col': target_col - 1, 'topline': 1,
        \ 'leftcol': 0, 'skipcol': 80, 'curswant': target_col - 1})
  call cursor(1, target_col)
  redraw!
  let saved = winsaveview()
  call assert_equal([1, target_col, 1, 80, 3, 10],
        \ [line('.'), col('.'), saved.topline, saved.skipcol,
        \ winline(), wincol()])
  call cursor(3, 1)
  normal! zt
  call winrestview(saved)
  redraw!
  call assert_equal([1, target_col, 1, 80, 3, 10],
        \ [line('.'), col('.'), winsaveview().topline,
        \ winsaveview().skipcol, winline(), wincol()])

  syntax clear Hidden
  call CloseWindow()

  call NewWindow(7, 40)
  setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
        \ scrolloff=0 scrolljump=1
  syntax match Hidden /HIDDEN / conceal

  let line = repeat('a', 42)
        \ .. ' HIDDEN target words after hidden text to force wrapping'
        \ .. ' and mapping checks'
  let target_col = stridx(line, 'target') + 1
  call setline(1, ['one', 'two', 'three', line, 'five', 'six', 'seven',
        \ 'eight', 'nine', 'ten', 'eleven'])

  let expected = #{zt: [2, 4], zz: [4, 4], zb: [5, 4]}
  for cmd in ['zt', 'zz', 'zb']
    call cursor(4, target_col)
    execute 'normal! ' .. cmd
    redraw!
    call assert_equal([4, target_col] + expected[cmd],
          \ [line('.'), col('.'), winline(), wincol()])
  endfor

  setlocal scrolloff=2 scrolljump=2
  call cursor(1, 1)
  normal! zt
  normal! 5j
  call assert_equal([6, 1, 4, 5],
        \ [line('.'), col('.'), winsaveview().topline, winline()])
  normal! 2k
  call assert_equal([4, 1, 2, 3],
        \ [line('.'), col('.'), winsaveview().topline, winline()])

  syntax clear Hidden
  call CloseWindow()
endfunc

func Test_conceallevel_three_scroll_wheel()
  call NewWindow(5, 40)
  set mouse=a
  setlocal wrap smoothscroll conceallevel=3 concealcursor=nvic
        \ signcolumn=no nonumber scrolloff=0 scroll=3
  syntax match Hidden /HIDDEN / conceal

  let line = repeat('a', 42) .. ' HIDDEN ' .. repeat('b', 240)
  let target_col = stridx(line, 'bbbb') + 120
  call setline(1, [line, 'second line', 'third line'])
  call winrestview({'lnum': 1, 'col': target_col - 1, 'topline': 1,
        \ 'leftcol': 0, 'skipcol': 80, 'curswant': target_col - 1})
  call cursor(1, target_col)
  redraw!

  call feedkeys("\<ScrollWheelDown>", 'xt')
  call assert_equal([1, target_col, 1, 160, 1, 3],
        \ [line('.'), col('.'), winsaveview().topline,
        \ winsaveview().skipcol, winline(), wincol()])
  call feedkeys("\<ScrollWheelUp>", 'xt')
  call assert_equal([1, target_col, 1, 80, 3, 3],
        \ [line('.'), col('.'), winsaveview().topline,
        \ winsaveview().skipcol, winline(), wincol()])

  call winrestview({'lnum': 1, 'col': 0, 'topline': 1, 'leftcol': 0,
        \ 'skipcol': 0, 'curswant': 0})
  call cursor(1, 1)
  call feedkeys("v\<ScrollWheelDown>", 'xt')
  call assert_equal([1, 121, 1, 80, 1, 34],
        \ [line('.'), col('.'), winsaveview().topline,
        \ winsaveview().skipcol, winline(), wincol()])
  call feedkeys("\<Esc>", 'tx')

  syntax clear Hidden
  call CloseWindow()
  set mouse&
endfunc

func s:CurrentScreenCell() abort
  let [win_row, win_col] = win_screenpos(0)
  let pos = screenpos(0, line('.'), col('.'))
  call assert_true(pos.row > 0)

  let cell = [pos.row - win_row + 1, pos.curscol - win_col + 1]
  call assert_equal(cell, [winline(), wincol()])
  return cell
endfunc

func s:FileEndsWith(file, line) abort
  return filereadable(a:file) && readfile(a:file)[-1 : ] == [a:line]
endfunc

func s:FileEquals(file, lines) abort
  return filereadable(a:file) && readfile(a:file) == a:lines
endfunc

func s:ConceallevelThreeCursorCell(bufcol) abort
  call cursor(1, a:bufcol)
  redraw!
  return s:CurrentScreenCell()
endfunc

func s:ConceallevelThreeScreenlineMoveCell(bufcol) abort
  call cursor(1, a:bufcol)
  redraw!
  normal! gk
  return s:CurrentScreenCell()
endfunc

func Test_conceallevel_three_option_invalidation()
  call NewWindow(10, 40)
  setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
  syntax match Hidden /HIDDEN / conceal

  let line = repeat('a', 42)
        \ .. ' HIDDEN target words after hidden text to force wrapping'
        \ .. ' and mapping checks'
  call setline(1, line)
  let target_col = stridx(line, 'target') + 1

  call assert_equal([2, 4], s:ConceallevelThreeCursorCell(target_col))

  setlocal concealcursor=
  call assert_equal([2, 11], s:ConceallevelThreeCursorCell(target_col))

  setlocal concealcursor=nvic
  call assert_equal([2, 4], s:ConceallevelThreeCursorCell(target_col))

  setlocal conceallevel=0
  call assert_equal([2, 11], s:ConceallevelThreeCursorCell(target_col))

  setlocal conceallevel=3
  call assert_equal([2, 4], s:ConceallevelThreeCursorCell(target_col))

  setlocal linebreak breakindent showbreak=++
  call assert_equal([2, 6], s:ConceallevelThreeCursorCell(target_col))

  setlocal nolinebreak nobreakindent showbreak=
  call assert_equal([2, 4], s:ConceallevelThreeCursorCell(target_col))

  vertical resize 50
  call assert_equal(50, winwidth(0))
  call assert_equal([1, 44], s:ConceallevelThreeCursorCell(target_col))

  vertical resize 40
  call assert_equal(40, winwidth(0))
  call assert_equal([2, 4], s:ConceallevelThreeCursorCell(target_col))

  syntax clear Hidden
  call CloseWindow()
endfunc

func Test_conceallevel_three_screenline_option_invalidation()
  let save_ambiwidth = &ambiwidth

  call NewWindow(10, 40)
  try
    setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
    syntax match Hidden /HIDDEN / conceal

    let line = 'a' .. "\t" .. ' HIDDEN target words after hidden text'
    call setline(1, line)
    let target_col = stridx(line, 'target') + 1
    setlocal tabstop=8
    call assert_equal([1, 10],
          \ s:ConceallevelThreeScreenlineMoveCell(target_col))

    setlocal tabstop=4
    call assert_equal([1, 6],
          \ s:ConceallevelThreeScreenlineMoveCell(target_col))

    setlocal tabstop=8
    call assert_equal([1, 10],
          \ s:ConceallevelThreeScreenlineMoveCell(target_col))

    if has('multi_byte')
      let line = 'a' .. repeat('·', 20)
            \ .. ' HIDDEN target words after hidden text'
      call setline(1, line)
      let target_col = stridx(line, 'target') + 1
      set ambiwidth=single
      call assert_equal([1, 23],
            \ s:ConceallevelThreeScreenlineMoveCell(target_col))

      set ambiwidth=double
      call assert_equal([1, 4],
            \ s:ConceallevelThreeScreenlineMoveCell(target_col))

      set ambiwidth=single
      call assert_equal([1, 23],
            \ s:ConceallevelThreeScreenlineMoveCell(target_col))
    endif

    if has('rightleft')
      let line = repeat('a', 42)
            \ .. ' HIDDEN target words after hidden text to force wrapping'
            \ .. ' and mapping checks'
      call setline(1, line)
      let target_col = stridx(line, 'target') + 1
      setlocal norightleft
      call assert_equal([1, 4],
            \ s:ConceallevelThreeScreenlineMoveCell(target_col))

      setlocal rightleft
      call cursor(1, target_col)
      call assert_equal([2, 37], s:CurrentScreenCell())
      call assert_equal([1, 37],
            \ s:ConceallevelThreeScreenlineMoveCell(target_col))

      setlocal norightleft
      call assert_equal([1, 4],
            \ s:ConceallevelThreeScreenlineMoveCell(target_col))

      if has('multi_byte')
        setlocal rightleft
        let line = 'aaあ' .. repeat('a', 38)
              \ .. ' HIDDEN target words after hidden text'
        call setline(1, line)
        let wide_col = stridx(line, 'あ') + 1
        let target_col = stridx(line, 'target') + 1

        call cursor(1, wide_col)
        let pos = screenpos(0, 1, wide_col)
        let [winrow, wincol] = win_screenpos(0)
        call assert_equal([1, 3, 37, 4],
              \ [pos.row - winrow + 1, pos.col - wincol + 1,
              \ pos.curscol - wincol + 1, pos.endcol - wincol + 1])
        call assert_equal([1, 37], s:CurrentScreenCell())

        call assert_equal([1, 37],
              \ s:ConceallevelThreeScreenlineMoveCell(target_col))
        call assert_equal(wide_col, col('.'))
      endif

      setlocal norightleft
    endif
  finally
    execute 'set ambiwidth=' .. save_ambiwidth
    syntax clear Hidden
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_syntax_match_cache_invalidation()
  call NewWindow(8, 20)
  try
    setlocal wrap conceallevel=3 concealcursor=n signcolumn=no nonumber
    let text = repeat('X ', 31)
    call setline(1, [text, 'after'])
    call cursor(2, 1)
    redraw!
    call assert_equal(5, screenpos(0, 2, 1).row)

    " Concealed keywords are not stored in b_syn_patterns.
    syntax keyword Hidden X conceal
    redraw!
    call assert_equal(3, screenpos(0, 2, 1).row)

    call cursor(1, 2)
    normal! gj
    call assert_equal([1, 42, 2, 1],
          \ [line('.'), col('.'), winline(), wincol()])

    " Exercise the case-insensitive keyword table as well.
    syntax clear Hidden
    syntax case ignore
    syntax keyword Hidden x conceal
    call cursor(1, 2)
    normal! gj
    call assert_equal([1, 42, 2, 1],
          \ [line('.'), col('.'), winline(), wincol()])
    syntax case match

    " Replacing a pattern can leave the pattern count unchanged.
    syntax clear Hidden
    syntax match Hidden /X/ conceal
    call cursor(2, 1)
    redraw!
    call assert_equal(3, screenpos(0, 2, 1).row)

    syntax clear Hidden
    syntax match Hidden /Z/ conceal
    redraw!
    call assert_equal(5, screenpos(0, 2, 1).row)

    " A syntax replacement with the same pattern count must invalidate both
    " the rendered-cell map and a cursor position saved by a previous motion.
    vertical resize 12
    call setline(1, 'XXXXabcdefghYYYYijklmnop')
    syntax clear Hidden
    syntax match Hidden /XXXX/ conceal
    call cursor(1, 5)
    normal! gj
    call assert_equal([17, 2, 1], [col('.'), winline(), wincol()])

    syntax clear Hidden
    syntax match Hidden /YYY/ conceal
    redraw!
    let [winrow, wincol] = win_screenpos(0)
    let pos = screenpos(0, line('.'), col('.'))
    call assert_equal([2, 2],
          \ [pos.row - winrow + 1, pos.curscol - wincol + 1])
    call assert_equal([2, 2], [winline(), wincol()])
    call cursor(1, 5)
    normal! gj
    call assert_equal(20, col('.'))

    call cursor(1, 5)
    syntax clear Hidden
    syntax match Hidden /YYYY/ conceal
    normal! gj
    call assert_equal(21, col('.'))
  finally
    syntax case match
    syntax clear Hidden
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_syntax_option_cache_invalidation()
  call NewWindow(10, 20)
  try
    setlocal wrap conceallevel=3 concealcursor=n signcolumn=no nonumber

    syntax match Hidden /X/ conceal
    call setline(1, [repeat('a', 60) .. repeat('X', 60), 'after'])
    setlocal synmaxcol=0
    call cursor(2, 1)
    redraw!
    let before = screenpos(0, 2, 1).row

    setlocal synmaxcol=50
    redraw!
    let changed = screenpos(0, 2, 1).row
    let saved = getline(1)
    call setline(1, saved .. 'x')
    call setline(1, saved)
    redraw!
    let fresh = screenpos(0, 2, 1).row
    call assert_notequal(before, fresh)
    call assert_equal(fresh, changed)

    syntax clear Hidden
    setlocal synmaxcol=0 iskeyword-=.
    syntax keyword Hidden foo conceal
    call setline(1, [repeat('foo.', 30), 'after'])
    call cursor(2, 1)
    redraw!
    let before = screenpos(0, 2, 1).row

    setlocal iskeyword+=.
    redraw!
    let changed = screenpos(0, 2, 1).row
    let saved = getline(1)
    call setline(1, saved .. 'x')
    call setline(1, saved)
    redraw!
    let fresh = screenpos(0, 2, 1).row
    call assert_notequal(before, fresh)
    call assert_equal(fresh, changed)

    setlocal iskeyword-=.
    call cursor(1, 4)
    normal! gj
    normal! gk
    call assert_equal(4, col('.'))
    let before = [line('.'), col('.'), winline(), wincol()]

    setlocal iskeyword+=.
    call cursor(1, 4)
    normal! gj
    redraw!
    let changed = [line('.'), col('.'), winline(), wincol()]

    let saved = getline(1)
    call setline(1, saved .. 'x')
    call setline(1, saved)
    call cursor(1, 4)
    normal! gj
    redraw!
    let fresh = [line('.'), col('.'), winline(), wincol()]
    call assert_notequal(before, fresh)
    call assert_equal(fresh, changed)

    " 'lisp' changes the buffer keyword table without changing 'iskeyword'.
    syntax clear Hidden
    setlocal nolisp
    syntax keyword Hidden foo-foo conceal
    call setline(1, [repeat('foo-foo ', 20), 'after'])
    call cursor(2, 1)
    redraw!
    let before = screenpos(0, 2, 1).row

    setlocal lisp
    redraw!
    let changed = screenpos(0, 2, 1).row
    let saved = getline(1)
    call setline(1, saved .. 'x')
    call setline(1, saved)
    redraw!
    let fresh = screenpos(0, 2, 1).row
    call assert_notequal(before, fresh)
    call assert_equal(fresh, changed)
  finally
    syntax clear Hidden
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_dynamic_visibility_cache_invalidation()
  let matchid = -1

  call NewWindow(10, 20)
  try
    setlocal wrap conceallevel=3 concealcursor=n signcolumn=no nonumber
    let text = repeat('X', 12) .. 'abcde' .. repeat('Y', 12)
          \ .. 'qrstuvwxyzabcdefghijklmnopqrstuv'
    call setline(1, text)
    syntax match Hidden /Y\+/ conceal

    call cursor(1, 13)
    normal! gj
    call assert_equal(45, col('.'))

    " Visual mode reveals the whole selected line when 'concealcursor' does
    " not contain "v".  Do not reuse its concealed Normal-mode map.
    call cursor(1, 13)
    normal! v
    redraw!
    normal! gj
    call assert_equal([33, 2, 13], [col('.'), winline(), wincol()])
    execute "normal! \<Esc>"

    syntax clear Hidden
    call setline(1, repeat('X ', 31))
    call cursor(1, 2)
    normal! gj
    normal! gk

    let matchid = matchadd('Conceal', 'X', 10, -1, #{conceal: ''})
    call cursor(1, 2)
    normal! gj
    call assert_equal([42, 2, 1], [col('.'), winline(), wincol()])

    call matchdelete(matchid)
    let matchid = -1
    syntax match Hidden /\%<'mX/ conceal
    call setline(1, [repeat('X', 160), 'after'])
    call setpos("'m", [0, 1, 161, 0])
    call cursor(2, 1)
    redraw!
    call assert_equal(2, screenpos(0, 2, 1).row)

    " Ordered mark atoms change without updating b:changedtick.
    call setpos("'m", [0, 1, 2, 0])
    redraw!
    call assert_equal(9, screenpos(0, 2, 1).row)

    syntax clear Hidden
    call setpos("'m", [0, 1, 161, 0])
    let matchid = matchadd('Conceal', '\%<''mX', 10, -1,
          \ #{conceal: ''})
    redraw!
    call assert_equal(2, screenpos(0, 2, 1).row)

    call setpos("'m", [0, 1, 2, 0])
    redraw!
    call assert_equal(9, screenpos(0, 2, 1).row)
  finally
    if matchid > 0
      silent! call matchdelete(matchid)
    endif
    syntax clear Hidden
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_textprop_map_cache_invalidation()
  CheckFeature textprop

  let global_type = 'ConcealCacheText'
  let local_type = 'ConcealCacheLocalText'
  let scratch_buf = -1

  call NewWindow(8, 20)
  let conceal_buf = bufnr()
  try
    setlocal wrap conceallevel=3 concealcursor=n signcolumn=no nonumber
    syntax match Hidden /^X/ conceal
    call prop_type_add(global_type, {})
    call setline(1, ['X' .. repeat('a', 50), 'after'])

    call cursor(1, 2)
    normal! gj
    call assert_equal([22, 2, 1], [col('.'), winline(), wincol()])
    normal! gk
    redraw!
    let base_row = screenpos(0, 2, 1).row

    let propid = prop_add(1, 2,
          \ #{type: global_type, text: repeat('V', 10)})
    call cursor(1, 2)
    normal! gj
    call assert_equal([22, 2, 11], [col('.'), winline(), wincol()])

    call assert_equal(1, prop_remove(#{id: propid}))
    call cursor(1, 2)
    normal! gj
    call assert_equal([22, 2, 1], [col('.'), winline(), wincol()])

    call prop_add(1, 0,
          \ #{type: global_type, text: 'above', text_align: 'above'})
    redraw!
    call assert_equal(base_row + 1, screenpos(0, 2, 1).row)
    call prop_clear(1)
    redraw!
    call assert_equal(base_row, screenpos(0, 2, 1).row)

    call prop_add(1, 2,
          \ #{type: global_type, text: repeat('V', 10)})
    call cursor(1, 2)
    normal! gj
    call assert_equal([22, 2, 11], [col('.'), winline(), wincol()])

    " A global type change must invalidate the cached state without walking
    " every buffer, including when the affected buffer is not current.
    hide enew
    let scratch_buf = bufnr()
    call prop_type_delete(global_type)
    execute 'buffer ' .. conceal_buf
    call cursor(1, 2)
    normal! gj
    call assert_equal([22, 2, 1], [col('.'), winline(), wincol()])

    call prop_type_add(local_type, #{bufnr: bufnr()})
    call prop_add(1, 2,
          \ #{type: local_type, text: repeat('V', 10)})
    call cursor(1, 2)
    normal! gj
    call assert_equal([22, 2, 11], [col('.'), winline(), wincol()])
    call prop_type_delete(local_type, #{bufnr: bufnr()})
    call cursor(1, 2)
    normal! gj
    call assert_equal([22, 2, 1], [col('.'), winline(), wincol()])
  finally
    silent! call prop_clear(1)
    silent! call prop_type_delete(global_type)
    silent! call prop_type_delete(local_type, #{bufnr: bufnr()})
    if scratch_buf > 0 && bufexists(scratch_buf)
      execute 'bwipe! ' .. scratch_buf
    endif
    syntax clear Hidden
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_display_width_cache_invalidation()
  let save_display = &display

  call NewWindow(10, 20)
  try
    setlocal wrap conceallevel=3 concealcursor=n signcolumn=no nonumber
    syntax match Hidden /HIDDEN\|X/ conceal

    set display=
    call setline(1, ['X' .. repeat(nr2char(1), 21), 'after'])
    call cursor(2, 1)
    redraw!
    call assert_equal(4, screenpos(0, 2, 1).row)

    set display=uhex
    redraw!
    call assert_equal(6, screenpos(0, 2, 1).row)

    call setline(1, repeat(nr2char(1), 30) .. ' HIDDEN tail')
    call deletebufline(bufnr(), 2, '$')
    set display=
    call cursor(1, 1)
    normal! gj
    call assert_equal(11, col('.'))

    call cursor(1, 1)
    set display=uhex
    normal! gj
    call assert_equal(6, col('.'))
  finally
    let &display = save_display
    syntax clear Hidden
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_combining_marks_screenpos()
  if !has('multi_byte') || &encoding !=# 'utf-8'
    return
  endif

  call NewWindow(10, 40)
  try
    setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
    syntax match Hidden /HIDDEN / conceal

    let plain_prefix = repeat('e', 42)
    let composed_prefix = repeat("e\u0301\u0308", 42)
    let suffix = ' HIDDEN target words after hidden text to force wrapping'
    call assert_equal(strdisplaywidth(plain_prefix),
          \ strdisplaywidth(composed_prefix))
    call assert_true(strlen(plain_prefix) < strlen(composed_prefix))

    let line = composed_prefix .. suffix
    call setline(1, line)
    let composed_col = stridx(line, 'target') + 1
    let composed_cell = s:ConceallevelThreeCursorCell(composed_col)

    let line = plain_prefix .. suffix
    call setline(1, line)
    let plain_col = stridx(line, 'target') + 1
    let plain_cell = s:ConceallevelThreeCursorCell(plain_col)

    call assert_equal(plain_cell, composed_cell)
    call assert_equal([2, 4], composed_cell)
  finally
    syntax clear Hidden
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_emoji_width_screenpos()
  if !has('multi_byte') || &encoding !=# 'utf-8'
    return
  endif

  let save_emoji = &emoji

  call NewWindow(10, 40)
  try
    setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
    syntax match Hidden /HIDDEN / conceal

    let emoji_char = nr2char(0x1f321)
    let prefix = repeat(emoji_char, 41)
    let line = prefix .. ' HIDDEN target words after hidden text'
    call setline(1, line)
    let target_col = stridx(line, 'target') + 1

    set emoji
    call assert_equal(82, strdisplaywidth(prefix))
    call assert_equal([3, 4], s:ConceallevelThreeCursorCell(target_col))

    set noemoji
    call assert_equal(41, strdisplaywidth(prefix))
    call assert_equal([2, 3], s:ConceallevelThreeCursorCell(target_col))

    set emoji
    call assert_equal([3, 4], s:ConceallevelThreeCursorCell(target_col))

    call setline(1, ['HIDDEN ' .. repeat(emoji_char, 21), 'after'])
    call cursor(2, 1)
    set noemoji
    redraw!
    call assert_equal(2, screenpos(0, 2, 1).row)

    set emoji
    redraw!
    call assert_equal(3, screenpos(0, 2, 1).row)
  finally
    let &emoji = save_emoji
    syntax clear Hidden
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_custom_cell_width_screenpos()
  if !has('multi_byte') || &encoding !=# 'utf-8'
    return
  endif

  call NewWindow(10, 40)
  try
    setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
    syntax match Hidden /HIDDEN / conceal

    let width_char = nr2char(0x279c)
    let prefix = repeat(width_char, 41)
    let line = prefix .. ' HIDDEN target words after hidden text'
    call setline(1, line)
    let target_col = stridx(line, 'target') + 1

    call setcellwidths([[0x279c, 0x279c, 1]])
    call assert_equal(41, strdisplaywidth(prefix))
    call assert_equal([2, 3], s:ConceallevelThreeCursorCell(target_col))

    call setcellwidths([[0x279c, 0x279c, 2]])
    call assert_equal(82, strdisplaywidth(prefix))
    call assert_equal([3, 4], s:ConceallevelThreeCursorCell(target_col))

    call setcellwidths([[0x279c, 0x279c, 1]])
    call assert_equal([2, 3], s:ConceallevelThreeCursorCell(target_col))

    call setline(1, ['HIDDEN ' .. repeat(width_char, 21), 'after'])
    call cursor(2, 1)
    call setcellwidths([[0x279c, 0x279c, 1]])
    redraw!
    call assert_equal(2, screenpos(0, 2, 1).row)

    call setcellwidths([[0x279c, 0x279c, 2]])
    redraw!
    call assert_equal(3, screenpos(0, 2, 1).row)
  finally
    call setcellwidths([])
    syntax clear Hidden
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_non_utf8_encoding_screenpos()
  let after =<< trim [CODE]
    scriptencoding utf-8
    source util/view_util.vim

    func CheckConcealedTarget(prefix) abort
      call NewWindow(10, 40)
      try
        setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
        syntax match Hidden /HIDDEN / conceal

        let line = a:prefix .. ' HIDDEN target words after hidden text'
        call setline(1, line)
        let target_col = stridx(line, 'target') + 1

        call assert_equal(42, strdisplaywidth(a:prefix), &encoding)
        call cursor(1, target_col)
        redraw!

        let [win_row, win_col] = win_screenpos(0)
        let pos = screenpos(0, 1, target_col)
        call assert_true(pos.row > 0, &encoding)

        let cell = [pos.row - win_row + 1, pos.curscol - win_col + 1]
        call assert_equal([2, 4], cell, &encoding)
        call assert_equal(cell, [winline(), wincol()], &encoding)
      finally
        syntax clear Hidden
        call CloseWindow()
      endtry
    endfunc

    if &encoding ==# 'latin1'
      call CheckConcealedTarget(repeat(nr2char(0xe9), 42))
    else
      call CheckConcealedTarget(repeat('口', 21))
    endif
    call writefile(v:errors, 'Xresult')
    qall!
  [CODE]

  let encodings = ['latin1']
  if has('multi_byte')
    let encodings += ['cp932', 'cp936', 'cp949', 'cp950']
    if !has('win32')
      let encodings += ['euc-jp']
    endif
  endif
  for enc in encodings
    let msg = 'enc=' .. enc
    if RunVim([], after, $'--clean --cmd "set encoding={enc}"')
      call assert_equal([], readfile('Xresult'), msg)
    endif
    call delete('Xresult')
  endfor
endfunc

func Test_conceallevel_three_hidden_tab_showbreak_vartabstop()
  CheckFeature vartabs

  call NewWindow(10, 40)
  try
    setlocal wrap linebreak showbreak=++ conceallevel=3 concealcursor=nvic
          \ signcolumn=no nonumber
    syntax match Hidden /\tHIDDEN / conceal

    let line = repeat('a', 40) .. "\tHIDDEN target words after hidden text"
    call setline(1, line)
    let target_col = stridx(line, 'target') + 1

    setlocal tabstop=3 vartabstop=
    call assert_equal([2, 3], s:ConceallevelThreeCursorCell(target_col))

    setlocal tabstop=8 vartabstop=5,11,17
    call assert_equal([2, 3], s:ConceallevelThreeCursorCell(target_col))

    setlocal tabstop=4 vartabstop=9,13
    call assert_equal([2, 3], s:ConceallevelThreeCursorCell(target_col))
  finally
    syntax clear Hidden
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_listchars_screenpos()
  if !has('multi_byte') || &encoding !=# 'utf-8'
    return
  endif

  call NewWindow(10, 40)
  try
    setlocal wrap list listchars=tab:>-,trail:<,nbsp:=,eol:$
          \ conceallevel=3 concealcursor=nvic signcolumn=no nonumber
          \ tabstop=8
    syntax match Hidden /HIDDEN / conceal

    let nbsp = nr2char(0xa0)
    let line = repeat('a', 34) .. "\t" .. nbsp .. ' HIDDEN target  '
    call setline(1, line)
    let target_col = stridx(line, 'target') + 1
    let trail_col = strlen(line) - 1

    call assert_equal([2, 3], s:ConceallevelThreeCursorCell(target_col))
    call assert_equal([2, 9], s:ConceallevelThreeCursorCell(trail_col))
    call assert_equal([repeat('a', 34) .. '>-----',
          \ '= target<<$                             '],
          \ ScreenLines([1, 2], 40))

    setlocal nolist
    call assert_equal([2, 3], s:ConceallevelThreeCursorCell(target_col))
    call assert_equal([2, 9], s:ConceallevelThreeCursorCell(trail_col))
  finally
    syntax clear Hidden
    call CloseWindow()
  endtry

  call NewWindow(10, 20)
  try
    setlocal nowrap list listchars=extends:>,precedes:<,eol:$
          \ conceallevel=3 concealcursor=nvic signcolumn=no nonumber
    syntax match Hidden /HIDDEN / conceal

    let line = repeat('a', 24) .. ' HIDDEN target words after hidden text '
          \ .. repeat('b', 20)
    call setline(1, line)
    let target_col = stridx(line, 'target') + 1

    call assert_equal([1, 4], s:ConceallevelThreeCursorCell(target_col))
    call assert_equal(['<a target words aft>'], ScreenLines(1, 20))
  finally
    syntax clear Hidden
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_screenline_list_invalidation()
  call NewWindow(10, 40)
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
        \ signcolumn=no nonumber listchars=tab:>-,eol:$
  syntax match Hidden /HIDDEN / conceal

  let line = 'a' .. "\t" .. ' HIDDEN target words after hidden text'
  call setline(1, line)
  let target_col = stridx(line, 'target') + 1

  setlocal nolist
  call assert_equal([1, 10],
        \ s:ConceallevelThreeScreenlineMoveCell(target_col))
  let nolist_line = ScreenLines(1, 40)[0]

  setlocal list
  call assert_equal([1, 10],
        \ s:ConceallevelThreeScreenlineMoveCell(target_col))
  let list_line = ScreenLines(1, 40)[0]
  call assert_match('^a>------ target words after hidden text', list_line)
  call assert_true(stridx(list_line, '$') >= 0)
  call assert_notequal(nolist_line, list_line)

  setlocal nolist
  call assert_equal([1, 10],
        \ s:ConceallevelThreeScreenlineMoveCell(target_col))

  syntax clear Hidden
  call CloseWindow()
endfunc

func Test_conceallevel_three_tabpage_screenline_state()
  try
    tabnew
    botright vertical new
    wincmd p
    vertical resize 40
    set winfixwidth
    call assert_equal(40, winwidth(0))

    setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
          \ showbreak=
    syntax match Hidden /HIDDEN / conceal

    let line = repeat('a', 42)
          \ .. ' HIDDEN target words after hidden text to force wrapping'
          \ .. ' and mapping checks'
    call setline(1, line)
    let target_col = stridx(line, 'target') + 1
    let plain_tab = tabpagenr()
    let plain_buf = bufnr()

    call assert_equal([1, 4],
          \ s:ConceallevelThreeScreenlineMoveCell(target_col))

    tab split
    botright vertical new
    wincmd p
    vertical resize 40
    set winfixwidth
    call assert_equal(40, winwidth(0))

    let lbr_tab = tabpagenr()
    call assert_equal(plain_buf, bufnr())
    setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
          \ signcolumn=no nonumber showbreak=++
    call assert_equal([1, 6],
          \ s:ConceallevelThreeScreenlineMoveCell(target_col))

    execute 'tabnext ' .. plain_tab
    call assert_equal(plain_buf, bufnr())
    call assert_equal([1, 4],
          \ s:ConceallevelThreeScreenlineMoveCell(target_col))

    execute 'tabnext ' .. lbr_tab
    call assert_equal(plain_buf, bufnr())
    call assert_equal([1, 6],
          \ s:ConceallevelThreeScreenlineMoveCell(target_col))
  finally
    silent! syntax clear Hidden
    silent! tabonly!
    silent! only!
    silent! enew!
  endtry
endfunc

func Test_conceallevel_three_diff_filler_screenpos()
  CheckFeature diff

  let save_diffopt = &diffopt
  let save_lines = &lines
  let save_columns = &columns

  try
    set lines=20 columns=80 diffopt=internal,filler
    tabnew
    setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
    let line = repeat('a', 42)
          \ .. ' HIDDEN target words after hidden text to force wrapping'
          \ .. ' and mapping checks'
    call setline(1, ['common', 'insert one', 'insert two', line, 'after'])

    vnew
    setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
    call setline(1, ['common', line, 'after'])
    let filler_win = win_getid()

    windo syntax match Hidden /HIDDEN / conceal
    windo diffthis
    call assert_true(win_gotoid(filler_win))

    let target_col = stridx(line, 'target') + 1
    call assert_equal(2, diff_filler(2))
    call cursor(2, target_col)
    redraw!

    let [win_row, win_col] = win_screenpos(0)
    let pos = screenpos(0, 2, target_col)
    call assert_true(pos.row > 0)
    call assert_equal([winline(), wincol()],
          \ [pos.row - win_row + 1, pos.curscol - win_col + 1])
  finally
    silent! windo diffoff!
    silent! windo syntax clear Hidden
    silent! tabonly!
    silent! only!
    silent! enew!
    let &diffopt = save_diffopt
    let &columns = save_columns
    let &lines = save_lines
  endtry
endfunc

func Test_conceallevel_three_diff_scrollbind_cursorbind()
  CheckFeature diff

  let save_diffopt = &diffopt
  let save_lines = &lines
  let save_columns = &columns

  try
    set lines=20 columns=80 diffopt=internal,filler,foldcolumn:0
    tabnew
    setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
    let line = repeat('a', 42)
          \ .. ' HIDDEN target words after hidden text to force wrapping'
          \ .. ' and mapping checks'
    call setline(1, ['common', 'insert one', 'insert two', line, 'after',
          \ 'insert three', line, 'tail'])
    let left_win = win_getid()

    vnew
    setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
    call setline(1, ['common', line, 'after', line, 'tail'])
    let right_win = win_getid()

    windo syntax match Hidden /HIDDEN / conceal
    windo diffthis
    windo setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no
          \ nonumber scrollbind cursorbind nofoldenable

    let target_col = stridx(line, 'target') + 1
    for [right_lnum, left_lnum] in [[2, 4], [4, 7]]
      call assert_true(win_gotoid(right_win))
      call cursor(right_lnum, target_col)
      normal! l
      normal! zt
      redraw!

      call assert_equal([right_lnum, target_col + 1],
            \ [line('.'), col('.')])
      call assert_equal(right_lnum, line('w0'))
      let [win_row, win_col] = win_screenpos(0)
      let pos = screenpos(0, line('.'), col('.'))
      call assert_true(pos.row > 0)
      call assert_equal([winline(), wincol()],
            \ [pos.row - win_row + 1, pos.curscol - win_col + 1])

      call assert_true(win_gotoid(left_win))
      redraw!
      call assert_equal([left_lnum, target_col + 1],
            \ [line('.'), col('.')])
      call assert_equal(left_lnum, line('w0'))
      let [win_row, win_col] = win_screenpos(0)
      let pos = screenpos(0, line('.'), col('.'))
      call assert_true(pos.row > 0)
      call assert_equal([winline(), wincol()],
            \ [pos.row - win_row + 1, pos.curscol - win_col + 1])
    endfor
  finally
    silent! windo diffoff!
    silent! windo syntax clear Hidden
    silent! tabonly!
    silent! only!
    silent! enew!
    let &diffopt = save_diffopt
    let &columns = save_columns
    let &lines = save_lines
  endtry
endfunc

func Test_conceallevel_three_fold_open_close()
  CheckFeature folding

  call NewWindow(8, 40)
  setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
        \ foldcolumn=0 foldmethod=manual foldenable foldlevel=99
  syntax match Hidden /HIDDEN / conceal

  let line = repeat('a', 42)
        \ .. ' HIDDEN target words after hidden text to force wrapping'
        \ .. ' and mapping checks'
  let target_col = stridx(line, 'target') + 1
  call setline(1, ['before', line, 'inside fold', 'after'])
  2,3fold
  2foldopen

  call cursor(2, target_col)
  redraw!
  let [win_row, win_col] = win_screenpos(0)
  let pos = screenpos(0, 2, target_col)
  call assert_true(pos.row > 0)
  call assert_equal([winline(), wincol()],
        \ [pos.row - win_row + 1, pos.curscol - win_col + 1])
  let open_cell = [winline(), wincol()]
  let open_after_row = screenpos(0, 4, 1).row - win_row + 1
  call assert_true(open_cell[0] < open_after_row)

  2foldclose
  redraw!
  call assert_equal(2, foldclosed(2))
  call assert_equal(3, foldclosedend(2))
  let pos = screenpos(0, 2, target_col)
  call assert_true(pos.row > 0)
  call assert_equal([winline(), wincol()],
        \ [pos.row - win_row + 1, pos.curscol - win_col + 1])
  call assert_equal(1, wincol())
  let closed_after_row = screenpos(0, 4, 1).row - win_row + 1
  call assert_true(closed_after_row < open_after_row)

  2foldopen
  call cursor(2, target_col)
  redraw!
  let pos = screenpos(0, 2, target_col)
  call assert_true(pos.row > 0)
  call assert_equal(open_cell, [winline(), wincol()])
  call assert_equal(open_cell,
        \ [pos.row - win_row + 1, pos.curscol - win_col + 1])
  call assert_equal(open_after_row, screenpos(0, 4, 1).row - win_row + 1)

  syntax clear Hidden
  call CloseWindow()
endfunc

func Test_conceallevel_three_foldcolumn_mouse_click()
  CheckFeature folding

  let save_mouse = &mouse

  try
    call NewWindow(8, 40)
    set mouse=a
    setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
          \ foldcolumn=1 foldmethod=manual foldenable foldlevel=99
    setlocal fillchars=foldopen:-,foldclose:+,foldsep:\|
    syntax match Hidden /HIDDEN / conceal

    let line = 'HIDDEN target words after hidden text to force wrapping'
          \ .. ' and mapping checks'
    let target_col = stridx(line, 'target') + 1
    call setline(1, ['before', line, 'inside fold', 'after'])
    2,3fold
    normal! zR
    call cursor(2, target_col)
    redraw!

    let [win_row, win_col] = win_screenpos(0)
    let pos = screenpos(0, 2, target_col)
    call assert_true(pos.row > 0)
    call assert_equal(win_col + 1, pos.curscol)
    call assert_equal('-', screenstring(pos.row, win_col))

    call test_setmouse(pos.row, pos.curscol)
    call feedkeys("\<LeftMouse>", 'tx')
    call assert_equal([-1, 2, target_col],
          \ [foldclosed(2), line('.'), col('.')])

    call test_setmouse(pos.row, win_col)
    call feedkeys("\<LeftMouse>", 'tx')
    redraw!
    call assert_equal([2, 3], [foldclosed(2), foldclosedend(2)])
    call assert_equal('+', screenstring(pos.row, win_col))

    call test_setmouse(pos.row, win_col)
    call feedkeys("\<LeftMouse>", 'tx')
    redraw!
    call assert_equal(-1, foldclosed(2))

    let pos = screenpos(0, 2, target_col)
    call assert_true(pos.row > 0)
    call assert_equal(win_col + 1, pos.curscol)
    call test_setmouse(pos.row, pos.curscol)
    call feedkeys("\<LeftMouse>", 'tx')
    call assert_equal([-1, 2, target_col],
          \ [foldclosed(2), line('.'), col('.')])
  finally
    let &mouse = save_mouse
    silent! syntax clear Hidden
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_closed_fold_screenline_motion()
  CheckFeature folding

  call NewWindow(8, 40)
  setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
        \ foldcolumn=0 foldmethod=manual foldenable foldlevel=0
  syntax match Hidden /HIDDEN / conceal

  let line = repeat('a', 38)
        \ .. ' HIDDEN target words after hidden text to force wrapping'
        \ .. ' and mapping checks'
  call setline(1, ['fold start', 'fold inside', line, 'after'])
  1,2fold
  normal! zM

  " A fresh move from below the fold must land on its displayed first line,
  " not on a hidden buffer line inside it.
  call cursor(3, 1)
  redraw!
  normal! gk
  call assert_equal([1, 1], [line('.'), col('.')])

  call cursor(1, 1)
  redraw!

  call assert_equal(1, foldclosed(line('.')))
  call assert_equal([1, 1], [winline(), wincol()])
  let [win_row, win_col] = win_screenpos(0)

  normal! gj
  redraw!
  call assert_equal([3, 1], [line('.'), col('.')])
  let pos = screenpos(0, line('.'), col('.'))
  call assert_true(pos.row > 0)
  call assert_equal([winline(), wincol()],
        \ [pos.row - win_row + 1, pos.curscol - win_col + 1])
  let first_row_cell = [winline(), wincol()]

  normal! gj
  redraw!
  call assert_equal(3, line('.'))
  call assert_true(col('.') > 1)
  let pos = screenpos(0, line('.'), col('.'))
  call assert_true(pos.row > 0)
  call assert_equal([winline(), wincol()],
        \ [pos.row - win_row + 1, pos.curscol - win_col + 1])
  call assert_true(winline() > first_row_cell[0])

  normal! gk
  redraw!
  call assert_equal([3, 1], [line('.'), col('.')])
  call assert_equal(first_row_cell, [winline(), wincol()])
  let pos = screenpos(0, line('.'), col('.'))
  call assert_true(pos.row > 0)
  call assert_equal(first_row_cell,
        \ [pos.row - win_row + 1, pos.curscol - win_col + 1])

  normal! gk
  redraw!
  call assert_equal(1, foldclosed(line('.')))
  call assert_equal([1, 1], [winline(), wincol()])
  let pos = screenpos(0, line('.'), col('.'))
  call assert_true(pos.row > 0)
  call assert_equal([winline(), wincol()],
        \ [pos.row - win_row + 1, pos.curscol - win_col + 1])

  syntax clear Hidden
  call CloseWindow()
endfunc

func Test_conceallevel_three_search_hidden_delimiters()
  let save_wrapscan = &wrapscan

  call NewWindow(8, 40)
  try
    set wrapscan
    setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
          \ signcolumn=no nonumber
    syntax region Code matchgroup=Tick start=/`/ end=/`/ concealends

    let line1 = repeat('a', 42) .. ' `target` one two three'
    let line2 = repeat('b', 42) .. ' `target` four five six'
    let line3 = repeat('c', 42) .. ' target bare word'
    call setline(1, [line1, line2, line3])

    let tick1 = stridx(line1, '`target`') + 1
    let tick2 = stridx(line2, '`target`') + 1
    let word1 = stridx(line1, 'target') + 1
    let word2 = stridx(line2, 'target') + 1
    let bare = stridx(line3, 'target') + 1
    redraw!
    let [win_row, win_col] = win_screenpos(0)

    call cursor(1, 1)
    call feedkeys("/`target\<CR>", 'tx')
    redraw!
    call assert_equal([1, tick1], [line('.'), col('.')])
    let pos = screenpos(0, line('.'), col('.'))
    call assert_true(pos.row > 0)
    call assert_true(winline() > 1)
    call assert_true(wincol() > 1)

    normal! n
    redraw!
    call assert_equal([2, tick2], [line('.'), col('.')])
    let pos = screenpos(0, line('.'), col('.'))
    call assert_true(pos.row > 0)
    call assert_true(winline() > 1)
    call assert_true(wincol() > 1)

    normal! N
    redraw!
    call assert_equal([1, tick1], [line('.'), col('.')])
    let pos = screenpos(0, line('.'), col('.'))
    call assert_true(pos.row > 0)
    call assert_true(winline() > 1)
    call assert_true(wincol() > 1)

    call cursor(3, bare)
    call feedkeys("?target`\<CR>", 'tx')
    redraw!
    call assert_equal([2, word2], [line('.'), col('.')])
    let pos = screenpos(0, line('.'), col('.'))
    call assert_true(pos.row > 0)
    call assert_equal([winline(), wincol()],
          \ [pos.row - win_row + 1, pos.curscol - win_col + 1])

    call cursor(1, word1)
    normal! *
    redraw!
    call assert_equal([2, word2], [line('.'), col('.')])
    let pos = screenpos(0, line('.'), col('.'))
    call assert_true(pos.row > 0)
    call assert_equal([winline(), wincol()],
          \ [pos.row - win_row + 1, pos.curscol - win_col + 1])

    normal! #
    redraw!
    call assert_equal([1, word1], [line('.'), col('.')])
    let pos = screenpos(0, line('.'), col('.'))
    call assert_true(pos.row > 0)
    call assert_equal([winline(), wincol()],
          \ [pos.row - win_row + 1, pos.curscol - win_col + 1])
  finally
    let &wrapscan = save_wrapscan
    silent! syntax clear Code
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_gn_gN_visual_selection()
  let save_search = @/
  let save_wrapscan = &wrapscan
  let save_reg = getreg('"')
  let save_regtype = getregtype('"')

  call NewWindow(8, 40)
  try
    set wrapscan
    setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
          \ signcolumn=no nonumber
    syntax region Code matchgroup=Tick start=/`/ end=/`/ concealends

    let line1 = repeat('a', 42) .. ' `target` alpha'
    let line2 = repeat('b', 42) .. ' `target` beta'
    call setline(1, [line1, line2])
    let tick1 = stridx(line1, '`target`') + 1
    let tick2 = stridx(line2, '`target`') + 1
    let word1 = stridx(line1, 'target') + 1
    let word2 = stridx(line2, 'target') + 1
    let match_len = strlen('`target`')
    let @/ = '`target`'
    redraw!
    let [win_row, win_col] = win_screenpos(0)

    call cursor(1, 1)
    normal! 0gny
    call assert_equal('`target`', @")
    call assert_equal([0, 1, tick1, 0], getpos("'<"))
    call assert_equal([0, 1, tick1 + match_len - 1, 0], getpos("'>"))
    call cursor(1, word1)
    redraw!
    let pos = screenpos(0, line('.'), col('.'))
    call assert_true(pos.row > 0)
    call assert_true(pos.row - win_row + 1 > 1)
    call assert_equal([winline(), wincol()],
          \ [pos.row - win_row + 1, pos.curscol - win_col + 1])

    call cursor(2, strlen(line2))
    normal! gNy
    call assert_equal('`target`', @")
    call assert_equal([0, 2, tick2, 0], getpos("'<"))
    call assert_equal([0, 2, tick2 + match_len - 1, 0], getpos("'>"))
    call cursor(2, word2)
    redraw!
    let pos = screenpos(0, line('.'), col('.'))
    call assert_true(pos.row > 0)
    call assert_true(pos.row - win_row + 1 > 1)
    call assert_equal([winline(), wincol()],
          \ [pos.row - win_row + 1, pos.curscol - win_col + 1])
  finally
    call setreg('"', save_reg, save_regtype)
    let @/ = save_search
    let &wrapscan = save_wrapscan
    silent! syntax clear Code
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_word_motions_text_objects()
  let save_reg = getreg('"')
  let save_regtype = getregtype('"')

  call NewWindow(8, 40)
  try
    setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
          \ signcolumn=no nonumber
    syntax region Code matchgroup=HiddenMarker start=/`/ end=/`/
          \ concealends
    syntax region Emphasis matchgroup=HiddenMarker start=/\*/ end=/\*/
          \ concealends

    let line1 = repeat('a', 42) .. ' `target` alpha'
    let line2 = repeat('b', 42) .. ' *focus* beta'
    call setline(1, [line1, line2])
    let tick1 = stridx(line1, '`target`') + 1
    let word1 = stridx(line1, 'target') + 1
    let word1_end = word1 + strlen('target') - 1
    let star2 = stridx(line2, '*focus*') + 1
    let word2 = stridx(line2, 'focus') + 1
    let word2_end = word2 + strlen('focus') - 1

    for [lnum, first_col, word_col, word_end] in [
          \ [1, tick1, word1, word1_end],
          \ [2, star2, word2, word2_end],
          \]
      call cursor(lnum, 1)
      normal! w
      redraw!
      call assert_equal([lnum, first_col], [line('.'), col('.')])
      let [win_row, win_col] = win_screenpos(0)
      let pos = screenpos(0, line('.'), col('.'))
      call assert_true(pos.row > 0)
      call assert_true(pos.row - win_row + 1 > 1)
      call assert_true(wincol() > 1)

      normal! w
      redraw!
      call assert_equal([lnum, word_col], [line('.'), col('.')])
      let pos = screenpos(0, line('.'), col('.'))
      call assert_true(pos.row > 0)
      call assert_true(pos.row - win_row + 1 > 1)
      call assert_equal([winline(), wincol()],
            \ [pos.row - win_row + 1, pos.curscol - win_col + 1])

      normal! e
      redraw!
      call assert_equal([lnum, word_end], [line('.'), col('.')])
      let pos = screenpos(0, line('.'), col('.'))
      call assert_true(pos.row > 0)
      call assert_true(pos.row - win_row + 1 > 1)
      call assert_equal([winline(), wincol()],
            \ [pos.row - win_row + 1, pos.curscol - win_col + 1])

      normal! b
      redraw!
      call assert_equal([lnum, word_col], [line('.'), col('.')])
      let pos = screenpos(0, line('.'), col('.'))
      call assert_true(pos.row > 0)
      call assert_true(pos.row - win_row + 1 > 1)
      call assert_equal([winline(), wincol()],
            \ [pos.row - win_row + 1, pos.curscol - win_col + 1])

      normal! ge
      redraw!
      call assert_equal([lnum, first_col], [line('.'), col('.')])
      let pos = screenpos(0, line('.'), col('.'))
      call assert_true(pos.row > 0)
      call assert_true(pos.row - win_row + 1 > 1)
      call assert_true(wincol() > 1)
    endfor

    call cursor(1, word1)
    normal! yiw
    call assert_equal('target', @")
    normal! yaw
    call assert_equal('target', @")
    normal! yiW
    call assert_equal('`target`', @")
    normal! yaW
    call assert_equal('`target` ', @")

    call cursor(2, word2)
    normal! yiw
    call assert_equal('focus', @")
    normal! yaw
    call assert_equal('focus', @")
    normal! yiW
    call assert_equal('*focus*', @")
    normal! yaW
    call assert_equal('*focus* ', @")
  finally
    call setreg('"', save_reg, save_regtype)
    silent! syntax clear Code
    silent! syntax clear Emphasis
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_matchadd_overlap_delete()
  call NewWindow(8, 40)
  try
    setlocal wrap linebreak breakindent conceallevel=3 concealcursor=n
          \ signcolumn=no nonumber

    let line = repeat('a', 42) .. ' HIDDEN target after'
    call setline(1, line)
    let hidden_col = stridx(line, ' HIDDEN ') + 1
    let target_col = stridx(line, 'target') + 1
    let hidden_pat = '\%1l\%' .. hidden_col .. 'c HIDDEN '
    let high_pat = '\%1l\%' .. (hidden_col + 1) .. 'cHIDDEN'

    let hidden_id = matchadd('Conceal', hidden_pat, 10, -1,
          \ #{conceal: ''})
    call cursor(1, target_col)
    redraw!
    let [win_row, win_col] = win_screenpos(0)
    let pos = screenpos(0, line('.'), col('.'))
    call assert_true(pos.row > 0)
    let hidden_cell = [pos.row - win_row + 1, pos.curscol - win_col + 1]
    call assert_equal(hidden_cell, [winline(), wincol()])
    call assert_true(hidden_cell[0] > 1)

    let high_id = matchadd('Search', high_pat, 20)
    redraw!
    let pos = screenpos(0, line('.'), col('.'))
    call assert_true(pos.row > 0)
    let overlap_cell = [pos.row - win_row + 1, pos.curscol - win_col + 1]
    call assert_equal(overlap_cell, [winline(), wincol()])
    call assert_true(overlap_cell[1] > hidden_cell[1])

    call matchdelete(high_id)
    redraw!
    let pos = screenpos(0, line('.'), col('.'))
    call assert_true(pos.row > 0)
    call assert_equal(hidden_cell,
          \ [pos.row - win_row + 1, pos.curscol - win_col + 1])
    call assert_equal(hidden_cell, [winline(), wincol()])

    call matchdelete(hidden_id)
    redraw!
    let pos = screenpos(0, line('.'), col('.'))
    call assert_true(pos.row > 0)
    let visible_cell = [pos.row - win_row + 1, pos.curscol - win_col + 1]
    call assert_equal(visible_cell, [winline(), wincol()])
    call assert_true(visible_cell[1] > overlap_cell[1])
  finally
    call clearmatches()
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_same_line_hidden_width_edits()
  let save_reg = getreg('"')
  let save_regtype = getregtype('"')

  call NewWindow(8, 40)
  try
    setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
          \ signcolumn=no nonumber
    syntax match Hidden /X\+/ conceal

    let line = repeat('a', 42) .. ' XXXX target after'
    call setline(1, line)
    let hidden_col = stridx(getline(1), 'XXXX') + 1
    let target_col = stridx(getline(1), 'target') + 1
    let target_cell = s:ConceallevelThreeCursorCell(target_col)
    call assert_true(target_cell[0] > 1)

    call cursor(1, hidden_col)
    execute "normal! iXX\<Esc>"
    call assert_match('XXXXXX target', getline(1))
    let target_col = stridx(getline(1), 'target') + 1
    call assert_equal(target_cell, s:ConceallevelThreeCursorCell(target_col))

    call cursor(1, hidden_col)
    normal! 2x
    call assert_match('XXXX target', getline(1))
    let target_col = stridx(getline(1), 'target') + 1
    call assert_equal(target_cell, s:ConceallevelThreeCursorCell(target_col))

    call cursor(1, hidden_col)
    execute "normal! cwXXXXXXXX\<Esc>"
    call assert_match('XXXXXXXX target', getline(1))
    let target_col = stridx(getline(1), 'target') + 1
    call assert_equal(target_cell, s:ConceallevelThreeCursorCell(target_col))

    s/XXXXXXXX/XXX/
    call assert_match('XXX target', getline(1))
    let target_col = stridx(getline(1), 'target') + 1
    call assert_equal(target_cell, s:ConceallevelThreeCursorCell(target_col))
    let &l:undolevels = &l:undolevels

    call cursor(1, hidden_col + 2)
    call setreg('"', 'XXXXX', 'v')
    normal! p
    call assert_match('XXXXXXXX target', getline(1))
    let pasted_line = getline(1)
    let target_col = stridx(getline(1), 'target') + 1
    call assert_equal(target_cell, s:ConceallevelThreeCursorCell(target_col))

    undo
    call assert_match('XXX target', getline(1))
    let target_col = stridx(getline(1), 'target') + 1
    call assert_equal(target_cell, s:ConceallevelThreeCursorCell(target_col))

    redo
    call assert_equal(pasted_line, getline(1))
    let target_col = stridx(getline(1), 'target') + 1
    call assert_equal(target_cell, s:ConceallevelThreeCursorCell(target_col))
  finally
    call setreg('"', save_reg, save_regtype)
    silent! syntax clear Hidden
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_edit_commands_hidden_width()
  let save_reg = getreg('"')
  let save_regtype = getregtype('"')

  call NewWindow(8, 40)
  try
    setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
          \ signcolumn=no nonumber noautoindent textwidth=80
          \ formatoptions=tcq formatexpr= formatprg=
    syntax match Hidden /[XY]\+/ conceal

    let prefix = repeat('a', 42)
    let base = prefix .. ' XXXX target after'
    call setline(1, base)
    let target_col = stridx(getline(1), 'target') + 1
    let target_cell = s:ConceallevelThreeCursorCell(target_col)
    call assert_true(target_cell[0] > 1)

    call setline(1, base)
    call cursor(1, stridx(getline(1), 'XXXX') + 1)
    normal! x
    call assert_equal(prefix .. ' XXX target after', getline(1))
    let target_col = stridx(getline(1), 'target') + 1
    call assert_equal(target_cell, s:ConceallevelThreeCursorCell(target_col))

    call setline(1, base)
    call cursor(1, stridx(getline(1), 'XXXX') + 3)
    normal! X
    call assert_equal(prefix .. ' XXX target after', getline(1))
    let target_col = stridx(getline(1), 'target') + 1
    call assert_equal(target_cell, s:ConceallevelThreeCursorCell(target_col))

    call setline(1, base)
    call cursor(1, stridx(getline(1), 'XXXX') + 1)
    execute "normal! sYYYY\<Esc>"
    call assert_equal(prefix .. ' YYYYXXX target after', getline(1))
    let target_col = stridx(getline(1), 'target') + 1
    call assert_equal(target_cell, s:ConceallevelThreeCursorCell(target_col))

    call setline(1, base)
    call cursor(1, stridx(getline(1), 'XXXX') + 1)
    normal! rY
    call assert_equal(prefix .. ' YXXX target after', getline(1))
    let target_col = stridx(getline(1), 'target') + 1
    call assert_equal(target_cell, s:ConceallevelThreeCursorCell(target_col))

    call setline(1, base)
    call cursor(1, stridx(getline(1), 'XXXX') + 1)
    execute "normal! cwYYYYYY\<Esc>"
    call assert_equal(prefix .. ' YYYYYY target after', getline(1))
    let target_col = stridx(getline(1), 'target') + 1
    call assert_equal(target_cell, s:ConceallevelThreeCursorCell(target_col))

    call setline(1, base)
    call cursor(1, 1)
    execute "normal! cc" .. prefix .. " YYYY target after\<Esc>"
    call assert_equal(prefix .. ' YYYY target after', getline(1))
    let target_col = stridx(getline(1), 'target') + 1
    call assert_equal(target_cell, s:ConceallevelThreeCursorCell(target_col))

    call setline(1, [prefix .. ' YYYY', 'target after'])
    call cursor(1, 1)
    normal! J
    call assert_equal(prefix .. ' YYYY target after', getline(1))
    call assert_equal(1, line('$'))
    let target_col = stridx(getline(1), 'target') + 1
    call assert_equal(target_cell, s:ConceallevelThreeCursorCell(target_col))

    call setline(1, [prefix .. ' YYYY', 'target after'])
    call cursor(1, 1)
    normal! gqj
    call assert_equal(prefix .. ' YYYY target after', getline(1))
    call assert_equal(1, line('$'))
    let target_col = stridx(getline(1), 'target') + 1
    call assert_equal(target_cell, s:ConceallevelThreeCursorCell(target_col))
  finally
    call setreg('"', save_reg, save_regtype)
    silent! syntax clear Hidden
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_dot_repeat_hidden_delimiters()
  let save_reg = getreg('"')
  let save_regtype = getregtype('"')

  call NewWindow(8, 40)
  try
    setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
          \ signcolumn=no nonumber
    syntax region Code matchgroup=HiddenMarker start=/`/ end=/`/
          \ concealends

    let prefix = repeat('a', 42)
    let line = prefix .. ' target alpha target beta'
    call setline(1, line)
    let first_target_col = stridx(getline(1), 'target alpha') + 1
    let alpha_col = stridx(getline(1), 'alpha') + 1
    let second_target_col = stridx(getline(1), 'target beta') + 1
    let beta_col = stridx(getline(1), 'beta') + 1

    let first_target_cell = s:ConceallevelThreeCursorCell(first_target_col)
    call assert_true(first_target_cell[0] > 1)
    let alpha_cell = s:ConceallevelThreeCursorCell(alpha_col)
    let second_target_cell = s:ConceallevelThreeCursorCell(second_target_col)
    let beta_cell = s:ConceallevelThreeCursorCell(beta_col)

    call cursor(1, first_target_col)
    execute "normal! ciw`target`\<Esc>"
    call assert_equal(prefix .. ' `target` alpha target beta', getline(1))
    let first_target_col = stridx(getline(1), 'target` alpha') + 1
    let alpha_col = stridx(getline(1), 'alpha') + 1
    let second_target_col = stridx(getline(1), 'target beta') + 1
    let beta_col = stridx(getline(1), 'beta') + 1
    call assert_equal(first_target_cell,
          \ s:ConceallevelThreeCursorCell(first_target_col))
    call assert_equal(alpha_cell, s:ConceallevelThreeCursorCell(alpha_col))
    call assert_equal(second_target_cell,
          \ s:ConceallevelThreeCursorCell(second_target_col))
    call assert_equal(beta_cell, s:ConceallevelThreeCursorCell(beta_col))

    call cursor(1, second_target_col)
    normal! .
    call assert_equal(prefix .. ' `target` alpha `target` beta',
          \ getline(1))
    let first_target_col = stridx(getline(1), 'target` alpha') + 1
    let alpha_col = stridx(getline(1), 'alpha') + 1
    let second_target_col = stridx(getline(1), 'target` beta') + 1
    let beta_col = stridx(getline(1), 'beta') + 1
    call assert_equal(first_target_cell,
          \ s:ConceallevelThreeCursorCell(first_target_col))
    call assert_equal(alpha_cell, s:ConceallevelThreeCursorCell(alpha_col))
    call assert_equal(second_target_cell,
          \ s:ConceallevelThreeCursorCell(second_target_col))
    call assert_equal(beta_cell, s:ConceallevelThreeCursorCell(beta_col))
  finally
    call setreg('"', save_reg, save_regtype)
    silent! syntax clear Code
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_completeopt_popup_placement()
  CheckRunVimInTerminal
  CheckNotGui
  CheckFeature textprop

  let code =<< trim [CODE]
    set completefunc=CompleteForConceal
    set wrap linebreak breakindent conceallevel=3 concealcursor=nvic
          \ signcolumn=no nonumber
    syntax match Hidden /X\+/ conceal

    func CompleteForConceal(findstart, base) abort
      if a:findstart
        return col('.') - 2
      endif
      return [
            \ #{word: 'foobar', menu: 'one', info: 'info foobar'},
            \ #{word: 'foobaz', menu: 'two', info: 'info foobaz'},
            \ #{word: 'foozap', menu: 'three', info: 'info foozap'},
            \ #{word: 'fooqux', menu: 'four', info: 'info fooqux'},
            \]
    endfunc

    func SetupCompleteCase(opt) abort
      silent! pclose
      let &completeopt = a:opt
      %delete _
      call setline(1, repeat('a', 40) .. ' XX fo')
      call cursor(1, strlen(getline(1)) + 1)
      redraw
    endfunc

    func WriteCompleteState() abort
      let info = complete_info(['pum_visible', 'selected'])
      call writefile([string(info.pum_visible), string(info.selected)],
            \ 'XTest_conceallevel_three_completeopt_popup_placement_state')
    endfunc

    inoremap <F6> <Cmd>call WriteCompleteState()<CR>
  [CODE]
  call writefile(code, 'XTest_conceallevel_three_completeopt_popup_placement',
        \ 'D')

  let cases = [
        \ #{opt: 'menu,menuone,noinsert,noselect',
        \   key: "\<C-N>", selected: 0, item: 'foobaz'},
        \ #{opt: 'menu,menuone,popup,noinsert,noselect',
        \   key: "\<C-N>", selected: 0, item: 'foobaz'},
        \ #{opt: 'menu,preview',
        \   key: "\<C-N>", selected: 1, item: 'foozap'},
        \ #{opt: 'menu,menuone,popup',
        \   key: "\<C-N>", selected: 1, item: 'foozap'},
        \]

  let buf = 0
  try
    let buf = RunVimInTerminal(
          \ '-S XTest_conceallevel_three_completeopt_popup_placement',
          \ #{rows: 12, cols: 34})
    call TermWait(buf, 100)

    for case in cases
      call term_sendkeys(buf, "\<Esc>:call SetupCompleteCase('"
            \ .. case.opt .. "')\<CR>")
      call TermWait(buf, 100)
      call term_sendkeys(buf, "A\<C-X>\<C-U>")
      call TermWait(buf, 100)

      let before = s:TermTextAttrs(buf, case.item, 1)
      call assert_true(before.row > 0, case.opt)

      call term_sendkeys(buf, case.key)
      call TermWait(buf, 100)
      let after = s:TermTextAttrs(buf, case.item, 1)
      call assert_equal(before.row, after.row, case.opt)
      call assert_equal(before.col, after.col, case.opt)

      call delete(
            \ 'XTest_conceallevel_three_completeopt_popup_placement_state')
      call term_sendkeys(buf, "\<F6>")
      call WaitForAssert({-> assert_true(s:FileEquals(
            \ 'XTest_conceallevel_three_completeopt_popup_placement_state',
            \ ['1', string(case.selected)]), case.opt)})
    endfor
  finally
    if buf > 0
      call term_sendkeys(buf, "\<Esc>")
      call StopVimInTerminal(buf)
    endif
    call delete('XTest_conceallevel_three_completeopt_popup_placement_state')
  endtry
endfunc

func Test_conceallevel_three_complete_anchor_width_cases()
  CheckRunVimInTerminal
  CheckNotGui

  let code =<< trim [CODE]
    set completeopt=menu,menuone,noinsert,noselect
    set completefunc=CompleteForConceal
    set wrap linebreak breakindent conceallevel=3 concealcursor=nvic
          \ signcolumn=no nonumber tabstop=4
    syntax match Hidden /\*\*/ conceal

    func CompleteForConceal(findstart, base) abort
      if a:findstart
        return stridx(getline('.'), 'fo') + 1
      endif
      return ['foobar', 'foobaz', 'foozap', 'fooqux']
    endfunc

    let s:complete_anchor_cases = #{
          \ delimiters: repeat('a', 40) .. '**fo',
          \ tab: repeat('a', 40) .. "\tfo",
          \ doublewidth: repeat('a', 36) .. '日本語fo',
          \}

    func SetupCompleteAnchorCase(name) abort
      %delete _
      call setline(1, s:complete_anchor_cases[a:name])
      call cursor(1, strlen(getline(1)) + 1)
      redraw
      let anchor_col = stridx(getline(1), 'fo') + 1
      let pos = screenpos(0, 1, anchor_col)
      call writefile([string(pos.row), string(pos.col), 'done'],
            \ 'XTest_conceallevel_three_complete_anchor_width_cases_state')
    endfunc
  [CODE]
  call writefile(code, 'XTest_conceallevel_three_complete_anchor_width_cases',
        \ 'D')

  let buf = 0
  try
    let buf = RunVimInTerminal(
          \ '-S XTest_conceallevel_three_complete_anchor_width_cases',
          \ #{rows: 10, cols: 34})
    call TermWait(buf, 100)

    for name in ['delimiters', 'tab', 'doublewidth']
      call delete(
            \ 'XTest_conceallevel_three_complete_anchor_width_cases_state')
      call term_sendkeys(buf, "\<Esc>:call SetupCompleteAnchorCase('"
            \ .. name .. "')\<CR>")
      call WaitForAssert({-> assert_true(s:FileEndsWith(
            \ 'XTest_conceallevel_three_complete_anchor_width_cases_state',
            \ 'done'))})
      let anchor = map(readfile(
            \ 'XTest_conceallevel_three_complete_anchor_width_cases_state')[:1],
            \ 'str2nr(v:val)')
      call assert_true(anchor[0] > 0, name)

      call term_sendkeys(buf, "A\<C-X>\<C-U>")
      call TermWait(buf, 100)
      let popup = s:TermTextAttrs(buf, 'foobar', 1)
      call assert_true(popup.row > anchor[0], name)
      call assert_equal(anchor[1] + 1, popup.col, name)
    endfor
  finally
    if buf > 0
      call term_sendkeys(buf, "\<Esc>")
      call StopVimInTerminal(buf)
    endif
    call delete(
          \ 'XTest_conceallevel_three_complete_anchor_width_cases_state')
  endtry
endfunc

func Test_conceallevel_three_complete_anchor_hidden_edit()
  CheckRunVimInTerminal
  CheckNotGui

  let code =<< trim [CODE]
    set completeopt=menu,menuone,noinsert,noselect
    set completefunc=CompleteForConceal
    set wrap linebreak breakindent conceallevel=3 concealcursor=nvic
          \ signcolumn=no nonumber
    syntax match Hidden /X\+/ conceal
    call setline(1, repeat('a', 18) .. ' XX fo')

    func CompleteForConceal(findstart, base) abort
      if a:findstart
        return col('.') - 2
      endif
      return ['foobar', 'foobaz', 'foozap', 'fooqux', 'foozip']
    endfunc

    func AddHiddenBeforeComplete() abort
      let cur = getcurpos()
      call setline('.', substitute(getline('.'), 'XX fo', 'XXXX fo', ''))
      call cursor(cur[1], cur[2] + 2)
      redraw
      call writefile([getline('.')],
            \ 'XTest_conceallevel_three_complete_anchor_hidden_edit_state')
    endfunc

    inoremap <F5> <Cmd>call AddHiddenBeforeComplete()<CR>
  [CODE]
  call writefile(code, 'XTest_conceallevel_three_complete_anchor_hidden_edit',
        \ 'D')

  let buf = 0
  try
    let buf = RunVimInTerminal(
          \ '-S XTest_conceallevel_three_complete_anchor_hidden_edit',
          \ #{rows: 8, cols: 30})
    call TermWait(buf, 100)

    call term_sendkeys(buf, "A\<C-X>\<C-U>")
    call TermWait(buf, 100)
    let before = s:TermTextAttrs(buf, 'foobar', 1)
    call assert_true(before.row > 0)

    call delete('XTest_conceallevel_three_complete_anchor_hidden_edit_state')
    call term_sendkeys(buf, "\<F5>")
    let expected = [repeat('a', 18) .. ' XXXX fo']
    call WaitForAssert({-> assert_true(s:FileEquals(
          \ 'XTest_conceallevel_three_complete_anchor_hidden_edit_state',
          \ expected))})
    let after = s:TermTextAttrs(buf, 'foobar', 1)
    call assert_equal(before.row, after.row)
    call assert_equal(before.col, after.col)
  finally
    if buf > 0
      call term_sendkeys(buf, "\<Esc>")
      call StopVimInTerminal(buf)
    endif
    call delete('XTest_conceallevel_three_complete_anchor_hidden_edit_state')
  endtry
endfunc

func s:TermTextAttrs(buf, text, occurrence) abort
  let seen = 0
  let rows = term_getsize(a:buf)[0] - 1
  for row in range(1, rows)
    let cells = term_scrape(a:buf, row)
    let line = join(map(copy(cells), 'v:val.chars'), '')
    let start = 0
    while 1
      let idx = stridx(line, a:text, start)
      if idx < 0
        break
      endif
      let seen += 1
      if seen == a:occurrence
        let attrs = map(copy(cells[idx : idx + strlen(a:text) - 1]),
              \ 'v:val.attr')
        return #{row: row, col: idx + 1, attrs: attrs}
      endif
      let start = idx + strlen(a:text)
    endwhile
  endfor

  call assert_report('did not find "' .. a:text .. '" occurrence '
        \ .. a:occurrence)
  return #{row: 0, col: 0, attrs: []}
endfunc

func Test_conceallevel_three_incsearch_hlsearch()
  CheckRunVimInTerminal
  CheckNotGui

  let code =<< trim [CODE]
    set incsearch hlsearch scrolloff=0
    set wrap linebreak breakindent conceallevel=3 concealcursor=nvic
          \ signcolumn=no nonumber
    highlight Search ctermfg=0 ctermbg=11
    highlight IncSearch ctermfg=15 ctermbg=9
    syntax region Code matchgroup=Tick start=/`/ end=/`/ concealends
    let line1 = repeat('a', 42) .. ' `target` alpha'
    let line2 = repeat('b', 42) .. ' `target` beta'
    call setline(1, [line1, line2])
    call cursor(1, 1)
  [CODE]
  call writefile(code, 'XTest_conceallevel_three_incsearch_hlsearch', 'D')

  let buf = 0
  try
    let buf = RunVimInTerminal(
          \ '-S XTest_conceallevel_three_incsearch_hlsearch',
          \ #{rows: 8, cols: 40})
    call TermWait(buf, 100)

    call term_sendkeys(buf, '/`target`')
    call TermWait(buf, 100)
    let normal_attr = term_scrape(buf, 1)[0].attr
    let inc_match = s:TermTextAttrs(buf, 'target', 1)
    let search_match = s:TermTextAttrs(buf, 'target', 2)
    call assert_equal(repeat([inc_match.attrs[0]], 6), inc_match.attrs)
    call assert_equal(repeat([search_match.attrs[0]], 6),
          \ search_match.attrs)
    call assert_notequal(normal_attr, inc_match.attrs[0])
    call assert_notequal(normal_attr, search_match.attrs[0])
    call assert_notequal(inc_match.attrs[0], search_match.attrs[0])

    call term_sendkeys(buf, "\<CR>")
    call TermWait(buf, 100)
    let first_hl = s:TermTextAttrs(buf, 'target', 1)
    let second_hl = s:TermTextAttrs(buf, 'target', 2)
    call assert_equal(repeat([first_hl.attrs[0]], 6), first_hl.attrs)
    call assert_equal(repeat([second_hl.attrs[0]], 6), second_hl.attrs)
    call assert_notequal(normal_attr, first_hl.attrs[0])
    call assert_equal(first_hl.attrs[0], second_hl.attrs[0])
  finally
    if buf > 0
      call StopVimInTerminal(buf)
    endif
  endtry
endfunc

func Test_conceallevel_three_split_window_options()
  call NewWindow(10, 40)
  setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
  syntax match Hidden /HIDDEN / conceal

  let line = repeat('a', 42)
        \ .. ' HIDDEN target words after hidden text to force wrapping'
        \ .. ' and mapping checks'
  call setline(1, line)
  let target_col = stridx(line, 'target') + 1
  let hidden_win = win_getid()

  let hidden_cell = s:ConceallevelThreeCursorCell(target_col)
  call assert_equal([2, 4], hidden_cell)

  let other_wins = filter(win_findbuf(bufnr()), 'v:val != hidden_win')
  call assert_true(!empty(other_wins))
  let visible_win = other_wins[0]
  call assert_true(win_gotoid(visible_win))
  call assert_equal(winbufnr(hidden_win), bufnr())

  setlocal wrap conceallevel=3 concealcursor= signcolumn=no nonumber
  let visible_cell = s:ConceallevelThreeCursorCell(target_col)
  call assert_notequal(hidden_cell, visible_cell)

  call assert_true(win_gotoid(hidden_win))
  call assert_equal(hidden_cell, s:ConceallevelThreeCursorCell(target_col))

  call assert_true(win_gotoid(visible_win))
  setlocal conceallevel=0 concealcursor=nvic
  let cole_zero_cell = s:ConceallevelThreeCursorCell(target_col)
  call assert_notequal(hidden_cell, cole_zero_cell)

  call assert_true(win_gotoid(hidden_win))
  call assert_equal(hidden_cell, s:ConceallevelThreeCursorCell(target_col))

  syntax clear Hidden
  call CloseWindow()
endfunc

func Test_conceallevel_three_split_window_wrap_options()
  call NewWindow(10, 40)
  setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
  syntax match Hidden /HIDDEN / conceal

  let line = repeat('a', 42)
        \ .. ' HIDDEN target words after hidden text to force wrapping'
        \ .. ' and mapping checks'
  call setline(1, line)
  let target_col = stridx(line, 'target') + 1
  let wrapped_win = win_getid()

  let wrapped_cell = s:ConceallevelThreeCursorCell(target_col)
  call assert_equal([2, 4], wrapped_cell)

  let other_wins = filter(win_findbuf(bufnr()), 'v:val != wrapped_win')
  call assert_true(!empty(other_wins))
  let nowrap_win = other_wins[0]
  call assert_true(win_gotoid(nowrap_win))
  call assert_equal(winbufnr(wrapped_win), bufnr())

  setlocal nowrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
  let nowrap_cell = s:ConceallevelThreeCursorCell(target_col)
  call assert_equal(1, nowrap_cell[0])
  call assert_notequal(wrapped_cell, nowrap_cell)

  call assert_true(win_gotoid(wrapped_win))
  call assert_equal(wrapped_cell, s:ConceallevelThreeCursorCell(target_col))

  call assert_true(win_gotoid(nowrap_win))
  call assert_equal(nowrap_cell, s:ConceallevelThreeCursorCell(target_col))

  syntax clear Hidden
  call CloseWindow()
endfunc

func Test_conceallevel_three_visual_drag_after_double_width()
  call NewWindow(12, 42)
  set mouse=a
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
        \ number signcolumn=no
  syntax region test matchgroup=test start=/`/ end=/`/ concealends

  let line = 'aaaaaaaaaa bbbbbbbbbb cccccccccc dddddddddd `日本語` eeeeeeeeee ffffffffff gggggggggg hhhhhhhhhh iiiiiiiiii'
  call setline(1, line)
  redraw

  let start_col = stridx(line, 'cccccccccc') + 1
  let start = screenpos(0, 1, start_col)
  let target_col = stridx(line, 'gggggggggg') + 4
  let target = screenpos(0, 1, target_col)
  call assert_true(start.row > 0)
  call assert_true(target.row > 0)

  call test_setmouse(start.row, start.curscol)
  call feedkeys("\<LeftMouse>", "tx")
  call test_setmouse(target.row, target.curscol)
  call feedkeys("\<LeftDrag>", "tx")
  redraw

  call assert_equal(target_col, col('.'))
  let selected_attr = screenattr(target.row, target.curscol - 2)
  call assert_notequal(0, selected_attr)
  call assert_equal(selected_attr, screenattr(target.row, target.curscol - 1))
  call assert_notequal(selected_attr, screenattr(target.row, target.curscol + 1))

  syntax clear test
  call CloseWindow()
  set mouse&
endfunc

func Test_conceallevel_three_visual_drag_into_concealed_marker()
  call NewWindow(10, 37)
  set mouse=a
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
        \ signcolumn=no number showbreak=
  syntax clear test
  syntax match test /\[/ conceal
  syntax match test /\](https:[^)]*)/ conceal
  syntax match test /\*\*/ conceal
  syntax region testCode matchgroup=test start=/`/ end=/`/ concealends

  let line = '- A dash list item should use Markdown list formatting, and this item intentionally contains **strong text**, `inline code`, [a link with 日本語](https://example.invalid/list), and enough trailing prose to wrap with breakindent.'
  call setline(1, line)
  redraw

  let start_col = stridx(line, 'intentionally') + 1
  let start = screenpos(0, 1, start_col)
  let target_col = stridx(line, 'strong text') + 4
  let target = screenpos(0, 1, target_col)
  call assert_true(start.row > 0)
  call assert_true(target.row > 0)

  call test_setmouse(start.row, start.curscol)
  call feedkeys("\<LeftMouse>", "tx")
  call test_setmouse(target.row, target.curscol)
  call feedkeys("\<LeftDrag>", "tx")
  redraw

  call assert_equal(target_col, col('.'))
  let selected_attr = screenattr(target.row, target.curscol - 1)
  call assert_notequal(0, selected_attr)
  call assert_equal(selected_attr, screenattr(target.row, target.curscol - 2))
  call assert_notequal(selected_attr, screenattr(target.row, target.curscol + 1))

  syntax clear test
  syntax clear testCode
  call CloseWindow()
  set mouse&
endfunc

" Test that cursor is drawn at the correct column when it is after end of the
" line with 'virtualedit' and concealing.
func Run_test_conceal_virtualedit_after_eol(wrap)
  CheckScreendump

  let code =<< trim eval [CODE]
    let &wrap = {a:wrap}
    call setline(1, 'abcdefgh|hidden|ijklmnpop')
    syntax match test /|hidden|/ conceal
    set conceallevel=2 concealcursor=n virtualedit=all
    normal! $
  [CODE]
  call writefile(code, 'XTest_conceal_ve_after_eol', 'D')
  let buf = RunVimInTerminal('-S XTest_conceal_ve_after_eol', {'rows': 3})
  call VerifyScreenDump(buf, 'Test_conceal_ve_after_eol_1', {})
  call term_sendkeys(buf, "l")
  call VerifyScreenDump(buf, 'Test_conceal_ve_after_eol_2', {})
  call term_sendkeys(buf, "l")
  call VerifyScreenDump(buf, 'Test_conceal_ve_after_eol_3', {})
  call term_sendkeys(buf, "l")
  call VerifyScreenDump(buf, 'Test_conceal_ve_after_eol_4', {})
  call term_sendkeys(buf, "rr")
  call VerifyScreenDump(buf, 'Test_conceal_ve_after_eol_5', {})

  " clean up
  call StopVimInTerminal(buf)
endfunc

func Test_conceal_virtualedit_after_eol()
  CheckScreendump

  call Run_test_conceal_virtualedit_after_eol(1)
  call Run_test_conceal_virtualedit_after_eol(0)
endfunc

" Same as Run_test_conceal_virtualedit_after_eol(), but with 'rightleft'.
func Run_test_conceal_virtualedit_after_eol_rightleft(wrap)
  CheckScreendump

  let code =<< trim eval [CODE]
    let &wrap = {a:wrap}
    call setline(1, 'abcdefgh|hidden|ijklmnpop')
    syntax match test /|hidden|/ conceal
    set conceallevel=2 concealcursor=n virtualedit=all rightleft
    normal! $
  [CODE]
  call writefile(code, 'XTest_conceal_ve_after_eol_rl', 'D')
  let buf = RunVimInTerminal('-S XTest_conceal_ve_after_eol_rl', {'rows': 3})
  call VerifyScreenDump(buf, 'Test_conceal_ve_after_eol_rl_1', {})
  call term_sendkeys(buf, "h")
  call VerifyScreenDump(buf, 'Test_conceal_ve_after_eol_rl_2', {})
  call term_sendkeys(buf, "h")
  call VerifyScreenDump(buf, 'Test_conceal_ve_after_eol_rl_3', {})
  call term_sendkeys(buf, "h")
  call VerifyScreenDump(buf, 'Test_conceal_ve_after_eol_rl_4', {})
  call term_sendkeys(buf, "rr")
  call VerifyScreenDump(buf, 'Test_conceal_ve_after_eol_rl_5', {})

  " clean up
  call StopVimInTerminal(buf)
endfunc

func Test_conceal_virtualedit_after_eol_rightleft()
  CheckFeature rightleft
  CheckScreendump

  call Run_test_conceal_virtualedit_after_eol_rightleft(1)
  call Run_test_conceal_virtualedit_after_eol_rightleft(0)
endfunc

" Test that cursor position is correct when double-width chars are concealed.
func Run_test_conceal_double_width(wrap)
  CheckScreendump

  let code =<< trim eval [CODE]
    let &wrap = {a:wrap}
    call setline(1, ['aaaaa口=口bbbbb口=口ccccc', 'foobar'])
    syntax match test /口=口/ conceal cchar=β
    set conceallevel=2 concealcursor=n colorcolumn=30
    normal! $
  [CODE]
  call writefile(code, 'XTest_conceal_double_width', 'D')
  let buf = RunVimInTerminal('-S XTest_conceal_double_width', {'rows': 4})
  call VerifyScreenDump(buf, 'Test_conceal_double_width_1', {})
  call term_sendkeys(buf, "gM")
  call VerifyScreenDump(buf, 'Test_conceal_double_width_2', {})
  call term_sendkeys(buf, ":set conceallevel=3\<CR>")
  call VerifyScreenDump(buf, 'Test_conceal_double_width_3', {})
  call term_sendkeys(buf, "$")
  call VerifyScreenDump(buf, 'Test_conceal_double_width_4', {})

  " clean up
  call StopVimInTerminal(buf)
endfunc

func Test_conceal_double_width()
  CheckScreendump

  call Run_test_conceal_double_width(1)
  call Run_test_conceal_double_width(0)
endfunc

func Test_conceallevel_three_wrap()
  call NewWindow(6, 80)
  setlocal wrap conceallevel=3 concealcursor=n signcolumn=no nonumber
  syntax match test /X\+/ conceal

  call setline(1, ['', repeat('X', winwidth(0) - 3) .. 'YYYY', 'after'])
  call cursor(1, 1)
  call assert_equal([
        \ repeat(' ', winwidth(0)),
        \ 'YYYY' .. repeat(' ', winwidth(0) - 4),
        \ 'after' .. repeat(' ', winwidth(0) - 5),
        \ ], ScreenLines([1, 3], winwidth(0)))

  call setline(1, repeat('X', winwidth(0) - 3) .. 'YYYY')
  call setline(2, 'after')
  call cursor(1, 1)
  call assert_equal([
        \ 'YYYY' .. repeat(' ', winwidth(0) - 4),
        \ 'after' .. repeat(' ', winwidth(0) - 5),
        \ ], ScreenLines([1, 2], winwidth(0)))

  call setline(1, repeat('X', winwidth(0) - 3) .. "\tY")
  call setline(2, 'after')
  call cursor(2, 1)
  call assert_equal(3, screenpos(0, 2, 1).row)

  let matchid = matchadd('Search', 'Y\nafter')
  redraw
  call assert_equal(3, screenpos(0, 2, 1).row)
  call matchdelete(matchid)

  call setline(1, repeat('X', winwidth(0) - 4) .. 'YYYY' .. "\tZ")
  call cursor(1, winwidth(0) + 1)
  call assert_equal(2, screenpos(0, 1, col('.')).row)

  call setline(1, 'hello, world ' .. repeat('X', winwidth(0) + 5)
        \ .. 'YYYY hello, world')
  call deletebufline(bufnr(), 2, '$')
  call cursor(1, winwidth(0) + 1)
  redraw
  call assert_equal(1, screenpos(0, 1, col('.')).row)
  call feedkeys("o\<Esc>", 'tx')
  redraw
  call assert_equal(2, screenpos(0, 2, 1).row)
  call assert_equal([
        \ 'hello, world YYYY hello, world'
        \ .. repeat(' ', winwidth(0) - 30),
        \ repeat(' ', winwidth(0)),
        \ '~' .. repeat(' ', winwidth(0) - 1),
        \ ], ScreenLines([1, 3], winwidth(0)))
  call deletebufline(bufnr(), 2, '$')

  setlocal linebreak showbreak=++
  call setline(1, repeat('X', winwidth(0)) .. repeat('Y', winwidth(0) - 1))
  call setline(2, 'after')
  call cursor(2, 1)
  call assert_equal(2, screenpos(0, 2, 1).row)
  setlocal nolinebreak showbreak=

  call CloseWindow()
  call NewWindow(6, 4)
  setlocal wrap conceallevel=3 concealcursor=n signcolumn=no nonumber showbreak=++
  syntax match test /X\+/ conceal
  call setline(1, repeat('X', winwidth(0) * 2 + 3)
        \ .. repeat('Y', winwidth(0) + 1))
  call setline(2, 'after')
  call cursor(2, 1)
  call assert_equal(3, screenpos(0, 2, 1).row)

  call setline(1, "X\tY")
  call setline(2, 'after')
  call cursor(2, 1)
  call assert_equal(2, screenpos(0, 2, 1).row)

  syntax clear test
  syntax match test /\t/ conceal
  call setline(1, "abc\tY")
  call setline(2, 'after')
  call cursor(1, 4)
  redraw
  call assert_equal(2, screenpos(0, 2, 1).row)

  setlocal showbreak=
  call CloseWindow()
  call NewWindow(6, 80)
  setlocal wrap conceallevel=3 concealcursor=n signcolumn=no nonumber
  syntax match test /X\+/ conceal
  call setline(2, 'after')

  call setline(1, repeat('X', winwidth(0) - 3) .. 'YYYY')
  call cursor(1, 1)
  call feedkeys("i" .. repeat("\<ScrollWheelRight>", 5) .. "\<Esc>", 'tx')
  redraw
  call assert_equal([
        \ 'YYYY' .. repeat(' ', winwidth(0) - 4),
        \ 'after' .. repeat(' ', winwidth(0) - 5),
        \ ], ScreenLines([1, 2], winwidth(0)))

  call feedkeys("\<Esc>", 'tx')
  redraw
  call assert_equal([
        \ 'YYYY' .. repeat(' ', winwidth(0) - 4),
        \ 'after' .. repeat(' ', winwidth(0) - 5),
        \ ], ScreenLines([1, 2], winwidth(0)))

  if has('folding')
    setlocal foldmethod=manual foldenable foldlevel=0
    call setline(1, repeat('A', winwidth(0) * 2) .. 'X folded text')
    call setline(2, 'inside')
    call setline(3, 'after')
    1,2fold
    call cursor(1, 1)
    normal! zM
    call cursor(1, winwidth(0) * 2 + 1)
    redraw!
    call assert_equal([1, 1], [winline(), wincol()])
    normal! zE
    setlocal foldmethod& foldenable& foldlevel&
  endif

  call setline(1, "X\tY")
  call setline(2, 'after')
  call cursor(1, 2)
  redraw
  call assert_equal(7, screenpos(0, 1, col('.')).curscol)

  call setline(1, "X\u3042Y")
  call cursor(1, 2)
  redraw
  call assert_equal(1, screenpos(0, 1, col('.')).curscol)

  call CloseWindow()
  call NewWindow(6, 20)
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=n
        \ signcolumn=no nonumber showbreak=
  syntax clear test
  syntax match test /X\+/ conceal

  call setline(1, ['one two three four five six seven', 'XXX', 'after'])
  call cursor(1, strlen(getline(1)))
  redraw
  normal! gj
  call assert_equal(2, line('.'))
  call cursor(3, 1)
  redraw
  normal! gk
  call assert_equal(2, line('.'))

  syntax clear test
  call CloseWindow()
  call NewWindow(6, 30)
  setlocal wrap linebreak conceallevel=3 concealcursor=n signcolumn=no
        \ nonumber showbreak=++
  syntax match test /<[^>]*>/ conceal

  call setline(1, 'alpha beta 日本語<hidden-target> followed words')
  call setline(2, 'alpha beta narrow text<hidden-target> after')
  call setline(3, 'alpha beta 日本語 text<hidden-target> after the wrap point')
  redraw
  call assert_equal([
        \ 'alpha beta 日本語 followed    ',
        \ '++words                       ',
        \ 'alpha beta narrow text after  ',
        \ 'alpha beta 日本語 text after  ',
        \ '++the wrap point              ',
        \ '~                             ',
        \ ], ScreenLines([1, 6], winwidth(0)))

  setlocal showbreak=
  call setline(1, 'alpha <hidden-target> second concealed link with text after')
  call setline(2, 'plain after')
  call setline(3, '')
  redraw
  call assert_equal([
        \ 'alpha  second concealed link  ',
        \ 'with text after               ',
        \ 'plain after                   ',
        \ ], ScreenLines([1, 3], winwidth(0)))

  call CloseWindow()
  call NewWindow(4, 21)
  setlocal wrap linebreak conceallevel=3 concealcursor=n signcolumn=no
        \ nonumber showbreak=
  syntax clear test
  syntax match test /\[/ conceal
  call setline(1, 'aaaa bbbb cccc [second concealed')
  redraw
  let expected = ['aaaa bbbb cccc', 'second concealed', '~']
  call map(expected, 'v:val .. repeat(" ", winwidth(0) - strdisplaywidth(v:val))')
  call assert_equal([
        \ expected[0],
        \ expected[1],
        \ expected[2],
        \ ], ScreenLines([1, 3], winwidth(0)))

  call CloseWindow()
  call NewWindow(7, 37)
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=n
        \ signcolumn=no number showbreak=
  syntax clear test
  syntax match test /\[/ conceal
  syntax match test /\](https:[^)]*)/ conceal
  call setline(1, 'This paragraph has bold text before 日本語, italic text before コンシール, and a [concealed link title 日本語](https://example.invalid/a/very/long/path/that/should-be-hidden-by-markdown-conceal) followed by enough words to wrap several times in a narrow window.')
  redraw
  call assert_equal([
        \ '  1 This paragraph has bold text',
        \ '    before 日本語, italic text',
        \ '    before コンシール, and a',
        \ '    concealed link title 日本語',
        \ '    followed by enough words to wrap',
        \ '    several times in a narrow window.',
        \ '~',
        \ ], map(ScreenLines([1, 7], winwidth(0)),
        \ 'substitute(v:val, "\\s\\+$", "", "")'))
  call assert_equal(5, screenpos(0, 1,
        \ stridx(getline(1), 'followed') + 1).row)

  call CloseWindow()
  call NewWindow(7, 42)
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=n
        \ signcolumn=no number showbreak=
  syntax clear test
  syntax match test /\[/ conceal
  syntax match test /\](https:[^)]*)/ conceal
  syntax match test /\*\*/ conceal
  syntax match test /\*/ conceal
  call setline(1, 'This paragraph has **bold text before 日本語**, *italic text before コンシール*, and a [concealed link title 日本語](https://example.invalid/a/very/long/path/that/should-be-hidden-by-markdown-conceal) followed by enough words to wrap several times in a narrow window.')
  redraw
  call cursor(1, 1)
  normal! g$
  call assert_equal(1, winline())
  call cursor(1, 1)
  normal! gjgjl
  call assert_equal(stridx(getline(1), ', and a') + 3, col('.'))
  call setline(2, '')
  call cursor(1, stridx(getline(1), 'several times') + 1)
  normal! gj
  call assert_equal(2, line('.'))
  call assert_equal(1, col('.'))
  normal! gk
  call assert_equal(1, line('.'))
  call assert_equal(stridx(getline(1), 'several times') + 1, col('.'))

  let line = getline(1)
  let start_col = stridx(line, '日本語](https') + 1
  call cursor(1, start_col)
  normal! l
  call assert_equal(start_col + strlen('日'), col('.'))
  normal! h
  call assert_equal(start_col, col('.'))

  let start_col = stridx(line, '](https:') + 1
  call cursor(1, start_col)
  normal! l
  call assert_equal(start_col + 1, col('.'))
  normal! 5l
  call assert_equal(start_col + 6, col('.'))
  normal! h
  call assert_equal(start_col + 5, col('.'))

  call CloseWindow()
  call NewWindow(7, 40)
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=n
        \ signcolumn=no number showbreak=
  syntax clear test
  syntax match test /\[/ conceal
  syntax match test /\](https:[^)]*)/ conceal
  syntax match test /\*\*/ conceal
  call setline(1, 'This paragraph puts the wide text later: ordinary words ordinary words ordinary words ordinary words ordinary words **bold marker hidden here** then 漢字かな交じり文 and a [second concealed link with 東京都 text](https://example.invalid/hidden-target) after the wrap point.')
  redraw
  call cursor(1, 1)
  normal! gj
  call assert_equal(stridx(getline(1), 'later') + 1, col('.'))
  call assert_equal(2, winline())

  call CloseWindow()
  call NewWindow(7, 42)
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=n
        \ signcolumn=no number showbreak=
  syntax clear test
  syntax match test /\[/ conceal
  syntax match test /\](https:[^)]*)/ conceal
  syntax match test /\*\*/ conceal
  call setline(1, 'This paragraph puts the wide text later: ordinary words ordinary words ordinary words ordinary words ordinary words **bold marker hidden here** then 漢字かな交じり文 and a [second concealed link with 東京都 text](https://example.invalid/hidden-target) after the wrap point.')
  redraw
  call cursor(1, stridx(getline(1), 'ordinary words **') + 1)
  let before = [line('.'), col('.'), winline(), wincol()]
  normal! gj
  call assert_true(col('.') > before[1])
  call assert_equal(5, winline())
  normal! gk
  call assert_equal(before[0:1], [line('.'), col('.')])

  call cursor(1, stridx(getline(1), 'ordinary words **bold') + 1)
  let before = [line('.'), col('.'), winline(), wincol()]
  normal! gj
  call assert_true(col('.') > before[1])
  call assert_equal(5, winline())
  normal! gk
  call assert_equal(before[0:1], [line('.'), col('.')])

  call cursor(1, stridx(getline(1), 'ordinary words **bold marker') + 9)
  let before = [line('.'), col('.'), winline(), wincol()]
  normal! gj
  call assert_true(col('.') > before[1])
  call assert_equal(5, winline())
  normal! gk
  call assert_equal(before[0:1], [line('.'), col('.')])

  call cursor(1, stridx(getline(1), '漢字かな') + strlen('漢字か') + 1)
  let before = [line('.'), col('.'), winline(), wincol()]
  normal! gk
  call assert_true(col('.') < before[1])
  call assert_equal(4, winline())
  normal! gj
  call assert_equal(before[0:1], [line('.'), col('.')])

  syntax clear test
  syntax match test /\[/ conceal
  syntax match test /\](https:[^)]*)/ conceal

  let line = getline(1)
  let start_col = stridx(line, '東京都 text](https') + 1
  call cursor(1, start_col)
  normal! l
  call assert_equal(start_col + strlen('東'), col('.'))
  normal! h
  call assert_equal(start_col, col('.'))

  let start_col = stridx(line, '](https:') + 1
  call cursor(1, start_col)
  normal! l
  call assert_equal(start_col + 1, col('.'))
  normal! 5l
  call assert_equal(start_col + 6, col('.'))
  normal! h
  call assert_equal(start_col + 5, col('.'))

  call CloseWindow()
  call NewWindow(6, 40)
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=n
        \ signcolumn=no nonumber showbreak=
  syntax clear test
  syntax region testCode matchgroup=test start=/`/ end=/`/ concealends
  call setline(1, 'The inline code `printf("日本語 %s", value)` should hide its backticks when Markdown conceal is active, while this escaped marker \# should display as a literal hash and not make following wrapped text appear one cell early or late.')
  redraw
  call cursor(1, 1)
  normal! g$
  call assert_equal(1, winline())
  call cursor(1, stridx(getline(1), 'printf') + 1)
  normal! g$
  call assert_equal(1, winline())

  call CloseWindow()
  call NewWindow(6, 42)
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=n
        \ signcolumn=no nonumber showbreak=
  syntax clear test
  syntax match test /\[/ conceal
  syntax match test /\](https:[^)]*)/ conceal
  syntax region testCode matchgroup=test start=/`/ end=/`/ concealends
  call setline(1, '- A dash list item should use Markdown list formatting, and this item intentionally contains **strong text**, `inline code`, [a link with 日本語](https://example.invalid/list), and enough trailing prose to wrap with breakindent.')
  redraw
  call cursor(1, stridx(getline(1), '[a link') + 1)
  let row = winline()
  normal! g$
  call assert_equal(row, winline())

  call setline(1, 'before [visible](https://example.invalid/path) after')
  redraw
  call cursor(1, stridx(getline(1), 'visible') + strlen('visible'))
  normal! l
  call assert_equal(stridx(getline(1), '](https:') + 1, col('.'))
  normal! h
  call assert_equal(stridx(getline(1), 'visible') + strlen('visible'),
        \ col('.'))

  syntax clear test
  syntax match test /.*/ conceal
  call setline(1, '``` {style="conceal-test"}')
  redraw
  call cursor(1, 1)
  normal! 10l
  call assert_equal(11, col('.'))
  normal! 10h
  call assert_equal(1, col('.'))

  syntax clear test
  syntax match test /\[/ conceal
  syntax match test /\](https:[^)]*)/ conceal
  call CloseWindow()
  call NewWindow(7, 52)
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=n
        \ signcolumn=no nonumber showbreak=
  call setline(1, [
        \ 'short',
        \ 'aaaa bbbb cccc dddd eeee ffff gggg hhhh iiii jjjj [visible text](https://example.invalid/hidden/target) kkkk llll mmmm nnnn oooo pppp qqqq rrrr ssss tttt uuuu vvvv wwww xxxx yyyy zzzz',
        \ ])
  if line('$') > 2
    3,$delete _
  endif
  redraw
  call cursor(2, 120)
  normal! gj
  let before = [line('.'), col('.'), winline(), wincol()]
  normal! gj
  call assert_equal(before, [line('.'), col('.'), winline(), wincol()])

  syntax clear test
  syntax match test /\[/ conceal
  syntax match test /\](https:[^)]*)/ conceal
  syntax region testCode matchgroup=test start=/`/ end=/`/ concealends
  call setline(1, '- A dash list item should use Markdown list formatting, and this item intentionally contains **strong text**, `inline code`, [a link with 日本語](https://example.invalid/list), and enough trailing prose to wrap with breakindent.')
  redraw
  for startcol in [
        \ stridx(getline(1), 'strong text') + strlen('strong t') + 1,
        \ stridx(getline(1), 'inline code') + strlen('inline') + 1,
        \ stridx(getline(1), 'a link with') + strlen('a link with ') + 1,
        \ ]
    call cursor(1, startcol)
    redraw
    let before = [winline(), wincol()]
    normal! gj
    redraw
    call assert_equal(before[0] + 1, winline(),
          \ printf('gj moved %d rows from list col %d',
          \ winline() - before[0], startcol))
    normal! gk
    redraw
    call assert_equal(before, [winline(), wincol()],
          \ printf('gj/gk changed visual position from list col %d',
          \ startcol))
  endfor

  call CloseWindow()
  call NewWindow(4, 30)
  setlocal wrap linebreak conceallevel=3 concealcursor=n signcolumn=no
        \ nonumber showbreak=
  syntax clear test
  syntax match test /\[/ conceal
  syntax match testCode /:set columns=60/
  highlight testCode ctermfg=Red guifg=Red
  call setline(1, repeat('a', winwidth(0) - 1) .. '[:set columns=60')
  redraw
  call assert_equal(':', screenstring(1, winwidth(0)))
  call assert_equal('s', screenstring(2, 1))
  call assert_equal(screenattr(2, 1), screenattr(1, winwidth(0)))

  call CloseWindow()
  call NewWindow(4, 12)
  setlocal wrap linebreak conceallevel=3 concealcursor=n signcolumn=no
        \ nonumber showbreak=
  syntax clear test
  syntax region test matchgroup=test start=/\*/ end=/\*/ concealends
  call setline(1, 'aaaa bbbb *italic words')
  redraw
  call assert_equal([
        \ 'aaaa bbbb   ',
        \ 'italic words',
        \ '~           ',
        \ ], ScreenLines([1, 3], winwidth(0)))

  call CloseWindow()
  call NewWindow(5, 49)
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=n
        \ signcolumn=no number showbreak=
  syntax clear test
  syntax region testItalic matchgroup=test start=/\*/ end=/\*/ concealends
  call setline(1, 'This paragraph has bold text before 日本語, *italic text before コンシール*, and trailing words.')
  redraw
  call assert_equal([
        \ '  1 This paragraph has bold text before 日本語,',
        \ '    italic text before コンシール, and trailing',
        \ '    words.',
        \ '~',
        \ ], map(ScreenLines([1, 4], winwidth(0)),
        \ 'substitute(v:val, "\\s\\+$", "", "")'))

  for width in [41, 50]
    call CloseWindow()
    call NewWindow(12, width)
    setlocal wrap linebreak breakindent conceallevel=3 concealcursor=n
          \ signcolumn=no number showbreak=
    syntax clear test
    syntax match test /\[/ conceal
    syntax match test /\](https:[^)]*)/ conceal
    syntax match test /\*\*/ conceal
    syntax match test /\*/ conceal

    call setline(1, 'This paragraph has **bold text before 日本語**, *italic text before コンシール*, and a [concealed link title 日本語](https://example.invalid/a/very/long/path/that/should-be-hidden-by-markdown-conceal) followed by enough words to wrap several times in a narrow window.')
    redraw
    let startcols = [
          \ stridx(getline(1), '日本語') + 1,
          \ stridx(getline(1), ', *italic') + 1,
          \ stridx(getline(1), 'concealed link title') + 1,
          \ stridx(getline(1), '日本語](https') + 1,
          \ ]
    for startcol in startcols
      call cursor(1, startcol)
      redraw
      let before = [winline(), wincol()]
      let before_pos = [line('.'), col('.')]
      normal! gj
      redraw
      call assert_notequal(before_pos, [line('.'), col('.')],
            \ printf('gj did not move from width %d col %d',
            \ width, startcol))
      call assert_true(winline() >= before[0]
            \ || line('.') > before_pos[0]
            \ || col('.') > before_pos[1],
            \ printf('gj moved backwards from width %d col %d',
            \ width, startcol))
      normal! gk
      redraw
      call assert_equal(before, [winline(), wincol()],
            \ printf('gj/gk changed visual position from width %d col %d',
            \ width, startcol))

      call cursor(1, startcol)
      redraw
      let before_row = winline()
      normal! g0
      redraw
      call assert_equal(before_row, winline(),
            \ printf('g0 changed screen row from width %d col %d',
            \ width, startcol))

      call cursor(1, startcol)
      redraw
      let before_row = winline()
      normal! g$
      redraw
      call assert_equal(before_row, winline(),
            \ printf('g$ changed screen row from width %d col %d',
            \ width, startcol))
    endfor

    call setline(1, 'This paragraph puts the wide text later: ordinary words ordinary words ordinary words ordinary words ordinary words **bold marker hidden here** then 漢字かな交じり文 and a [second concealed link with 東京都 text](https://example.invalid/hidden-target) after the wrap point.')
    redraw
    for startcol in [111, 115, 175]
      call cursor(1, startcol)
      redraw
      let before = [winline(), wincol()]
      let before_pos = [line('.'), col('.')]
      normal! gj
      redraw
      call assert_notequal(before_pos, [line('.'), col('.')],
            \ printf('gj did not move from width %d col %d',
            \ width, startcol))
      call assert_true(winline() >= before[0]
            \ || line('.') > before_pos[0]
            \ || col('.') > before_pos[1],
            \ printf('gj moved backwards from width %d col %d',
            \ width, startcol))
      normal! gk
      redraw
      call assert_equal(before, [winline(), wincol()],
            \ printf('gj/gk changed visual position from width %d col %d',
            \ width, startcol))

      call cursor(1, startcol)
      redraw
      let before_row = winline()
      normal! g0
      redraw
      call assert_equal(before_row, winline(),
            \ printf('g0 changed screen row from width %d col %d',
            \ width, startcol))

      call cursor(1, startcol)
      redraw
      let before_row = winline()
      normal! g$
      redraw
      call assert_equal(before_row, winline(),
            \ printf('g$ changed screen row from width %d col %d',
            \ width, startcol))
    endfor
  endfor

  call CloseWindow()
  call NewWindow(7, 42)
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=n
        \ signcolumn=no nonumber showbreak=
  syntax clear test
  call setline(1, '- Another item starts normally and then places double-width characters near the middle of the wrapped display line: alpha beta gamma delta epsilon zeta eta theta 東京大阪京都神戸札幌福岡 then more ASCII words.')
  call setline(2, '* A star list item checks the other list marker with bold 日本語.')
  redraw
  call cursor(1, 1)
  let seen = []
  for i in range(1, 12)
    normal! gj
    let pos = [line('.'), col('.')]
    call assert_equal(-1, index(seen, pos),
          \ printf('gj repeated position %s', string(pos)))
    call add(seen, pos)
    if line('.') == 2
      break
    endif
  endfor
  call assert_equal(2, line('.'))
  let col = col('.')
  normal! gj
  call assert_equal(2, line('.'))
  call assert_notequal(col, col('.'))

  call CloseWindow()
  call NewWindow(10, 37)
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=n
        \ signcolumn=no number showbreak=
  syntax clear test
  syntax match test /\[/ conceal
  syntax match test /\](https:[^)]*)/ conceal
  syntax match test /\*\*/ conceal
  syntax region testCode matchgroup=test start=/`/ end=/`/ concealends
  call setline(1, '- A dash list item should use Markdown list formatting, and this item intentionally contains **strong text**, `inline code`, [a link with 日本語](https://example.invalid/list), and enough trailing prose to wrap with breakindent.')
  redraw
  call cursor(1, stridx(getline(1), 'inline code') + 1)
  normal! g$
  call assert_equal(stridx(getline(1), 'inline code') + strlen('inline code'),
        \ col('.'))
  call cursor(1, stridx(getline(1), 'a link with') + 1)
  normal! g$
  call assert_equal(stridx(getline(1), 'and enough')
        \ + strlen('and enough') + 1,
        \ col('.'))
  call cursor(1, stridx(getline(1), 'trailing prose') + 1)
  normal! g$
  call assert_equal(stridx(getline(1), 'wrap with')
        \ + strlen('wrap with') + 1,
        \ col('.'))
  let sp = screenpos(0, line('.'), col('.'))
  call assert_equal(get(sp, 'curscol', -1), wincol())

  call cursor(1, stridx(getline(1), 'breakindent.') + 1)
  normal! g$
  call assert_equal(strlen(getline(1)), col('.'))
  let sp = screenpos(0, line('.'), col('.'))
  call assert_equal(get(sp, 'curscol', -1), wincol())

  call CloseWindow()
  call NewWindow(10, 59)
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=n
        \ signcolumn=no number showbreak=
  syntax clear test
  call setline(1, '- Another item starts normally and then places double-width characters near the middle of the wrapped display line: alpha beta gamma delta epsilon zeta eta theta 東京大阪京都神戸札幌福岡 then more ASCII words.')
  redraw
  call cursor(1, 130)
  let before = [line('.'), col('.'), winline(), wincol()]
  normal! gj
  normal! gk
  call assert_equal(before, [line('.'), col('.'), winline(), wincol()])

  syntax clear test
  syntax match test /\[/ conceal
  syntax match test /\](https:[^)]*)/ conceal
  syntax match test /\*\*/ conceal
  call setline(1, '0123456789 0123456789 0123456789 **日本語** 0123456789 0123456789 [link](https://example.invalid/hidden) 0123456789 0123456789')
  redraw
  call cursor(1, 55)
  let before = [line('.'), col('.'), winline(), wincol()]
  normal! gj
  normal! gk
  call assert_equal(before, [line('.'), col('.'), winline(), wincol()])

  call CloseWindow()
  call NewWindow(7, 42)
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=n
        \ signcolumn=no number showbreak=
  syntax clear test
  syntax match test /\[/ conceal
  syntax match test /\](https:[^)]*)/ conceal
  syntax match test /\*\*/ conceal
  syntax match test /\*/ conceal
  call setline(1, [
        \ '- A dash list item should use Markdown list formatting, and this item intentionally contains **strong text**, `inline code`, [a link with 日本語](https://example.invalid/list), and enough trailing prose to wrap with breakindent.',
        \ '- Another item starts normally and then places double-width characters near the middle of the wrapped display line: alpha beta gamma delta epsilon zeta eta theta 東京大阪京都神戸札幌福岡 then more ASCII words.',
        \ ])
  redraw
  call winrestview({'lnum': 2, 'col': 0, 'topline': 2, 'leftcol': 0,
        \ 'skipcol': 0, 'curswant': 0})
  call cursor(2, 1)
  redraw
  call assert_equal(2, line('w0'))
  normal! gk
  redraw
  call assert_equal(1, line('.'))
  call assert_equal(1, line('w0'))

  call CloseWindow()
  call NewWindow(7, 59)
  setlocal wrap linebreak breakindent conceallevel=3 concealcursor=n
        \ signcolumn=no nonumber showbreak= scrolloff=0
  syntax clear test
  syntax region testCode matchgroup=test start=/`/ end=/`/ concealends
  syntax region testHtml matchgroup=test start=/<code>/ end=/<\/code>/ concealends
  syntax region testPre matchgroup=test start=/<pre>/ end=/<\/pre>/ concealends
  call setline(1, [
        \ 'The inline code `printf("日本語 %s", value)` should hide its backticks when Markdown conceal is active, while this escaped marker \# should display as a literal hash and not make following wrapped text appear one cell early or late.',
        \ '',
        \ 'Here is an HTML code tag: <code>wide_value = "日本語コンシール"</code> and a pre tag: <pre>alpha beta 日本語 gamma delta</pre> followed by enough plain text to make the line wrap after the concealed tag boundaries.',
        \ ])
  redraw
  call cursor(3, stridx(getline(3), 'alpha beta') + 1)
  normal! zt
  redraw
  normal! gk
  call assert_equal(3, line('.'))
  normal! gk
  call assert_equal(2, line('.'))
  call assert_equal(2, line('w0'))
  call assert_equal(repeat(' ', winwidth(0)),
        \ ScreenLines([1, 1], winwidth(0))[0])
  normal! gk
  call assert_equal(1, line('.'))
  call assert_equal(1, line('w0'))
  call assert_match('^The inline code printf',
        \ ScreenLines([1, 1], winwidth(0))[0])

  syntax clear test
  call CloseWindow()
endfunc

func Test_conceallevel_three_wrap_single_char_syntax()
  call NewWindow(6, 80)
  try
    setlocal wrap conceallevel=3 concealcursor=n signcolumn=no nonumber
    syntax match test /X/ conceal

    call setline(1, repeat('X', winwidth(0) - 3) .. 'YYYY')
    call setline(2, 'after')
    call cursor(2, 1)
    call assert_equal([
          \ 'YYYY' .. repeat(' ', winwidth(0) - 4),
          \ 'after' .. repeat(' ', winwidth(0) - 5),
          \ ], ScreenLines([1, 2], winwidth(0)))
    call assert_equal(2, screenpos(0, 2, 1).row)

  finally
    syntax clear test
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_wrap_matchadd_multiline()
  let matchid = -1
  call NewWindow(6, 4)
  try
    setlocal wrap conceallevel=3 concealcursor=n signcolumn=no nonumber

    let matchid = matchadd('Conceal', 'x\nXXXXX', 10, -1, #{conceal: ''})
    call setline(1, ['x', 'XXXXXYY', 'after'])
    call cursor(3, 1)
    redraw
    call assert_equal(3, screenpos(0, 3, 1).row)
    call matchdelete(matchid)
    let matchid = -1

    syntax match test /Z/ conceal
    call setline(1, 'abcdEZ')
    call deletebufline(bufnr(), 2, '$')
    call cursor(1, 5)
    redraw
    call assert_equal(2, winline())

  finally
    if matchid > 0
      silent! call matchdelete(matchid)
    endif
    syntax clear test
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_wrap_matchaddpos()
  let matchid = -1
  call NewWindow(6, 4)
  try
    setlocal wrap conceallevel=3 concealcursor=n signcolumn=no nonumber

    call setline(1, repeat('X ', 12))
    let positions = map(range(12), {_, i -> [1, i * 2 + 1]})

    " Prime the no-source path before adding a position-only match.
    call cursor(1, 2)
    normal! gj
    call assert_equal([1, 6, 2, 1],
          \ [line('.'), col('.'), winline(), wincol()])

    let matchid = matchaddpos('Conceal', positions, 10, -1,
          \ #{conceal: ''})
    call cursor(1, 2)
    normal! gj
    call assert_equal([1, 10, 2, 1],
          \ [line('.'), col('.'), winline(), wincol()])

    call matchdelete(matchid)
    let matchid = -1
    call cursor(1, 2)
    normal! gj
    call assert_equal([1, 6, 2, 1],
          \ [line('.'), col('.'), winline(), wincol()])

  finally
    if matchid > 0
      silent! call matchdelete(matchid)
    endif
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_wrap_visible_screenpos()
  call NewWindow(6, 10)
  try
    setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
    syntax match test /X\+/ conceal

    call setline(1, repeat('X', 5) .. repeat('Y', 15))
    redraw
    call assert_equal(#{col: 5, row: 2, endcol: 5, curscol: 5},
          \ screenpos(0, 1, 20))

    call setline(1, repeat('X', 5) .. repeat('Y', 95))
    redraw
    let offscreen = #{col: 0, row: 0, endcol: 0, curscol: 0}
    call assert_equal(offscreen, screenpos(0, 1, 100))

    " Also reject the nearest visible cell when the requested byte itself is
    " concealed beyond the bottom of the window.
    call setline(1, repeat('Y', 95) .. repeat('X', 5))
    redraw
    call assert_equal(offscreen, screenpos(0, 1, 100))

    setlocal nowrap
    call assert_equal(offscreen, screenpos(0, 1, 100))

  finally
    syntax clear test
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_wrap_number_line_height()
  call NewWindow(12, 42)
  try
    setlocal wrap linebreak breakindent conceallevel=3 concealcursor=n
    setlocal number signcolumn=no
    syntax region test matchgroup=test start=/`/ end=/`/ concealends

    call setline(1, [
          \ 'top',
          \ '',
          \ 'aaaaaaaaaa bbbbbbbbbb cccccccccc dddddddddd `日本語` eeeeeeeeee ffffffffff gggggggggg hhhhhhhhhh iiiiiiiiii',
          \ '',
          \ 'Plain comparison line without Markdown conceal but with wide text near the same area: 0123456789 0123456789 0123456789 日本語 0123456789 0123456789 0123456789.'])
    redraw

    let row_l3 = screenpos(0, 3, 1).row
    let row_l4 = screenpos(0, 4, 1).row
    let row_l5 = screenpos(0, 5, 1).row
    call assert_equal(row_l3 + 4, row_l4)
    call assert_equal(row_l4 + 1, row_l5)

    call cursor(5, 1)
    redraw
    normal! k
    redraw
    call assert_equal([4, 1], [line('.'), col('.')])
    call assert_equal(row_l4, screenpos(0, line('.'), col('.')).row)
    normal! j
    redraw
    call assert_equal([5, 1], [line('.'), col('.')])
    call assert_equal(row_l5, screenpos(0, line('.'), col('.')).row)

  finally
    syntax clear test
    call CloseWindow()
  endtry
endfunc

func s:TerminalConcealMotionState(buf, statefile) abort
  let termcursor = term_getcursor(a:buf)[0:1]
  let expr = "[line('.'), col('.'), virtcol('.'), winline(), wincol(),"
        \ .. " screenpos(0, line('.'), col('.'))]"
  call delete(a:statefile)
  call term_sendkeys(a:buf,
        \ "\<Esc>:\<C-U>call writefile([string(" .. expr .. "), 'done'], "
        \ .. string(a:statefile) .. ")\<CR>")
  call WaitForAssert({-> assert_true(s:FileEndsWith(a:statefile, 'done'))})
  let state = eval(readfile(a:statefile)[0])
  return [state[0], state[1], state[2], state[3], state[4],
        \ termcursor[0], termcursor[1], state[5].col, state[5].row,
        \ state[5].curscol]
endfunc

func Test_conceallevel_three_queued_screenline_motion()
  call NewWindow(10, 20)
  try
    setlocal wrap linebreak breakindent conceallevel=3 concealcursor=n
          \ signcolumn=no nonumber
    syntax match Hidden /X/ conceal
    call setline(1, repeat('X ', 100))
    call cursor(1, 2)
    redraw!

    call feedkeys('gjgjgjgkgk', 'xt')
    redraw!
    call assert_equal([1, 42, 2, 1],
          \ [line('.'), col('.'), winline(), wincol()])

  finally
    syntax clear Hidden
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_queued_scroll_motion()
  call NewWindow(10, 50)
  try
    setlocal wrap linebreak breakindent conceallevel=3 concealcursor=nvic
          \ signcolumn=no nonumber scrolloff=0
    syntax match Hidden /{{[^}]*}}/ conceal
    let body = repeat('alpha beta gamma delta ', 3)
          \ .. '{{hidden words hidden words}} '
          \ .. repeat('epsilon zeta eta theta ', 3)
    call setline(1, map(range(1, 80),
          \ {_, n -> printf('%03d %s', n, body)}))

    call cursor(1, 1)
    redraw!
    for _ in range(1, 15)
      execute "normal! \<C-D>"
    endfor
    for _ in range(1, 7)
      execute "normal! \<C-U>"
    endfor
    redraw!
    let pos = screenpos(0, line('.'), col('.'))
    let expected = [line('.'), col('.'), winsaveview().topline,
          \ winsaveview().skipcol, winline(), wincol(), pos.row, pos.curscol]

    call winrestview(#{lnum: 1, col: 0, topline: 1, leftcol: 0,
          \ skipcol: 0, curswant: 0})
    call cursor(1, 1)
    redraw!
    call feedkeys(repeat("\<C-D>", 15) .. repeat("\<C-U>", 7), 'xt')
    redraw!
    let pos = screenpos(0, line('.'), col('.'))
    call assert_equal(expected,
          \ [line('.'), col('.'), winsaveview().topline,
          \ winsaveview().skipcol, winline(), wincol(), pos.row, pos.curscol])
  finally
    syntax clear Hidden
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_terminal_linebreak_screenline_motion()
  CheckRunVimInTerminal

  let code =<< trim [CODE]
    set number wrap linebreak breakindent conceallevel=3 concealcursor=nvic
    set showbreak= noshowcmd scrolloff=0 sidescrolloff=0 signcolumn=no
    syntax enable
    highlight test ctermfg=Red guifg=Red
    syntax match test /\[/ conceal
    syntax match test /\](https:[^)]*)/ conceal
    syntax match test /\*\*/ conceal
    syntax match test /\*/ conceal
    syntax region testCode matchgroup=test start=/`/ end=/`/ concealends
    call setline(1, [
          \ '# Markdown Conceal Wrapping Check',
          \ '',
          \ 'This file is for manual testing of wrapped screen lines with Markdown conceal and double-width characters. Use a narrow terminal or try `:set columns=40`, `:set columns=52`, and `:set columns=60`.',
          \ '',
          \ '## Emphasis, Links, and Wide Characters',
          \ '',
          \ 'This paragraph has **bold text before 日本語**, *italic text before コンシール*, and a [concealed link title 日本語](https://example.invalid/a/very/long/path/that/should-be-hidden-by-markdown-conceal) followed by enough words to wrap several times in a narrow window.',
          \ '',
          \ 'This paragraph puts the wide text later: ordinary words ordinary words ordinary words ordinary words ordinary words **bold marker hidden here** then 漢字かな交じり文 and a [second concealed link with 東京都 text](https://example.invalid/hidden-target) after the wrap point.',
          \ '',
          \ '## Lists',
          \ '',
          \ '- A dash list item should use Markdown list formatting, and this item intentionally contains **strong text**, `inline code`, [a link with 日本語](https://example.invalid/list), and enough trailing prose to wrap with breakindent.',
          \ '- Another item starts normally and then places double-width characters near the middle of the wrapped display line: alpha beta gamma delta epsilon zeta eta theta 東京大阪京都神戸札幌福岡 then more ASCII words.',
          \ '* A star list item checks the other list marker with **bold 日本語**, *italic 日本語*, ~~struck 日本語~~, and a long link [visible title](https://example.invalid/star-list-target).',
          \ '+ A plus list item checks the third list marker and keeps adding text after 漢字 so that screen wrapping must account for both conceal and East Asian width.',
          \ '',
          \ '## Inline Code, Escapes, and HTML',
          \ '',
          \ 'The inline code `printf("日本語 %s", value)` should hide its backticks when Markdown conceal is active, while this escaped marker \# should display as a literal hash and not make following wrapped text appear one cell early or late.',
          \ '',
          \ 'Here is an HTML code tag: <code>wide_value = "日本語コンシール"</code> and a pre tag: <pre>alpha beta 日本語 gamma delta</pre> followed by enough plain text to make the line wrap after the concealed tag boundaries.',
          \ '',
          \ '## Fenced Code',
          \ '',
          \ '``` {style="conceal-test"}',
          \ '// Fence markers may be concealed by plugins; this long comment keeps double-width text near a wrap boundary: alpha beta gamma delta 日本語 epsilon zeta eta theta iota kappa lambda.',
          \ 'const message = "The string contains 日本語 and markdown-looking **markers** that should not be treated as emphasis inside the code block.";',
          \ '```',
          \ '',
          \ '## Boundary Stress Lines',
          \ '',
          \ '0123456789 0123456789 0123456789 **日本語** 0123456789 0123456789 [link](https://example.invalid/hidden) 0123456789 0123456789',
          \ '',
          \ 'aaaaaaaaaa bbbbbbbbbb cccccccccc dddddddddd `日本語` eeeeeeeeee ffffffffff gggggggggg hhhhhhhhhh iiiiiiiiii',
          \ '',
          \ 'Plain comparison line without Markdown conceal but with wide text near the same area: 0123456789 0123456789 0123456789 日本語 0123456789 0123456789 0123456789.'])
  [CODE]
  call writefile(code,
        \ 'XTest_conceallevel_three_terminal_linebreak_screenline_motion',
        \ 'D')
  let statefile =
        \ 'XTest_conceallevel_three_terminal_linebreak_screenline_motion_state'
  let buf = 0
  try
    let buf = RunVimInTerminal(
          \ '-S XTest_conceallevel_three_terminal_linebreak_screenline_motion',
          \ #{rows: 20, cols: 42, wait_for_ruler: 0})
    call TermWait(buf, 300)

    let keys = ['20G', 'gj', 'gj', 'gj', 'gk', 'gk', 'gj', 'g$', 'g0']
    let expected = [
          \ [20, 1, 1, 4, 5, 4, 5, 5, 4, 5],
          \ [20, 41, 39, 5, 5, 5, 5, 5, 5, 5],
          \ [20, 80, 82, 6, 5, 6, 5, 5, 6, 5],
          \ [20, 114, 121, 7, 5, 7, 5, 5, 7, 5],
          \ [20, 80, 82, 6, 5, 6, 5, 5, 6, 5],
          \ [20, 41, 39, 5, 5, 5, 5, 5, 5, 5],
          \ [20, 80, 82, 6, 5, 6, 5, 5, 6, 5],
          \ [20, 113, 120, 6, 38, 6, 38, 38, 6, 38],
          \ [20, 80, 82, 6, 5, 6, 5, 5, 6, 5],
          \ ]
    for i in range(len(keys))
      call term_sendkeys(buf, keys[i])
      call WaitForAssert({-> assert_equal(expected[i][5:6],
            \ term_getcursor(buf)[0:1])})
      call assert_equal(expected[i],
            \ s:TerminalConcealMotionState(buf, statefile), keys[i])
    endfor

  finally
    if buf > 0
      call StopVimInTerminal(buf)
    endif
    call delete(statefile)
  endtry
endfunc

func Test_conceallevel_three_wrap_virtual_text()
  CheckFeature textprop

  let did_add_prop_type = v:false
  call NewWindow(6, 80)
  try
    setlocal wrap conceallevel=3 concealcursor=n signcolumn=no nonumber
    syntax match test /X\+/ conceal
    call prop_type_add('test', #{highlight: 'Search'})
    let did_add_prop_type = v:true

    call setline(1, [repeat('X', 10), 'after'])
    call prop_add(1, col([1, '$']),
          \ #{type: 'test', text: repeat('V', winwidth(0) + 1)})
    call cursor(2, 1)
    call assert_equal(3, screenpos(0, 2, 1).row)

    call prop_clear(1)
    call setline(1, [repeat('X', 10), 'after'])
    call prop_add(1, 1, #{type: 'test', text: repeat('V', winwidth(0) + 1)})
    call cursor(2, 1)
    call assert_equal(3, screenpos(0, 2, 1).row)

    setlocal showbreak=++
    call prop_clear(1)
    call setline(1, [repeat('X', 10), 'after'])
    call prop_add(1, 1, #{type: 'test', text: repeat('V', winwidth(0) * 2 - 1)})
    call cursor(2, 1)
    call assert_equal(4, screenpos(0, 2, 1).row)

    call prop_clear(1)
    call setline(1, [repeat('X', winwidth(0) * 2 + 3) .. 'Y', 'after'])
    call prop_add(1, col([1, '$']), #{type: 'test', text: 'V'})
    call cursor(2, 1)
    call assert_equal(2, screenpos(0, 2, 1).row)

  finally
    call prop_clear(1)
    if did_add_prop_type
      call prop_type_delete('test')
    endif
    syntax clear test
    call CloseWindow()
  endtry
endfunc

func Test_conceallevel_three_popup()
  CheckScreendump

  let lines =<< trim END
    call setline(1, ['aaaa XXXXXXXXXX bbbb', 'second line'])
    syntax match test /X\+/ conceal
    let g:winid = popup_create(bufnr(), #{
          \ line: 3, col: 5, maxwidth: 30, wrap: v:true,
          \ border: [],
          \ })
    call win_execute(g:winid, 'setlocal conceallevel=3 concealcursor=n')
    call win_execute(g:winid, 'syntax match test /X\+/ conceal')
  END
  call writefile(lines, 'XpopupConceal', 'D')
  let buf = RunVimInTerminal('-S XpopupConceal', #{rows: 12, cols: 50})
  try
    call VerifyScreenDump(buf, 'Test_conceallevel_three_popup_1', {})
  finally
    call StopVimInTerminal(buf)
  endtry
endfunc

func Test_conceallevel_three_cursor_moved_redraw()
  CheckRunVimInTerminal

  let code =<< trim [CODE]
    set wrap conceallevel=3 concealcursor=n signcolumn=no nonumber
    syntax match test /X\+/ conceal
    call setline(1, ['', repeat('X', &columns - 3) .. 'YYYY', 'after'])
    call cursor(1, 1)
  [CODE]
  call writefile(code, 'XTest_conceallevel_three_cursor_moved_redraw', 'D')
  let buf = RunVimInTerminal('-S XTest_conceallevel_three_cursor_moved_redraw',
        \ {'rows': 6, 'cols': 80})
  try
    call WaitForAssert({-> assert_equal('YYYY', term_getline(buf, 2))})
    call term_sendkeys(buf, 'j')
    call WaitForAssert({-> assert_equal('YYYY', term_getline(buf, 2))})
    call assert_equal('after', term_getline(buf, 3))
    call term_sendkeys(buf, 'k')
    call WaitForAssert({-> assert_equal('YYYY', term_getline(buf, 2))})

    call term_sendkeys(buf, ":set concealcursor=\<CR>")
    call term_sendkeys(buf, 'j')
    call WaitForAssert({-> assert_equal(repeat('X', 77) .. 'YYY',
          \ term_getline(buf, 2))})
    call assert_equal('Y', term_getline(buf, 3))
    call assert_equal('after', term_getline(buf, 4))
    call term_sendkeys(buf, 'k')
    call WaitForAssert({-> assert_equal('YYYY', term_getline(buf, 2))})
    call assert_equal('after', term_getline(buf, 3))

  finally
    call StopVimInTerminal(buf)
  endtry
endfunc

func Test_conceallevel_three_queued_horizontal_redraw()
  CheckRunVimInTerminal

  let code =<< trim [CODE]
    set wrap linebreak breakindent conceallevel=3 concealcursor=nvic
    set showbreak=+\  noshowcmd scrolloff=0 signcolumn=no
    let line = 'a`a`a`a`' .. repeat('a', &columns - 15)
          \ .. ' b`b`' .. repeat('b', &columns - 10) .. ' cccccc'
    call setline(1, [repeat('x', &columns), '', line,
          \ 'BELOW ONE', 'BELOW TWO'])
    syntax region CodeSpan matchgroup=Delimiter start=/\z(`\+\)/
          \ end=/\z1/ concealends
    call cursor(3, 1)
    redraw!
  [CODE]
  let script = 'XTest_conceallevel_three_queued_horizontal_redraw'
  call writefile(code, script, 'D')

  let expected = [repeat('x', 75), '',
        \ repeat('a', 64) .. ' ' .. repeat('b', 10),
        \ '+ ' .. repeat('b', 57) .. ' cccccc',
        \ 'BELOW ONE', 'BELOW TWO', '~' .. repeat(' ', 74)]

  let buf = 0
  try
    let buf = RunVimInTerminal('-S ' .. script,
          \ #{rows: 8, cols: 75, wait_for_ruler: 0})
    call TermWait(buf, 100)
    call term_sendkeys(buf, repeat("\<C-L>", 3))
    call WaitForAssert({-> assert_equal(expected,
          \ map(range(1, 7), 'term_getline(buf, v:val)'))})

    for [motion, col, cursor] in [['l', 1, [3, 2]], ['h', 3, [3, 1]]]
      " Queue the motion inside Vim so that char_avail() reliably sees the
      " following character.  A PTY may deliver separately written or even
      " adjacent characters one at a time.
      call term_sendkeys(buf, ":call cursor(3, " .. col .. ")"
            \ .. " | call feedkeys('" .. motion .. "g', 't')\<CR>")
      call WaitForAssert({-> assert_equal(cursor,
            \ term_getcursor(buf)[0:1])})
      call assert_equal(expected,
            \ map(range(1, 7), 'term_getline(buf, v:val)'), motion)
      call term_sendkeys(buf, "\<Esc>")
      call TermWait(buf, 50)
    endfor
  finally
    if buf > 0
      call StopVimInTerminal(buf)
    endif
  endtry
endfunc

func Test_conceallevel_three_open_above_redraw()
  CheckRunVimInTerminal

  let code =<< trim [CODE]
    set wrap conceallevel=3 signcolumn=no nonumber
    execute 'syntax match test /X\+/ conceal cchar=' .. ' '
    call setline(1, repeat('X', &columns - 3) .. 'YYYY')
    call cursor(1, 1)
  [CODE]
  call writefile(code, 'XTest_conceallevel_three_open_above_redraw', 'D')
  let buf = RunVimInTerminal('-S XTest_conceallevel_three_open_above_redraw',
        \ {'rows': 6, 'cols': 80})
  try
    call WaitForAssert({-> assert_equal(repeat('X', 77) .. 'YYY',
          \ term_getline(buf, 1))})
    call assert_equal('Y', term_getline(buf, 2))

    call term_sendkeys(buf, 'O')
    call WaitForAssert({-> assert_equal('', term_getline(buf, 1))})
    call assert_equal('YYYY', term_getline(buf, 2))
    call assert_equal('~' .. repeat(' ', 79), term_getline(buf, 3))

  finally
    call StopVimInTerminal(buf)
  endtry
endfunc

func s:Run_conceallevel_three_open_above_redraw(name, setup, keys, expected)
  let code = [
        \ 'set wrap conceallevel=3 signcolumn=no nonumber',
        \ 'execute ''syntax match test /X\+/ conceal cchar='' .. '' ''',
        \ 'call setline(1, repeat("X", &columns - 3) .. "YYYY")',
        \ ] + a:setup
  call writefile(code, 'XTest_conceallevel_three_' .. a:name, 'D')
  let buf = RunVimInTerminal('-S XTest_conceallevel_three_' .. a:name,
        \ {'rows': 8, 'cols': 80})
  try
    call WaitForAssert({-> assert_notequal('', term_getline(buf, 1))})
    call term_sendkeys(buf, a:keys)
    call WaitForAssert({-> assert_equal(a:expected[0], term_getline(buf, 1))})
    for i in range(1, len(a:expected) - 1)
      call assert_equal(a:expected[i], term_getline(buf, i + 1))
    endfor

  finally
    call StopVimInTerminal(buf)
  endtry
endfunc

func Test_conceallevel_three_insert_above_redraw()
  CheckRunVimInTerminal

  call s:Run_conceallevel_three_open_above_redraw('open_above_cocu_n',
        \ ['set concealcursor=n', 'call cursor(1, 1)'],
        \ 'O',
        \ ['', 'YYYY', '~' .. repeat(' ', 79)])

  call s:Run_conceallevel_three_open_above_redraw('open_above_showbreak',
        \ ['set showbreak=++', 'call cursor(1, 1)'],
        \ 'O',
        \ ['', 'YYYY', '~' .. repeat(' ', 79)])

  if has('rightleft')
    call s:Run_conceallevel_three_open_above_redraw('open_above_rightleft',
          \ ['set rightleft', 'call cursor(1, 1)'],
          \ 'O',
          \ ['', repeat(' ', 76) .. 'YYYY', repeat(' ', 79) .. '~'])
  endif

  call s:Run_conceallevel_three_open_above_redraw('put_above_normal',
        \ ['call cursor(1, 1)', 'call setreg("a", "NEW", "l")'],
        \ '"aP',
        \ ['NEW', 'YYYY', '~' .. repeat(' ', 79)])

  call s:Run_conceallevel_three_open_above_redraw('put_above_ex',
        \ ['call cursor(1, 1)', 'call setreg("a", "NEW", "l")'],
        \ ':put! a' .. "\<CR>",
        \ ['NEW', 'YYYY', '~' .. repeat(' ', 79)])
endfunc

" Test that line wrapping is correct when double-width chars are concealed.
func Test_conceal_double_width_wrap()
  CheckScreendump

  let code =<< trim [CODE]
    call setline(1, 'aaaaaaaaaa口=口bbbbbbbbbb口=口cccccccccc')
    syntax match test /口=口/ conceal cchar=β
    set conceallevel=2 concealcursor=n
    normal! $
  [CODE]
  call writefile(code, 'XTest_conceal_double_width_wrap', 'D')
  let buf = RunVimInTerminal('-S XTest_conceal_double_width_wrap', {'rows': 4, 'cols': 20})
  try
    call VerifyScreenDump(buf, 'Test_conceal_double_width_wrap_1', {})
    call term_sendkeys(buf, "gM")
    call VerifyScreenDump(buf, 'Test_conceal_double_width_wrap_2', {})
    call term_sendkeys(buf, ":set conceallevel=3\<CR>")
    call VerifyScreenDump(buf, 'Test_conceal_double_width_wrap_3', {})
    call term_sendkeys(buf, "$")
    call VerifyScreenDump(buf, 'Test_conceal_double_width_wrap_4', {})

  finally
    call StopVimInTerminal(buf)
  endtry
endfunc

" vim: shiftwidth=2 sts=2 expandtab
