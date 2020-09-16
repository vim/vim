" Tests for ":highlight" and highlighting.

source view_util.vim
source screendump.vim
source check.vim

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

func HighlightArgs(name)
  return 'hi ' . substitute(split(execute('hi ' . a:name), '\n')[0], '\<xxx\>', '', '')
endfunc

func IsColorable()
  return has('gui_running') || str2nr(&t_Co) >= 8
endfunc

func HiCursorLine()
  let hiCursorLine = HighlightArgs('CursorLine')
  if has('gui_running')
    let guibg = matchstr(hiCursorLine, 'guibg=\w\+')
    let hi_ul = 'hi CursorLine gui=underline guibg=NONE'
    let hi_bg = 'hi CursorLine gui=NONE ' . guibg
  else
    let hi_ul = 'hi CursorLine cterm=underline ctermbg=NONE'
    let hi_bg = 'hi CursorLine cterm=NONE ctermbg=Gray'
  endif
  return [hiCursorLine, hi_ul, hi_bg]
endfunc

func Check_lcs_eol_attrs(attrs, row, col)
  let save_lcs = &lcs
  set list

  call assert_equal(a:attrs, ScreenAttrs(a:row, a:col)[0])

  set nolist
  let &lcs = save_lcs
endfunc

func Test_highlight_eol_with_cursorline()
  let [hiCursorLine, hi_ul, hi_bg] = HiCursorLine()

  call NewWindow('topleft 5', 20)
  call setline(1, 'abcd')
  call matchadd('Search', '\n')

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

  " expected:
  " 'abcd      '
  "  ^^^^         underline
  "      ^        'Search' highlight with underline
  "       ^^^^^   underline
  let attrs = ScreenAttrs(1, 10)[0]
  call assert_equal(repeat([attrs[0]], 4), attrs[0:3])
  call assert_equal([attrs[4]] + repeat([attrs[5]], 5), attrs[4:9])
  call assert_notequal(attrs[0], attrs[4])
  call assert_notequal(attrs[4], attrs[5])
  call assert_notequal(attrs0[0], attrs[0])
  call assert_notequal(attrs0[4], attrs[4])
  call Check_lcs_eol_attrs(attrs, 1, 10)

  if IsColorable()
    " bg-color
    exe hi_bg

    " expected:
    " 'abcd      '
    "  ^^^^         bg-color of 'CursorLine'
    "      ^        'Search' highlight
    "       ^^^^^   bg-color of 'CursorLine'
    let attrs = ScreenAttrs(1, 10)[0]
    call assert_equal(repeat([attrs[0]], 4), attrs[0:3])
    call assert_equal(repeat([attrs[5]], 5), attrs[5:9])
    call assert_equal(attrs0[4], attrs[4])
    call assert_notequal(attrs[0], attrs[4])
    call assert_notequal(attrs[4], attrs[5])
    call assert_notequal(attrs0[0], attrs[0])
    call assert_notequal(attrs0[5], attrs[5])
    call Check_lcs_eol_attrs(attrs, 1, 10)
  endif

  call CloseWindow()
  exe hiCursorLine
endfunc

func Test_highlight_eol_with_cursorline_vertsplit()
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
  call Check_lcs_eol_attrs(attrs, 1, 15)

  if IsColorable()
    " bg-color
    exe hi_bg

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
    call Check_lcs_eol_attrs(attrs, 1, 15)
  endif

  call CloseWindow()
  exe hiCursorLine
endfunc

