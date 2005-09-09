" Vim completion script
" Language:	C
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2005 Sep 09


" This function is used for the 'occultfunc' option.
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

  " Find the variable items[0].
  " 1. in current function (like with "gd")
  " 2. in tags file(s) (like with ":tag")
  " 3. in current file (like with "gD")
  let res = []
  if searchdecl(items[0]) == 0
    " Found, now figure out the type.
    " TODO: join previous line if it makes sense
    let line = getline('.')
    let col = col('.')
    let res = ccomplete#Nextitem(strpart(line, 0, col), items[1:])
  endif

  if len(res) == 0
    " Find the variable in the tags file(s)
    let diclist = taglist('^' . items[0] . '$')

    let res = []
    for i in range(len(diclist))
      " New ctags has the "typename" field.
      if has_key(diclist[i], 'typename')
	call extend(res, ccomplete#StructMembers(diclist[i]['typename'], items[1:]))
      endif

      " For a variable use the command, which must be a search pattern that
      " shows the declaration of the variable.
      if diclist[i]['kind'] == 'v'
	let line = diclist[i]['cmd']
	if line[0] == '/' && line[1] == '^'
	  let col = match(line, items[0])
	  call extend(res, ccomplete#Nextitem(strpart(line, 2, col - 2), items[1:])
	endif
      endif
    endfor
  endif

  if len(res) == 0 && searchdecl(items[0], 1) == 0
    " Found, now figure out the type.
    " TODO: join previous line if it makes sense
    let line = getline('.')
    let col = col('.')
    let res = ccomplete#Nextitem(strpart(line, 0, col), items[1:])
  endif

  " The basetext is up to the last "." or "->" and won't be changed.  The
  " matching members are concatenated to this.
  let basetext = matchstr(a:base, '.*\(\.\|->\)')
  return map(res, 'basetext . v:val')
endfunc

" Find composing type in "lead" and match items[0] with it.
" Repeat this recursively for items[1], if it's there.
" Return the list of matches.
function! ccomplete#Nextitem(lead, items)

  " Use the text up to the variable name and split it in tokens.
  let tokens = split(a:lead, '\s\+\|\<')

  " Try to recognize the type of the variable.  This is rough guessing...
  let res = []
  for tidx in range(len(tokens))

    " Recognize "struct foobar" and "union foobar".
    if (tokens[tidx] == 'struct' || tokens[tidx] == 'union') && tidx + 1 < len(tokens)
      let res = ccomplete#StructMembers(tokens[tidx] . ':' . tokens[tidx + 1], a:items)
      break
    endif

    " TODO: add more reserved words
    if index(['int', 'float', 'static', 'unsigned', 'extern'], tokens[tidx]) >= 0
      continue
    endif

    " Use the tags file to find out if this is a typedef.
    let diclist = taglist('^' . tokens[tidx] . '$')
    for i in range(len(diclist))
      " New ctags has the "typename" field.
      if has_key(diclist[i], 'typename')
	call extend(res, ccomplete#StructMembers(diclist[i]['typename'], a:items))
	continue
      endif

      " For old ctags we only recognize "typedef struct foobar" in the tags
      " file command.
      let cmd = diclist[i]['cmd']
      let ci = matchend(cmd, 'typedef\s\+struct\s\+')
      if ci > 1
	let name = matchstr(cmd, '\w*', ci)
	call extend(res, ccomplete#StructMembers('struct:' . name, a:items))
      endif
    endfor
    if len(res) > 0
      break
    endif
  endfor

  return res
endfunction


" Return a list with resulting matches
function! ccomplete#StructMembers(typename, items)
  " Todo: What about local structures?
  let fnames = join(map(tagfiles(), 'escape(v:val, " \\")'))
  if fnames == ''
    return [[], []]
  endif

  let typename = a:typename
  let qflist = []
  while 1
    exe 'silent! vimgrep /\t' . typename . '\>/j ' . fnames
    let qflist = getqflist()
    if len(qflist) > 0 || match(typename, "::") < 0
      break
    endif
    " No match for "struct:context::name", remove "context::" and try again.
    let typename = substitute(typename, ':[^:]*::', ':', '')
  endwhile

  let members = []
  let taglines = []
  for l in qflist
    let memb = matchstr(l['text'], '[^\t]*')
    if memb =~ '^' . a:items[0]
      call add(members, memb)
      call add(taglines, l['text'])
    endif
  endfor

  if len(members) > 0
    " No further items, return the result.
    if len(a:items) == 1
      return members
    endif

    " More items following.  For each of the possible members find the
    " matching following members.
    let res = []
    for i in range(len(members))
      let line = taglines[i]
      let e = matchend(line, '\ttypename:')
      if e > 0
	" Use typename field
	let name = matchstr(line, '[^\t]*', e)
	call extend(res, ccomplete#StructMembers(name, a:items[1:]))
      else
	let s = match(line, '\t\zs/^')
	if s > 0
	  let e = match(line, members[i], s)
	  if e > 0
	    call extend(res, ccomplete#Nextitem(strpart(line, s, e - s), a:items[1:]))
	  endif
	endif
      endif
    endfor
    return res
  endif

  " Failed to find anything.
  return []
endfunction
