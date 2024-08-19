vim9script

exec 'source ' .. (((cwdpath: string) => cwdpath
  ->strpart(0, cwdpath->strridx('/vim')))(getcwd()))
  .. '/vim/src/testdir/commondumps.vim'
g:Init('\<syntax\>', -1)

# THE FOLLOWING SETTINGS PERTAIN TO "input/" FILES THAT ARE LIKELY TO BE
# LOADED SIDE BY SIDE WHENEVER BATCHES OF NEW SCREENDUMPS ARE GENERATED.

# Match "LC_ALL=C" of Makefile.
language C

# Match the settings from term_util.vim#RunVimInTerminal().
set t_Co=256 background=light
hi Normal ctermfg=NONE ctermbg=NONE

# Match the settings from runtest.vim#Xtestscript#SetUpVim().
set display=lastline ruler scrolloff=5 t_ZH= t_ZR=

# Anticipate non-Latin-1 characters in "input/" files.
set encoding=utf-8 termencoding=utf-8

# vim:fdm=syntax:sw=2:ts=8:noet:nolist:nosta:
