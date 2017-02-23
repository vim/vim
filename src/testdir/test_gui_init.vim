" Tests specifically for the GUI features/options that need to be set up at
" startup to take effect at runtime.

if !has('gui') || ($DISPLAY == "" && !has('gui_running'))
  finish
endif

source setup_gui.vim

func Setup()
  call GUISetUpCommon()
endfunc

func TearDown()
  call GUITearDownCommon()
endfunc

" Make sure that the tests will be done with the GUI activated.
gui -f

func Test_set_guiheadroom()
  let skipped = ''

  if !g:x11_based_gui
    let skipped = g:not_supported . 'guiheadroom'
  else
    " The 'expected' value must be consistent with the value specified with
    " gui_init.vim.
    call assert_equal(0, &guiheadroom)
  endif

  if !empty(skipped)
    throw skipped
  endif
endfunc
