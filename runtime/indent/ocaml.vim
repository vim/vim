" Vim indent file
" Language:     OCaml
" Maintainers:  Jean-Francois Yuen  <jfyuen@happycoders.org>
"               Mike Leary          <leary@nwlink.com>
"               Markus Mottl        <markus@oefai.at>
" URL:          http://www.oefai.at/~markus/vim/indent/ocaml.vim
" Last Change:  2004 Apr 11 - Added indent for 'class' (JY)
"               2003 Sep 16 - Added 'private' as keyword (JY)
"               2003 Mar 29 - Fixed bug with 'if' and 'else' (JY)

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
  finish
endif
let b:did_indent = 1

setlocal expandtab
setlocal indentexpr=GetOCamlIndent()
setlocal indentkeys+=0=and,0=class,0=constraint,0=done,0=else,0=end,0=exception,0=external,0=if,0=in,0=include,0=inherit,0=initializer,0=let,0=method,0=open,0=then,0=type,0=val,0=with,0;;,0>\],0\|\],0>},0\|,0},0\],0)
setlocal nolisp
setlocal nosmartindent
setlocal textwidth=80

" Comment formatting
if (has("comments"))
  setlocal comments=sr:(*,mb:*,ex:*)
  setlocal fo=cqort
endif

" Only define the function once.
if exists("*GetOCamlIndent")
  finish
endif

" Define some patterns:
let s:beflet = '^\s*\(initializer\|method\|try\)\|\(\<\(begin\|do\|else\|in\|then\|try\)\|->\|<-\|=\|;\|(\)\s*$'
let s:letpat = '^\s*\(let\|type\|module\|class\|open\|exception\|val\|include\|external\)\>'
let s:letlim = '\(\<\(sig\|struct\)\|;;\)\s*$'
let s:lim = '^\s*\(exception\|external\|include\|let\|module\|open\|type\|val\)\>'
let s:module = '\<\%(begin\|sig\|struct\|object\)\>'
let s:obj = '^\s*\(constraint\|inherit\|initializer\|method\|val\)\>\|\<\(object\|object\s*(.*)\)\s*$'
let s:type = '^\s*\%(class\|let\|type\)\>.*='

" Skipping pattern, for comments
function s:SkipPattern(lnum, pat)
  let def = prevnonblank(a:lnum - 1)
  while def > 0 && getline(def) =~ a:pat
    let def = prevnonblank(def - 1)
  endwhile
  return def
endfunction

" Indent for ';;' to match multiple 'let'
function s:GetInd(lnum, pat, lim)
  let llet = search(a:pat, 'bW')
  let old = indent(a:lnum)
  while llet > 0
    let old = indent(llet)
    let nb = s:SkipPattern(llet, '^\s*(\*.*\*)\s*$')
    if getline(nb) =~ a:lim
      return old
    endif
    let llet = search(a:pat, 'bW')
  endwhile
  return old
endfunction

" Indent pairs
function s:FindPair(pstart, pmid, pend)
  call search(a:pend, 'bW')
  return indent(searchpair(a:pstart, a:pmid, a:pend, 'bWn', 'synIDattr(synID(line("."), col("."), 0), "name") =~? "string\\|comment"'))
endfunction

" Indent 'let'
function s:FindLet(pstart, pmid, pend)
  call search(a:pend, 'bW')
  return indent(searchpair(a:pstart, a:pmid, a:pend, 'bWn', 'synIDattr(synID(line("."), col("."), 0), "name") =~? "string\\|comment" || getline(".") =~ "^\\s*let\\>.*=.*\\<in\\s*$" || getline(prevnonblank(".") - 1) =~ s:beflet'))
endfunction