func Test_highlight_eol_with_cursorline_rightleft()
  CheckFeature rightleft

  let [hiCursorLine, hi_ul, hi_bg] = HiCursorLine()

  call NewWindow('topleft 5', 10)
  setlocal rightleft
  call setline(1, 'abcd')
  call matchadd('Search', '\n')
  let attrs0 = ScreenAttrs(1, 10)[0]

  setlocal cursorline

  " underline
  exe hi_ul

  " expected:
  " '      dcba'
  "        ^^^^   underline
  "       ^       'Search' highlight with underline
  "  ^^^^^        underline
  let attrs = ScreenAttrs(1, 10)[0]
  call assert_equal(repeat([attrs[9]], 4), attrs[6:9])
  call assert_equal(repeat([attrs[4]], 5) + [attrs[5]], attrs[0:5])
  call assert_notequal(attrs[9], attrs[5])
  call assert_notequal(attrs[4], attrs[5])
  call assert_notequal(attrs0[9], attrs[9])
  call assert_notequal(attrs0[5], attrs[5])
  call Check_lcs_eol_attrs(attrs, 1, 10)

  if IsColorable()
    " bg-color
    exe hi_bg

    " expected:
    " '      dcba'
    "        ^^^^   bg-color of 'CursorLine'
    "       ^       'Search' highlight
    "  ^^^^^        bg-color of 'CursorLine'
    let attrs = ScreenAttrs(1, 10)[0]
    call assert_equal(repeat([attrs[9]], 4), attrs[6:9])
    call assert_equal(repeat([attrs[4]], 5), attrs[0:4])
    call assert_equal(attrs0[5], attrs[5])
    call assert_notequal(attrs[9], attrs[5])
    call assert_notequal(attrs[5], attrs[4])
    call assert_notequal(attrs0[9], attrs[9])
    call assert_notequal(attrs0[4], attrs[4])
    call Check_lcs_eol_attrs(attrs, 1, 10)
  endif

  call CloseWindow()
  exe hiCursorLine
endfunc

func Test_highlight_eol_with_cursorline_linewrap()
  let [hiCursorLine, hi_ul, hi_bg] = HiCursorLine()

  call NewWindow('topleft 5', 10)
  call setline(1, [repeat('a', 51) . 'bcd', ''])
  call matchadd('Search', '\n')

  setlocal wrap
  normal! gg$
  let attrs0 = ScreenAttrs(5, 10)[0]
  setlocal cursorline

  " underline
  exe hi_ul

  " expected:
  " 'abcd      '
  "  ^^^^         underline
  "      ^        'Search' highlight with underline
  "       ^^^^^   underline
  let attrs = ScreenAttrs(5, 10)[0]
  call assert_equal(repeat([attrs[0]], 4), attrs[0:3])
  call assert_equal([attrs[4]] + repeat([attrs[5]], 5), attrs[4:9])
  call assert_notequal(attrs[0], attrs[4])
  call assert_notequal(attrs[4], attrs[5])
  call assert_notequal(attrs0[0], attrs[0])
  call assert_notequal(attrs0[4], attrs[4])
  call Check_lcs_eol_attrs(attrs, 5, 10)

  if IsColorable()
    " bg-color
    exe hi_bg

    " expected:
    " 'abcd      '
    "  ^^^^         bg-color of 'CursorLine'
    "      ^        'Search' highlight
    "       ^^^^^   bg-color of 'CursorLine'
    let attrs = ScreenAttrs(5, 10)[0]
    call assert_equal(repeat([attrs[0]], 4), attrs[0:3])
    call assert_equal(repeat([attrs[5]], 5), attrs[5:9])
    call assert_equal(attrs0[4], attrs[4])
    call assert_notequal(attrs[0], attrs[4])
    call assert_notequal(attrs[4], attrs[5])
    call assert_notequal(attrs0[0], attrs[0])
    call assert_notequal(attrs0[5], attrs[5])
    call Check_lcs_eol_attrs(attrs, 5, 10)
  endif

  setlocal nocursorline nowrap
  normal! gg$
  let attrs0 = ScreenAttrs(1, 10)[0]
  setlocal cursorline

  " underline
  exe hi_ul

  " expected:
  " 'aaabcd    '
  "  ^^^^^^       underline
  "        ^      'Search' highlight with underline
  "         ^^^   underline
  let attrs = ScreenAttrs(1, 10)[0]
  call assert_equal(repeat([attrs[0]], 6), attrs[0:5])
  call assert_equal([attrs[6]] + repeat([attrs[7]], 3), attrs[6:9])
  call assert_notequal(attrs[0], attrs[6])
  call assert_notequal(attrs[6], attrs[7])
  call assert_notequal(attrs0[0], attrs[0])
  call assert_notequal(attrs0[6], attrs[6])
  call Check_lcs_eol_attrs(attrs, 1, 10)

  if IsColorable()
    " bg-color
    exe hi_bg

    " expected:
    " 'aaabcd    '
    "  ^^^^^^       bg-color of 'CursorLine'
    "        ^      'Search' highlight
    "         ^^^   bg-color of 'CursorLine'
    let attrs = ScreenAttrs(1, 10)[0]
    call assert_equal(repeat([attrs[0]], 6), attrs[0:5])
    call assert_equal(repeat([attrs[7]], 3), attrs[7:9])
    call assert_equal(attrs0[6], attrs[6])
    call assert_notequal(attrs[0], attrs[6])
    call assert_notequal(attrs[6], attrs[7])
    call assert_notequal(attrs0[0], attrs[0])
    call assert_notequal(attrs0[7], attrs[7])
    call Check_lcs_eol_attrs(attrs, 1, 10)
  endif

  call CloseWindow()
  exe hiCursorLine
