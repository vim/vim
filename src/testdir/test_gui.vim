" Tests specifically for the GUI

if !has('gui') || ($DISPLAY == "" && !has('gui_running'))
  finish
endif

let s:x11_based_gui = has('gui_athena') || has('gui_motif')
	\ || has('gui_gtk2') || has('gui_gnome') || has('gui_gtk3')

" For KDE set a font, empty 'guifont' may cause a hang.
func SetUp()
  if has("gui_kde")
    set guifont=Courier\ 10\ Pitch/8/-1/5/50/0/0/0/0/0
  endif

  " Gnome insists on creating $HOME/.gnome2/, set $HOME to avoid changing the
  " actual home directory.  But avoid triggering fontconfig by setting the
  " cache directory.  Only needed for Unix.
  if $XDG_CACHE_HOME == '' && exists('g:tester_HOME')
    let $XDG_CACHE_HOME = g:tester_HOME . '/.cache'
  endif
  call mkdir('Xhome')
  let $HOME = fnamemodify('Xhome', ':p')
endfunc

func TearDown()
  call delete('Xhome', 'rf')
endfunc

" Test for resetting "secure" flag after GUI has started.
" Must be run first.
func Test_1_set_secure()
  set exrc secure
  gui -f
  call assert_equal(1, has('gui_running'))
endfunc

func Test_getfontname_with_arg()
  if has('gui_athena') || has('gui_motif')
    " Invalid font name. The result should be an empty string.
    call assert_equal('', getfontname('notexist'))

    " Valid font name. This is usually the real name of 7x13 by default.
    let l:fname = '-misc-fixed-medium-r-normal--13-120-75-75-c-70-iso8859-1'
    call assert_equal(l:fname, getfontname(l:fname))

  elseif has('gui_gtk2') || has('gui_gnome') || has('gui_gtk3')
    " Invalid font name. The result should be the name plus the default size.
    call assert_equal('notexist 10', getfontname('notexist'))

    " Valid font name. This is usually the real name of Monospace by default.
    let l:fname = 'Bitstream Vera Sans Mono 12'
    call assert_equal(l:fname, getfontname(l:fname))
  else
    throw "Skipped: Matched font name unpredictable to test on this GUI"
  endif
endfunc

func Test_getfontname_without_arg()
  let l:fname = getfontname()
  if has('gui_kde')
    " 'expected' is the value specified by SetUp() above.
    call assert_equal('Courier 10 Pitch/8/-1/5/50/0/0/0/0/0', l:fname)
  elseif has('gui_athena') || has('gui_motif')
    " 'expected' is DFLT_FONT of gui_x11.c.
    call assert_equal('7x13', l:fname)
  elseif has('gui_gtk2') || has('gui_gnome') || has('gui_gtk3')
    " 'expected' is DEFAULT_FONT of gui_gtk_x11.c.
    call assert_equal('Monospace 10', l:fname)
  else
    throw "Skipped: Default font name unpredictable to test on this GUI"
  endif
endfunc

func Test_set_guifont()
  let l:guifont_saved = &guifont
  if has('xfontset')
    " Prevent 'guifontset' from canceling 'guifont'.
    let l:guifontset_saved = &guifontset
    set guifontset=
  endif

  let skipped = 0
  if has('gui_athena') || has('gui_motif')
    " Non-empty font list with invalid font names.
    "
    " This test is twofold: (1) It checks if the command fails as expected
    " when there are no loadable fonts found in the list. (2) It checks if
    " 'guifont' remains the same after the command loads none of the fonts
    " listed.
    let l:flist = &guifont
    call assert_fails('set guifont=-notexist1-*,-notexist2-*')
    call assert_equal(l:flist, &guifont)

    " Non-empty font list with a valid font name.  Should pick up the first
    " valid font.
    set guifont=-notexist1-*,fixed,-notexist2-*
    call assert_equal('fixed', getfontname())

    " Empty list. Should fallback to the built-in default.
    set guifont=
    call assert_equal('7x13', getfontname())

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

  else
    let skipped = 1
  endif

  if has('xfontset')
    let &guifontset = l:guifontset_saved
  endif
  let &guifont = l:guifont_saved

  if skipped
    throw "Skipped: Test not implemented yet for this GUI"
  endif
endfunc

