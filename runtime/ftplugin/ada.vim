" Vim Ada plugin file
" Language:	Ada
" Maintainer:	Neil Bird <neil@fnxweb.com>
" Last Change:	2003 May 11
" Version:	$Id$
" Look for the latest version at http://vim.sourceforge.net/
"
" Perform Ada specific completion & tagging.
"
"
" Provides mapping overrides for tag jumping that figure out the current
" Ada object and tag jump to that, not the 'simple' vim word.
" Similarly allows <Ctrl-N> matching of full-length ada entities from tags.
" Exports 'AdaWord()' function to return full name of Ada entity under the
" cursor( or at given line/column), stripping whitespace/newlines as necessary.


" Only do this when not done yet for this buffer
if exists("b:did_ftplugin")
  finish
endif

" Don't load another plugin for this buffer
let b:did_ftplugin = 1

" Temporarily set cpoptions to ensure the script loads OK
let s:cpoptions = &cpoptions
set cpo-=C


" Ada comments
setlocal comments+=O:--


" Make local tag mappings for this buffer (if not already set)
if mapcheck('<C-]>','n') == ''
  nnoremap <unique> <buffer> <C-]>    :call JumpToTag_ada('')<cr>
endif
if mapcheck('g<C-]>','n') == ''
  nnoremap <unique> <buffer> g<C-]>   :call JumpToTag_ada('','stj')<cr>
endif

if mapcheck('<C-N>','i') == ''
  inoremap <unique> <buffer> <C-N> <C-R>=<SID>AdaCompletion("\<lt>C-N>")<cr>
endif
if mapcheck('<C-P>','i') == ''
  inoremap <unique> <buffer> <C-P> <C-R>=<SID>AdaCompletion("\<lt>C-P>")<cr>
endif
if mapcheck('<C-X><C-]>','i') == ''
  inoremap <unique> <buffer> <C-X><C-]> <C-R>=<SID>AdaCompletion("\<lt>C-X>\<lt>C-]>")<cr>
endif
if mapcheck('<bs>','i') == ''
  inoremap <silent> <unique> <buffer> <bs> <C-R>=<SID>AdaInsertBackspace()<cr>
endif


" Only do this when not done yet for this buffer & matchit is used
if ! exists("b:match_words")  &&  exists("loaded_matchit")
  " The following lines enable the macros/matchit.vim plugin for
  " Ada-specific extended matching with the % key.
  let s:notend = '\%(\<end\s\+\)\@<!'
  let b:match_words=
  \ s:notend . '\<if\>:\<elsif\>:\<else\>:\<end\>\s\+\<if\>,' .
  \ s:notend . '\<case\>:\<when\>:\<end\>\s\+\<case\>,' .
  \ '\%(\<while\>.*\|\<for\>.*\|'.s:notend.'\)\<loop\>:\<end\>\s\+\<loop\>,' .
  \ '\%(\<do\>\|\<begin\>\):\<exception\>:\<end\>\s*\%($\|[;A-Z]\),' .
  \ s:notend . '\<record\>:\<end\>\s\+\<record\>'
endif


" Prevent re-load of functions
if exists('s:id')
  finish
endif

" Get this script's unique id
map <script> <SID>?? <SID>??
let s:id = substitute( maparg('<SID>??'), '^<SNR>\(.*\)_??$', '\1', '' )
unmap <script> <SID>??


" Extract current Ada word across multiple lines
" AdaWord( [line, column] )\
let s:AdaWordRegex = '\a\w*\(\_s*\.\_s*\a\w*\)*'
let s:AdaComment   = "\\v^(\"[^\"]*\"|'.'|[^\"']){-}\\zs\\s*--.*"

