" Vim plugin for showing matching parens
" Maintainer:  Bram Moolenaar <Bram@vim.org>
" Last Change: 2006 Mar 03

" Exit quickly when:
" - this plugin was already loaded (or disabled)
" - when 'compatible' is set
" - the "CursorMoved" autocmd event is not availble.
if exists("g:loaded_matchparen") || &cp || !exists("##CursorMoved")
  finish
endif
let g:loaded_matchparen = 1

augroup matchparen
  " Replace all matchparen autocommands
  autocmd! CursorMoved,CursorMovedI * call s:Highlight_Matching_Pair()
augroup END

let s:paren_hl_on = 0

" Skip the rest if it was already done.
if exists("*s:Highlight_Matching_Pair")
  finish
endif

" The function that is invoked (very often) to define a ":match" highlighting
" for any matching paren.
function! s:Highlight_Matching_Pair()
  " Remove any previous match.
  if s:paren_hl_on
    3match none
    let s:paren_hl_on = 0
  endif

  " Get the character under the cursor and check if it's in 'matchpairs'.
  let c_lnum = line('.')
  let c_col = col('.')
  let before = 0

  let c = getline(c_lnum)[c_col - 1]
  let plist = split(&matchpairs, ':\|,')
  let i = index(plist, c)
  if i < 0
    " not found, in Insert mode try character before the cursor
    if c_col > 1 && (mode() == 'i' || mode() == 'R')
      let before = 1
      let c = getline(c_lnum)[c_col - 2]
      let i = index(plist, c)
    endif
    if i < 0
      " not found, nothing to do
      return
    endif
  endif

  " Figure out the arguments for searchpairpos().
  " Restrict the search to visible lines with "stopline".
  if i % 2 == 0
    let s_flags = 'nW'
    let c2 = plist[i + 1]
    let stopline = line('w$')
  else
    let s_flags = 'nbW'
    let c2 = c
    let c = plist[i - 1]
    let stopline = line('w0')
  endif
  if c == '['
    let c = '\['
    let c2 = '\]'
  endif

  " When not in a string or comment ignore matches inside them.
  let s_skip ='synIDattr(synID(c_lnum, c_col - before, 0), "name") ' .
	\ '=~?  "string\\|comment"'
  execute 'if' s_skip '| let s_skip = 0 | endif'

  " Find the match.  When it was just before the cursor move it there for a
  " moment.
  if before > 0
    let save_cursor = getpos('.')
    call cursor(c_lnum, c_col - before)
  endif
  let [m_lnum, m_col] = searchpairpos(c, '', c2, s_flags, s_skip, stopline)
  if before > 0
    call setpos('.', save_cursor)
  endif

  " If a match is found setup match highlighting.
  if m_lnum > 0 && m_lnum >= line('w0') && m_lnum <= line('w$')
    exe '3match MatchParen /\(\%' . c_lnum . 'l\%' . (c_col - before) .
	  \ 'c\)\|\(\%' . m_lnum . 'l\%' . m_col . 'c\)/'
    let s:paren_hl_on = 1
  endif
endfunction

" Define commands that will disable and enable the plugin.
command! NoMatchParen 3match none | unlet! g:loaded_matchparen | au! matchparen
command! DoMatchParen runtime plugin/matchparen.vim | doau CursorMoved