endfunc

func Test_highlight_eol_with_cursorline_sign()
  CheckFeature signs

  let [hiCursorLine, hi_ul, hi_bg] = HiCursorLine()

  call NewWindow('topleft 5', 10)
  call setline(1, 'abcd')
  call matchadd('Search', '\n')

  sign define Sign text=>>
  exe 'sign place 1 line=1 name=Sign buffer=' . bufnr('')
  let attrs0 = ScreenAttrs(1, 10)[0]
  setlocal cursorline

  " underline
  exe hi_ul

  " expected:
  " '>>abcd    '
  "  ^^           sign
  "    ^^^^       underline
  "        ^      'Search' highlight with underline
  "         ^^^   underline
  let attrs = ScreenAttrs(1, 10)[0]
  call assert_equal(repeat([attrs[2]], 4), attrs[2:5])
  call assert_equal([attrs[6]] + repeat([attrs[7]], 3), attrs[6:9])
  call assert_notequal(attrs[2], attrs[6])
  call assert_notequal(attrs[6], attrs[7])
  call assert_notequal(attrs0[2], attrs[2])
  call assert_notequal(attrs0[6], attrs[6])
  call Check_lcs_eol_attrs(attrs, 1, 10)

  if IsColorable()
    " bg-color
    exe hi_bg

    " expected:
    " '>>abcd    '
    "  ^^           sign
    "    ^^^^       bg-color of 'CursorLine'
    "        ^      'Search' highlight
    "         ^^^   bg-color of 'CursorLine'
    let attrs = ScreenAttrs(1, 10)[0]
    call assert_equal(repeat([attrs[2]], 4), attrs[2:5])
    call assert_equal(repeat([attrs[7]], 3), attrs[7:9])
    call assert_equal(attrs0[6], attrs[6])
    call assert_notequal(attrs[2], attrs[6])
    call assert_notequal(attrs[6], attrs[7])
    call assert_notequal(attrs0[2], attrs[2])
    call assert_notequal(attrs0[7], attrs[7])
    call Check_lcs_eol_attrs(attrs, 1, 10)
  endif

  sign unplace 1
  call CloseWindow()
  exe hiCursorLine
endfunc

