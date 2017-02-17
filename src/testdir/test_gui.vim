" Tests specifically for the GUI

if !has('gui') || ($DISPLAY == "" && !has('gui_running'))
  finish
endif

let s:x11_based_gui = has('gui_athena') || has('gui_motif')
	\ || has('gui_gtk2') || has('gui_gnome') || has('gui_gtk3')

" Reasons for 'skipped'.
let s:not_supported   = "Skipped: Feature/Option not supported by this GUI: "
let s:not_implemented = "Skipped: Test not implemented yet for this GUI"
let s:not_hosted      = "Skipped: Test not hosted by the system/environment"

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
  let skipped = ''

  if !s:x11_based_gui
    let skipped = s:not_implemented
  elseif has('gui_athena') || has('gui_motif')
    " Invalid font name. The result should be an empty string.
    call assert_equal('', getfontname('notexist'))

    " Valid font name. This is usually the real name of 7x13 by default.
    let fname = '-misc-fixed-medium-r-normal--13-120-75-75-c-70-iso8859-1'
    call assert_equal(fname, getfontname(fname))

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

  if !s:x11_based_gui
    let skipped = s:not_implemented
  elseif has('gui_kde')
    " 'expected' is the value specified by SetUp() above.
    call assert_equal('Courier 10 Pitch/8/-1/5/50/0/0/0/0/0', fname)
  elseif has('gui_athena') || has('gui_motif')
    " 'expected' is DFLT_FONT of gui_x11.c.
    call assert_equal('7x13', fname)
  elseif has('gui_gtk2') || has('gui_gnome') || has('gui_gtk3')
    " 'expected' is DEFAULT_FONT of gui_gtk_x11.c.
    call assert_equal('Monospace 10', fname)
  endif

  if !empty(skipped)
    throw skipped
  endif
endfunc

func Test_set_guifont()
  let skipped = ''

  let guifont_saved = &guifont
  if has('xfontset')
    " Prevent 'guifontset' from canceling 'guifont'.
    let guifontset_saved = &guifontset
    set guifontset=
  endif

  if !s:x11_based_gui
    let skipped = s:not_implemented
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
    let skipped = s:not_supported . 'xfontset'
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
      let skipped = s:not_hosted
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

  if !s:x11_based_gui
    let skipped = s:not_implemented
  elseif has('gui_gtk')
    let guifont_saved = &guifont
    let guifontwide_saved = &guifontwide

    let fc_match = exepath('fc-match')
    if empty(fc_match)
      let skipped = s:not_hosted
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
      let skipped = s:not_supported . 'xfontset'
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
        call assert_false(1, "'set guifontset=-*-notexist-*' should have failed")
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
