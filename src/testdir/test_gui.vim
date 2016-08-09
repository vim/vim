" Tests specifically for the GUI

if !has('gui') || ($DISPLAY == "" && !has('gui_running'))
  finish
endif

" For KDE set a font, empty 'guifont' may cause a hang.
func SetUp()
  if has("gui_kde")
    set guifont=Courier\ 10\ Pitch/8/-1/5/50/0/0/0/0/0
  endif

  " Gnome insists on creating $HOME/.gnome2/..
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

func Test_shell_command()
  new
  r !echo hello
  call assert_equal('hello', substitute(getline(2), '\W', '', 'g'))
  bwipe!
  call assert_true(1, match(execute('winpos'), 'Window position: X \d\+, Y \d\+') >= 0)
endfunc
