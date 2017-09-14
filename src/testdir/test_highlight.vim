" Tests for ":highlight" and highlighting.

source view_util.vim

func Test_highlight()
  " basic test if ":highlight" doesn't crash
  highlight
  hi Search

  " test setting colors.
  " test clearing one color and all doesn't generate error or warning
  silent! hi NewGroup term=bold cterm=italic ctermfg=DarkBlue ctermbg=Grey gui= guifg=#00ff00 guibg=Cyan
  silent! hi Group2 term= cterm=
  hi Group3 term=underline cterm=bold

  let res = split(execute("hi NewGroup"), "\n")[0]
  " filter ctermfg and ctermbg, the numbers depend on the terminal
  let res = substitute(res, 'ctermfg=\d*', 'ctermfg=2', '')
  let res = substitute(res, 'ctermbg=\d*', 'ctermbg=3', '')
  call assert_equal("NewGroup       xxx term=bold cterm=italic ctermfg=2 ctermbg=3",
				\ res)
  call assert_equal("Group2         xxx cleared",
				\ split(execute("hi Group2"), "\n")[0])
  call assert_equal("Group3         xxx term=underline cterm=bold",
				\ split(execute("hi Group3"), "\n")[0])

  hi clear NewGroup
  call assert_equal("NewGroup       xxx cleared",
				\ split(execute("hi NewGroup"), "\n")[0])
  call assert_equal("Group2         xxx cleared",
				\ split(execute("hi Group2"), "\n")[0])
  hi Group2 NONE
  call assert_equal("Group2         xxx cleared",
				\ split(execute("hi Group2"), "\n")[0])
  hi clear
  call assert_equal("Group3         xxx cleared",
				\ split(execute("hi Group3"), "\n")[0])
  call assert_fails("hi Crash term='asdf", "E475:")
endfunc

function! HighlightArgs(name)
  return 'hi ' . substitute(split(execute('hi ' . a:name), '\n')[0], '\<xxx\>', '', '')
endfunction

function! HiCursorLine()
  let hiCursorLine = HighlightArgs('CursorLine')
  if has('gui_running')
    let hi_ul = 'hi CursorLine gui=underline guibg=NONE'
    let hi_bg = hiCursorLine
  else
    let hi_ul = hiCursorLine
    let hi_bg = 'hi CursorLine cterm=NONE ctermbg=Gray'
  endif
  return [hiCursorLine, hi_ul, hi_bg]
endfunction

func Test_highlight_eol_with_cursorline()
  let [hiCursorLine, hi_ul, hi_bg] = HiCursorLine()

  call NewWindow('topleft 5', 20)
  call setline(1, 'abcd')
  call matchadd('Search', '\n')

  let expected = "abcd      "
  let actual = ScreenLines(1, 10)[0]
  call assert_equal(expected, actual)

  " expected:
  " 'abcd      '
  "  ^^^^ ^^^^^   no highlight
  "      ^        'Search' highlight
  let attrs0 = ScreenAttrs(1, 10)[0]
  call assert_equal(repeat([attrs0[0]], 4), attrs0[0:3])
  call assert_equal(repeat([attrs0[0]], 5), attrs0[5:9])
  call assert_notequal(attrs0[0], attrs0[4])

  setlocal cursorline

  " underline
  exe hi_ul
  redraw!

  let actual = ScreenLines(1, 10)[0]
  call assert_equal(expected, actual)
  let attrs = ScreenAttrs(1, 10)[0]

  " expected:
  " 'abcd      '
  "  ^^^^         underline
  "      ^^^^^^   'Search' highlight with underline
  call assert_equal(repeat([attrs[0]], 4), attrs[0:3])
  call assert_equal(repeat([attrs[4]], 6), attrs[4:9])
  call assert_notequal(attrs[0], attrs[4])
  call assert_notequal(attrs0[0], attrs[0])
  call assert_notequal(attrs0[4], attrs[4])

  " bg-color
  exe hi_bg
  redraw!

  let actual = ScreenLines(1, 10)[0]
  call assert_equal(expected, actual)
  let attrs = ScreenAttrs(1, 10)[0]

  " expected:
  " 'abcd      '
  "  ^^^^         bg-color of 'CursorLine'
  "      ^        'Search' highlight
  "       ^^^^^   bg-color of 'CursorLine'
  call assert_equal(repeat([attrs[0]], 4), attrs[0:3])
  call assert_equal(repeat([attrs[5]], 5), attrs[5:9])
  call assert_equal(attrs0[4], attrs[4])
  call assert_notequal(attrs[0], attrs[4])
  call assert_notequal(attrs[4], attrs[5])
  call assert_notequal(attrs0[0], attrs[0])
  call assert_notequal(attrs0[5], attrs[5])

  call CloseWindow()
  exe hiCursorLine
