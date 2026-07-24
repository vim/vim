" Tests for 'rightmargin'

scriptencoding utf-8
set encoding=utf-8

source util/view_util.vim
source util/screendump.vim

let s:lines = [
  \'Vim is a greatly improved version of the good old UNIX editor Vi.  Many new',
  \'features have been added: multi-level undo, syntax highlighting, command line',
  \'history, on-line help, spell checking, filename completion, block operations,',
  \'script language, etc.',
\]

let s:double_cell_lines = [
  \'Ｖｉｍ ｉｓ ａ ｇｒｅａｔｌｙ ｉｍｐｒｏｖｅｄ ｖｅｒｓｉｏｎ ｏｆ ｔｈｅ',
  \'ｇｏｏｄ ｏｌｄ ＵＮＩＸ ｅｄｉｔｏｒ Ｖｉ．  Ｍａｎｙ ｎｅｗ',
  \'ｆｅａｔｕｒｅｓ ｈａｖｅ ｂｅｅｎ ａｄｄｅｄ： ｍｕｌｔｉ－ｌｅｖｅｌ',
  \'ｕｎｄｏ， ｓｙｎｔａｘ ｈｉｇｈｌｉｇｈｔｉｎｇ， ｃｏｍｍａｎｄ ｌｉｎｅ',
  \'ｈｉｓｔｏｒｙ， ｏｎ－ｌｉｎｅ ｈｅｌｐ， ｓｐｅｌｌ ｃｈｅｃｋｉｎｇ，',
  \'ｆｉｌｅｎａｍｅ ｃｏｍｐｌｅｔｉｏｎ， ｂｌｏｃｋ ｏｐｅｒａｔｉｏｎｓ，',
  \'ｓｃｒｉｐｔ ｌａｎｇｕａｇｅ， ｅｔｃ．',
\]

function MouseClick(lnum, col)
  call test_setmouse(a:lnum, a:col)
  call feedkeys("\<LeftMouse>\<Ignore>", "xt")
endfunction

" test how rightmargin affects window content:
function Test_rightmargin_basics()
  enew!
  set ff=unix

  let visible_width = 50
  call NewWindow(20, visible_width)

  call setline(1, s:lines)

  set list listchars=extends:> nowrap

  " no rightmargin, no wrap:
  let expected_lines = [
    \'Vim is a greatly improved version of the good old>',
    \'features have been added: multi-level undo, synta>',
    \'history, on-line help, spell checking, filename c>',
    \'script language, etc.                             ',
    \'~                                                 ',
    \'~                                                 ',
    \'~                                                 ',
    \'~                                                 ',
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)

  " set rightmargin and increase window width by same amount:
  vertical resize +5
  set rightmargin=5
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)

  " resize back, unset rightmargin and enable wrap:
  execute "vertical resize" visible_width
  set rightmargin=0 wrap
  let expected_lines = [
    \'Vim is a greatly improved version of the good old ',
    \'UNIX editor Vi.  Many new                         ',
    \'features have been added: multi-level undo, syntax',
    \' highlighting, command line                       ',
    \'history, on-line help, spell checking, filename co',
    \'mpletion, block operations,                       ',
    \'script language, etc.                             ',
    \'~                                                 ',
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)

  " set rightmargin and increase window width by same amount:
  vertical resize +5
  set rightmargin=5
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)

  if has('rightleft')
    " resize back, unset rightmargin, disable wrap, set rightleft:
    execute "vertical resize" visible_width
    set rightmargin=0 rightleft nowrap
    let expected_lines = [
      \'>dlo doog eht fo noisrev devorpmi yltaerg a si miV',
      \'>atnys ,odnu level-itlum :dedda neeb evah serutaef',
      \'>c emanelif ,gnikcehc lleps ,pleh enil-no ,yrotsih',
      \'                             .cte ,egaugnal tpircs',
      \'                                                 ~',
      \'                                                 ~',
      \'                                                 ~',
      \'                                                 ~',
    \]
    let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
    call assert_equal(expected_lines, actual_lines)

    " set rightmargin and increase window width by same amount:
    vertical resize +5
    set rightmargin=5
    let actual_lines = ScreenLines([1, expected_lines->len()], visible_width + 5)->map({_, v -> v[5 :]})
    call assert_equal(expected_lines, actual_lines)

    " resize back, unset rightmargin, enable wrap:
    execute "vertical resize" visible_width
    set rightmargin=0 wrap
    let expected_lines = [
      \' dlo doog eht fo noisrev devorpmi yltaerg a si miV',
      \'                         wen ynaM  .iV rotide XINU',
      \'xatnys ,odnu level-itlum :dedda neeb evah serutaef',
      \'                       enil dnammoc ,gnithgilhgih ',
      \'oc emanelif ,gnikcehc lleps ,pleh enil-no ,yrotsih',
      \'                       ,snoitarepo kcolb ,noitelpm',
      \'                             .cte ,egaugnal tpircs',
      \'                                                 ~',
    \]
    let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
    call assert_equal(expected_lines, actual_lines)

    " set rightmargin and increase window width by same amount:
    vertical resize +5
    set rightmargin=5
    let actual_lines = ScreenLines([1, expected_lines->len()], visible_width + 5)->map({_, v -> v[5 :]})
    call assert_equal(expected_lines, actual_lines)
  endif

  only!
  enew!
  set ff& list& listchars& wrap& rightmargin& rightleft&
endfunction

" test 'rightleft' with empty lines:
function Test_rightmargin_rightleft_empty_line()
  CheckFeature rightleft

  enew!
  set ff=unix

  let visible_width = 10
  call NewWindow(20, visible_width)

  call setline(1, ['', 'abcde', '', 'fghi'])

  set list listchars=extends:> nowrap rightmargin=6 rightleft

  let expected_lines = [
    \'          ',
    \'      >cba',
    \'          ',
    \'      ihgf',
    \'         ~',
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)

  only!
  enew!
  set ff& list& listchars& nowrap& rightmargin& rightleft&
endfunction

