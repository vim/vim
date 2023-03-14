" Vim ftplugin file
" Language:             Free-Form ILE RPG
" Maintainer:           Andreas Louv <andreas@louv.dk>
" Last Change:          Mar 14, 2023
" Version:              1

if exists('b:did_ftplugin')
  finish
endif

let b:did_ftplugin = 1

setlocal iskeyword+=-,%

setlocal suffixesadd=.rpgle,.rpgleinc
setlocal include=^\\s*/\\%(include\\\|copy\\)

setlocal comments=s1:/*,mb:*,ex:*/,://,:*

" ILE RPG is in case-sensitive
setlocal tagcase=ignore nosmartcase ignorecase

let b:match_words = '\<select\>:\<when\>:\<other\>:\<endsl\>'
                \ . ',\<if\>:\<elseif\>:\<else\>:\<endif\>'
                \ . ',\<do[uw]\>:\<iter\>:\<leave\>:\<enddo\>'
                \ . ',\<for\>:\<iter\>:\<leave\>:\<endfor\>'
                \ . ',\<begsr\>:\<endsr\>'
                \ . ',\<dcl-proc\>:\<return\>:\<end-proc\>'
                \ . ',\<dcl-pi\>:\<end-pi\>'
                \ . ',\<dcl-pr\>:\<end-pr\>'
                \ . ',\<monitor\>:\<on-error\>:\<endmon\>'
                \ . ',\<dcl-ds\>:\<\%(likeds\|extname\|end-ds\)\>'

function s:GoToDecl(curword) abort
  keepj call rpgle#NextSection('^\s*dcl-proc', 'b', '')
  call setreg('/', '\<' .. a:curword .. '\>', 'c')
  keepj norm! n
endfunction

" section jumping
nnoremap <script> <buffer> <silent> <Plug>RpgleGoToDeclaration
       \ :<C-U>call <Sid>GoToDecl(expand('<cword>'))<Cr>
nnoremap <script> <buffer> <silent> <Plug>RpgleNextProcStart
       \ :<C-U>call rpgle#NextSection('^\s*dcl-proc', '', '')<CR>
nnoremap <script> <buffer> <silent> <Plug>RpgleNextProcEnd
       \ :<C-U>call rpgle#NextSection('^\s*end-proc', '', '')<CR>
nnoremap <script> <buffer> <silent> <Plug>RpglePrevProcStart
       \ :<C-U>call rpgle#NextSection('^\s*dcl-proc', 'b', '')<CR>
nnoremap <script> <buffer> <silent> <Plug>RpglePrevProcEnd
       \ :<C-U>call rpgle#NextSection('^\s*end-proc', 'b', '')<CR>
xnoremap <script> <buffer> <silent> <Plug>XRpgleNextProcStart
       \ :<C-U>call rpgle#NextSection('^\s*dcl-proc', '', 'x')<CR>
xnoremap <script> <buffer> <silent> <Plug>XRpgleNextProcEnd
       \ :<C-U>call rpgle#NextSection('^\s*end-proc', '', 'x')<CR>
xnoremap <script> <buffer> <silent> <Plug>XRpglePrevProcStart
       \ :<C-U>call rpgle#NextSection('^\s*dcl-proc', 'b', 'x')<CR>
xnoremap <script> <buffer> <silent> <Plug>XRpglePrevProcEnd
       \ :<C-U>call rpgle#NextSection('^\s*end-proc', 'b', 'x')<CR>

" Nest jumping
nnoremap <script> <buffer> <silent> <Plug>RpglePrevBlock
       \ :call rpgle#NextNest('b')<CR>
nnoremap <script> <buffer> <silent> <Plug>RpgleNextBlock
       \ :call rpgle#NextNest('')<CR>

" Operator Pending brackets
noremap <script> <buffer> <silent> <Plug>RpgleAroundBlock
       \ :<C-U>call rpgle#Operator('a')<CR>
noremap <script> <buffer> <silent> <Plug>RpgleInnerBlock
       \ :<C-U>call rpgle#Operator('i')<CR>

if !exists("g:no_plugin_maps") && !exists("g:no_rpgle_maps")
  nmap <buffer> <silent> gd <Plug>RpgleGoToDeclaration
  nmap <buffer> <silent> ]] <Plug>RpgleNextProcStart
  nmap <buffer> <silent> ][ <Plug>RpgleNextProcEnd
  nmap <buffer> <silent> [[ <Plug>RpglePrevProcStart
  nmap <buffer> <silent> [] <Plug>RpglePrevProcEnd

  xmap <buffer> <silent> ]] <Plug>XRpgleNextProcStart
  xmap <buffer> <silent> ][ <Plug>XRpgleNextProcEnd
  xmap <buffer> <silent> [[ <Plug>XRpglePrevProcStart
  xmap <buffer> <silent> [] <Plug>XRpglePrevProcEnd

  nmap <buffer> [{ <Plug>RpglePrevBlock
  nmap <buffer> ]} <Plug>RpgleNextBlock

  omap <buffer> a} <Plug>RpgleAroundBlock
  omap <buffer> a{ <Plug>RpgleAroundBlock
  omap <buffer> aB <Plug>RpgleAroundBlock
  omap <buffer> i} <Plug>RpgleInnerBlock
  omap <buffer> i{ <Plug>RpgleInnerBlock
  omap <buffer> iB <Plug>RpgleInnerBlock

  xmap <buffer> a} <Plug>RpgleAroundBlock
  xmap <buffer> a{ <Plug>RpgleAroundBlock
  xmap <buffer> aB <Plug>RpgleAroundBlock
  xmap <buffer> i} <Plug>RpgleInnerBlock
  xmap <buffer> i{ <Plug>RpgleInnerBlock
  xmap <buffer> iB <Plug>RpgleInnerBlock
endif

let b:undo_ftplugin = 'setlocal iskeyword<'
  \ . '|setlocal suffixesadd<'
  \ . '|setlocal include<'
  \ . '|setlocal comments<'
  \ . '|setlocal tagcase<'
  \ . '|setlocal nosmartcase<'
  \ . '|setlocal ignorecase<'
  \ . '|silent! nunmap <buffer> gd'
  \ . '|silent! nunmap <buffer> ]]'
  \ . '|silent! nunmap <buffer> ]['
  \ . '|silent! nunmap <buffer> [['
  \ . '|silent! nunmap <buffer> []'
  \ . '|silent! xunmap <buffer> ]]'
  \ . '|silent! xunmap <buffer> ]['
  \ . '|silent! xunmap <buffer> [['
  \ . '|silent! xunmap <buffer> []'
  \ . '|silent! nunmap <buffer> [{'
  \ . '|silent! nunmap <buffer> ]}'
  \ . '|silent! ounmap <buffer> a}'
  \ . '|silent! ounmap <buffer> a{'
  \ . '|silent! ounmap <buffer> aB'
  \ . '|silent! ounmap <buffer> i}'
  \ . '|silent! ounmap <buffer> i{'
  \ . '|silent! ounmap <buffer> iB'
  \ . '|silent! xunmap <buffer> a}'
  \ . '|silent! xunmap <buffer> a{'
  \ . '|silent! xunmap <buffer> aB'
  \ . '|silent! xunmap <buffer> i}'
  \ . '|silent! xunmap <buffer> i{'
  \ . '|silent! xunmap <buffer> iB'
  \ . '|unlet! b:match_words'