endfunc

func Test_highlight_eol_with_cursorline_vertsplit()
  if !has('vertsplit')
    return
  endif

  let [hiCursorLine, hi_ul, hi_bg] = HiCursorLine()

  call NewWindow('topleft 5', 5)
  call setline(1, 'abcd')
  call matchadd('Search', '\n')

  let expected = "abcd |abcd     "
  let actual = ScreenLines(1, 15)[0]
  call assert_equal(expected, actual)

  " expected:
  " 'abcd |abcd     '
  "  ^^^^  ^^^^^^^^^   no highlight
  "      ^             'Search' highlight
  "       ^            'VertSplit' highlight
  let attrs0 = ScreenAttrs(1, 15)[0]
  call assert_equal(repeat([attrs0[0]], 4), attrs0[0:3])
  call assert_equal(repeat([attrs0[0]], 9), attrs0[6:14])
  call assert_notequal(attrs0[0], attrs0[4])
  call assert_notequal(attrs0[0], attrs0[5])
  call assert_notequal(attrs0[4], attrs0[5])

  setlocal cursorline

  " expected:
  " 'abcd |abcd     '
  "  ^^^^              underline
  "      ^             'Search' highlight with underline
  "       ^            'VertSplit' highlight
  "        ^^^^^^^^^   no highlight

  " underline
  exe hi_ul
  redraw!

  let actual = ScreenLines(1, 15)[0]
  call assert_equal(expected, actual)

  let attrs = ScreenAttrs(1, 15)[0]
  call assert_equal(repeat([attrs[0]], 4), attrs[0:3])
  call assert_equal(repeat([attrs[6]], 9), attrs[6:14])
  call assert_equal(attrs0[5:14], attrs[5:14])
  call assert_notequal(attrs[0], attrs[4])
  call assert_notequal(attrs[0], attrs[5])
  call assert_notequal(attrs[0], attrs[6])
  call assert_notequal(attrs[4], attrs[5])
  call assert_notequal(attrs[5], attrs[6])
  call assert_notequal(attrs0[0], attrs[0])
  call assert_notequal(attrs0[4], attrs[4])

  " bg-color
  exe hi_bg
  redraw!

  let actual = ScreenLines(1, 15)[0]
  call assert_equal(expected, actual)

  let attrs = ScreenAttrs(1, 15)[0]
  call assert_equal(repeat([attrs[0]], 4), attrs[0:3])
  call assert_equal(repeat([attrs[6]], 9), attrs[6:14])
  call assert_equal(attrs0[5:14], attrs[5:14])
  call assert_notequal(attrs[0], attrs[4])
  call assert_notequal(attrs[0], attrs[5])
  call assert_notequal(attrs[0], attrs[6])
  call assert_notequal(attrs[4], attrs[5])
  call assert_notequal(attrs[5], attrs[6])
  call assert_notequal(attrs0[0], attrs[0])
  call assert_equal(attrs0[4], attrs[4])

  call CloseWindow()
  exe hiCursorLine
endfunc