" test double-cell characters:
function Test_rightmargin_double_cell()
  enew!
  set ff=unix

  let visible_width = 50
  call NewWindow(20, visible_width)

  call setline(1, s:double_cell_lines)

  set list listchars=extends:> wrap mouse=a

  " no rightmargin:
  let expected_lines = [
    \'Ｖｉｍ ｉｓ ａ ｇｒｅａｔｌｙ ｉｍｐｒｏｖｅｄ ｖ>',
    \'ｅｒｓｉｏｎ ｏｆ ｔｈｅ                          ',
    \'ｇｏｏｄ ｏｌｄ ＵＮＩＸ ｅｄｉｔｏｒ Ｖｉ．  Ｍａ',
    \'ｎｙ ｎｅｗ                                       ',
    \'ｆｅａｔｕｒｅｓ ｈａｖｅ ｂｅｅｎ ａｄｄｅｄ： ｍ',
    \'ｕｌｔｉ－ｌｅｖｅｌ                              ',
    \'ｕｎｄｏ， ｓｙｎｔａｘ ｈｉｇｈｌｉｇｈｔｉｎｇ，',
    \' ｃｏｍｍａｎｄ ｌｉｎｅ                          '
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)

  " double-cell character that doesn't fit at the end of a wrapped line is put to next line,
  " clicking such character also causes the cursor to jump to next line:
  call MouseClick(1, visible_width)
  let expected_winpos = [2, 1]
  let actual_winpos = [winline(), wincol()]
  call assert_equal(expected_winpos, actual_winpos)

  " set rightmargin and increase window width by same amount:
  vertical resize +5
  set rightmargin=5
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)

  call MouseClick(1, visible_width)
  let actual_winpos = [winline(), wincol()]
  call assert_equal(expected_winpos, actual_winpos)

  if has('rightleft')
    " reset window layout and check rightleft:
    execute "vertical resize" visible_width
    set rightmargin=0 rightleft
    let expected_lines = [
      \'>ｖ ｄｅｖｏｒｐｍｉ ｙｌｔａｅｒｇ ａ ｓｉ ｍｉＶ',
      \'                          ｅｈｔ ｆｏ ｎｏｉｓｒｅ',
      \'ａＭ  ．ｉＶ ｒｏｔｉｄｅ ＸＩＮＵ ｄｌｏ ｄｏｏｇ',
      \'                                       ｗｅｎ ｙｎ',
      \'ｍ ：ｄｅｄｄａ ｎｅｅｂ ｅｖａｈ ｓｅｒｕｔａｅｆ',
      \'                              ｌｅｖｅｌ－ｉｔｌｕ',
      \'，ｇｎｉｔｈｇｉｌｈｇｉｈ ｘａｔｎｙｓ ，ｏｄｎｕ',
      \'                          ｅｎｉｌ ｄｎａｍｍｏｃ '
    \]
    let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
    call assert_equal(expected_lines, actual_lines)

    call MouseClick(1, 1)
    " for rightleft the cursor is at leftmost cell of a double-cell character:
    let expected_winpos = [2, visible_width - 1]
    let actual_winpos = [winline(), wincol()]
    call assert_equal(expected_winpos, actual_winpos)

    " set rightmargin and increase window width by same amount:
    vertical resize +5
    set rightmargin=5
    let actual_lines = ScreenLines([1, expected_lines->len()], visible_width + 5)->map({_, v -> v[5 :]})
    call assert_equal(expected_lines, actual_lines)

    call MouseClick(1, 6)
    let expected_winpos = [2, visible_width + 4]
    let actual_winpos = [winline(), wincol()]
    call assert_equal(expected_winpos, actual_winpos)
    set rightleft&
  endif

  " similar as above but with breakindent,
  " double-cell characters can cause cursor placement problems:
  set breakindent breakindentopt=min:20 expandtab
  execute "norm! gg5i\<Tab>"

  call MouseClick(6, 17)
  let expected_winpos = [6, 17]
  let actual_winpos = [winline(), wincol()]
  call assert_equal(expected_winpos, actual_winpos)

  only!
  enew!
  set ff& list& listchars& wrap& rightmargin& breakindent& breakindentopt& expandtab& mouse& rightleft&
endfunction

" test cursor when no room for text:
function Test_rightmargin_cursor_no_text_room()
  enew!
  set ff=unix

  let visible_width = 10
  let visible_height = 5
  call NewWindow(visible_height, visible_width)

  call setline(1, 'abc')
  set wrap scrolloff=0

  " leave only 1 column for text:
  let &rightmargin = visible_width - 1
  let expected_lines = [
    \'a         ',
    \'b         ',
    \'c         ',
    \'~         ',
    \'~         ',
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)

  " rightmargin is equak window width:
  let &rightmargin = visible_width
  let expected_lines = [
    \'          ',
    \'          ',
    \'          ',
    \'          ',
    \'          ',
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)
  call assert_equal(visible_width, winwidth(0))
  call assert_equal(1, wincol())
  call assert_equal(visible_height, winline())

  " rightmargin is larger than window width:
  let &rightmargin = visible_width + 1
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)
  call assert_equal(visible_width, winwidth(0))
  call assert_equal(1, wincol())
  call assert_equal(visible_height, winline())

  only!
  enew!
  set ff& wrap& rightmargin& scrolloff&
endfunction

" test scrolling with 'smoothscroll':
function Test_rightmargin_scroll_smoothscroll()
  enew!
  set ff=unix

  let visible_width = 40
  call NewWindow(10, visible_width)

  set wrap rightmargin=10 smoothscroll scrolloff=0

  " create first long enough line to occupy 3 lines when wrapped:
  let line_len = (visible_width - &rightmargin) * 3
  call setline(1, [repeat('a', line_len), 'line2'])

  " scroll down 3 screen lines:
  execute "norm! 3\<C-e>"
  " should make the first line off screen:
  call assert_equal(2, winsaveview().topline)

  " scroll back up 2 screen lines:
  execute "norm! 2\<C-y>"
  " should reveal the first line partially:
  call assert_equal(1, winsaveview().topline)
  call assert_equal(30, winsaveview().skipcol)

  only!
  enew!
  set wrap& rightmargin& smoothscroll& scrolloff&
endfunction

