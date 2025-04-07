" Vim filetype plugin file
" Language:             Vim help file
" Previous Maintainer:  Nikolai Weibull <now@bitwi.se>
" Latest Revision:      2018-12-29

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

let s:cpo_save = &cpo
set cpo&vim

let b:undo_ftplugin = "setl isk< fo< tw< cole< cocu< keywordprg< omnifunc<"

setlocal formatoptions+=tcroql textwidth=78 keywordprg=:help
let &l:iskeyword='!-~,^*,^|,^",192-255'
setlocal omnifunc=s:Complete
if has("conceal")
  setlocal cole=2 cocu=nc
endif

let &cpo = s:cpo_save
unlet s:cpo_save

if !exists('*s:Complete')
  func s:Complete(findstart, base)
    if a:findstart
      let colnr = col('.') - 1 " Get the column number before the cursor
      let line = getline('.')
      for i in range(colnr - 1, 0, -1)
        if line[i] ==# '|'
          return i + 1 " Don't include the `|` in base
        elseif line[i] ==# "'"
          return i " Include the `'` in base
        endif
      endfor
    else
      return taglist('^' .. a:base)
            \ ->map({_, item -> #{word: item->get('name'), kind: item->get('kind')}})
            \ ->extend(getcompletion(a:base, 'help'))
    endif
  endfunc
endif
