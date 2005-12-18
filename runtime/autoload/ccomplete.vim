" Vim completion script
" Language:	C
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2005 Dec 18


" This function is used for the 'omnifunc' option.
function! ccomplete#Complete(findstart, base)
  if a:findstart
    " Locate the start of the item, including "." and "->".
    let line = getline('.')
    let start = col('.') - 1
    let lastword = -1
    while start > 0
      if line[start - 1] =~ '\w'
	let start -= 1
      elseif line[start - 1] =~ '\.'
	if lastword == -1
	  let lastword = start
	endif
	let start -= 1
      elseif start > 1 && line[start - 2] == '-' && line[start - 1] == '>'
	if lastword == -1
	  let lastword = start
	endif
	let start -= 2
      else
	break
      endif
    endwhile

    " Return the column of the last word, which is going to be changed.
    " Remember the text that comes before it in s:prepended.
    if lastword == -1
      let s:prepended = ''
      return start
    endif
    let s:prepended = strpart(line, start, lastword - start)
    return lastword
  endif

  " Return list of matches.

  let base = s:prepended . a:base

  " Split item in words, keep empty word after "." or "->".
  " "aa" -> ['aa'], "aa." -> ['aa', ''], "aa.bb" -> ['aa', 'bb'], etc.
  let items = split(base, '\.\|->', 1)
  if len(items) <= 1
    " Don't do anything for an empty base, would result in all the tags in the
    " tags file.
    if base == ''
      return []
    endif

    " Only one part, no "." or "->": complete from tags file.
    " When local completion is wanted CTRL-N would have been used.
    return map(taglist('^' . base), 'v:val["name"]')
  endif

  " Find the variable items[0].
  " 1. in current function (like with "gd")
  " 2. in tags file(s) (like with ":tag")
  " 3. in current file (like with "gD")
  let res = []
  if searchdecl(items[0], 0, 1) == 0
    " Found, now figure out the type.
    " TODO: join previous line if it makes sense
    let line = getline('.')
    let col = col('.')
    let res = s:Nextitem(strpart(line, 0, col), items[1:])
  endif

  if len(res) == 0
    " Find the variable in the tags file(s)
    let diclist = taglist('^' . items[0] . '$')

    let res = []
    for i in range(len(diclist))
      " New ctags has the "typename" field.
      if has_key(diclist[i], 'typename')
	call extend(res, s:StructMembers(diclist[i]['typename'], items[1:]))
      endif

      " For a variable use the command, which must be a search pattern that
      " shows the declaration of the variable.
      if diclist[i]['kind'] == 'v'
	let line = diclist[i]['cmd']
	if line[0] == '/' && line[1] == '^'
	  let col = match(line, '\<' . items[0] . '\>')
	  call extend(res, s:Nextitem(strpart(line, 2, col - 2), items[1:]))
	endif
      endif
    endfor
  endif

  if len(res) == 0 && searchdecl(items[0], 1) == 0
    " Found, now figure out the type.
    " TODO: join previous line if it makes sense
    let line = getline('.')
    let col = col('.')
    let res = s:Nextitem(strpart(line, 0, col), items[1:])
  endif

  " If the one and only match was what's already there and it is a composite
  " type, add a "." or "->".
  if len(res) == 1 && res[0]['match'] == items[-1] && len(s:SearchMembers(res, [''])) > 0
    " If there is a '*' before the name use "->".
    if match(res[0]['tagline'], '\*\s*' . res[0]['match']) > 0
      let res[0]['match'] .= '->'
    else
      let res[0]['match'] .= '.'
    endif
  endif

  return map(res, 'v:val["match"]')
endfunc

