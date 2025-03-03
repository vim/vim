" Vim filetype plugin
" Language:     Markdown
" Maintainer:   Tim Pope <https://github.com/tpope/vim-markdown>
" Last Change:  2023 Dec 28
"               2024 May 24 by Riley Bruins <ribru17@gmail.com> ('commentstring')

if exists("b:did_ftplugin")
  finish
endif

runtime! ftplugin/html.vim ftplugin/html_*.vim ftplugin/html/*.vim

let s:keepcpo= &cpo
set cpo&vim

setlocal comments=fb:*,fb:-,fb:+,n:> commentstring=<!--\ %s\ -->
setlocal formatoptions+=tcqln formatoptions-=r formatoptions-=o
setlocal formatlistpat=^\\s*\\d\\+\\.\\s\\+\\\|^\\s*[-*+]\\s\\+\\\|^\\[^\\ze[^\\]]\\+\\]:\\&^.\\{4\\}

if exists('b:undo_ftplugin')
  let b:undo_ftplugin .= "|setl cms< com< fo< flp< et< ts< sts< sw<"
else
  let b:undo_ftplugin = "setl cms< com< fo< flp< et< ts< sts< sw<"
endif

if get(g:, 'markdown_recommended_style', 1)
  setlocal expandtab tabstop=4 softtabstop=4 shiftwidth=4
endif

if !exists("g:no_plugin_maps") && !exists("g:no_markdown_maps")
  function! s:NextSection(dir)
    let flags = a:dir > 0 ? "sW" : "bsW"
    for _ in range(v:count1)
      let x = search('\%(^#\{1,5\}\s\+\S\|^\S.*\n^[=-]\+$\)', flags)
    endfor
  endfunction

  nnoremap <silent><buffer> [[ :<C-U>call <SID>NextSection(-1)<CR>
  nnoremap <silent><buffer> ]] :<C-U>call <SID>NextSection(+1)<CR>
  xnoremap <silent><buffer> [[ :<C-U>exe "normal! gv"<Bar>call <SID>NextSection(-1)<CR>
  xnoremap <silent><buffer> ]] :<C-U>exe "normal! gv"<Bar>call <SID>NextSection(+1)<CR>
  let b:undo_ftplugin .= '|sil! nunmap <buffer> [[|sil! nunmap <buffer> ]]|sil! xunmap <buffer> [[|sil! xunmap <buffer> ]]'
endif

function! s:NotCodeBlock(lnum) abort
  return synIDattr(synID(a:lnum, 1, 1), 'name') !=# 'markdownCodeBlock'
endfunction

function! MarkdownFold() abort
  let line = getline(v:lnum)

  if line =~# '^#\+ ' && s:NotCodeBlock(v:lnum)
    return ">" . match(line, ' ')
  endif

  let nextline = getline(v:lnum + 1)
  if (line =~ '^.\+$') && (nextline =~ '^=\+$') && s:NotCodeBlock(v:lnum + 1)
    return ">1"
  endif

  if (line =~ '^.\+$') && (nextline =~ '^-\+$') && s:NotCodeBlock(v:lnum + 1)
    return ">2"
  endif

  return "="
endfunction

function! s:HashIndent(lnum) abort
  let hash_header = matchstr(getline(a:lnum), '^#\{1,6}')
  if len(hash_header)
    return hash_header
  else
    let nextline = getline(a:lnum + 1)
    if nextline =~# '^=\+\s*$'
      return '#'
    elseif nextline =~# '^-\+\s*$'
      return '##'
    endif
  endif
endfunction

function! MarkdownFoldText() abort
  let hash_indent = s:HashIndent(v:foldstart)
  let title = substitute(getline(v:foldstart), '^#\+\s*', '', '')
  let foldsize = (v:foldend - v:foldstart + 1)
  let linecount = '['.foldsize.' lines]'
  return hash_indent.' '.title.' '.linecount
endfunction

if has("folding") && get(g:, "markdown_folding", 0)
  setlocal foldexpr=MarkdownFold()
  setlocal foldmethod=expr
  setlocal foldtext=MarkdownFoldText()
  let b:undo_ftplugin .= "|setl foldexpr< foldmethod< foldtext<"
endif

let &cpo = s:keepcpo
unlet s:keepcpo

" vim:set sw=2:
