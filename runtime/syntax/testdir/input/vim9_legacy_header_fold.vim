" Vim9 legacy script header
" VIM_TEST_SETUP let g:vimsyn_folding = "H"
" VIM_TEST_SETUP setl fdc=2 fdl=99 fdm=syntax


" comment

if !has('vim9script')
  # 42 " comment
  source foo.vim
  finish
endif

" comment

vim9script noclear

# comment

 # string only recognised with leading char
 "useless string"