" Find composing type in "lead" and match items[0] with it.
" Repeat this recursively for items[1], if it's there.
" Return the list of matches.
function! s:Nextitem(lead, items)

  " Use the text up to the variable name and split it in tokens.
  let tokens = split(a:lead, '\s\+\|\<')

  " Try to recognize the type of the variable.  This is rough guessing...
  let res = []
  for tidx in range(len(tokens))

    " Recognize "struct foobar" and "union foobar".
    if (tokens[tidx] == 'struct' || tokens[tidx] == 'union') && tidx + 1 < len(tokens)
      let res = s:StructMembers(tokens[tidx] . ':' . tokens[tidx + 1], a:items)
      break
    endif

    " TODO: add more reserved words
    if index(['int', 'float', 'static', 'unsigned', 'extern'], tokens[tidx]) >= 0
      continue
    endif

    " Use the tags file to find out if this is a typedef.
    let diclist = taglist('^' . tokens[tidx] . '$')
    for tagidx in range(len(diclist))
      " New ctags has the "typename" field.
      if has_key(diclist[tagidx], 'typename')
	call extend(res, s:StructMembers(diclist[tagidx]['typename'], a:items))
	continue
      endif

      " Only handle typedefs here.
      if diclist[tagidx]['kind'] != 't'
	continue
      endif

      " For old ctags we recognize "typedef struct aaa" and
      " "typedef union bbb" in the tags file command.
      let cmd = diclist[tagidx]['cmd']
      let ei = matchend(cmd, 'typedef\s\+')
      if ei > 1
	let cmdtokens = split(strpart(cmd, ei), '\s\+\|\<')
	if len(cmdtokens) > 1
	  if cmdtokens[0] == 'struct' || cmdtokens[0] == 'union'
	    let name = ''
	    " Use the first identifier after the "struct" or "union"
	    for ti in range(len(cmdtokens) - 1)
	      if cmdtokens[ti] =~ '^\w'
		let name = cmdtokens[ti]
		break
	      endif
	    endfor
	    if name != ''
	      call extend(res, s:StructMembers(cmdtokens[0] . ':' . name, a:items))
	    endif
	  else
	    " Could be "typedef other_T some_T".
	    call extend(res, s:Nextitem(cmdtokens[0], a:items))
	  endif
	endif
      endif
    endfor
    if len(res) > 0
      break
    endif
  endfor

  return res
endfunction


" Return a list with resulting matches.
" Each match is a dictionary with "match" and "tagline" entries.
function! s:StructMembers(typename, items)
  " Todo: What about local structures?
  let fnames = join(map(tagfiles(), 'escape(v:val, " \\")'))
  if fnames == ''
    return []
  endif

  let typename = a:typename
  let qflist = []
  while 1
    exe 'silent! vimgrep /\t' . typename . '\(\t\|$\)/j ' . fnames
    let qflist = getqflist()
    if len(qflist) > 0 || match(typename, "::") < 0
      break
    endif
    " No match for "struct:context::name", remove "context::" and try again.
    let typename = substitute(typename, ':[^:]*::', ':', '')
  endwhile

  let matches = []
  for l in qflist
    let memb = matchstr(l['text'], '[^\t]*')
    if memb =~ '^' . a:items[0]
      call add(matches, {'match': memb, 'tagline': l['text']})
    endif
  endfor

  if len(matches) > 0
    " No further items, return the result.
    if len(a:items) == 1
      return matches
    endif

    " More items following.  For each of the possible members find the
    " matching following members.
    return s:SearchMembers(matches, a:items[1:])
  endif

  " Failed to find anything.
  return []
endfunction

" For matching members, find matches for following items.
function! s:SearchMembers(matches, items)
  let res = []
  for i in range(len(a:matches))
    let line = a:matches[i]['tagline']
    let e = matchend(line, '\ttypename:')
    if e > 0
      " Use typename field
      let name = matchstr(line, '[^\t]*', e)
      call extend(res, s:StructMembers(name, a:items))
    else
      " Use the search command (the declaration itself).
      let s = match(line, '\t\zs/^')
      if s > 0
	let e = match(line, a:matches[i]['match'], s)
	if e > 0
	  call extend(res, s:Nextitem(strpart(line, s, e - s), a:items))
	endif
      endif
    endif
  endfor
  return res
endfunc
