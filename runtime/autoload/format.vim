" Support script for the "formatter" feature
" Maintainer:  Romain Lafourcade <romainlafourcade@gmail.com>
" Last Change: 2025 Apr 18

let s:keepcpo= &cpoptions
set cpoptions&vim

" Provide a default cross-platform function for :help 'formatexpr'
function! format#FormatExpr()
  if empty(&formatprg)
    return 1
  endif

  let l1 = v:lnum
  let l2 = (v:lnum + v:count) - 1

  silent let output = systemlist(expandcmd(&formatprg), getline(l1, l2))

  if v:shell_error
    echohl WarningMsg | echo 'Shell error: ' .. v:shell_error | echohl None

    return v:shell_error
  else
    execute 'silent ' .. l1 .. ',' .. l2 .. 'd_'

    if wordcount().bytes == 0
      call setline('.', output)
    else
      execute 'silent ' .. (l1 - 1) .. 'put=output'
    endif

    return 0
  endif
endfunction

let &cpoptions = s:keepcpo
unlet s:keepcpo
