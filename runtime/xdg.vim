" XDG Base Directory support

if empty($XDG_CONFIG_HOME) | let $XDG_CONFIG_HOME = $HOME."/.config"      | endif
if empty($XDG_DATA_HOME)   | let $XDG_DATA_HOME   = $HOME."/.local/share" | endif
if empty($XDG_STATE_HOME)  | let $XDG_STATE_HOME  = $HOME."/.local/state" | endif

set packpath^=$XDG_DATA_HOME/vim
set packpath+=$XDG_DATA_HOME/vim/after

if isdirectory(expand($XDG_CONFIG_HOME."/vim"))
  set viminfofile=$XDG_STATE_HOME/vim/viminfo | call mkdir($XDG_STATE_HOME."/vim", 'p', 0700)

  " These options are not essential for XDG, but you might want to set them:
  " set backupdir=$XDG_STATE_HOME/vim/backup// | call mkdir(&backupdir, 'p', 0700)
  " set directory=$XDG_STATE_HOME/vim/swap   | call mkdir(&directory, 'p', 0700)
  " set viewdir=$XDG_STATE_HOME/vim/view     | call mkdir(&viewdir,   'p', 0700)
  " set undodir=$XDG_STATE_HOME/vim/undo     | call mkdir(&undodir,   'p', 0700)

  " let g:netrw_home = $XDG_DATA_HOME."/vim"
  " call mkdir($XDG_DATA_HOME."/vim/spell", 'p', 0700)
endif
