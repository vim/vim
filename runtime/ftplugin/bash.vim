" Vim filetype plugin file
" Language:	bash
" Maintainer:	The Vim Project <https://github.com/vim/vim>
" Last Changed: 2023 Aug 13
"
" This is not a real filetype plugin.  It allows for someone to set 'filetype'
" to "bash" in the modeline, and gets the effect of filetype "sh" with
" b:is_bash set.  Idea from Mahmode Al-Qudsi.

if exists("b:did_ftplugin")
  finish
endif

unlet! b:is_sh
unlet! b:is_kornshell
let b:is_bash = 1

runtime! ftplugin/sh.vim ftplugin/sh_*.vim ftplugin/sh/*.vim

if exists(':terminal') == 2
  command! -buffer -nargs=1 ShKeywordPrg silent exe ':term bash -c "help "<args>" 2>/dev/null || man "<args>""'
else
  command! -buffer -nargs=1 ShKeywordPrg echo system('bash -c "help <args>" 2>/dev/null || MANPAGER= man "<args>"')
endif
setlocal keywordprg=:ShKeywordPrg
let b:undo_ftplugin ..= " | setl kp< | sil! delc -buffer ShKeywordPrg"

if !exists('current_compiler')
  if executable('shellcheck')
    compiler shellcheck
  else
    compiler bash
  endif
  let b:undo_ftplugin ..= ' | compiler make'
endif

" vim: ts=8
