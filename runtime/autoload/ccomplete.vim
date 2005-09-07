" Vim completion script
" Language:	C
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2005 Sep 07

function! ccomplete#Complete(findstart, base)
  if a:findstart
    " Locate the start of the item, including "." and "->".
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

  " Return list of matches.

  " Split item in words, keep empty word after "." or "->".
  " "aa" -> ['aa'], "aa." -> ['aa', ''], "aa.bb" -> ['aa', 'bb'], etc.
  let items = split(a:base, '\.\|->', 1)
  if len(items) <= 1
    " Only one part, no "." or "->": complete from tags file.
    " When local completion is wanted CTRL-N would have been used.
    return map(taglist('^' . a:base), 'v:val["name"]')
  endif

  let basetext = matchstr(a:base, '.*\(\.\|->\)')

  " Find variable locally in current function, current file or tags file.
  if searchdecl(items[0]) == 0 || searchdecl(items[0], 1) == 0
    " Found, now figure out the type.
    " TODO: join previous line if it makes sense
    let line = getline('.')
    let col = col('.')
    let res = ccomplete#Nextitem(strpart(line, 0, col), items[1:], basetext)
  else
    " Find the variable in the tags file
    let diclist = taglist('^' . items[0] . '$')

    let res = []
    for i in range(len(diclist))
      " For now we only recognize a variable.
      " The command in the tags file must be a search pattern that shows the
      " declaration of the variable.
      if diclist[i]['kind'] == 'v'
	let line = diclist[i]['cmd']
	if line[0] == '/' && line[1] == '^'
	  let line = strpart(line, 2)		" Remove /^ from the cmd
	  let col = match(line, items[0])
	  call extend(res, ccomplete#Nextitem(strpart(line, 0, col), items[1:], basetext)
	endif
      endif
    endfor
  endif

  return res
endfunc

function! ccomplete#Nextitem(lead, items, basetext)

  " Use the text up to the variable name and split it in tokens.
  let tokens = split(a:lead, '\s\+\|\<')

  " Try to recognize the type of the variable.  This is rough guessing...
  let members = []
  let taglines = []
  for tidx in range(len(tokens))

    " Recognize 'struct foobar'.
    if tokens[tidx] == 'struct' && tidx + 1 < len(tokens)
      let [members, taglines] = ccomplete#StructMembers(tokens[tidx + 1], a:items[0])
      break
    endif

    " Recognize a typedef: 'foobar_t'.
    let diclist = taglist('^' . tokens[tidx] . '$')
    for i in range(len(diclist))
      " For now we only recognize "typedef struct foobar".
      " The command in the tags file must be a search pattern that shows the
      " typedef.
      let cmd = diclist[i]['cmd']
      let ci = matchend(cmd, 'typedef\s\+struct\s\+')
      if ci > 1
	let name = matchstr(cmd, '\w*', ci)
	let [m, l] = ccomplete#StructMembers(name, a:items[0])
	call extend(members, m)
	call extend(taglines, l)
      endif
    endfor
    if len(members) > 0
      break
    endif

  endfor

  if len(members) > 0
    if len(a:items) == 1
      return map(members, 'a:basetext . v:val')
    endif

    " More items following.  For each of the possible members find the
    " matching following members.
    let res = []
    for i in range(len(members))
      let line = taglines[i]
      let memb = members[i]
      let s = match(line, '\t\zs/^')
      if s > 0
	let e = match(line, members[i], s)
	if e > 0
	  call extend(res, ccomplete#Nextitem(strpart(line, s, e - s), a:items[1:], a:basetext))
	endif
      endif
    endfor
    return res
  endif

  " Failed to find anything.
  return []
endfunction


" Return a list with two lists:
" - a list of members of structure "name" starting with string "item".
" - a list of the tag lines where the member is defined.
function! ccomplete#StructMembers(name, item)
  " Todo: Use all tags files; What about local structures?
  exe 'vimgrep /\<struct:' . a:name . '\>/j tags'

  let members = []
  let taglines = []
  for l in getqflist()
    let memb = matchstr(l['text'], '[^\t]*')
    if memb =~ '^' . a:item
      call add(members, memb)
      call add(taglines, l['text'])
    endif
  endfor
  return [members, taglines]
endfunction
