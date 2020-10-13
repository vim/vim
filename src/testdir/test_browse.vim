" Tests for ":browse" command on console

source check.vim
" NOTE: has('browse') doesn't return true on console.
" CheckFeature browse
CheckFeature gui
CheckNotGui

" Must not crash ":browse xxx"
func Test_browse_view()
  " ":browse xxx" just fails without FileExplorer augroup.
  augroup FileExplorer
  augroup END

  browse enew

  %bwipe!
endfunc

" vim: shiftwidth=2 sts=2 expandtab
