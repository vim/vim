" Tests specifically for the GUI

source shared.vim
if !CanRunGui()
  finish
endif

source setup_gui.vim

func Setup()
  call GUISetUpCommon()
endfunc

func TearDown()
  call GUITearDownCommon()
endfunc

" Test for resetting "secure" flag after GUI has started.
" Must be run first, since it starts the GUI on Unix.
func Test_1_set_secure()
  set exrc secure
  gui -f
  call assert_equal(1, has('gui_running'))
endfunc

" As for non-GUI, a balloon_show() test was already added with patch 8.0.0401
func Test_balloon_show()
  if has('balloon_eval')
    " This won't do anything but must not crash either.
    call balloon_show('hi!')
  endif
endfunc

func Test_colorscheme()
  let colorscheme_saved = exists('g:colors_name') ? g:colors_name : 'default'
  let g:color_count = 0
  augroup TestColors
    au!
    au ColorScheme * let g:color_count += 1| let g:after_colors = g:color_count
    au ColorSchemePre * let g:color_count += 1 |let g:before_colors = g:color_count
  augroup END

  colorscheme torte
  redraw!
  call assert_equal('dark', &background)
  call assert_equal(1, g:before_colors)
  call assert_equal(2, g:after_colors)
  call assert_equal("\ntorte", execute('colorscheme'))

  let a = substitute(execute('hi Search'), "\n\\s\\+", ' ', 'g')
  call assert_match("\nSearch         xxx term=reverse ctermfg=0 ctermbg=12 gui=bold guifg=Black guibg=Red", a)

  call assert_fails('colorscheme does_not_exist', 'E185:')

  exec 'colorscheme' colorscheme_saved
  augroup TestColors
    au!
  augroup END
  unlet g:color_count g:after_colors g:before_colors
  redraw!
endfunc

func Test_getfontname_with_arg()
  let skipped = ''

  if !g:x11_based_gui
    let skipped = g:not_implemented
  elseif has('gui_athena') || has('gui_motif')
    " Invalid font name. The result should be an empty string.
    call assert_equal('', getfontname('notexist'))

    " Valid font name. This is usually the real name of 7x13 by default.
    let fname = '-Misc-Fixed-Medium-R-Normal--13-120-75-75-C-70-ISO8859-1'
    call assert_match(fname, getfontname(fname))

  elseif has('gui_gtk2') || has('gui_gnome') || has('gui_gtk3')
    " Invalid font name. The result should be the name plus the default size.
    call assert_equal('notexist 10', getfontname('notexist'))

    " Valid font name. This is usually the real name of Monospace by default.
    let fname = 'Bitstream Vera Sans Mono 12'
    call assert_equal(fname, getfontname(fname))
  endif

  if !empty(skipped)
    throw skipped
  endif
endfunc

func Test_getfontname_without_arg()
  let skipped = ''

  let fname = getfontname()

  if !g:x11_based_gui
    let skipped = g:not_implemented
  elseif has('gui_kde')
    " 'expected' is the value specified by SetUp() above.
    call assert_equal('Courier 10 Pitch/8/-1/5/50/0/0/0/0/0', fname)
  elseif has('gui_athena') || has('gui_motif')
    " 'expected' is DFLT_FONT of gui_x11.c or its real name.
    let pat = '\(7x13\)\|\(\c-Misc-Fixed-Medium-R-Normal--13-120-75-75-C-70-ISO8859-1\)'
    call assert_match(pat, fname)
  elseif has('gui_gtk2') || has('gui_gnome') || has('gui_gtk3')
    " 'expected' is DEFAULT_FONT of gui_gtk_x11.c.
    call assert_equal('Monospace 10', fname)
  endif

  if !empty(skipped)
    throw skipped
  endif
endfunc

func Test_getwinpos()
  call assert_match('Window position: X \d\+, Y \d\+', execute('winpos'))
  call assert_true(getwinposx() >= 0)
  call assert_true(getwinposy() >= 0)
  call assert_equal([getwinposx(), getwinposy()], getwinpos())
endfunc

