" Vim completion script
" Language:	C
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2005 Sep 05

function! ccomplete#Complete(findstart, base)
  if a:findstart
    " locate the start of the word
    let line = getline('.')
    let start = col('.') - 1
    while start > 0
      if line[start - 1] =~ '\w\|\.'
	let start -= 1
      elseif start > 1 && line[start - 2] == '-' && line[start - 1] == '>'
	let start -= 2
      else
	break
      endif
    endwhile
    return start
  endif

  " return list of matches
  if a:base !~ '\.\|->'
    " Only one part, no "." or "->": complete from tags file.
    let diclist = taglist(a:base)
    return map(diclist, 'v:val["name"]')
  endif

  " Find variable locally in function or file.
  let items = split(a:base, '\.\|->')

  " At the moment we only do "aa.bb", not "aa.bb.cc"
  if len(items) > 2
    return []
  endif

  let line = ''
  if searchdecl(items[0]) == 0 || searchdecl(items[0], 1) == 0
    " Found, now figure out the type.
    " TODO: join previous line if it makes sense
    let line = getline('.')
    let col = col('.')
  else
    " Find the variable in the tags file
    let diclist = taglist(items[0])
    for i in range(len(diclist))
      " For now we only recognize a variable.
      if diclist[i]['kind'] == 'v'
	let line = diclist[i]['cmd']
	if line[0] == '/' && line[1] == '^'
	  " the command is a search pattern, remove the leading /^
	  let line = strpart(line, 2)
	endif
	let col = match(line, items[0])
	break
      endif
    endfor
  endif

  if line == ''
    return []
  endif

  " Is there a * before the variable name?
  let col -= 1
  let star = 0
  while col > 0
    let col -= 1
    if line[col] == '*'
      let star = 1
    elseif line[col] !~ '\s'
      break
    endif
  endwhile

  " Use the line up to the variable name and split it in tokens.
  let lead = strpart(line, 0, col + 1)
  let tokens = split(lead, '\s\+\|\<')

  let basetext = matchstr(a:base, '.*\.\|->')

  for i in range(len(tokens) - 1)
    if tokens[i] == 'struct'
      let name = tokens[i + 1]
      " Todo: Use all tags files; What about local structures?
      exe 'vimgrep /\<struct:' . name . '\>/j tags'
      let res = []
      for l in getqflist()
	let memb = matchstr(l['text'], '[^\t]*')
	if len(items) == 1 || memb =~ '^' . items[1]
	  call add(res, basetext . memb)
	endif
      endfor
      return res
    endif
  endfor

  return tokens
endfunction