function GetOCamlIndent()
  " Find a non-blank line above the current line.
  let lnum = prevnonblank(v:lnum - 1)

  " At the start of the file use zero indent.
  if lnum == 0
    return 0
  endif

  let ind = indent(lnum)
  let lline = getline(lnum)

  " Return double 'shiftwidth' after lines matching:
  if lline =~ '^\s*|.*->\s*$'
    return ind + &sw + &sw
  endif

  let line = getline(v:lnum)

  " Indent if current line begins with 'end':
  if line =~ '^\s*end\>'
    return s:FindPair(s:module, '','\<end\>')

  " Indent if current line begins with 'done' for 'do':
  elseif line =~ '^\s*done\>'
    return s:FindPair('\<do\>', '','\<done\>')

  " Indent if current line begins with '}' or '>}':
  elseif line =~ '^\s*\(\|>\)}'
    return s:FindPair('{', '','}')

  " Indent if current line begins with ']', '|]' or '>]':
  elseif line =~ '^\s*\(\||\|>\)\]'
    return s:FindPair('\[', '','\]')

  " Indent if current line begins with ')':
  elseif line =~ '^\s*)'
    return s:FindPair('(', '',')')

  " Indent if current line begins with 'let':
  elseif line =~ '^\s*let\>'
    if lline !~ s:lim . '\|' . s:letlim . '\|' . s:beflet
      return s:FindLet(s:type, '','\<let\s*$')
    else return ind
    endif

  " Indent if current line begins with 'class' or 'type':
  elseif line =~ '^\s*\(class\|type\)\>'
    if lline !~ s:lim . '\|\<and\s*$\|' . s:letlim
      return s:FindLet(s:type, '','\<\(class\|type\)\s*$')
    else return ind
    endif

  " Indent for pattern matching:
  elseif line =~ '^\s*|'
    if lline !~ '^\s*\(|[^\]]\|\(match\|type\|with\)\>\)\|\<\(function\|parser\|private\|with\)\s*$'
      call search('|', 'bW')
      return indent(searchpair('^\s*\(match\|type\)\>\|\<\(function\|parser\|private\|with\)\s*$', '', '^\s*|', 'bWn', 'synIDattr(synID(line("."), col("."), 0), "name") =~? "string\\|comment" || getline(".") !~ "^\\s*|.*->"'))
    else return ind
    endif

  " Indent if current line begins with ';;':
  elseif line =~ '^\s*;;'
    if lline !~ ';;\s*$'
      return s:GetInd(v:lnum, s:letpat, s:letlim)
    else return ind
    endif

  " Indent if current line begins with 'in':
  elseif line =~ '^\s*in\>'
    if lline !~ '^\s*\(let\|and\)\>'
      return s:FindPair('\<let\>', '', '\<in\>')
    else return ind
    endif

  " Indent if current line begins with 'else':
  elseif line =~ '^\s*else\>'
    if lline !~ '^\s*\(if\|then\)\>'
      return s:FindPair('\<if\>', '', '\<else\>')
    else return ind
    endif

  " Indent if current line begins with 'then':
  elseif line =~ '^\s*then\>'
    if lline !~ '^\s*\(if\|else\)\>'
      return s:FindPair('\<if\>', '', '\<then\>')
    else return ind
    endif

  " Indent if current line begins with 'and':
  elseif line =~ '^\s*and\>'
    if lline !~ '^\s*\(and\|let\|type\)\>\|\<end\s*$'
      return ind - &sw
    else return ind
    endif

  " Indent if current line begins with 'with':
  elseif line =~ '^\s*with\>'
    if lline !~ '^\s*\(match\|try\)\>'
      return s:FindPair('\<\%(match\|try\)\>', '','\<with\>')
    else return ind
    endif

  " Indent if current line begins with 'exception':
  elseif line =~ '^\s*exception\>'
    if lline !~ s:lim . '\|' . s:letlim
      return indent(search('^\s*\(\(external\|include\|open\|type\)\>\|val\>.*:\)', 'bW'))
    else return ind
    endif

  " Indent if current line begins with 'external':
  elseif line =~ '^\s*external\>'
    if lline !~ s:lim . '\|' . s:letlim
      return indent(search('^\s*\(\(exception\|external\|include\|open\|type\)\>\|val\>.*:\)', 'bW'))
    else return ind
    endif

  " Indent if current line begins with 'include':
  elseif line =~ '^\s*include\>'
    if lline !~ s:lim . '\|' . s:letlim
      return indent(search('^\s*\(\(exception\|external\|open\|type\)\>\|val\>.*:\)', 'bW'))
    else return ind
    endif

  " Indent if current line begins with 'open':
  elseif line =~ '^\s*open\>'
    if lline !~ s:lim . '\|' . s:letlim
      return indent(search('^\s*\(\(exception\|external\|include\|type\)\>\|val\>.*:\)', 'bW'))
    else return ind
    endif

  " Indent if current line begins with 'val':
  elseif line =~ '^\s*val\>'
    if lline !~ '^\s*\(exception\|external\|include\|open\)\>\|' . s:obj . '\|' . s:letlim
      return indent(search('^\s*\(\(exception\|include\|initializer\|method\|open\|type\|val\)\>\|external\>.*:\)', 'bW'))
    else return ind
    endif

  " Indent if current line begins with 'constraint':
  elseif line =~ '^\s*constraint\>'
    if lline !~ s:obj
      return indent(search('^\s*\(inherit\|initializer\|method\|val\)\>', 'bW'))
    else return ind
    endif

  " Indent if current line begins with 'inherit':
  elseif line =~ '^\s*inherit\>'
    if lline !~ s:obj
      return indent(search('^\s*\(constraint\|initializer\|method\|val\)\>', 'bW'))
    else return ind
    endif

  " Indent if current line begins with 'inherit':
  elseif line =~ '^\s*initializer\>'
    if lline !~ s:obj
      return indent(search('^\s*\(constraint\|inherit\|method\|val\)\>', 'bW'))
    else return ind
    endif

  " Indent if current line begins with 'method':
  elseif line =~ '^\s*method\>'
    if lline !~ s:obj
      return indent(search('^\s*\(\(constraint\|inherit\|initializer\|val\)\>\|method\>.*\(:\|=\)\)', 'bW'))
    else return ind
    endif

  endif

  " Add a 'shiftwidth' after lines ending with:
  if lline =~ '\(:\|=\|->\|<-\|(\|\[\|{\|{<\|\[|\|\[<\|\<\(begin\|do\|else\|fun\|function\|functor\|if\|initializer\|object\|parser\|private\|sig\|struct\|then\|try\)\|\<object\s*(.*)\)\s*$'
    let ind = ind + &sw

  " Back to normal indent after lines ending with ';;':
  elseif lline =~ ';;\s*$' && lline !~ '^\s*;;'
    let ind = s:GetInd(v:lnum, s:letpat, s:letlim)

  " Back to normal indent after lines ending with 'end':
  elseif lline =~ '\<end\s*$'
    let ind = s:FindPair(s:module, '','\<end\>')

  " Back to normal indent after lines ending with 'in':
  elseif lline =~ '\<in\s*$' && lline !~ '^\s*in\>'
    let ind = s:FindPair('\<let\>', '', '\<in\>')

  " Back to normal indent after lines ending with 'done':
  elseif lline =~ '\<done\s*$'
    let ind = s:FindPair('\<do\>', '','\<done\>')

  " Back to normal indent after lines ending with '}' or '>}':
  elseif lline =~ '\(\|>\)}\s*$'
    let ind = s:FindPair('{', '','}')

  " Back to normal indent after lines ending with ']', '|]' or '>]':
  elseif lline =~ '\(\||\|>\)\]\s*$'
    let ind = s:FindPair('\[', '','\]')

  " Back to normal indent after comments:
  elseif lline =~ '\*)\s*$'
    call search('\*)', 'bW')
    let ind = indent(searchpair('(\*', '', '\*)', 'bWn', 'synIDattr(synID(line("."), col("."), 0), "name") =~? "string"'))

  " Back to normal indent after lines ending with ')':
  elseif lline =~ ')\s*$'
    let ind = s:FindPair('(', '',')')

  endif

  " Subtract a 'shiftwidth' after lines matching 'match ... with parser':
  if lline =~ '^\s*match\>.*\<with\>\s*\<parser\s*$'
    let ind = ind - &sw
  endif

  return ind

endfunction

" vim:sw=2
