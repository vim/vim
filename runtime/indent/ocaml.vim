" Vim indent file
" Language:	OCaml
" Maintainers:	Jean-Francois Yuen  <jfyuen@ifrance.com>
"		Mike Leary	    <leary@nwlink.com>
"		Markus Mottl	    <markus@oefai.at>
" URL:		http://www.oefai.at/~markus/vim/indent/ocaml.vim
" Last Change:	2003 Apr 14
"		2003 Mar 05 - Added '{<' and some fixes (JY)
"		2002 Nov 06 - Some fixes (JY)

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
  finish
endif
let b:did_indent = 1

setlocal expandtab
setlocal indentexpr=GetOCamlIndent()
setlocal indentkeys+=0=and,0=constraint,0=done,0=else,0=end,0=exception,0=external,0=if,0=in,0=include,0=inherit,0=initializer,0=let,0=method,0=open,0=then,0=type,0=val,0=with,0=;;,0=>\],0=\|\],0=\|,0=*),0=>},0},0\],0)
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
let s:beflet = '^\s*\(initializer\|method\|try\)\|\(\<\(begin\|do\|else\|in\|then\|try\)\|->\|;\|(\)\s*$'
let s:letpat = '^\s*\(let\|type\|module\|class\|open\|exception\|val\|include\|external\)\>'
let s:letlim = '\(\<\(sig\|struct\)\|;;\)\s*$'
let s:lim = '^\s*\(exception\|external\|include\|let\|module\|open\|type\|val\)\>'
let s:module = '\<\%(begin\|sig\|struct\|object\)\>'
let s:obj = '^\s*\(constraint\|inherit\|initializer\|method\|val\)\>\|\<\(object\|object\s*(.*)\)\s*$'
let s:type = '^\s*\%(let\|type\)\>.*='
let s:val = '^\s*\(val\|external\)\>.*:'

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
  return indent(searchpair(a:pstart, a:pmid, a:pend, 'bWn', 'synIDattr(synID(line("."), col("."), 0), "name") =~? "string\\|comment" || getline(".") =~ "^\\s*let\\>.*=.*\\<in\\s*$" || getline(prevnonblank(".") - 1) =~ "^\\s*let\\>.*=\\s*$\\|" . s:beflet'))
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

  " Indent if current line begins with 'end'
  " for 'sig', 'struct', 'object' and 'begin':
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

  " Indent if current line begins with 'let'
  " and last line does not begin with 'let' or end with 'in' or ';;':
  elseif line =~ '^\s*let\>'
    if lline !~ s:lim . '\|' . s:letlim . '\|' . s:beflet
      return s:FindLet(s:type, '','\<let\s*$')
    else return ind
    endif

  " Indent if current line begins with 'type'
  " and last line does not end with 'and' or ';;':
  elseif line =~ '^\s*type\>'
    if lline !~ s:lim . '\|\<and\s*$\|' . s:letlim
      return s:FindLet(s:type, '','\<type\s*$')
    else return ind
    endif

  " Indent for pattern matching:
  elseif line =~ '^\s*|'
    if lline !~ '^\s*\(|\|\(match\|with\|type\)\>\)\|\<\(function\|parser\|with\)\s*$'
      call search('|', 'bW')
      return indent(searchpair('^\s*\(type\|match\)\>\|\<\(with\|function\|parser\)\s*$', '', '|', 'bWn', 'synIDattr(synID(line("."), col("."), 0), "name") =~? "string\\|comment" || getline(".") =~ "\\[|\\||\\]" && getline(".") !~ "^\\s*|.*->"'))
    else return ind
    endif

  " Indent if current line begins with ';;':
  elseif line =~ '^\s*;;'
    if lline !~ ';;\s*$'
      return s:GetInd(v:lnum, s:letpat, s:letlim)
    else return ind
    endif

  " Indent if current line begins with 'in' and previous
  " line does not start with 'let' or 'and':
  elseif line =~ '^\s*in\>'
    if lline !~ '^\s*\(let\|and\)\>'
      return s:FindPair('\<let\>', '', '\<in\>')
    else return ind
    endif

  " Indent if current line begins with 'else'
  " and previous line does not start with 'if', 'then' or 'else':
  elseif line =~ '^\s*else\>'
    if lline !~ '^\s*\(if\|else\|then\)\>'
      return s:FindPair('\<if\>', '', '\<else\>')
    else return ind
    endif

  " Indent if current line begins with 'then'
  " and previous line does not start with 'if', 'then' or 'else':
  elseif line =~ '^\s*then\>'
    if lline !~ '^\s*\(if\|else\|then\)\>'
      return s:FindPair('\<if\>', '', '\<then\>')
    else return ind
    endif

  " Subtract a 'shiftwidth' if current line begins with 'and' and previous
  " line does not start with 'let', 'and' or 'type' or end with 'end'
  " (for classes):
  elseif line =~ '^\s*and\>'
    if lline !~ '^\s*\(and\|let\|type\)\>\|\<end\s*$'
      return ind - &sw
    else return ind
    endif

  " Indent if current line begins with 'with'
  " and previous line does not start with 'match' or 'try':
  elseif line =~ '^\s*with\>'
    if lline !~ '^\s*\(match\|try\)\>'
      return s:FindPair('\<\%(match\|try\)\>', '','\<with\>')
    else return ind
    endif

  " Indent if current line begins with 'exception':
  elseif line =~ '^\s*exception\>'
    if lline !~ s:lim . '\|' . s:letlim
      return indent(search(s:val . '\|^\s*\(external\|include\|open\|type\)\>', 'bW'))
    else return ind
    endif

  " Indent if current line begins with 'external':
  elseif line =~ '^\s*external\>'
    if lline !~ s:lim . '\|' . s:letlim
      return indent(search(s:val . '\|^\s*\(exception\|include\|open\|type\)\>', 'bW'))
    else return ind
    endif

  " Indent if current line begins with 'include':
  elseif line =~ '^\s*include\>'
    if lline !~ s:lim . '\|' . s:letlim
      return indent(search(s:val . '\|^\s*\(exception\|external\|open\|type\)\>', 'bW'))
    else return ind
    endif

  " Indent if current line begins with 'open':
  elseif line =~ '^\s*open\>'
    if lline !~ s:lim . '\|' . s:letlim
      return indent(search(s:val . '\|^\s*\(exception\|external\|include\|type\)\>', 'bW'))
    else return ind
    endif

  " Indent if current line begins with 'val':
  elseif line =~ '^\s*val\>'
    if lline !~ '^\s*\(exception\|external\|include\|open\)\>\|' . s:obj . '\|' . s:letlim
      return indent(search(s:val . '\|^\s*\(exception\|include\|initializer\|method\|open\|type\)\>', 'bW'))
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

  " Indent back to normal after comments:
  elseif line =~ '^\s*\*)'
    call search('\*)', 'bW')
    return indent(searchpair('(\*', '', '\*)', 'bWn', 'synIDattr(synID(line("."), col("."), 0), "name") =~? "string"'))

  endif

  " Add a 'shiftwidth' after lines ending with:
  if lline =~ '\(:\|=\|->\|<-\|(\|\[\|{\|{<\|\[|\|\[<\|\<\(begin\|struct\|sig\|functor\|initializer\|object\|try\|do\|if\|then\|else\|fun\|function\|parser\)\|\<object\s*(.*)\)\s*$'
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