func Test_quoteplus()
  let skipped = ''

  if !g:x11_based_gui
    let skipped = g:not_supported . 'quoteplus'
  else
    let quoteplus_saved = @+

    let test_call     = 'Can you hear me?'
    let test_response = 'Yes, I can.'
    let vim_exe = exepath(v:progpath)
    let testee = 'VIMRUNTIME=' . $VIMRUNTIME . '; export VIMRUNTIME;'
          \ . vim_exe
	  \ . ' -u NONE -U NONE --noplugin --not-a-term -c ''%s'''
    " Ignore the "failed to create input context" error.
    let cmd = 'call test_ignore_error("E285") | '
	  \ . 'gui -f | '
	  \ . 'call feedkeys("'
          \ . '\"+p'
          \ . ':s/' . test_call . '/' . test_response . '/\<CR>'
          \ . '\"+yis'
          \ . ':q!\<CR>", "tx")'
    let run_vimtest = printf(testee, cmd)

    " Set the quoteplus register to test_call, and another gvim will launched.
    " Then, it first tries to paste the content of its own quotedplus register
    " onto it.  Second, it tries to substitute test_response for the pasted
    " sentence.  If the sentence is identical to test_call, the substitution
    " should succeed.  Third, it tries to yank the result of the substitution
    " to its own quoteplus register, and last it quits.  When system()
    " returns, the content of the quoteplus register should be identical to
    " test_response if those quoteplus registers are synchronized properly
    " with/through the X11 clipboard.
    let @+ = test_call
    call system(run_vimtest)
    call assert_equal(test_response, @+)

    let @+ = quoteplus_saved
  endif

  if !empty(skipped)
    throw skipped
  endif
endfunc

func Test_set_background()
  let background_saved = &background

  set background&
  call assert_equal('light', &background)

  set background=dark
  call assert_equal('dark', &background)

  let &background = background_saved
endfunc

func Test_set_balloondelay()
  if !exists('+balloondelay')
    return
  endif

  let balloondelay_saved = &balloondelay

  " Check if the default value is identical to that described in the manual.
  set balloondelay&
  call assert_equal(600, &balloondelay)

  " Edge cases

  " XXX This fact should be hidden so that people won't be tempted to write
  " plugin/TimeMachine.vim.  TODO Add reasonable range checks to the source
  " code.
  set balloondelay=-1
  call assert_equal(-1, &balloondelay)

  " Though it's possible to interpret the zero delay to be 'as soon as
  " possible' or even 'indefinite', its actual meaning depends on the GUI
  " toolkit in use after all.
  set balloondelay=0
  call assert_equal(0, &balloondelay)

  set balloondelay=1
  call assert_equal(1, &balloondelay)

  " Since p_bdelay is of type long currently, the upper bound can be
  " impractically huge and machine-dependent.  Practically, it's sufficient
  " to check if balloondelay works with 0x7fffffff (32 bits) for now.
  set balloondelay=2147483647
  call assert_equal(2147483647, &balloondelay)

  let &balloondelay = balloondelay_saved
endfunc

func Test_set_ballooneval()
  if !exists('+ballooneval')
    return
  endif

  let ballooneval_saved = &ballooneval

  set ballooneval&
  call assert_equal(0, &ballooneval)

  set ballooneval
  call assert_notequal(0, &ballooneval)

  set noballooneval
  call assert_equal(0, &ballooneval)

  let &ballooneval = ballooneval_saved
endfunc

