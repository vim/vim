" Vim filetype plugin
" Language:	git config file
" Maintainer:	Tim Pope <vimNOSPAM@tpope.org>
" Last Change:	2009 Dec 24

" Only do this when not done yet for this buffer
if (exists("b:did_ftplugin"))
  finish
endif
let b:did_ftplugin = 1

setlocal formatoptions-=t formatoptions+=croql
setlocal comments=:#,:; commentstring=;\ %s

let b:undo_ftplugin = "setl fo< com< cms<"

if !has('unix')
  finish
endif

if !has('gui_running')
  command! -buffer -nargs=1 Sman
        \ silent exe '!' . 'LESS= MANPAGER="less --pattern=''^\s+' . KeywordLookup_gitconfig() . <q-args> . '\b'' --hilite-search" man ' . 'git-config' |
        \ redraw!
elseif has('terminal')
  command! -buffer -nargs=1 Sman
        \ silent exe 'term ' . 'env LESS= MANPAGER="less --pattern=''' . escape('^\s+' . KeywordLookup_gitconfig() . <q-args> . '\b', '\') . ''' --hilite-search" man ' . 'git-config'
else
  finish
endif

if !exists('*KeywordLookup_gitconfig')
function KeywordLookup_gitconfig() abort
  let matches = matchlist(getline(search('\v^\s*\[\s*.+\s*\]\s*$', 'nbWz')), '\v^\s*\[\s*(\k+).*\]\s*$')
  return len(matches) > 1 ? matches[1] . '\.' : ''
endfunction
endif

setlocal iskeyword+=-
setlocal keywordprg=:Sman
let b:undo_ftplugin .= '| setlocal keywordprg< iskeyword<'
