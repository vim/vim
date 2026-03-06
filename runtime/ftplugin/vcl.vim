if exists('b:did_ftplugin') | finish | endif
let b:did_ftplugin = 1
let s:cpo_save = &cpo
set cpo&vim

setl comments=s1:/*,mb:*,ex:*/,://,:#
setl commentstring=#\ %s
setl iskeyword+=-

let b:undo_ftplugin = 'setl com< cms< isk<'

if !exists('no_plugin_maps')
  noremap <silent> <buffer> ]] <Cmd>call <SID>FindSection('next_start', v:count1)<CR>
  noremap <silent> <buffer> [[ <Cmd>call <SID>FindSection('prev_start', v:count1)<CR>
  let b:undo_ftplugin ..= ''
                      \ .. "| silent! exe 'unmap <buffer> ]]'"
                      \ .. "| silent! exe 'unmap <buffer> [['"
endif

function! <SID>FindSection(dir, count)
  mark '
  let c = a:count
  while c > 0
    if a:dir == 'next_start'
      keepjumps call search('^\v%(sub|backend|acl)>', 'W')
    elseif a:dir == 'prev_start'
      keepjumps call search('^\v%(sub|backend|acl)>', 'bW')
    endif
    let c -= 1
  endwhile
endfunction

let &cpo = s:cpo_save
unlet s:cpo_save