func Test_set_balloonexpr()
  if !exists('+balloonexpr')
    return
  endif

  let balloonexpr_saved = &balloonexpr

  " Default value
  set balloonexpr&
  call assert_equal('', &balloonexpr)

  " User-defined function
  new
  func MyBalloonExpr()
      return 'Cursor is at line ' . v:beval_lnum .
	      \', column ' . v:beval_col .
	      \ ' of file ' .  bufname(v:beval_bufnr) .
	      \ ' on word "' . v:beval_text . '"' .
	      \ ' in window ' . v:beval_winid . ' (#' . v:beval_winnr . ')'
  endfunc
  setl balloonexpr=MyBalloonExpr()
  setl ballooneval
  call assert_equal('MyBalloonExpr()', &balloonexpr)
  " TODO Read non-empty text, place the pointer at a character of a word,
  " and check if the content of the balloon is the same as what is expected.
  " Also, check if textlock works as expected.
  setl balloonexpr&
  call assert_equal('', &balloonexpr)
  delfunc MyBalloonExpr
  bwipe!

  " Multiline support
  if has('balloon_multiline')
    " Multiline balloon using NL
    new
    func MyBalloonFuncForMultilineUsingNL()
      return "Multiline\nSuppported\nBalloon\nusing NL"
    endfunc
    setl balloonexpr=MyBalloonFuncForMultilineUsingNL()
    setl ballooneval
    call assert_equal('MyBalloonFuncForMultilineUsingNL()', &balloonexpr)
    " TODO Read non-empty text, place the pointer at a character of a word,
    " and check if the content of the balloon is the same as what is
    " expected.  Also, check if textlock works as expected.
    setl balloonexpr&
    delfunc MyBalloonFuncForMultilineUsingNL
    bwipe!

    " Multiline balloon using List
    new
    func MyBalloonFuncForMultilineUsingList()
      return [ 'Multiline', 'Suppported', 'Balloon', 'using List' ]
    endfunc
    setl balloonexpr=MyBalloonFuncForMultilineUsingList()
    setl ballooneval
    call assert_equal('MyBalloonFuncForMultilineUsingList()', &balloonexpr)
    " TODO Read non-empty text, place the pointer at a character of a word,
    " and check if the content of the balloon is the same as what is
    " expected.  Also, check if textlock works as expected.
    setl balloonexpr&
    delfunc MyBalloonFuncForMultilineUsingList
    bwipe!
  endif

  let &balloonexpr = balloonexpr_saved
endfunc

" Invalid arguments are tested with test_options in conjunction with segfaults
" caused by them (Patch 8.0.0357, 24922ec233).
func Test_set_guicursor()
  let guicursor_saved = &guicursor

  let default = [
        \ "n-v-c:block-Cursor/lCursor",
        \ "ve:ver35-Cursor",
        \ "o:hor50-Cursor",
        \ "i-ci:ver25-Cursor/lCursor",
        \ "r-cr:hor20-Cursor/lCursor",
        \ "sm:block-Cursor-blinkwait175-blinkoff150-blinkon175"
        \ ]

  " Default Value
  set guicursor&
  call assert_equal(join(default, ','), &guicursor)

  " Argument List Example 1
  let opt_list = copy(default)
  let opt_list[0] = "n-c-v:block-nCursor"
  exec "set guicursor=" . join(opt_list, ',')
  call assert_equal(join(opt_list, ','), &guicursor)
  unlet opt_list

  " Argument List Example 2
  let opt_list = copy(default)
  let opt_list[3] = "i-ci:ver30-iCursor-blinkwait300-blinkon200-blinkoff150"
  exec "set guicursor=" . join(opt_list, ',')
  call assert_equal(join(opt_list, ','), &guicursor)
  unlet opt_list

  " 'a' Mode
  set guicursor&
  let &guicursor .= ',a:blinkon0'
  call assert_equal(join(default, ',') . ",a:blinkon0", &guicursor)

  let &guicursor = guicursor_saved
endfunc

func Test_set_guifont()
  let skipped = ''

  let guifont_saved = &guifont
  if has('xfontset')
    " Prevent 'guifontset' from canceling 'guifont'.
    let guifontset_saved = &guifontset
    set guifontset=
  endif

  if !g:x11_based_gui
    let skipped = g:not_implemented
  elseif has('gui_athena') || has('gui_motif')
    " Non-empty font list with invalid font names.
    "
    " This test is twofold: (1) It checks if the command fails as expected
    " when there are no loadable fonts found in the list. (2) It checks if
    " 'guifont' remains the same after the command loads none of the fonts
    " listed.
    let flist = &guifont
    call assert_fails('set guifont=-notexist1-*,-notexist2-*')
    call assert_equal(flist, &guifont)

    " Non-empty font list with a valid font name.  Should pick up the first
    " valid font.
    set guifont=-notexist1-*,fixed,-notexist2-*
    let pat = '\(fixed\)\|\(\c-Misc-Fixed-Medium-R-SemiCondensed--13-120-75-75-C-60-ISO8859-1\)'
    call assert_match(pat, getfontname())

    " Empty list. Should fallback to the built-in default.
    set guifont=
    let pat = '\(7x13\)\|\(\c-Misc-Fixed-Medium-R-Normal--13-120-75-75-C-70-ISO8859-1\)'
    call assert_match(pat, getfontname())

  elseif has('gui_gtk2') || has('gui_gnome') || has('gui_gtk3')
    " For GTK, what we refer to as 'font names' in our manual are actually
    " 'initial font patterns'.  A valid font which matches the 'canonical font
    " pattern' constructed from a given 'initial pattern' is to be looked up
    " and loaded.  That explains why the GTK GUIs appear to accept 'invalid
    " font names'.
    "
    " Non-empty list.  Should always pick up the first element, no matter how
    " strange it is, as explained above.
    set guifont=(´・ω・｀)\ 12,Courier\ 12
    call assert_equal('(´・ω・｀) 12', getfontname())

    " Empty list. Should fallback to the built-in default.
    set guifont=
    call assert_equal('Monospace 10', getfontname())
  endif

  if has('xfontset')
    let &guifontset = guifontset_saved
  endif
  let &guifont = guifont_saved

  if !empty(skipped)
    throw skipped
  endif
