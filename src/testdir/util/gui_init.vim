" gvimrc for test_gui_init.vim

" Keep the GUI at the minimum size required by runtest.vim, even when the
" default font is large and the display is small (for example, CI's Xvfb).
if has('gui_gtk')
  set guifont=Monospace\ 8
endif
set lines=24 columns=80

if has('gui_motif') || has('gui_gtk2') || has('gui_gtk3')
  set guiheadroom=0
  set guioptions+=p
endif
