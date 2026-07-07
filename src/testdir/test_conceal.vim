" Tests for 'conceal'.

CheckFeature conceal

source util/screendump.vim

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
  call assert_true(pos.row > 0)

  call test_setmouse(pos.row, pos.curscol)
  let mousepos = getmousepos()
  call assert_equal(1, mousepos.line)
  call assert_equal(target_col, mousepos.column)
  call assert_equal(0, mousepos.coladd)

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

  syntax clear test
  call CloseWindow()
endfunc

func Test_conceallevel_three_wrap_matchadd_multiline()
  call NewWindow(6, 4)
  setlocal wrap conceallevel=3 concealcursor=n signcolumn=no nonumber

  let matchid = matchadd('Conceal', 'x\nXXXXX', 10, -1, #{conceal: ''})
  call setline(1, ['x', 'XXXXXYY', 'after'])
  call cursor(3, 1)
  redraw
  call assert_equal(3, screenpos(0, 3, 1).row)
  call matchdelete(matchid)

  syntax match test /Z/ conceal
  call setline(1, 'abcdEZ')
  call deletebufline(bufnr(), 2, '$')
  call cursor(1, 5)
  redraw
  call assert_equal(2, winline())

  syntax clear test
  call CloseWindow()
endfunc

func Test_conceallevel_three_wrap_visible_screenpos()
  call NewWindow(6, 10)
  setlocal wrap conceallevel=3 concealcursor=nvic signcolumn=no nonumber
  syntax match test /X\+/ conceal

  call setline(1, repeat('X', 5) .. repeat('Y', 15))
  redraw
  call assert_equal(#{col: 5, row: 2, endcol: 5, curscol: 5},
        \ screenpos(0, 1, 20))

  syntax clear test
  call CloseWindow()
endfunc

func Test_conceallevel_three_wrap_number_line_height()
  call NewWindow(12, 42)
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

  syntax clear test
  call CloseWindow()
endfunc

func Test_conceallevel_three_wrap_virtual_text()
  CheckFeature textprop

  call NewWindow(6, 80)
  setlocal wrap conceallevel=3 concealcursor=n signcolumn=no nonumber
  syntax match test /X\+/ conceal
  call prop_type_add('test', #{highlight: 'Search'})

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

  call prop_type_delete('test')
  syntax clear test
  call CloseWindow()
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
  call VerifyScreenDump(buf, 'Test_conceallevel_three_popup_1', {})

  call StopVimInTerminal(buf)
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

  call StopVimInTerminal(buf)
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
  call WaitForAssert({-> assert_equal(repeat('X', 77) .. 'YYY',
        \ term_getline(buf, 1))})
  call assert_equal('Y', term_getline(buf, 2))

  call term_sendkeys(buf, 'O')
  call WaitForAssert({-> assert_equal('', term_getline(buf, 1))})
  call assert_equal('YYYY', term_getline(buf, 2))
  call assert_equal('~' .. repeat(' ', 79), term_getline(buf, 3))

  call StopVimInTerminal(buf)
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
  call WaitForAssert({-> assert_notequal('', term_getline(buf, 1))})
  call term_sendkeys(buf, a:keys)
  call WaitForAssert({-> assert_equal(a:expected[0], term_getline(buf, 1))})
  for i in range(1, len(a:expected) - 1)
    call assert_equal(a:expected[i], term_getline(buf, i + 1))
  endfor

  call StopVimInTerminal(buf)
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

  call s:Run_conceallevel_three_open_above_redraw('open_above_rightleft',
        \ ['set rightleft', 'call cursor(1, 1)'],
        \ 'O',
        \ ['', repeat(' ', 76) .. 'YYYY', repeat(' ', 79) .. '~'])

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
  call VerifyScreenDump(buf, 'Test_conceal_double_width_wrap_1', {})
  call term_sendkeys(buf, "gM")
  call VerifyScreenDump(buf, 'Test_conceal_double_width_wrap_2', {})
  call term_sendkeys(buf, ":set conceallevel=3\<CR>")
  call VerifyScreenDump(buf, 'Test_conceal_double_width_wrap_3', {})
  call term_sendkeys(buf, "$")
  call VerifyScreenDump(buf, 'Test_conceal_double_width_wrap_4', {})

  " clean up
  call StopVimInTerminal(buf)
endfunc

" vim: shiftwidth=2 sts=2 expandtab