function! AdaWord(...)
  if a:0 > 1
    let linenr = a:1
    let colnr  = a:2 - 1
  else
    let linenr = line('.')
    let colnr  = col('.') - 1
  endif
  let line = substitute( getline(linenr), s:AdaComment, '', '' )
  " Cope with tag searching for items in comments; if we are, don't loop
  " backards looking for previous lines
  if colnr > strlen(line)
    " We were in a comment
    let line = getline(linenr)
    let search_prev_lines = 0
  else
    let search_prev_lines = 1
  endif

  " Go backwards until we find a match (Ada ID) that *doesn't* include our
  " location - i.e., the previous ID. This is because the current 'correct'
  " match will toggle matching/not matching as we traverse characters
  " backwards. Thus, we have to find the previous unrelated match, exclude
  " it, then use the next full match (ours).
  " Remember to convert vim column 'colnr' [1..n] to string offset [0..(n-1)]
  " ... but start, here, one after the required char.
  let newcol = colnr + 1
  while 1
    let newcol = newcol - 1
    if newcol < 0
      " Have to include previous line from file
      let linenr = linenr - 1
      if linenr < 1  ||  !search_prev_lines
	" Start of file or matching in a comment
	let linenr = 1
	let newcol = 0
	let ourmatch = match( line, s:AdaWordRegex )
	break
      endif
      " Get previous line, and prepend it to our search string
      let newline = substitute( getline(linenr), s:AdaComment, '', '' )
      let newcol  = strlen(newline) - 1
      let colnr   = colnr + newcol
      let line    = newline . line
    endif
    " Check to see if this is a match excluding 'us'
    let mend = newcol + matchend( strpart(line,newcol), s:AdaWordRegex ) - 1
    if mend >= newcol  &&  mend < colnr
      " Yes
      let ourmatch = mend+1 + match( strpart(line,mend+1), s:AdaWordRegex )
      break
    endif
  endwhile

  " Got anything?
  if ourmatch < 0
    return ''
  else
    let line = strpart( line, ourmatch)
  endif

  " Now simply add further lines until the match gets no bigger
  let matchstr = matchstr( line, s:AdaWordRegex )
  let lastline  = line('$')
  let linenr    = line('.') + 1
  while linenr <= lastline
    let lastmatch = matchstr
    let line = line . substitute( getline(linenr), s:AdaComment, '', '' )
    let matchstr = matchstr( line, s:AdaWordRegex )
    if matchstr == lastmatch
      break
    endif
  endwhile

  " Strip whitespace & return
  return substitute( matchstr, '\s\+', '', 'g' )
endfunction


" Word tag - include '.' and if Ada make uppercase
" Name allows a common JumpToTag() to look for an ft specific JumpToTag_ft().
function! JumpToTag_ada(word,...)
  if a:word == ''
    " Get current word
    let word = AdaWord()
    if word == ''
      return
    endif
  else
    let word = a:word
  endif
  if a:0 > 0
    let mode = a:1
  else
    let mode = 'tj'
  endif

  let v:errmsg = ''
  execute 'silent!' mode word
  if v:errmsg != ''
    if v:errmsg =~ '^E426:'  " Tag not found
      let ignorecase = &ignorecase
      set ignorecase
      execute mode word
      let &ignorecase = ignorecase
    else
      " Repeat to give error
      execute mode word
    endif
  endif
endfunction


" Word completion (^N/^R/^X^]) - force '.' inclusion
function! s:AdaCompletion(cmd)
  set iskeyword+=46
  return a:cmd . "\<C-R>=<SNR>" . s:id . "_AdaCompletionEnd()\<CR>"
endfunction
function! s:AdaCompletionEnd()
  set iskeyword-=46
  return ''
endfunction


" Backspace at end of line after auto-inserted commentstring '-- ' wipes it
function! s:AdaInsertBackspace()
  let line = getline('.')
  if col('.') > strlen(line) && match(line,'-- $') != -1 && match(&comments,'--') != -1
    return "\<bs>\<bs>\<bs>"
  else
    return "\<bs>"
  endif
endfunction


" Reset cpoptions
let &cpoptions = s:cpoptions
unlet s:cpoptions

" vim: sts=2 sw=2 :
