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
