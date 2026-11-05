" gvimrc for test_gui_init.vim

set lines=50 columns=100

if has('gui_motif') || has('gui_gtk2') || has('gui_gtk3')
  set guiheadroom=0
  set guioptions+=p
endif
