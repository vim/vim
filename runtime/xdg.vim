" XDG Base Directory support
" This script sets up paths for XDG compliance.
" Maintainer:	The Vim Project <https://github.com/vim/vim>
" Last Change:	2026 Feb 19

let s:config = empty($XDG_CONFIG_HOME) ? expand("~/.config") : expand("$XDG_CONFIG_HOME")
let s:data   = empty($XDG_DATA_HOME)   ? expand("~/.local/share") : expand("$XDG_DATA_HOME")
let s:state  = empty($XDG_STATE_HOME)  ? expand("~/.local/state") : expand("$XDG_STATE_HOME")

if isdirectory(s:config .. '/vim')
  func s:mkvimdir(dir)
    if !isdirectory(a:dir)
      call mkdir(a:dir, 'p', 0700)
    endif
    return a:dir
  endfunc

  " Use Data for packages, prevent duplicates
  if index(split(&packpath, ','), s:data .. '/vim') == -1
    exe $"set packpath^={s:data}/vim"
    exe $"set packpath+={s:data}/vim/after"
  endif

  " These options are not set by default because they change the behavior of
  " where files are saved. Uncomment them if you want to fully move all
  " transient/persistent files to XDG directories.
  " Note: Undo/Views/Spell are placed in DATA_HOME as they are persistent,
  " while Swap/Backups use STATE_HOME as transient session state.

  " Persistent Data:
  " let &undodir = s:mkvimdir(s:data .. '/vim/undo')
  " let &viewdir = s:mkvimdir(s:data .. '/vim/view')
  " let g:netrw_home = s:mkvimdir(s:data .. '/vim')
  " call s:mkvimdir(s:data .. '/vim/spell')

  " Transient State:
  let &viminfofile = s:mkvimdir(s:state .. '/vim') .. '/viminfo'
  " let &directory = s:mkvimdir(s:state .. '/vim/swap')   .. '//'
  " let &backupdir = s:mkvimdir(s:state .. '/vim/backup') .. '//'

  delfunction s:mkvimdir
endif

unlet s:config s:data s:state