" test scrolling using skipcol:
function Test_rightmargin_scroll_smoothscroll_skipcol()
  enew!
  set ff=unix

  let visible_width = 20
  call NewWindow(5, visible_width)

  set wrap rightmargin=10 smoothscroll scrolloff=0

  call setline(1, 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz')

  " scroll using skipcol:
  let skipcol = visible_width - &rightmargin + 3
  call winrestview({'topline': 1, 'lnum': 1, 'col': skipcol + 5, 'skipcol': skipcol})

  let expected_lines = [
    \'<<<NOPQRST          ',
    \'UVWXYZabcd          ',
    \'efghijklmn          ',
    \'opqrstuvwx          ',
    \'yz                  ',
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)

  only!
  enew!
  set ff& wrap& rightmargin& smoothscroll& scrolloff&
endfunction

" test skipcol with no room for text:
function Test_rightmargin_smoothscroll_skipcol_no_text_room()
  enew!
  set ff=unix

  let visible_width = 20
  let visible_height = 5
  call NewWindow(visible_height, visible_width)

  set wrap smoothscroll scrolloff=0 number

  let &rightmargin = visible_width

  call setline(1, repeat('a', visible_width * visible_height))

  call winrestview({'topline': 1, 'lnum': 1, 'col': 1, 'skipcol': 1})
  redraw
  call assert_equal(1, winsaveview().skipcol)

  only!
  enew!
  set ff& wrap& rightmargin& smoothscroll& scrolloff& number&
endfunction

" test correcting cursor position with 'smoothscroll':
function Test_rightmargin_smoothscroll_cursor_correct()
  enew!
  set ff=unix

  let visible_width = 20
  call NewWindow(5, visible_width)

  set wrap rightmargin=10 smoothscroll scrolloff=0

  let text_width = visible_width - &rightmargin
  call setline(1, repeat('a', text_width * 20))

  " scroll down one screen line, the cursor becomes is off screen:
  execute "normal! \<C-e>"
  " cursor is moved to next fully visible screen line:
  call assert_equal(text_width, winsaveview().skipcol)
  call assert_equal(text_width * 2 + 1, col('.'))
  call assert_equal(2, winline())

  only!
  enew!
  set ff& wrap& rightmargin& smoothscroll& scrolloff&
endfunction

" test stepping over 'smoothscroll' marker:
function Test_rightmargin_step_over_smoothscroll_marker()
  enew!
  set ff=unix

  let visible_width = 20
  call NewWindow(3, visible_width)

  set wrap rightmargin=10 smoothscroll scrolloff=0

  " create first long enough line to not fit in the window:
  let line_len = (visible_width - &rightmargin) * 4
  call setline(1, repeat('a', line_len))

  " place the cursor right after smoothscroll marker:
  call winrestview({'topline': 1, 'lnum': 1, 'col': 13, 'skipcol': 10})
  call assert_equal(10, winsaveview().skipcol)

  " move cursor onto smoothscroll marker, should scroll up to reveal start of the line:
  norm! h
  call assert_equal(0, winsaveview().skipcol)

  only!
  enew!
  set ff& wrap& rightmargin& smoothscroll& scrolloff&
endfunction

" test scrolling with wrapped lines correctly preserves the cursor:
function Test_rightmargin_scroll_wrapped()
  enew!

  let visible_width = 20
  call NewWindow(3, visible_width)

  setlocal wrap rightmargin=10 scrolloff=0

  " set 3 lines but make the second line long enough to wrap:
  let line_len = (visible_width - &rightmargin) * 2
  call setline(1, ['line1', repeat('a', line_len), 'line3'])

  " scroll down 1 to make first line off screen, should force cursor to second line:
  execute "norm! \<C-e>"

  " move cursor to last character:
  norm! $

  let pos = getpos('.')
  call assert_equal(pos, [0, 2, line_len, 0])

  " scroll up to reveal back the first line (second line is still fully visible):
  execute "norm! \<C-y>"

  " cursor position should not have changed:
  call assert_equal(pos, getpos('.'))

  only!
  enew!
  set ff& nowrap& rightmargin& scrolloff&
endfunction

" test mouse click with 'smoothscroll':
function Test_rightmargin_smoothscroll_mouse_click()
  enew!
  set ff=unix

  call NewWindow(20, 30)

  call setline(1, s:lines)

  set wrap rightmargin=20 smoothscroll mouse=a

  execute "norm! 2\<C-e>"

  call MouseClick(7, 1)
  let expected_pos = [2, 1]
  let actual_pos = [line('.'), col('.')]
  call assert_equal(expected_pos, actual_pos)

  only!
  enew!
  set ff& wrap& rightmargin& smoothscroll& mouse&
endfunction

" test breakindent and breakindentopt min:
function Test_rightmargin_breakindent_min()
  enew!
  set ff=unix

  let visible_width = 50
  call NewWindow(20, visible_width)

  call setline(1, s:lines)

  set wrap breakindent breakindentopt=min:25 rightmargin=5 mouse=a
  execute "norm! 5i\<Tab>"

  let expected_lines = [
    \'                                        Vim i     ',
    \'                    s a greatly improved vers     ',
    \'                    ion of the good old UNIX      ',
    \'                    editor Vi.  Many new          ',
    \'features have been added: multi-level undo, s     ',
    \'yntax highlighting, command line                  ',
    \'history, on-line help, spell checking, filena     ',
    \'me completion, block operations,                  ',
    \'script language, etc.                             ',
    \'~                                                 ',
    \'~                                                 ',
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)

  call MouseClick(3, 22)
  let expected_winpos = [3, 22]
  let actual_winpos = [winline(), wincol()]
  call assert_equal(expected_winpos, actual_winpos)

  only!
  enew!
  set wrap& rightmargin& ff& breakindent& breakindentopt& mouse&
endfunction

" test gq, :center and :right commands:
function Test_rightmargin_format_and_align()
  enew!
  set ff=unix

  let visible_width = 50
  call NewWindow(20, visible_width)

  call setline(1, s:lines)

  set rightmargin=10 expandtab

  " wrapmargin plus rightmargin exceed window width, wrapmargin will be ignored:
  set wrapmargin=45
  norm! ggVGgq
  let expected_lines = [
    \'Vim is a greatly improved version of              ',
    \'the good old UNIX editor Vi.  Many new            ',
    \'features have been added: multi-level             ',
    \'undo, syntax highlighting, command line           ',
    \'history, on-line help, spell checking,            ',
    \'filename completion, block operations,            ',
    \'script language, etc.                             '
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)

  " format to window width minus 'wrapmargin' minus 'rightmargin':
  set wrapmargin=10
  norm! ggVGgq
  let expected_lines = [
    \'Vim is a greatly improved                         ',
    \'version of the good old UNIX                      ',
    \'editor Vi.  Many new features                     ',
    \'have been added: multi-level                      ',
    \'undo, syntax highlighting,                        ',
    \'command line history, on-line                     ',
    \'help, spell checking, filename                    ',
    \'completion, block operations,                     ',
    \'script language, etc.                             '
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)

  set wrapmargin=1

  " align center:
  :%center
  let expected_lines = [
    \'       Vim is a greatly improved                  ',
    \'     version of the good old UNIX                 ',
    \'     editor Vi.  Many new features                ',
    \'     have been added: multi-level                 ',
    \'      undo, syntax highlighting,                  ',
    \'     command line history, on-line                ',
    \'    help, spell checking, filename                ',
    \'     completion, block operations,                ',
    \'         script language, etc.                    '
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)

  " align right:
  :%right
  let expected_lines = [
    \'              Vim is a greatly improved           ',
    \'           version of the good old UNIX           ',
    \'          editor Vi.  Many new features           ',
    \'           have been added: multi-level           ',
    \'             undo, syntax highlighting,           ',
    \'          command line history, on-line           ',
    \'         help, spell checking, filename           ',
    \'          completion, block operations,           ',
    \'                  script language, etc.           '
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)

  only!
  enew!
  set ff& rightmargin& wrapmargin& expandtab&
endfunction

" test cursor scrolling with 'nowrap':
function Test_rightmargin_cursor_scroll_nowrap()
  enew!
  set ff=unix

  let visible_width = 40
  call NewWindow(10, visible_width)

  call setline(1, repeat('x', 60))

  set nowrap rightmargin=10 scrolloff=0 sidescrolloff=0 sidescroll=0

  " stepping onto rightmargin area should scroll horizontally:
  call cursor(1, visible_width - &rightmargin)
  norm! l
  call assert_equal(15, getwininfo(win_getid())[0].leftcol)
  call assert_equal(16, wincol())

  only!
  enew!
  set ff& nowrap& rightmargin& scrolloff& sidescrolloff& sidescroll&
endfunction

" test motions: g^ / g0 / gm / g$ / zH / zL / ze / zl / zh
function Test_rightmargin_motions()
  enew!
  set ff=unix

  let visible_width = 50
  call NewWindow(20, visible_width)

  call setline(1, s:lines)

  set rightmargin=10 scrolloff=0

  " START: testing g$ / gm / zH / zL / ze / zl / zh
  "-----------------------
  " first test with wrap and smoothscroll disabled:
  set nosmoothscroll nowrap

  " reset to initial state:
  norm! 0gg

  " go to middle visible character:
  norm! gm
  let expected_col = 21
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)
  call assert_equal(expected_col, (visible_width - &rightmargin) / 2 + 1)

  " go to last visible character:
  norm! g$
  let expected_col = 40
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)
  call assert_equal(expected_col, visible_width - &rightmargin)

  " reset to initial state:
  norm! 0gg

  " scroll right half-page:
  norm! zL
  let expected_leftcol = 20
  let actual_leftcol = getwininfo(win_getid())[0].leftcol
  call assert_equal(expected_leftcol, actual_leftcol)
  call assert_equal(expected_leftcol, (visible_width - &rightmargin) / 2)
  let expected_col = expected_leftcol + 1 " 21
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)

  " scroll right another half-page:
  norm! zL
  let expected_leftcol = 40
  let actual_leftcol = getwininfo(win_getid())[0].leftcol
  call assert_equal(expected_leftcol, actual_leftcol)
  call assert_equal(expected_leftcol, visible_width - &rightmargin)
  let expected_col = 41
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)
  call assert_equal(expected_col, expected_leftcol + 1)

  " scroll left half-page:
  norm! zH
  let expected_leftcol = 20
  let actual_leftcol = getwininfo(win_getid())[0].leftcol
  call assert_equal(expected_leftcol, actual_leftcol)
  call assert_equal(expected_leftcol, (visible_width - &rightmargin) / 2)
  let expected_col = expected_col " did not change, 41
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)

  " scroll left another half-page:
  norm! zH
  let expected_leftcol = 0
  let actual_leftcol = getwininfo(win_getid())[0].leftcol
  call assert_equal(expected_leftcol, actual_leftcol)
  let expected_col = 40
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)
  call assert_equal(expected_col, visible_width - &rightmargin)

  " reset to initial state:
  norm! 0gg

  " jump to last character:
  norm! $
  " scroll to position the cursor at the end of the screen:
  norm! ze
  let expected_leftcol = 35
  let actual_leftcol = getwininfo(win_getid())[0].leftcol
  call assert_equal(expected_leftcol, actual_leftcol)
  call assert_equal(expected_leftcol, s:lines[0]->strdisplaywidth() - visible_width + &rightmargin)

  " reset to initial state:
  norm! 0gg

  " scroll to the most right:
  norm! 99zl
  let expected_leftcol = 74
  let actual_leftcol = getwininfo(win_getid())[0].leftcol
  call assert_equal(expected_leftcol, actual_leftcol)
  call assert_equal(expected_leftcol, s:lines[0]->strdisplaywidth() - 1)
  let expected_col = 75
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)
  call assert_equal(expected_col, s:lines[0]->strdisplaywidth())

  " scroll to the most left:
  norm! 99zh
  let expected_leftcol = 0
  let actual_leftcol = getwininfo(win_getid())[0].leftcol
  call assert_equal(expected_leftcol, actual_leftcol)
  let expected_col = 40
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)
  call assert_equal(expected_col, visible_width - &rightmargin)

  " repeat with wrap enabled:
  set wrap

  " reset to initial state:
  norm! 0gg

  " scroll down:
  execute "norm! \<C-e>"
  " jump one screen line down:
  norm! gj

  " go to middle visible character:
  norm! gm
  let expected_col = 61
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)

  " go to last visible character:
  norm! g$
  let expected_col = 77
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)

  " repeat with smoothscroll enabled:
  set smoothscroll

  " reset to initial state:
  norm! 0gg

  " scroll down:
  execute "norm! \<C-e>"
  " jump one screen line down:
  norm! gj

  " go to middle visible character:
  norm! gm
  let expected_col = 21
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)

  " go to last visible character:
  norm! g$
  let expected_col = 40
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)
  " END: testing g$ / gm / zH / zL / ze / zl / zh
  "-----------------------

  " START: testing g^ and g0
  "-----------------------
  " first test with smoothscroll disabled:
  set nowrap nosmoothscroll

  " reset to initial state:
  norm! 0gg

  " scroll left to set the first visible and non-blank characters to be
  " different and also not equal to 1:
  norm! 3zl

  " go to first non-blank visible character:
  norm! g^
  let expected_col = 5
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)

  " go to first visible character:
  norm! g0
  let expected_col = 4
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)

  " repeat with wrap enabled:
  set wrap

  " jump one screen line down (expected to have first character blank):
  norm! gj

  " go to first non-blank visible character:
  norm! g^
  let expected_col = 42
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)

  " go to first visible character:
  norm! g0
  let expected_col = 41
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)

  " repeat with smoothscroll enabled:
  set wrap smoothscroll

  " scroll down 3 lines (expected to have first character blank):
  execute "norm! 3\<C-e>"

  " go to first non-blank visible character:
  norm! g^
  let expected_col = 45
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)

  " go to first visible character:
  norm! g0
  let expected_col = 44
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)
  " END: testing g^ and g0
  "-----------------------

  only!
  enew!
  set ff& rightmargin& wrapmargin& wrap& smoothscroll& scrolloff&
