" Elixir filetype plugin
" Language: Elixir
" Maintainer:	Mitchell Hanberg <vimNOSPAM@mitchellhanberg.com>
" Last Change: 2022 August 10

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

let s:save_cpo = &cpo
set cpo&vim

" Matchit support
if exists('loaded_matchit') && !exists('b:match_words')
  let b:match_ignorecase = 0

  let b:match_words = '\:\@<!\<\%(do\|fn\)\:\@!\>' .
        \ ':' .
        \ '\<\%(else\|catch\|after\|rescue\)\:\@!\>' .
        \ ':' .
        \ '\:\@<!\<end\>' .
        \ ',{:},\[:\],(:)'
endif

setlocal shiftwidth=2 softtabstop=2 expandtab iskeyword+=!,?
setlocal comments=:#
setlocal commentstring=#\ %s

let &cpo = s:save_cpo
unlet s:save_cpo

let &l:path =
      \ join([
      \   'lib/**',
      \   'src/**',
      \   'test/**',
      \   'deps/**/lib/**',
      \   'deps/**/src/**',
      \   &g:path
      \ ], ',')
setlocal includeexpr=elixir#util#get_filename(v:fname)
setlocal suffixesadd=.ex,.exs,.eex,.heex,.leex,.sface,.erl,.xrl,.yrl,.hrl

let &l:define = 'def\(macro\|guard\|delegate\)\=p\='

silent! setlocal formatoptions-=t formatoptions+=croqlj

let b:block_begin = '\<\(do$\|fn\>\)'
let b:block_end = '\<end\>'

nnoremap <buffer> <silent> <expr> ]] ':silent keeppatterns /'.b:block_begin.'<CR>'
nnoremap <buffer> <silent> <expr> [[ ':silent keeppatterns ?'.b:block_begin.'<CR>'
nnoremap <buffer> <silent> <expr> ][ ':silent keeppatterns /'.b:block_end  .'<CR>'
nnoremap <buffer> <silent> <expr> [] ':silent keeppatterns ?'.b:block_end  .'<CR>'

onoremap <buffer> <silent> <expr> ]] ':silent keeppatterns /'.b:block_begin.'<CR>'
onoremap <buffer> <silent> <expr> [[ ':silent keeppatterns ?'.b:block_begin.'<CR>'
onoremap <buffer> <silent> <expr> ][ ':silent keeppatterns /'.b:block_end  .'<CR>'
onoremap <buffer> <silent> <expr> [] ':silent keeppatterns ?'.b:block_end  .'<CR>'

let b:undo_ftplugin = 'setlocal sw< sts< et< isk< com< cms< path< inex< sua< def< fo<'.
      \ '| unlet! b:match_ignorecase b:match_words b:block_begin b:block_end'
