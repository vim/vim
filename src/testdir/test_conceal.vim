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
