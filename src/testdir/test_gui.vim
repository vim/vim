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

func Test_getwinpos()
  call assert_match('Window position: X \d\+, Y \d\+', execute('winpos'))
  call assert_true(getwinposx() >= 0)
  call assert_true(getwinposy() >= 0)
endfunction

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
endfunction