func Test_highlight_eol_with_cursorline_breakindent()
  CheckFeature linebreak

  let [hiCursorLine, hi_ul, hi_bg] = HiCursorLine()

  call NewWindow('topleft 5', 10)
  set showbreak=xxx
  setlocal breakindent breakindentopt=min:0,shift:1 showbreak=>
  call setline(1, ' ' . repeat('a', 9) . 'bcd')
  call matchadd('Search', '\n')
  let attrs0 = ScreenAttrs(2, 10)[0]
  setlocal cursorline

  " underline
  exe hi_ul

  " expected:
  " '  >bcd    '
  "  ^^^          breakindent and showbreak
  "     ^^^       underline
  "        ^      'Search' highlight with underline
  "         ^^^   underline
  let attrs = ScreenAttrs(2, 10)[0]
  call assert_equal(repeat([attrs[0]], 2), attrs[0:1])
  call assert_equal(repeat([attrs[3]], 3), attrs[3:5])
  call assert_equal([attrs[6]] + repeat([attrs[7]], 3), attrs[6:9])
  call assert_equal(attrs0[0], attrs[0])
  call assert_notequal(attrs[0], attrs[2])
  call assert_notequal(attrs[2], attrs[3])
  call assert_notequal(attrs[3], attrs[6])
  call assert_notequal(attrs[6], attrs[7])
  call assert_notequal(attrs0[2], attrs[2])
  call assert_notequal(attrs0[3], attrs[3])
  call assert_notequal(attrs0[6], attrs[6])
  call Check_lcs_eol_attrs(attrs, 2, 10)

  if IsColorable()
    " bg-color
    exe hi_bg

    " expected:
    " '  >bcd    '
    "  ^^^          breakindent and showbreak
    "     ^^^       bg-color of 'CursorLine'
    "        ^      'Search' highlight
    "         ^^^   bg-color of 'CursorLine'
    let attrs = ScreenAttrs(2, 10)[0]
    call assert_equal(repeat([attrs[0]], 2), attrs[0:1])
    call assert_equal(repeat([attrs[3]], 3), attrs[3:5])
    call assert_equal(repeat([attrs[7]], 3), attrs[7:9])
    call assert_equal(attrs0[0], attrs[0])
    call assert_equal(attrs0[6], attrs[6])
    call assert_notequal(attrs[0], attrs[2])
    call assert_notequal(attrs[2], attrs[3])
    call assert_notequal(attrs[3], attrs[6])
    call assert_notequal(attrs[6], attrs[7])
    call assert_notequal(attrs0[2], attrs[2])
    call assert_notequal(attrs0[3], attrs[3])
    call assert_notequal(attrs0[7], attrs[7])
    call Check_lcs_eol_attrs(attrs, 2, 10)
  endif

  call CloseWindow()
  set showbreak=
  setlocal showbreak=
  exe hiCursorLine
endfunc

func Test_highlight_eol_on_diff()
  call setline(1, ['abcd', ''])
  call matchadd('Search', '\n')
  let attrs0 = ScreenAttrs(1, 10)[0]

  diffthis
  botright new
  diffthis

  " expected:
  " '  abcd    '
  "  ^^           sign
  "    ^^^^ ^^^   'DiffAdd' highlight
  "        ^      'Search' highlight
  let attrs = ScreenAttrs(1, 10)[0]
  call assert_equal(repeat([attrs[0]], 2), attrs[0:1])
  call assert_equal(repeat([attrs[2]], 4), attrs[2:5])
  call assert_equal(repeat([attrs[2]], 3), attrs[7:9])
  call assert_equal(attrs0[4], attrs[6])
  call assert_notequal(attrs[0], attrs[2])
  call assert_notequal(attrs[0], attrs[6])
  call assert_notequal(attrs[2], attrs[6])
  call Check_lcs_eol_attrs(attrs, 1, 10)

  bwipe!
  diffoff
endfunc

func Test_termguicolors()
  CheckOption termguicolors
  if has('vtp') && !has('vcon') && !has('gui_running')
    " Win32: 'guicolors' doesn't work without virtual console.
    call assert_fails('set termguicolors', 'E954:')
    return
  endif

  " Basic test that setting 'termguicolors' works with one color.
  set termguicolors
  redraw
  set t_Co=1
  redraw
  set t_Co=0
  redraw
endfunc