endfunction

" test motions: gj / gk
function Test_rightmargin_gj_gk()
  enew!
  set ff=unix

  call NewWindow(20, 30)

  call setline(1, s:lines)

  set wrap rightmargin=5 smoothscroll scrolloff=0 noshowmode noshowcmd

  execute "norm! 2\<C-e>"
  norm! 3G8|

  let expected_line = 3
  let actual_line = line('.')
  call assert_equal(expected_line, actual_line)

  let expected_col = 8
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)
  let actual_line = line('.')
  call assert_equal(expected_line, actual_line)

  " jump screen lines down:
  norm! gj
  let expected_col = 33
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)
  let actual_line = line('.')
  call assert_equal(expected_line, actual_line)

  norm! gj
  let expected_col = 58
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)
  let actual_line = line('.')
  call assert_equal(expected_line, actual_line)

  norm! gj
  let expected_col = 77
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)
  let actual_line = line('.')
  call assert_equal(expected_line, actual_line)

  " jump screen lines up:
  norm! gk
  let expected_col = 58
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)
  let actual_line = line('.')
  call assert_equal(expected_line, actual_line)

  norm! gk
  let expected_col = 33
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)
  let actual_line = line('.')
  call assert_equal(expected_line, actual_line)

  norm! gk
  let expected_col = 8
  let actual_col = col('.')
  call assert_equal(expected_col, actual_col)
  let actual_line = line('.')
  call assert_equal(expected_line, actual_line)

  "set ff& rightmargin& smoothscroll& wrap& scrolloff&
endfunction

" test page scroll down with 'smoothscroll' disabled:
function Test_rightmargin_page_scroll_no_smoothscroll()
  enew!
  set ff=unix

  let visible_width = 20
  let visible_height = 5
  call NewWindow(visible_height, visible_width)

  set wrap nosmoothscroll rightmargin=10 scrolloff=0

  " create long enough line to occupy off screen 2 lines when wrapped:
  let line_len = (visible_width - &rightmargin) * (visible_height + 2)
  call setline(1, [repeat('a', line_len), 'line2'])

  " page down should reveal second line and move cursor to it:
  execute "norm! \<C-F>"
  call assert_equal(2, winsaveview().topline)
  call assert_equal(2, line('.'))

  only!
  enew!
  set ff& smoothscroll& rightmargin& wrap& scrolloff&
endfunction