endfunc

func Test_set_guifontset()
  let skipped = ''

  if !has('xfontset')
    let skipped = g:not_supported . 'xfontset'
  else
    let ctype_saved = v:ctype

    " First, since XCreateFontSet(3) is very sensitive to locale, fonts must
    " be chosen meticulously.
    let font_head = '-misc-fixed-medium-r-normal--14'

    let font_aw70 = font_head . '-130-75-75-c-70'
    let font_aw140 = font_head . '-130-75-75-c-140'

    let font_jisx0201 = font_aw70 . '-jisx0201.1976-0'
    let font_jisx0208 = font_aw140 . '-jisx0208.1983-0'

    let full_XLFDs = join([ font_jisx0208, font_jisx0201 ], ',')
    let short_XLFDs = join([ font_aw140, font_aw70 ], ',')
    let singleton = font_head . '-*'
    let aliases = 'k14,r14'

    " Second, among 'locales', look up such a locale that gets 'set
    " guifontset=' to work successfully with every fontset specified with
    " 'fontsets'.
    let locales = [ 'ja_JP.UTF-8', 'ja_JP.eucJP', 'ja_JP.SJIS' ]
    let fontsets = [ full_XLFDs, short_XLFDs, singleton, aliases ]

    let feasible = 0
    for locale in locales
      try
        exec 'language ctype' locale
      catch /^Vim\%((\a\+)\)\=:E197/
        continue
      endtry
      let done = 0
      for fontset in fontsets
        try
          exec 'set guifontset=' . fontset
        catch /^Vim\%((\a\+)\)\=:E\%(250\|252\|234\|597\|598\)/
          break
        endtry
        let done += 1
      endfor
      if done == len(fontsets)
        let feasible = 1
        break
      endif
    endfor

    " Third, give a set of tests if it is found feasible.
    if !feasible
      let skipped = g:not_hosted
    else
      " N.B. 'v:ctype' has already been set to an appropriate value in the
      " previous loop.
      for fontset in fontsets
        exec 'set guifontset=' . fontset
        call assert_equal(fontset, &guifontset)
      endfor
    endif

    " Finally, restore ctype.
    exec 'language ctype' ctype_saved
  endif

  if !empty(skipped)
    throw skipped
  endif
endfunc

