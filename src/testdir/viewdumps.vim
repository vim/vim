vim9script

exec 'source ' .. (((cwdpath: string) => cwdpath
  ->strpart(0, cwdpath->strridx('/vim')))(getcwd()))
  .. '/vim/src/testdir/commondumps.vim'
g:Init('\<src\>', 0)

# Match ":language" of runtest.vim.
language messages C

# vim:fdm=syntax:sw=2:ts=8:noet:nolist:nosta:
