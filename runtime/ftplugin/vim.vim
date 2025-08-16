" Vim filetype plugin
" Language:          Vim
" Maintainer:        Doug Kearns <dougkearns@gmail.com>
" Former Maintainer: Bram Moolenaar <Bram@vim.org>
" Contributors:      Riley Bruins <ribru17@gmail.com> ('commentstring')
"                    @Konfekt
"                    @tpope (s:Help())
"                    @lacygoill
" Last Change:       2025 Aug 07
" 2025 Aug 06 by Vim Project (add gf maps #17881)
" 2025 Aug 08 by Vim Project (add Vim script complete function #17871)
" 2025 Aug 12 by Vim Project (improve vimgoto script #17970))
" 2025 Aug 16 by Vim Project set com depending on Vim9 or legacy script

" Only do this when not done yet for this buffer
if exists("b:did_ftplugin")
  finish
endif

" Don't load another plugin for this buffer
let b:did_ftplugin = 1

let s:cpo_save = &cpo
set cpo&vim

if !exists('*VimFtpluginUndo')
  func VimFtpluginUndo()
    setl fo< isk< com< tw< commentstring< include< define< keywordprg< omnifunc<
    sil! delc -buffer VimKeywordPrg
    if exists('b:did_add_maps')
      silent! nunmap <buffer> [[
      silent! xunmap <buffer> [[
      silent! nunmap <buffer> ]]
      silent! xunmap <buffer> ]]
      silent! nunmap <buffer> []
      silent! xunmap <buffer> []
      silent! nunmap <buffer> ][
      silent! xunmap <buffer> ][
      silent! nunmap <buffer> ]"
      silent! xunmap <buffer> ]"
      silent! nunmap <buffer> ["
      silent! xunmap <buffer> ["
      silent! nunmap <buffer> gf
      silent! nunmap <buffer> <C-W>f
      silent! nunmap <buffer> <C-W>gf
    endif
    unlet! b:match_ignorecase b:match_words b:match_skip b:did_add_maps
  endfunc
endif

let b:undo_ftplugin = "call VimFtpluginUndo()"

" Set 'formatoptions' to break comment lines but not other lines,
" and insert the comment leader when hitting <CR> or using "o".
setlocal fo-=t fo+=croql

" To allow tag lookup via CTRL-] for autoload functions, '#' must be a
" keyword character.  E.g., for netrw#Nread().
setlocal isk+=#

" Use :help to lookup the keyword under the cursor with K.
" Distinguish between commands, options and functions.
if !exists("*" .. expand("<SID>") .. "Help")
  function s:Help(topic) abort
    let topic = a:topic

    " keyword is not necessarily under the cursor, see :help K
    let line = getline('.')
    let i = match(line, '\V' .. escape(topic, '\'), col('.') - len(topic))
    let pre = strpart(line, 0, i)
    let post = strpart(line, i + len(topic))

    " local/global option vars
    if topic =~# '[lg]' && pre ==# '&' && post =~# ':\k\+'
      let topic = matchstr(post, '\k\+')
    endif

    if get(g:, 'syntax_on', 0)
      let syn = synIDattr(synID(line('.'), col('.'), 1), 'name')
      if syn ==# 'vimFuncName'
        return topic .. '()'
      elseif syn ==# 'vimOption' || syn ==# 'vimOptionVarName'
        return "'" .. topic .. "'"
      elseif syn ==# 'vimUserCmdAttrKey'
        return ':command-' .. topic
      elseif syn ==# 'vimCommand'
        return ':' .. topic
      endif
    endif

    if pre =~# '^\s*:\=$' || pre =~# '\%(\\\||\)\@<!|\s*:\=$'
      return ':' .. topic
    elseif pre =~# '\<v:$'
      return 'v:' .. topic
    elseif pre =~# '<$'
      return '<' .. topic .. '>'
    elseif pre =~# '\\$'
      return '/\' .. topic
    elseif topic ==# 'v' && post =~# ':\w\+'
      return 'v' .. matchstr(post, ':\w\+')
    elseif pre =~# '&\%([lg]:\)\=$'
      return "'" .. topic .. "'"
    else
      return topic
    endif
  endfunction
endif
command! -buffer -nargs=1 VimKeywordPrg :exe 'help' s:Help(<q-args>)
setlocal keywordprg=:VimKeywordPrg

" Comments starts with # in Vim9 script.  We have to guess which one to use.
if "\n" .. getline(1, 32)->join("\n") =~# '\n\s*vim9\%[script]\>'
  setlocal commentstring=#\ %s
  " Set 'comments' to format dashed lists in comments, for Vim9 script.
  setlocal com=sO:#\ -,mO:#\ \ ,eO:##,:#\\\ ,:#
else
  setlocal commentstring=\"%s
  " Set 'comments' to format dashed lists in comments, for legacy Vim script.
  setlocal com=sO:\"\ -,mO:\"\ \ ,eO:\"\",:\"\\\ ,:\"
endif

" set 'include' to recognize import commands
setlocal include=\\v^\\s*import\\s*(autoload)?

" set 'define' to recognize export commands
setlocal define=\\v^\\s*export\\s*(def\|const\|var\|final)

if has("vim9script")
  " set omnifunc completion
  setlocal omnifunc=vimcomplete#Complete
endif

" Format comments to be up to 78 characters long
if &tw == 0
  setlocal tw=78
endif

if !exists("no_plugin_maps") && !exists("no_vim_maps")
  let b:did_add_maps = 1

  " Move around functions.
  nnoremap <silent><buffer> [[ m':call search('^\s*\(fu\%[nction]\\|\(export\s\+\)\?def\)\>', "bW")<CR>
  xnoremap <silent><buffer> [[ m':<C-U>exe "normal! gv"<Bar>call search('^\s*\(fu\%[nction]\\|\(export\s\+\)\?def\)\>', "bW")<CR>
  nnoremap <silent><buffer> ]] m':call search('^\s*\(fu\%[nction]\\|\(export\s\+\)\?def\)\>', "W")<CR>
  xnoremap <silent><buffer> ]] m':<C-U>exe "normal! gv"<Bar>call search('^\s*\(fu\%[nction]\\|\(export\s\+\)\?def\)\>', "W")<CR>
  nnoremap <silent><buffer> [] m':call search('^\s*end\(f\%[unction]\\|\(export\s\+\)\?def\)\>', "bW")<CR>
  xnoremap <silent><buffer> [] m':<C-U>exe "normal! gv"<Bar>call search('^\s*end\(f\%[unction]\\|\(export\s\+\)\?def\)\>', "bW")<CR>
  nnoremap <silent><buffer> ][ m':call search('^\s*end\(f\%[unction]\\|\(export\s\+\)\?def\)\>', "W")<CR>
  xnoremap <silent><buffer> ][ m':<C-U>exe "normal! gv"<Bar>call search('^\s*end\(f\%[unction]\\|\(export\s\+\)\?def\)\>', "W")<CR>

  " Move around comments
  nnoremap <silent><buffer> ]" :call search('\%(^\s*".*\n\)\@<!\%(^\s*"\)', "W")<CR>
  xnoremap <silent><buffer> ]" :<C-U>exe "normal! gv"<Bar>call search('\%(^\s*".*\n\)\@<!\%(^\s*"\)', "W")<CR>
  nnoremap <silent><buffer> [" :call search('\%(^\s*".*\n\)\%(^\s*"\)\@!', "bW")<CR>
  xnoremap <silent><buffer> [" :<C-U>exe "normal! gv"<Bar>call search('\%(^\s*".*\n\)\%(^\s*"\)\@!', "bW")<CR>

  " Purpose: Handle :import, :colorscheme and  :packadd lines in a smarter way. {{{
  "
  " `:import` is followed by a filename or filepath.  Find it.
  "
  " `:packadd`  is  followed  by the  name  of  a  package,  which we  might  have
  " configured in scripts under `~/.vim/plugin`.  Find it.
  "
  " ---
  "
  " We can't handle the `:import` lines simply by setting `'includeexpr'`, because
  " the option would be ignored if:
  "
  "    - the name of the imported script is the same as the current one
  "    - `'path'` includes the `.` item
  "
  " Indeed,  in that  case, Vim  finds the  current file,  and simply  reloads the
  " buffer.
  " }}}
  " We use the `F` variants, instead of the `f` ones, because they're smarter.
  " See $VIMRUNTIME/autoload/vimgoto.vim
  nnoremap <silent><buffer> gf :<C-U>call vimgoto#Find('gF')<CR>
  nnoremap <silent><buffer> <C-W>f :<C-U>call vimgoto#Find("\<lt>C-W>F")<CR>
  nnoremap <silent><buffer> <C-W>gf :<C-U>call vimgoto#Find("\<lt>C-W>gF")<CR>
endif

" Let the matchit plugin know what items can be matched.
if exists("loaded_matchit")
  let b:match_ignorecase = 0
  " "func" can also be used as a type:
  "   var Ref: func
  " or to list functions:
  "   func name
  " require a parenthesis following, then there can be an "endfunc".
  let b:match_words =
  \ '\<\%(fu\%[nction]\|def\)!\=\s\+\S\+\s*(:\%(\%(^\||\)\s*\)\@<=\<retu\%[rn]\>:\%(\%(^\||\)\s*\)\@<=\<\%(endf\%[unction]\|enddef\)\>,' ..
  \ '\<\%(wh\%[ile]\|for\)\>:\%(\%(^\||\)\s*\)\@<=\<brea\%[k]\>:\%(\%(^\||\)\s*\)\@<=\<con\%[tinue]\>:\%(\%(^\||\)\s*\)\@<=\<end\%(w\%[hile]\|fo\%[r]\)\>,' ..
  \ '\<if\>:\%(\%(^\||\)\s*\)\@<=\<el\%[seif]\>:\%(\%(^\||\)\s*\)\@<=\<en\%[dif]\>,' ..
  \ '{:},' ..
  \ '\<try\>:\%(\%(^\||\)\s*\)\@<=\<cat\%[ch]\>:\%(\%(^\||\)\s*\)\@<=\<fina\%[lly]\>:\%(\%(^\||\)\s*\)\@<=\<endt\%[ry]\>,' ..
  \ '\<aug\%[roup]\s\+\%(END\>\)\@!\S:\<aug\%[roup]\s\+END\>,' ..
  \ '\<class\>:\<endclass\>,' ..
  \ '\<interface\>:\<endinterface\>,' ..
  \ '\<enum\>:\<endenum\>'

  " Ignore syntax region commands and settings, any 'en*' would clobber
  " if-endif.
  " - set spl=de,en
  " - au! FileType javascript syntax region foldBraces start=/{/ end=/}/ …
  " Also ignore here-doc and dictionary keys (vimVar).
  let b:match_skip = 'synIDattr(synID(line("."), col("."), 1), "name")
                    \ =~? "comment\\|string\\|vimSynReg\\|vimSet\\|vimLetHereDoc\\|vimVar"'
endif

let &cpo = s:cpo_save
unlet s:cpo_save

" removed this, because 'cpoptions' is a global option.
" setlocal cpo+=M		" makes \%( match \)
"
" vim: sw=2 et