func Test_cursorline_after_yank()
  CheckScreendump

  call writefile([
	\ 'set cul rnu',
	\ 'call setline(1, ["","1","2","3",""])',
	\ ], 'Xtest_cursorline_yank')
  let buf = RunVimInTerminal('-S Xtest_cursorline_yank', {'rows': 8})
  call TermWait(buf)
  call term_sendkeys(buf, "Gy3k")
  call TermWait(buf)
  call term_sendkeys(buf, "jj")

  call VerifyScreenDump(buf, 'Test_cursorline_yank_01', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('Xtest_cursorline_yank')
endfunc

" test for issue #4862
func Test_put_before_cursorline()
  new
  only!
  call setline(1, 'A')
  redraw
  let std_attr = screenattr(1, 1)
  set cursorline
  redraw
  let cul_attr = screenattr(1, 1)
  normal yyP
  redraw
  " Line 1 has cursor so it should be highlighted with CursorLine.
  call assert_equal(cul_attr, screenattr(1, 1))
  " And CursorLine highlighting from the second line should be gone.
  call assert_equal(std_attr, screenattr(2, 1))
  set nocursorline
  bwipe!
endfunc

func Test_cursorline_with_visualmode()
  CheckScreendump

  call writefile([
	\ 'set cul',
	\ 'call setline(1, repeat(["abc"], 50))',
	\ ], 'Xtest_cursorline_with_visualmode')
  let buf = RunVimInTerminal('-S Xtest_cursorline_with_visualmode', {'rows': 12})
  call TermWait(buf)
  call term_sendkeys(buf, "V\<C-f>kkkjk")

  call VerifyScreenDump(buf, 'Test_cursorline_with_visualmode_01', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('Xtest_cursorline_with_visualmode')
endfunc

func Test_wincolor()
  CheckScreendump
  " make sure the width is enough for the test
  set columns=80

  let lines =<< trim END
	set cursorline cursorcolumn rnu
	call setline(1, ["","1111111111","22222222222","3 here 3","","the cat is out of the bag"])
	set wincolor=Pmenu
	hi CatLine guifg=green ctermfg=green
	hi Reverse gui=reverse cterm=reverse
	syn match CatLine /^the.*/
	call prop_type_add("foo", {"highlight": "Reverse", "combine": 1})
	call prop_add(6, 12, {"type": "foo", "end_col": 15})
	/here
  END
  call writefile(lines, 'Xtest_wincolor')
  let buf = RunVimInTerminal('-S Xtest_wincolor', {'rows': 8})
  call TermWait(buf)
  call term_sendkeys(buf, "2G5lvj")
  call TermWait(buf)

  call VerifyScreenDump(buf, 'Test_wincolor_01', {})

  " clean up
  call term_sendkeys(buf, "\<Esc>")
  call StopVimInTerminal(buf)
  call delete('Xtest_wincolor')
endfunc

func Test_wincolor_listchars()
  CheckScreendump
  CheckFeature conceal

  let lines =<< trim END
	call setline(1, ["one","\t\tsome random text enough long to show 'extends' and 'precedes' includingnbsps, preceding tabs and trailing spaces    ","three"])
	set wincolor=Todo
	set nowrap cole=1 cocu+=n
	set list lcs=eol:$,tab:>-,space:.,trail:_,extends:>,precedes:<,conceal:*,nbsp:#
	call matchadd('Conceal', 'text')
	normal 2G5zl
  END
  call writefile(lines, 'Xtest_wincolorlcs')
  let buf = RunVimInTerminal('-S Xtest_wincolorlcs', {'rows': 8})

  call VerifyScreenDump(buf, 'Test_wincolor_lcs', {})

  " clean up
  call term_sendkeys(buf, "\<Esc>")
  call StopVimInTerminal(buf)
  call delete('Xtest_wincolorlcs')
endfunc

func Test_colorcolumn()
  CheckScreendump

  " check that setting 'colorcolumn' when entering a buffer works
  let lines =<< trim END
	split
	edit X
	call setline(1, ["1111111111","22222222222","3333333333"])
	set nomodified
	set colorcolumn=3,9
	set number cursorline cursorlineopt=number
	wincmd w
	buf X
  END
  call writefile(lines, 'Xtest_colorcolumn')
  let buf = RunVimInTerminal('-S Xtest_colorcolumn', {'rows': 10})
  call term_sendkeys(buf, ":\<CR>")
  call TermWait(buf)
  call VerifyScreenDump(buf, 'Test_colorcolumn_1', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('Xtest_colorcolumn')
endfunc

func Test_colorcolumn_bri()
  CheckScreendump

  " check 'colorcolumn' when 'breakindent' is set
  let lines =<< trim END
	call setline(1, 'The quick brown fox jumped over the lazy dogs')
  END
  call writefile(lines, 'Xtest_colorcolumn_bri')
  let buf = RunVimInTerminal('-S Xtest_colorcolumn_bri', {'rows': 10,'columns': 40})
  call term_sendkeys(buf, ":set co=40 linebreak bri briopt=shift:2 cc=40,41,43\<CR>")
  call TermWait(buf)
  call VerifyScreenDump(buf, 'Test_colorcolumn_2', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('Xtest_colorcolumn_bri')
endfunc

func Test_colorcolumn_sbr()
  CheckScreendump

  " check 'colorcolumn' when 'showbreak' is set
  let lines =<< trim END
	call setline(1, 'The quick brown fox jumped over the lazy dogs')
  END
  call writefile(lines, 'Xtest_colorcolumn_srb')
  let buf = RunVimInTerminal('-S Xtest_colorcolumn_srb', {'rows': 10,'columns': 40})
  call term_sendkeys(buf, ":set co=40 showbreak=+++>\\  cc=40,41,43\<CR>")
  call TermWait(buf)
  call VerifyScreenDump(buf, 'Test_colorcolumn_3', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('Xtest_colorcolumn_srb')
endfunc

" This test must come before the Test_cursorline test, as it appears this
" defines the Normal highlighting group anyway.
func Test_1_highlight_Normalgroup_exists()
  let hlNormal = HighlightArgs('Normal')
  if !has('gui_running')
    call assert_match('hi Normal\s*clear', hlNormal)
  elseif has('gui_gtk2') || has('gui_gnome') || has('gui_gtk3')
    " expect is DEFAULT_FONT of gui_gtk_x11.c
    call assert_match('hi Normal\s*font=Monospace 10', hlNormal)
  elseif has('gui_motif') || has('gui_athena')
    " expect is DEFAULT_FONT of gui_x11.c
    call assert_match('hi Normal\s*font=7x13', hlNormal)
  elseif has('win32')
    " expect any font
    call assert_match('hi Normal\s*font=.*', hlNormal)
  endif
endfunc

" Do this test last, sometimes restoring the columns doesn't work
func Test_z_no_space_before_xxx()
  let l:org_columns = &columns
  set columns=17
  let l:hi_StatusLineTermNC = join(split(execute('hi StatusLineTermNC')))
  call assert_match('StatusLineTermNC xxx', l:hi_StatusLineTermNC)
  let &columns = l:org_columns
endfunc

" Test for :highlight command errors
func Test_highlight_cmd_errors()
  if has('gui_running')
    " This test doesn't fail in the MS-Windows console version.
    call assert_fails('hi Xcomment ctermbg=fg', 'E419:')
    call assert_fails('hi Xcomment ctermfg=bg', 'E420:')
    call assert_fails('hi Xcomment ctermfg=ul', 'E453:')
  endif

  " Try using a very long terminal code. Define a dummy terminal code for this
  " test.
  let &t_fo = "\<Esc>1;"
  let c = repeat("t_fo,", 100) . "t_fo"
  call assert_fails('exe "hi Xgroup1 start=" . c', 'E422:')
  let &t_fo = ""
endfunc

" Test for 'highlight' option
func Test_highlight_opt()
  let save_hl = &highlight
  call assert_fails('set highlight=j:b', 'E474:')
  set highlight=f\ r
  call assert_equal('f r', &highlight)
  set highlight=fb
  call assert_equal('fb', &highlight)
  set highlight=fi
  call assert_equal('fi', &highlight)
  set highlight=f-
  call assert_equal('f-', &highlight)
  set highlight=fr
  call assert_equal('fr', &highlight)
  set highlight=fs
  call assert_equal('fs', &highlight)
  set highlight=fu
  call assert_equal('fu', &highlight)
  set highlight=fc
  call assert_equal('fc', &highlight)
  set highlight=ft
  call assert_equal('ft', &highlight)
  call assert_fails('set highlight=fr:Search', 'E474:')
  set highlight=f:$#
  call assert_match('W18:', v:statusmsg)
  let &highlight = save_hl
endfunc

" Test for User group highlighting used in the statusline
func Test_highlight_User()
  CheckNotGui
  hi User1 ctermfg=12
  redraw!
  call assert_equal('12', synIDattr(synIDtrans(hlID('User1')), 'fg'))
  hi clear
endfunc

" Test for using RGB color values in a highlight group
func Test_highlight_RGB_color()
  CheckGui
  hi MySearch guifg=#110000 guibg=#001100 guisp=#000011
  call assert_equal('#110000', synIDattr(synIDtrans(hlID('MySearch')), 'fg#'))
  call assert_equal('#001100', synIDattr(synIDtrans(hlID('MySearch')), 'bg#'))
  call assert_equal('#000011', synIDattr(synIDtrans(hlID('MySearch')), 'sp#'))
  hi clear
endfunc

" Test for using default highlighting group
func Test_highlight_default()
  highlight MySearch ctermfg=7
  highlight default MySearch ctermfg=5
  let hlSearch = HighlightArgs('MySearch')
  call assert_match('ctermfg=7', hlSearch)

  highlight default QFName ctermfg=3
  call assert_match('ctermfg=3', HighlightArgs('QFName'))
  hi clear
endfunc

" Test for 'ctermul in a highlight group
func Test_highlight_ctermul()
  CheckNotGui
  call assert_notmatch('ctermul=', HighlightArgs('Normal'))
  highlight Normal ctermul=3
  call assert_match('ctermul=3', HighlightArgs('Normal'))
  highlight Normal ctermul=NONE
endfunc

" Test for specifying 'start' and 'stop' in a highlight group
func Test_highlight_start_stop()
  hi HlGrp1 start=<Esc>[27h;<Esc>[<Space>r;
  call assert_match("start=^[[27h;^[[ r;", HighlightArgs('HlGrp1'))
  hi HlGrp1 start=NONE
  call assert_notmatch("start=", HighlightArgs('HlGrp1'))
  hi HlGrp2 stop=<Esc>[27h;<Esc>[<Space>r;
  call assert_match("stop=^[[27h;^[[ r;", HighlightArgs('HlGrp2'))
  hi HlGrp2 stop=NONE
  call assert_notmatch("stop=", HighlightArgs('HlGrp2'))
  hi clear
endfunc

" Test for setting various 'term' attributes
func Test_highlight_term_attr()
  hi HlGrp3 term=bold,underline,undercurl,strikethrough,reverse,italic,standout
  call assert_equal('hi HlGrp3          term=bold,standout,underline,undercurl,italic,reverse,strikethrough', HighlightArgs('HlGrp3'))
  hi HlGrp3 term=NONE
  call assert_equal('hi HlGrp3          cleared', HighlightArgs('HlGrp3'))
  hi clear
endfunc

" Test default highlighting is restored
func Test_highlight_restore_defaults()
  hi! link TestLink Identifier
  hi! TestHi ctermbg=red

  let hlTestLinkPre = HighlightArgs('TestLink')
  let hlTestHiPre = HighlightArgs('TestHi')

  " Test colorscheme
  hi clear
  if exists('syntax_on')
    syntax reset
  endif
  let g:colors_name = 'test'
  hi! link TestLink ErrorMsg
  hi! TestHi ctermbg=green

  " Restore default highlighting
  colorscheme default
  syntax on
  " 'default' should work no matter if highlight group was cleared
  hi def link TestLink Identifier
  hi def TestHi ctermbg=red

  let hlTestLinkPost = HighlightArgs('TestLink')
  let hlTestHiPost = HighlightArgs('TestHi')

  call assert_equal(hlTestLinkPre, hlTestLinkPost)
  call assert_equal(hlTestHiPre, hlTestHiPost)
  hi clear
endfunc

" vim: shiftwidth=2 sts=2 expandtab
