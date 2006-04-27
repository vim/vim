" Vim plugin for showing matching parens
" Maintainer:  Bram Moolenaar <Bram@vim.org>
" Last Change: 2006 Apr 27

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

" Skip the rest if it was already done.
if exists("*s:Highlight_Matching_Pair")
  finish
endif

let cpo_save = &cpo
set cpo-=C

" The function that is invoked (very often) to define a ":match" highlighting
" for any matching paren.
function! s:Highlight_Matching_Pair()
  " Remove any previous match.
  if exists('w:paren_hl_on') && w:paren_hl_on
    3match none
    let w:paren_hl_on = 0
  endif

  " Avoid that we remove the popup menu.
  if pumvisible()
    return
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
  " And avoid searching very far (e.g., for closed folds and long lines)
  if i % 2 == 0
    let s_flags = 'nW'
    let c2 = plist[i + 1]
    if has("byte_offset") && has("syntax_items") && &smc > 0
      let stopbyte = min([line2byte("$"), line2byte(".") + col(".") + &smc * 2])
      let stopline = min([line('w$'), byte2line(stopbyte)])
    else
      let stopline = min([line('w$'), c_lnum + 100])
    endif
  else
    let s_flags = 'nbW'
    let c2 = c
    let c = plist[i - 1]
    if has("byte_offset") && has("syntax_items") && &smc > 0
      let stopbyte = max([1, line2byte(".") + col(".") - &smc * 2])
      let stopline = max([line('w0'), byte2line(stopbyte)])
    else
      let stopline = max([line('w0'), c_lnum - 100])
    endif
  endif
  if c == '['
    let c = '\['
    let c2 = '\]'
  endif

  " Find the match.  When it was just before the cursor move it there for a
  " moment.
  if before > 0
    let save_cursor = getpos('.')
    call cursor(c_lnum, c_col - before)
  endif

  " When not in a string or comment ignore matches inside them.
  let s_skip ='synIDattr(synID(line("."), col("."), 0), "name") ' .
	\ '=~?  "string\\|comment"'
  execute 'if' s_skip '| let s_skip = 0 | endif'

  let [m_lnum, m_col] = searchpairpos(c, '', c2, s_flags, s_skip, stopline)

  if before > 0
    call setpos('.', save_cursor)
  endif

  " If a match is found setup match highlighting.
  if m_lnum > 0 && m_lnum >= line('w0') && m_lnum <= line('w$')
    exe '3match MatchParen /\(\%' . c_lnum . 'l\%' . (c_col - before) .
	  \ 'c\)\|\(\%' . m_lnum . 'l\%' . m_col . 'c\)/'
    let w:paren_hl_on = 1
  endif
endfunction

" Define commands that will disable and enable the plugin.
command! NoMatchParen 3match none | unlet! g:loaded_matchparen | au! matchparen
command! DoMatchParen runtime plugin/matchparen.vim | doau CursorMoved

let &cpo = cpo_save
