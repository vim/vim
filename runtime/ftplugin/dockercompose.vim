" Vim filetype plugin file
" Language:             Docker compose files
" Maintainer:           Jan Claußen <jan.claussen10@web.de>
" Last Change:          2026 Jan 17

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

let s:cpo_save = &cpo
set cpo&vim

let b:undo_ftplugin = "setl com< cms< et< fo<"

setlocal comments=:# commentstring=#\ %s expandtab
setlocal formatoptions-=t formatoptions+=croql

if get(g:, "yaml_recommended_style",1)
  let b:undo_ftplugin ..= " sw< sts<"
  setlocal shiftwidth=2 softtabstop=2
endif

" rime input method engine(https://rime.im/)
" uses `*.custom.yaml` as its config files
if expand('%:r:e') ==# 'custom'
  " `__include` command in `*.custom.yaml`
  " see: https://github.com/rime/home/wiki/Configuration#%E5%8C%85%E5%90%AB
  setlocal include=__include:\\s*
  let b:undo_ftplugin ..= " inc<"

  if !exists('current_compiler')
    compiler rime_deployer
    let b:undo_ftplugin ..= " | compiler make"
  endif
endif


let &cpo = s:cpo_save
unlet s:cpo_save