" test that scrolling and resizing windows properly redraws blank areas, statuslines and window splits:
function Test_rightmargin_scroll_and_resize()
  CheckScreendump
  CheckRunVimInTerminal

  let script_lines =<< trim eval END
    set list listchars=extends:>
    colorscheme koehler
    set nowrap rightmargin=5 wincolor=Error scrolloff=0

    botright vsplit
    call setline(1, {s:lines})

    set rightmargin=5 rightleft
    windo norm zt
  END

  call writefile(script_lines, 'XTest_rightmargin_scroll_and_resize', 'D')
  let buf = RunVimInTerminal('-S XTest_rightmargin_scroll_and_resize', {'rows': 20, 'cols': 70})

  call VerifyScreenDump(buf, 'Test_rightmargin_scroll_and_resize_1', {})

  " scrolling down:
  call term_sendkeys(buf, ":windo norm Gzt\<CR>")
  call VerifyScreenDump(buf, 'Test_rightmargin_scroll_and_resize_2', {})

  " scrolling back to top:
  call term_sendkeys(buf, ":windo norm gg\<CR>")
  call VerifyScreenDump(buf, 'Test_rightmargin_scroll_and_resize_3', {})

  " increasing width:
  call term_sendkeys(buf, ":vertical resize +20\<CR>")
  call VerifyScreenDump(buf, 'Test_rightmargin_scroll_and_resize_4', {})

  " decreasing width:
  call term_sendkeys(buf, ":vertical resize -40\<CR>")
  call VerifyScreenDump(buf, 'Test_rightmargin_scroll_and_resize_5', {})

  " clean up
  call StopVimInTerminal(buf)
endfunction

" test 'wrap' and 'smoothscroll' enabled:
function Test_rightmargin_wrap()
  CheckScreendump
  CheckRunVimInTerminal

  let script_lines =<< trim eval END
    function MouseDrag(lnum_start, col_start, lnum_end, col_end)
      call test_setmouse(a:lnum_start, a:col_start)
      call feedkeys("\<LeftMouse>\<Ignore>", "xt")

      call test_setmouse(a:lnum_end, a:col_end)
      call feedkeys("\<LeftDrag>\<Ignore>", "xt")

      call feedkeys("\<LeftRelease>\<Ignore>", "xt")
    endfunction

    call setline(1, {s:lines})
    30vsplit
    set wrap rightmargin=5 smoothscroll scrolloff=0 noshowmode noshowcmd
    execute "norm! 3\<C-e>"

    call MouseDrag(5, 8, 7, 11)
    echo 'visual range: ' .. line('v') .. ':' .. col('v') .. ' - ' .. line('.') .. ':' .. col('.')
  END

  call writefile(script_lines, 'XTest_rightmargin_wrap_and_visual', 'D')
  let buf = RunVimInTerminal('-S XTest_rightmargin_wrap_and_visual', {'rows': 20, 'cols': 70})
  call VerifyScreenDump(buf, 'Test_rightmargin_wrap_1', {})

  " stop visual:
  call term_sendkeys(buf, "\<Esc>")

  " set cursorline:
  call term_sendkeys(buf, ":set cursorline\<CR>")
  call VerifyScreenDump(buf, 'Test_rightmargin_wrap_2', {})

  " set cursorlineopt=screenline:
  call term_sendkeys(buf, ":set cursorlineopt=screenline\<CR>")
  call VerifyScreenDump(buf, 'Test_rightmargin_wrap_3', {})

  " clean up
  call StopVimInTerminal(buf)
endfunction