func Test_set_guifontwide()
  let skipped = ''

  if !g:x11_based_gui
    let skipped = g:not_implemented
  elseif has('gui_gtk')
    let guifont_saved = &guifont
    let guifontwide_saved = &guifontwide

    let fc_match = exepath('fc-match')
    if empty(fc_match)
      let skipped = g:not_hosted
    else
      let &guifont = system('fc-match -f "%{family[0]} %{size}" monospace:size=10:lang=en')
      let wide = system('fc-match -f "%{family[0]} %{size}" monospace:size=10:lang=ja')
      exec 'set guifontwide=' . fnameescape(wide)
      call assert_equal(wide, &guifontwide)
    endif

    let &guifontwide = guifontwide_saved
    let &guifont = guifont_saved

  elseif has('gui_athena') || has('gui_motif')
    " guifontwide is premised upon the xfontset feature.
    if !has('xfontset')
      let skipped = g:not_supported . 'xfontset'
    else
      let encoding_saved    = &encoding
      let guifont_saved     = &guifont
      let guifontset_saved  = &guifontset
      let guifontwide_saved = &guifontwide

      let nfont = '-misc-fixed-medium-r-normal-*-18-120-100-100-c-90-iso10646-1'
      let wfont = '-misc-fixed-medium-r-normal-*-18-120-100-100-c-180-iso10646-1'

      set encoding=utf-8

      " Case 1: guifontset is empty
      set guifontset=

      " Case 1-1: Automatic selection
      set guifontwide=
      exec 'set guifont=' . nfont
      call assert_equal(wfont, &guifontwide)

      " Case 1-2: Manual selection
      exec 'set guifontwide=' . wfont
      exec 'set guifont=' . nfont
      call assert_equal(wfont, &guifontwide)

      " Case 2: guifontset is invalid
      try
        set guifontset=-*-notexist-*
        call assert_report("'set guifontset=-*-notexist-*' should have failed")
      catch
        call assert_exception('E598')
      endtry
      " Set it to an invalid value brutally for preparation.
      let &guifontset = '-*-notexist-*'

      " Case 2-1: Automatic selection
      set guifontwide=
      exec 'set guifont=' . nfont
      call assert_equal(wfont, &guifontwide)

      " Case 2-2: Manual selection
      exec 'set guifontwide=' . wfont
      exec 'set guifont=' . nfont
      call assert_equal(wfont, &guifontwide)

      let &guifontwide = guifontwide_saved
      let &guifontset  = guifontset_saved
      let &guifont     = guifont_saved
      let &encoding    = encoding_saved
    endif
  endif

  if !empty(skipped)
    throw skipped
  endif
endfunc

func Test_set_guiheadroom()
  let skipped = ''

  if !g:x11_based_gui
    let skipped = g:not_supported . 'guiheadroom'
  else
    " Since this script is to be read together with '-U NONE', the default
    " value must be preserved.
    call assert_equal(50, &guiheadroom)
  endif

  if !empty(skipped)
    throw skipped
  endif
endfunc

func Test_set_guioptions()
  let guioptions_saved = &guioptions
  let duration = '200m'

  if has('win32')
    " Default Value
    set guioptions&
    call assert_equal('egmrLtT', &guioptions)

  else
    " Default Value
    set guioptions&
    call assert_equal('aegimrLtT', &guioptions)

    " To activate scrollbars of type 'L' or 'R'.
    wincmd v
    redraw!

    " Remove all default GUI ornaments
    set guioptions-=T
    exec 'sleep' . duration
    call assert_equal('aegimrLt', &guioptions)
    set guioptions-=t
    exec 'sleep' . duration
    call assert_equal('aegimrL', &guioptions)
    set guioptions-=L
    exec 'sleep' . duration
    call assert_equal('aegimr', &guioptions)
    set guioptions-=r
    exec 'sleep' . duration
    call assert_equal('aegim', &guioptions)
    set guioptions-=m
    exec 'sleep' . duration
    call assert_equal('aegi', &guioptions)

    " Try non-default GUI ornaments
    set guioptions+=l
    exec 'sleep' . duration
    call assert_equal('aegil', &guioptions)
    set guioptions-=l
    exec 'sleep' . duration
    call assert_equal('aegi', &guioptions)

    set guioptions+=R
    exec 'sleep' . duration
    call assert_equal('aegiR', &guioptions)
    set guioptions-=R
    exec 'sleep' . duration
    call assert_equal('aegi', &guioptions)

    set guioptions+=b
    exec 'sleep' . duration
    call assert_equal('aegib', &guioptions)
    set guioptions+=h
    exec 'sleep' . duration
    call assert_equal('aegibh', &guioptions)
    set guioptions-=h
    exec 'sleep' . duration
    call assert_equal('aegib', &guioptions)
    set guioptions-=b
    exec 'sleep' . duration
    call assert_equal('aegi', &guioptions)

    set guioptions+=v
    exec 'sleep' . duration
    call assert_equal('aegiv', &guioptions)
    set guioptions-=v
    exec 'sleep' . duration
    call assert_equal('aegi', &guioptions)

    if has('gui_motif')
      set guioptions+=F
      exec 'sleep' . duration
      call assert_equal('aegiF', &guioptions)
      set guioptions-=F
      exec 'sleep' . duration
      call assert_equal('aegi', &guioptions)
    endif

    " Restore GUI ornaments to the default state.
    set guioptions+=m
    exec 'sleep' . duration
    call assert_equal('aegim', &guioptions)
    set guioptions+=r
    exec 'sleep' . duration
    call assert_equal('aegimr', &guioptions)
    set guioptions+=L
    exec 'sleep' . duration
    call assert_equal('aegimrL', &guioptions)
    set guioptions+=t
    exec 'sleep' . duration
    call assert_equal('aegimrLt', &guioptions)
    set guioptions+=T
    exec 'sleep' . duration
    call assert_equal("aegimrLtT", &guioptions)

    wincmd o
    redraw!
  endif

  let &guioptions = guioptions_saved