func Test_set_guifontset()
  let skipped = 0

  if has('xfontset')
    let l:ctype_saved = v:ctype

    " For UTF-8 locales, XCreateFontSet(3) is likely to fail in constructing a
    " fontset automatically from one or two simple XLFDs because it requires
    " the host system to have a fairly comprehensive collection of fixed-width
    " fonts with various sizes and registries/encodings in order to get the
    " job done.  To make the test meaningful for a wide variety of hosts, we
    " confine ourselves to the following locale for which X11 historically has
    " the fonts to use with.
    language ctype ja_JP.eucJP

    " Since XCreateFontSet(3) is very sensitive to locale, fonts must be
    " chosen meticulously.
    let l:font_head = '-misc-fixed-medium-r-normal--14'

    let l:font_aw70 = l:font_head . '-130-75-75-c-70'
    let l:font_aw140 = l:font_head . '-130-75-75-c-140'

    let l:font_jisx0201 = l:font_aw70 . '-jisx0201.1976-0'
    let l:font_jisx0208 = l:font_aw140 . '-jisx0208.1983-0'

    " Full XLFDs
    let l:fontset_name = join([ l:font_jisx0208, l:font_jisx0201 ], ',')
    exec 'set guifontset=' . l:fontset_name
    call assert_equal(l:fontset_name, &guifontset)

    " XLFDs w/o CharSetRegistry and CharSetEncoding
    let l:fontset_name = join([ l:font_aw140, l:font_aw70 ], ',')
    exec 'set guifontset=' . l:fontset_name
    call assert_equal(l:fontset_name, &guifontset)

    " Singleton
    let l:fontset_name = l:font_head . '-*'
    exec 'set guifontset=' . l:fontset_name
    call assert_equal(l:fontset_name, &guifontset)

    " Aliases
    let l:fontset_name = 'k14,r14'
    exec 'set guifontset=' . l:fontset_name
    call assert_equal(l:fontset_name, &guifontset)

    exec 'language ctype' l:ctype_saved

  else
    let skipped = 1
  endif

  if skipped
    throw "Skipped: Not supported by this GUI"
  endif
endfunc

func Test_set_guifontwide()
  let skipped = 0

  if has('gui_gtk')
    let l:guifont_saved = &guifont
    let l:guifontwide_saved = &guifontwide

    let l:fc_match = exepath('fc-match')
    if l:fc_match != ''
      let &guifont = system('fc-match -f "%{family[0]} %{size}" monospace:size=10')
      let l:wide = system('fc-match -f "%{family[0]} %{size}" monospace:size=10:lang=ja')
      exec 'set guifontwide=' . fnameescape(l:wide)
      call assert_equal(l:wide, &guifontwide)
    else
      let skipped = 3
    endif

    let &guifontwide = l:guifontwide_saved
    let &guifont = l:guifont_saved

  elseif has('gui_athena') || has('gui_motif')
    " guifontwide is premised upon the xfontset feature.
    if has('xfontset')
      let l:encoding_saved = &encoding
      let l:guifont_saved = &guifont
      let l:guifontset_saved = &guifontset
      let l:guifontwide_saved = &guifontwide

      let l:nfont = '-misc-fixed-medium-r-normal-*-18-120-100-100-c-90-iso10646-1'
      let l:wfont = '-misc-fixed-medium-r-normal-*-18-120-100-100-c-180-iso10646-1'

      set encoding=utf-8

      " Case 1: guifontset is empty
      set guifontset=

      " Case 1-1: Automatic selection
      set guifontwide=
      exec 'set guifont=' . l:nfont
      call assert_equal(l:wfont, &guifontwide)

      " Case 1-2: Manual selection
      exec 'set guifontwide=' . l:wfont
      exec 'set guifont=' . l:nfont
      call assert_equal(l:wfont, &guifontwide)

      " Case 2: guifontset is invalid
      try
        set guifontset=-*-notexist-*
        call assert_false(1, "'set guifontset=notexist' should have failed")
      catch
        call assert_exception('E598')
      endtry
      " Set it to an invalid value brutally for preparation.
      let &guifontset = '-*-notexist-*'

      " Case 2-1: Automatic selection
      set guifontwide=
      exec 'set guifont=' . l:nfont
      call assert_equal(l:wfont, &guifontwide)

      " Case 2-2: Manual selection
      exec 'set guifontwide=' . l:wfont
      exec 'set guifont=' . l:nfont
      call assert_equal(l:wfont, &guifontwide)

      let &guifontwide = l:guifontwide_saved
      let &guifontset = l:guifontset_saved
      let &guifont = l:guifont_saved
      let &encoding = l:encoding_saved
    else
      let skipped = 2
    endif
  else
    let skipped = 1
  endif

  if skipped == 1
    throw "Skipped: Test not implemented yet for this GUI"
  elseif skipped == 2
    throw "Skipped: Not supported by this GUI"
  elseif skipped == 3
    throw "Skipped: Test not supported by the environment"
  endif
endfunc

func Test_getwinpos()
  call assert_match('Window position: X \d\+, Y \d\+', execute('winpos'))
  call assert_true(getwinposx() >= 0)
  call assert_true(getwinposy() >= 0)
endfunc

func Test_shell_command()
  new
  r !echo hello
  call assert_equal('hello', substitute(getline(2), '\W', '', 'g'))
  bwipe!
endfunc

func Test_windowid_variable()
  if s:x11_based_gui || has('win32')
    call assert_true(v:windowid > 0)
  else
    call assert_equal(0, v:windowid)
  endif
endfunc