" test cursor skipping text property with wrapped virtual text
" and jumping to next line:
function Test_rightmargin_textprop_wrapped_virtual_text()
  enew!
  set ff=unix

  call NewWindow(20, 30)

  call setline(1, s:lines)

  set wrap rightmargin=15 smoothscroll scrolloff=0 noshowmode noshowcmd

  call prop_type_add('virtual_text_prop', #{highlight: "ErrorMsg", bufnr: bufnr()})
  call prop_add(2, 0, #{
    \type: 'virtual_text_prop',
    \text: 'some long virtual text that should wrap',
    \text_align: 'after',
    \text_padding_left: 10
  \})

  norm 11gj
  let expected_winpos = [13, 1]
  let actual_winpos = [winline(), wincol()]
  call assert_equal(expected_winpos, actual_winpos)

  call prop_clear(2)
  call prop_type_delete('virtual_text_prop', #{bufnr: bufnr()})
  only!
  enew!
  set ff& wrap& rightmargin& smoothscroll& scrolloff& showmode& showcmd&
endfunction

" test cursor is rendered on correct screen line when placed
" after a property that was set on an empty line with 'below' align:
function Test_rightmargin_textprop_below_empty_line()
  enew!
  set ff=unix

  let visible_width = 30
  call NewWindow(20, visible_width)

  call setline(1, ['', 'foo'])

  call prop_type_add('virtual_text_prop', #{highlight: "ErrorMsg", bufnr: bufnr()})
  let virtual_text = 'some long virtual text'
  call prop_add(1, 0, #{
    \type: 'virtual_text_prop',
    \text: virtual_text,
    \text_align: 'below',
  \})

  " go to line 2:
  norm 2G

  " set rightmargin small enough to have plenty of space for virtual text:
  set rightmargin=5
  let expected_lines = [
    \'                              ',
    \'some long virtual text        ',
    \'foo                           ',
    \'~                             ',
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)
  " screen line should be 3 (skipping virtual line):
  call assert_equal(3, winline())

  " set rightmargin to maximum value so virtual text still fits:
  let &rightmargin = visible_width - len(virtual_text)
  let expected_lines = [
    \'                              ',
    \'some long virtual text        ',
    \'foo                           ',
    \'~                             ',
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)
  " screen line should be 3 (skipping virtual line):
  call assert_equal(3, winline())

  " virtual text should be trimmed with '…':
  set rightmargin=15
  let expected_lines = [
    \'                              ',
    \'some long virt…               ',
    \'foo                           ',
    \'~                             ',
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)
  " screen line should be 3 (skipping virtual line):
  call assert_equal(3, winline())

  call prop_clear(1)
  call prop_type_delete('virtual_text_prop', #{bufnr: bufnr()})
  only!
  enew!
  set ff& wrap& rightmargin& smoothscroll& scrolloff& showmode& showcmd&
endfunction

" test text property virtual text alignment:
function Test_rightmargin_textprop_align()
  enew!
  set ff=unix

  let visible_width = 30
  call NewWindow(20, visible_width)

  set wrap rightmargin=10
  call prop_type_add('virtual_text_prop', #{highlight: "ErrorMsg", bufnr: bufnr()})

  call setline(1, 'foo bar baz')

  " virtual text should be trimmed with '…':
  call prop_add(1, 0, #{
    \type: 'virtual_text_prop',
    \text: 'some long virtual text',
    \text_align: 'above',
  \})
  let expected_lines = [
    \'some long virtual t…          ',
    \'foo bar baz                   ',
    \'~                             ',
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)
  call prop_clear(1)

  " virtual text should be right-aligned to next line instead of wrap:
  call prop_add(1, 0, #{
    \type: 'virtual_text_prop',
    \text: 'long virtual text',
    \text_align: 'right',
    \text_wrap: 'wrap'
  \})
  let expected_lines = [
    \'foo bar baz                   ',
    \'   long virtual text          ',
    \'~                             ',
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)
  call prop_clear(1)

  call prop_type_delete('virtual_text_prop', #{bufnr: bufnr()})
  only!
  enew!
  set ff& wrap& rightmargin& number&
endfunction

" test virtual text above with 'nowrap':
function Test_rightmargin_textprop_above_nowrap_cursor()
  enew!
  set ff=unix

  let visible_width = 30
  call NewWindow(20, visible_width)

  call setline(1, repeat('a', 10))

  set nowrap rightmargin=10

  call prop_type_add('virtual_text_prop', #{highlight: "ErrorMsg", bufnr: bufnr()})
  call prop_add(1, 0, #{
    \type: 'virtual_text_prop',
    \text: 'some long virtual text',
    \text_align: 'above',
  \})

  " virtual text should be correctly displayed above:
  let expected_lines = [
    \'some long virtual te          ',
    \'aaaaaaaaaa                    ',
    \'~                             ',
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)

  call prop_clear(1)
  call prop_type_delete('virtual_text_prop', #{bufnr: bufnr()})
  only!
  enew!
  set ff& nowrap& rightmargin&
endfunction

function Test_rightmargin_textprop_above_smoothscroll_offscreen()
  CheckScreendump
  CheckRunVimInTerminal

  let script_lines =<< trim END
    call setline(1, repeat('x', 40))
    set wrap rightmargin=5 smoothscroll scrolloff=0 number
    call prop_type_add('virtual_text_prop', #{highlight: "ErrorMsg", bufnr: bufnr()})
    call prop_add(1, 0, #{
      \type: 'virtual_text_prop',
      \text: 'virtual text',
      \text_align: 'above',
    \})
    call winrestview({'topline': 1, 'lnum': 1, 'col': 1, 'skipcol': 11})
  END
  call writefile(script_lines, 'XTest_rightmargin_textprop_above_smoothscroll_offscreen', 'D')
  let buf = RunVimInTerminal('-S XTest_rightmargin_textprop_above_smoothscroll_offscreen', {'rows': 10, 'cols': 20})
  call VerifyScreenDump(buf, 'Test_rightmargin_textprop_above_smoothscroll_offscreen', {})
  call StopVimInTerminal(buf)
endfunction

" test virtual text above with no room for text:
function Test_rightmargin_textprop_above_no_text_room()
  enew!
  set ff=unix

  let visible_width = 10
  call NewWindow(5, visible_width)

  call setline(1, 'x')

  set wrap smoothscroll scrolloff=0 number
  let &rightmargin = visible_width

  call prop_type_add('virtual_text_prop', #{highlight: "ErrorMsg", bufnr: bufnr()})
  call prop_add(1, 0, #{
    \type: 'virtual_text_prop',
    \text: 'virtual text',
    \text_align: 'above',
  \})

  call winrestview({'topline': 1, 'lnum': 1, 'col': 1, 'skipcol': 1})
  let expected_lines = [
    \'          ',
    \'          ',
    \'          ',
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)

  call prop_clear(1)
  call prop_type_delete('virtual_text_prop', #{bufnr: bufnr()})
  only!
  enew!
  set ff& wrap& rightmargin& smoothscroll& scrolloff& number&
endfunction

" test virtual text below with small inner width:
function Test_rightmargin_textprop_below_small_inner_width()
  enew!
  set ff=unix

  let visible_width = 10
  call NewWindow(5, visible_width)

  call setline(1, 'x')

  set wrap rightmargin=8

  call prop_type_add('virtual_text_prop', #{highlight: "ErrorMsg", bufnr: bufnr()})
  call prop_add(1, 0, #{
    \type: 'virtual_text_prop',
    \text: 'virtual text',
    \text_align: 'below',
  \})

  let expected_lines = [
    \'xv        ',
    \'ir        ',
    \'tu        ',
    \'al        ',
    \' t        '
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)

  call prop_clear(1)
  call prop_type_delete('virtual_text_prop', #{bufnr: bufnr()})
  only!
  enew!
  set ff& wrap& rightmargin&
endfunction

" test virtual text below with 'nowrap':
function Test_rightmargin_textprop_nowrap_flush()
  enew!
  set ff=unix

  let visible_width = 30
  call NewWindow(20, visible_width)

  set nowrap rightmargin=10

  " line long enough to exceed inner width:
  call setline(1, repeat('x', visible_width))

  call prop_type_add('virtual_text_prop', #{highlight: "ErrorMsg", bufnr: bufnr()})
  call prop_add(1, 0, #{
    \type: 'virtual_text_prop',
    \text: 'virtual text',
    \text_align: 'below',
  \})

  let expected_lines = [
    \'xxxxxxxxxxxxxxxxxxxx          ',
    \'virtual text                  ',
    \'~                             ',
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)

  call prop_clear(1)
  call prop_type_delete('virtual_text_prop', #{bufnr: bufnr()})
  only!
  enew!
  set ff& nowrap& rightmargin&
endfunction

" test virtual text right aligned with 'nowrap':
function Test_rightmargin_textprop_nowrap_right()
  enew!
  set ff=unix

  let visible_width = 30
  call NewWindow(20, visible_width)

  set nowrap rightmargin=10

  " create line long enough, so right aligned property will be off screen:
  call setline(1, repeat('x', visible_width))

  call prop_type_add('virtual_text_prop', #{highlight: "ErrorMsg", bufnr: bufnr()})
  call prop_add(1, 0, #{
    \type: 'virtual_text_prop',
    \text: 'virtual text',
    \text_align: 'right',
  \})

  " nothing else is attempted to be rendered on following lines, except '~':
  let expected_lines = [
    \'xxxxxxxxxxxxxxxxxxxx          ',
    \'~                             ',
    \'~                             ',
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)

  call prop_clear(1)
  call prop_type_delete('virtual_text_prop', #{bufnr: bufnr()})
  only!
  enew!
  set ff& nowrap& rightmargin&
endfunction

" test balloon feature:
function Test_rightmargin_popup_beval()
  CheckFeature balloon_eval_term
  CheckScreendump
  CheckRunVimInTerminal

  let script_lines =<< trim eval END
    function MyBalloonExpr()
      return "line " .. v:beval_lnum .. " column " .. v:beval_col .. ":\n" .. v:beval_text
    endfunction

    function MouseMove(lnum, col)
      call test_setmouse(a:lnum, a:col)
      call feedkeys("\<MouseMove>\<Ignore>", "xt")
    endfunction

    set balloonevalterm balloonexpr=MyBalloonExpr() balloondelay=100 updatetime=300 mouse=a
    set list listchars=extends:> nowrap

    call setline(1, {s:lines})
  END
  call writefile(script_lines, 'XTest_rightmargin_beval', 'D')

  let buf = RunVimInTerminal('-S XTest_rightmargin_beval', {'rows': 20, 'cols': 50})

  " balloon at first visible column:
  call term_sendkeys(buf, ":call MouseMove(3, 1)\<CR>")
  sleep 150m
  call VerifyScreenDump(buf, 'Test_rightmargin_beval_1', {})

  " balloon at last visible column:
  call term_sendkeys(buf, ":call MouseMove(3, 49)\<CR>")
  sleep 150m
  call VerifyScreenDump(buf, 'Test_rightmargin_beval_2', {})

  call term_sendkeys(buf, ":set rightmargin=5\<CR>")

  " balloon at first visible column:
  call term_sendkeys(buf, ":call MouseMove(3, 1)\<CR>")
  sleep 150m
  call VerifyScreenDump(buf, 'Test_rightmargin_beval_3', {})

  " no balloon in rightmargin area:
  call term_sendkeys(buf, ":call MouseMove(3, 49)\<CR>")
  sleep 150m
  call VerifyScreenDump(buf, 'Test_rightmargin_beval_4', {})

  " clean up
  call StopVimInTerminal(buf)
endfunction

" test foldtext and fillchars with rightmargin and rightleft:
function Test_rightmargin_foldtext_and_fillchars()
  CheckFeature rightleft
  CheckScreendump
  CheckRunVimInTerminal

  let script_lines =<< trim eval END
    let g:lines           = {s:lines[0 : 3]}            ->map({{_, v -> strcharpart(v, 0, 30)}})
    let g:multibute_lines = {s:double_cell_lines[0 : 3]}->map({{_, v -> strcharpart(v, 0, 20)}})

    call setline(1, g:lines)
    1,3fold

    setlocal rightmargin=5 nowrap
    set noshowmode noshowcmd
  END
  call writefile(script_lines, 'XTest_rightmargin_foldtext_and_fillchars', 'D')
  let buf = RunVimInTerminal('-S XTest_rightmargin_foldtext_and_fillchars', {'rows': 10, 'cols': 70})

  call VerifyScreenDump(buf, 'Test_rightmargin_foldtext_and_fillchars_1', {})

  " enable rightleft:
  call term_sendkeys(buf, ":set rightleft\<CR>")
  call VerifyScreenDump(buf, 'Test_rightmargin_foldtext_and_fillchars_2', {})

  call term_sendkeys(buf, ":set norightleft\<CR>")
  call term_sendkeys(buf, ":call setline(1, g:multibute_lines)\<CR>")
  call VerifyScreenDump(buf, 'Test_rightmargin_foldtext_and_fillchars_3', {})

  " enable rightleft:
  call term_sendkeys(buf, ":set rightleft\<CR>")
  call VerifyScreenDump(buf, 'Test_rightmargin_foldtext_and_fillchars_4', {})

  " clean up
  call StopVimInTerminal(buf)
endfunction

" test diff feature:
function Test_rightmargin_diff()
  CheckScreendump
  CheckRunVimInTerminal

  let script_lines =<< trim eval END
    vnew

    let win1_lines = {s:lines}->copy()
    call remove(win1_lines, 2)
    call setbufline(winbufnr(1), 1, win1_lines)

    let win2_lines = {s:lines}->copy() + ['']
    let win2_lines[2] = 'foo bar'
    call setbufline(winbufnr(2), 1, win2_lines)

    windo setlocal list listchars=extends:> nowrap diffopt+=context:0
    windo diffthis
    wincmd =
  END
  call writefile(script_lines, 'XTest_rightmargin_diff', 'D')

  let buf = RunVimInTerminal('-S XTest_rightmargin_diff', {'rows': 20, 'cols': 70})

  " no rightmargin:
  call VerifyScreenDump(buf, 'Test_rightmargin_diff_1', {})

  " set rightmargin:
  call term_sendkeys(buf, ":windo set rightmargin=5\<CR>")
  call VerifyScreenDump(buf, 'Test_rightmargin_diff_2', {})

  " no rightmargin, enable rightleft:
  call term_sendkeys(buf, ":windo set rightmargin=0 rightleft\<CR>")
  call VerifyScreenDump(buf, 'Test_rightmargin_diff_3', {})

  " set rightmargin, rightleft enabled:
  call term_sendkeys(buf, ":windo set rightmargin=5\<CR>")
  call VerifyScreenDump(buf, 'Test_rightmargin_diff_4', {})

  " clean up
  call StopVimInTerminal(buf)
endfunction

" test screenpos():
function Test_rightmargin_screenpos()
  enew!
  set ff=unix

  let visible_width = 30
  call NewWindow(20, visible_width)

  call setline(1, s:lines)

  set rightmargin=15

  let lnum = 1
  let non_visible_result = #{col: 0, row: 0, endcol: 0, curscol: 0}

  " first test with wrap disabled:
  set nowrap

  let col = 10 " visible
  let expected_col = col
  let expected_lnum = lnum
  let pos = screenpos(win_getid(), lnum, col)
  call assert_equal(#{col: expected_col, row: expected_lnum, endcol: expected_col, curscol: expected_col}, pos)

  let col = 20 " not visible
  let pos = screenpos(win_getid(), lnum, col)
  call assert_equal(non_visible_result, pos)

  let col = 35 " also not visible
  let pos = screenpos(win_getid(), lnum, col)
  call assert_equal(non_visible_result, pos)

  " same tests with wrap enabled:
  set wrap

  let col = 10 " does not wrap yet
  let expected_col = col
  let expected_lnum = lnum
  let pos = screenpos(win_getid(), lnum, col)
  call assert_equal(#{col: expected_col, row: expected_lnum, endcol: expected_col, curscol: expected_col}, pos)

  let col = 20 " wraps to second line
  let expected_col = (col - 1) % (visible_width - &rightmargin) + 1
  let expected_lnum = lnum + 1
  let pos = screenpos(win_getid(), lnum, col)
  call assert_equal(#{col: expected_col, row: expected_lnum, endcol: expected_col, curscol: expected_col}, pos)

  let col = 35 " wraps to third line
  let expected_col = (col - 1) % (visible_width - &rightmargin) + 1
  let expected_lnum = lnum + 2
  let pos = screenpos(win_getid(), lnum, col)
  call assert_equal(#{col: expected_col, row: expected_lnum, endcol: expected_col, curscol: expected_col}, pos)

  set smoothscroll scrolloff=0
  " scroll down 1 screen line:
  exe "normal! \<C-e>"
  let text_width = visible_width - &rightmargin
  call assert_equal(text_width, winsaveview().skipcol)
  " characters that wrap to second screen line should be reported as being of first screen line:
  let pos = screenpos(win_getid(), 1, visible_width - &rightmargin + 1)
  call assert_equal(#{col: 1, row: 1, endcol: 1, curscol: 1}, pos)

  only!
  enew!
  set ff& wrap& rightmargin& smoothscroll& scrolloff&
endfunction

" test horizontal page scroll:
function Test_rightmargin_mouse_horizontal_scroll()
  enew!
  set ff=unix

  let visible_width = 40
  call NewWindow(20, visible_width)

  call setline(1, s:lines)

  set nowrap rightmargin=10 mouse=a

  " reset to initial state:
  norm! 0gg

  " scroll one page right:
  call feedkeys("\<S-ScrollWheelRight>\<Ignore>", "xt")
  let leftcol = getwininfo(win_getid())[0].leftcol
  let expected_leftcol = visible_width - &rightmargin
  call assert_equal(expected_leftcol, leftcol)

  only!
  enew!
  set ff& nowrap& rightmargin& mouse&
endfunction

" test virtual text with double-cell character:
function Test_rightmargin_textprop_extra_double_cell()
  enew!
  set ff=unix

  let visible_width = 20
  call NewWindow(10, visible_width)

  " line long enough, so double-cell character does not fit fully:
  call setline(1, 'abcdefghijklmnopq')

  set rightmargin=2 wrap

  call prop_type_add('virtual_text_prop', #{highlight: "ErrorMsg", bufnr: bufnr()})
  call prop_add(1, 0, #{
    \type: 'virtual_text_prop',
    \text: '木',
    \text_align: 'after',
  \})

  let expected_lines = [
    \'abcdefghijklmnopq>  ',
    \'木                  ',
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)

  if has('rightleft')
    set rightleft
    let expected_lines = [
      \'  >qponmlkjihgfedcba',
      \'                  木',
    \]
    let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
    call assert_equal(expected_lines, actual_lines)
    set norightleft
  endif

  call prop_clear(1)
  call prop_type_delete('virtual_text_prop', #{bufnr: bufnr()})

  only!
  enew!
  set ff& rightmargin& wrap&
endfunction

" test tab wrapping with 'linebreak':
function Test_rightmargin_linebreak_tab()
  enew!
  set ff=unix

  let visible_width = 30
  call NewWindow(10, visible_width)

  set linebreak rightmargin=10 wrap showbreak=+++ list listchars=tab:<->

  " create a line where a tab character extends to rightmargin area:
  call setline(1, '1234567890123456789' .. "\t" .. 'pqrstuvwxyz')

  " check that the line with tab character are properly wrapped:
  let expected_lines = [
    \'1234567890123456789<          ',
    \'+++--->pqrstuvwxyz            ',
    \'~                             ',
    \'~                             '
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)

  only!
  enew!
  set ff& linebreak& rightmargin& wrap& showbreak&
endfunction

" test line wrapping with 'linebreak':
function Test_rightmargin_linebreak_wrap()
  enew!
  set ff=unix

  let visible_width = 30
  call NewWindow(10, visible_width)

  set linebreak rightmargin=10 wrap

  " second word extends to rightmargin area and will move to second line:
  call setline(1, '123456789012345 abcde')

  let expected_lines = [
    \'123456789012345               ',
    \'abcde                         ',
    \'~                             ',
    \'~                             ',
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)

  only!
  enew!
  set ff& linebreak& rightmargin& wrap&
endfunction

" test hlsearch at end of line does not leak to rightmargin area
" and instead last visible character is highlighted:
function Test_rightmargin_hlsearch_eol()
  CheckScreendump
  CheckRunVimInTerminal

  let script_lines =<< trim eval END
    call setline(1, '123456789012345')
    set rightmargin=5 nowrap hlsearch
    let @/ = '$'
  END

  call writefile(script_lines, 'XTest_rightmargin_hlsearch_eol', 'D')
  let buf = RunVimInTerminal('-S XTest_rightmargin_hlsearch_eol', {'rows': 5, 'cols': 20})

  call VerifyScreenDump(buf, 'Test_rightmargin_hlsearch_eol_1', {})

  if has('rightleft')
    call term_sendkeys(buf, ":set rightleft\<CR>")
    call VerifyScreenDump(buf, 'Test_rightmargin_hlsearch_eol_2', {})
  endif

  call StopVimInTerminal(buf)
endfunction

" test visual highlighting past end of line does not leak to rightmargin area
" with 'rightleft' enabled:
function Test_rightmargin_visual_eol_rl()
  CheckFeature rightleft
  CheckScreendump
  CheckRunVimInTerminal

  let script_lines =<< trim eval END
    call setline(1, '123456789012345')
    set rightmargin=5 nowrap rightleft
    set noshowmode noshowcmd
    norm! V
  END

  call writefile(script_lines, 'XTest_rightmargin_visual_eol_rl', 'D')
  let buf = RunVimInTerminal('-S XTest_rightmargin_visual_eol_rl', {'rows': 5, 'cols': 20})

  call VerifyScreenDump(buf, 'Test_rightmargin_visual_eol_rl', {})
  call StopVimInTerminal(buf)
endfunction

" test virtualedit visual past end of line does not leak to rightmargin area
" with 'rightleft' enabled:
function Test_rightmargin_visual_virtualedit_rl()
  CheckFeature rightleft
  CheckScreendump
  CheckRunVimInTerminal

  let script_lines =<< trim eval END
    call setline(1, 'abc')
    set rightmargin=10 virtualedit=all rightleft
    set noshowmode noshowcmd
    norm! 1G2lv25l
  END

  call writefile(script_lines, 'XTest_rightmargin_visual_virtualedit_rl', 'D')
  let buf = RunVimInTerminal('-S XTest_rightmargin_visual_virtualedit_rl', {'rows': 10, 'cols': 30})

  call VerifyScreenDump(buf, 'Test_rightmargin_visual_virtualedit_rl', {})

  call StopVimInTerminal(buf)
endfunction

" test that foldtext with double-cell character does not leak to rightmargin area:
function Test_rightmargin_foldtext_double_cell()
  enew!
  set ff=unix

  let visible_width = 20
  call NewWindow(10, visible_width)

  " line long enough, so that last double-cell character does not fit fully:
  call setline(1, '123木')
  call setline(2, 'x')
  1,2fold

  set rightmargin=2 nowrap fillchars=fold:-

  let expected_lines = [
    \'+--  2 lines: 123-  ',
    \'~                   ',
  \]
  let actual_lines = ScreenLines([1, expected_lines->len()], visible_width)
  call assert_equal(expected_lines, actual_lines)

  only!
  enew!
  set ff& rightmargin& nowrap& fillchars&
endfunction

function Test_rightmargin_split()
  setlocal rightmargin=7

  split
  call assert_equal(7, &rightmargin)
  close
  call assert_equal(7, &rightmargin)

  setlocal rightmargin&
endfunction

function Test_rightmargin_negative()
  set rightmargin=5

  call assert_fails('set rightmargin=-1', 'E487:')
  call assert_equal(0, &rightmargin)

  call assert_fails('setglobal rightmargin=-1', 'E487:')
  call assert_equal(0, &g:rightmargin)

  set rightmargin&
endfunction

" test that 'colorcolumn' and 'cursorcolumn' rendered properly:
function Test_rightmargin_colorcolumn_cursorcolumn()
  CheckFeature syntax
  CheckScreendump
  CheckRunVimInTerminal

  let script_lines =<< trim eval END
    colorscheme koehler
    set rightmargin=5 colorcolumn=20 cursorcolumn nowrap
    highlight ColorColumn  ctermbg=Red
    highlight CursorColumn ctermbg=Green
    highlight Cursor       ctermbg=Blue

    call setline(1, {s:lines})
    norm! 3G10|

    set noshowmode noshowcmd
  END

  call writefile(script_lines, 'XTest_rightmargin_colorcolumn_cursorcolumn', 'D')
  let buf = RunVimInTerminal('-S XTest_rightmargin_colorcolumn_cursorcolumn', {'rows': 6, 'cols': 30})

  " both colorcolumn and cursorcolumn visible:
  call VerifyScreenDump(buf, 'Test_rightmargin_colorcolumn_cursorcolumn_1', {})

  " increase rightmargin to make colorcolumn off screen:
  call term_sendkeys(buf, ":set rightmargin=15\<CR>")
  call VerifyScreenDump(buf, 'Test_rightmargin_colorcolumn_cursorcolumn_2', {})

  " increase further to leave no room for text:
  call term_sendkeys(buf, ":set rightmargin=9999\<CR>")
  call VerifyScreenDump(buf, 'Test_rightmargin_colorcolumn_cursorcolumn_3', {})

  " clean up
  call StopVimInTerminal(buf)
endfunction

" vim: shiftwidth=2 sts=2 expandtab