endfunc

func Test_scrollbars()
  new
  " buffer with 200 lines
  call setline(1, repeat(['one', 'two'], 100))
  set guioptions+=rlb

  " scroll to move line 11 at top, moves the cursor there
  call test_scrollbar('left', 10, 0)
  redraw
  call assert_equal(1, winline())
  call assert_equal(11, line('.'))

  " scroll to move line 1 at top, cursor stays in line 11
  call test_scrollbar('right', 0, 0)
  redraw
  call assert_equal(11, winline())
  call assert_equal(11, line('.'))

  set nowrap
  call setline(11, repeat('x', 150))
  redraw
  call assert_equal(1, wincol())
  call assert_equal(1, col('.'))

  " scroll to character 11, cursor is moved
  call test_scrollbar('hor', 10, 0)
  redraw
  call assert_equal(1, wincol())
  call assert_equal(11, col('.'))

  set guioptions&
  set wrap&
  bwipe!
endfunc

func Test_menu()
  " Check Help menu exists
  let help_menu = execute('menu Help')
  call assert_match('Overview', help_menu)

  " Check Help menu works
  emenu Help.Overview
  call assert_equal('help', &buftype)
  close

  " Check deleting menu doesn't cause trouble.
  aunmenu Help
  call assert_fails('menu Help', 'E329:')
endfunc

func Test_set_guipty()
  let guipty_saved = &guipty

  " Default Value
  set guipty&
  call assert_equal(1, &guipty)

  set noguipty
  call assert_equal(0, &guipty)

  let &guipty = guipty_saved
endfunc

func Test_encoding_conversion()
  " GTK supports conversion between 'encoding' and "utf-8"
  if has('gui_gtk')
    let encoding_saved = &encoding
    set encoding=latin1

    " would be nice if we could take a screenshot
    intro
    " sets the window title
    edit SomeFile

    let &encoding = encoding_saved
  endif
endfunc

func Test_shell_command()
  new
  r !echo hello
  call assert_equal('hello', substitute(getline(2), '\W', '', 'g'))
  bwipe!
endfunc

func Test_syntax_colortest()
  runtime syntax/colortest.vim
  redraw!
  sleep 200m
  bwipe!
endfunc

func Test_set_term()
  " It's enough to check the current value since setting 'term' to anything
  " other than builtin_gui makes no sense at all.
  call assert_equal('builtin_gui', &term)
endfunc

func Test_windowid_variable()
  if g:x11_based_gui || has('win32')
    call assert_true(v:windowid > 0)
  else
    call assert_equal(0, v:windowid)
  endif
endfunc

" Test "vim -g" and also the GUIEnter autocommand.
func Test_gui_dash_g()
  let cmd = GetVimCommand('Xscriptgui')
  call writefile([""], "Xtestgui")
  call writefile([
	\ 'au GUIEnter * call writefile(["insertmode: " . &insertmode], "Xtestgui")',
	\ 'au GUIEnter * qall',
	\ ], 'Xscriptgui')
  call system(cmd . ' -g')
  call WaitForAssert({-> assert_equal(['insertmode: 0'], readfile('Xtestgui'))})

  call delete('Xscriptgui')
  call delete('Xtestgui')
endfunc

" Test "vim -7" and also the GUIEnter autocommand.
func Test_gui_dash_y()
  let cmd = GetVimCommand('Xscriptgui')
  call writefile([""], "Xtestgui")
  call writefile([
	\ 'au GUIEnter * call writefile(["insertmode: " . &insertmode], "Xtestgui")',
	\ 'au GUIEnter * qall',
	\ ], 'Xscriptgui')
  call system(cmd . ' -y')
  call WaitForAssert({-> assert_equal(['insertmode: 1'], readfile('Xtestgui'))})

  call delete('Xscriptgui')
  call delete('Xtestgui')
endfunc
