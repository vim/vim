" Vim completion script
" Language:	C
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2005 Sep 01

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
  let items = split(a:base, '\.\|->')
  if len(items) == 1
    " Only one part, no "." or "->": complete from tags file.
    let diclist = taglist(items[0])
    return map(diclist, 'v:val["name"]')
  endif
  return items
endfunction

