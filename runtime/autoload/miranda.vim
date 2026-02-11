" Vim filetype plugin file
" Language:	Miranda
" Maintainer:	Doug Kearns <dougkearns@gmail.com>
" Last Change:	2024 Sep 20

function miranda#GetFileTypeInfo() abort
  if exists("b:miranda")
    return b:miranda
  endif

  let literate = get(g:, "miranda_default_literate", v:false)

  return #{ literate: literate }
endfunction

function miranda#SetFileTypeInfo(info) abort
  if exists("b:miranda")
    unlockvar! b:miranda
  endif

  let b:miranda = a:info
  lockvar! b:miranda
endfunction

" vim: nowrap sw=2 sts=2 ts=8 noet fdm=marker:
