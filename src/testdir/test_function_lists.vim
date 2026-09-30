" Test to verify that the three function lists:
"
"  - global_functions[] in src/evalfunc.c
"  - *functions* in runtime/doc/builtin.txt
"  - *function-list* in runtime/doc/usr_41.txt
"
" contain the same functions and that the global_functions and
" ":help functions" lists are in ASCII order.  Also that the arguments in
" global_functions[] are the ones in the help.

func Test_function_lists()

  " Delete any files left over from an earlier run of this test.
  call delete("Xglobal_functions.diff")
  call delete("Xfunctions.diff")
  call delete("Xfunction-list.diff")

  " Create a file of the functions in evalfunc.c:global_functions[].
  enew!
  read ../evalfunc.c
  1,/^static const funcentry_T global_functions\[\] =$/d
  call search('^};$')
  .,$d
  v/^    {/d
  %s/^    {"//
  %s/".*//
  w! Xglobal_functions

  " Verify that those functions are in ASCII order.
  sort u
  w! Xsorted_global_functions
  let l:unequal = assert_equalfile("Xsorted_global_functions", "Xglobal_functions",
      \ "global_functions[] not sorted")
  if l:unequal && executable("diff")
    call system("diff -u Xsorted_global_functions Xglobal_functions > Xglobal_functions.diff")
  endif

  " Create a file of the functions in evalfunc.c:global_functions[] that are
  " not obsolete, sorted in ASCII order.
  enew!
  read ../evalfunc.c
  1,/^static const funcentry_T global_functions\[\] =$/d
  call search('^};$')
  .,$d
  v/^    {/d
  g/\/\/ obsolete$/d
  %s/^    {"//
  %s/".*//
  sort u
  w! ++ff=unix Xsorted_current_global_functions

  " Verify that the ":help functions" list is complete and in ASCII order.
  enew!
  if filereadable('../../doc/builtin.txt')
    " unpacked MS-Windows zip archive
    read ../../doc/builtin.txt
  else
    read ../../runtime/doc/builtin.txt
  endif
  call search('^USAGE')
  1,.d
  call search('^==========')
  .,$d
  v/^\S/d
  %s/(.*//
  let l:lines = getline(1, '$')
  call uniq(l:lines)
  call writefile(l:lines, "Xfunctions")
  let l:unequal = assert_equalfile("Xsorted_current_global_functions", "Xfunctions",
      \ "\":help functions\" not sorted or incomplete")
  if l:unequal && executable("diff")
    call system("diff -u Xsorted_current_global_functions Xfunctions > Xfunctions.diff")
  endif

  " Verify that the ":help function-list" list is complete.
  enew!
  if filereadable('../../doc/usr_41.txt')
    " unpacked MS-Windows zip archive
    read ../../doc/usr_41.txt
  else
    read ../../runtime/doc/usr_41.txt
  endif
  call search('\*function-list\*$')
  1,.d
  call search('^==*$')
  .,$d
  v/^\t\S/d
  %s/(.*//
  %left
  sort u
  w! ++ff=unix Xfunction-list
  let l:unequal = assert_equalfile("Xsorted_current_global_functions", "Xfunction-list",
      \ "\":help function-list\" incomplete")
  if l:unequal && executable("diff")
    call system("diff -u Xsorted_current_global_functions Xfunction-list > Xfunction-list.diff")
  endif

  " Clean up.
  call delete("Xglobal_functions")
  call delete("Xsorted_global_functions")
  call delete("Xsorted_current_global_functions")
  call delete("Xfunctions")
  call delete("Xfunction-list")
  enew!

endfunc

" Parse the arguments of a help signature, "{lnum}, {col} [, {off}]", into a
" Dict: "names" (a value such as the 1 of getreg() and a List literal have
" ''), "min" (the required arguments), "max" (-1 for "..."), "rest" (TRUE
" for "[...]", the shorthand of the overview for more optional arguments) and
" "bad" (TRUE for unbalanced brackets or a stray parenthesis).
func s:ParseHelpArgs(args)
  let r = #{names: [], min: 0, max: 0, rest: 0, bad: 0}
  let optional = 0
  let depth = 0
  let s = a:args
  while 1
    let comma = s =~ '^\s*,'
    let s = substitute(s, '^[ ,]*', '', '')
    if s == ''
      break
    endif
    if s =~ '^\[\.\.\.\]'
      let r.rest = 1
      let s = s[5:]
      continue
    endif
    if s[0] == '['
      if comma && !empty(r.names)
        " A List literal after a comma is an argument without a name.
        let nested = 0
        let i = 0
        while i < len(s)
          let nested += s[i] == '[' ? 1 : s[i] == ']' ? -1 : 0
          let i += 1
          if nested == 0
            break
          endif
        endwhile
        let s = s[i:]
      else
        let optional = 1
        let depth += 1
        let s = s[1:]
        continue
      endif
    elseif s[0] == ']'
      let depth -= 1
      let s = s[1:]
      continue
    elseif s =~ '^\.\.\.'
      let r.max = -1
      let s = s[3:]
      continue
    elseif s =~ '^{'
      call add(r.names, substitute(matchstr(s, '{\zs[^}]*'), '-', '_', 'g'))
      let s = substitute(s, '^{[^}]*}', '', '')
    elseif s =~ '^[()]'
      let r.bad = 1
      let s = s[1:]
      continue
    else
      call add(r.names, '')
      let s = substitute(s, '^[^], ]*', '', '')
    endif
    if !optional
      let r.min += 1
    endif
    if r.max >= 0
      let r.max += 1
    endif
  endwhile
  if depth != 0
    let r.bad = 1
  endif
  " Two arguments with the same name are numbered: {expr}, {expr} is expr1
  " and expr2.
  for name in copy(r.names)
    if name != '' && count(r.names, name) > 1
      let n = 1
      for i in range(len(r.names))
        if r.names[i] == name
          let r.names[i] = name .. n
          let n += 1
        endif
      endfor
    endif
  endfor
  return r
endfunc

" Combine the forms of one function, a List of what s:ParseHelpArgs()
" returned, into what the table must have: the names of the form with the
" most arguments, the least required arguments, the most arguments.
func s:CombineForms(forms)
  let r = #{names: [], min: a:forms[0].min, max: 0, rest: 0, bad: 0}
  for f in a:forms
    if len(f.names) > len(r.names)
      let r.names = f.names
    endif
    let r.min = min([r.min, f.min])
    let r.max = f.max < 0 || r.max < 0 ? -1 : max([r.max, f.max])
    let r.rest = r.rest || f.rest
    let r.bad = r.bad || f.bad
  endfor
  return r
endfunc

" Compare the help forms "forms" of function "name" with what getinfo()
" reports in "info".  "where" names the help in the messages.  The overview
" abbreviates the names, they are only checked in the entry.
func s:CheckForms(name, forms, info, where)
  let h = s:CombineForms(a:forms)
  call assert_false(h.bad, a:name .. '() brackets in ' .. a:where)
  if a:where != 'the overview'
    let names = map(copy(a:info.args), {_, arg -> get(arg, 'name', '')})
    call assert_equal(h.names, slice(names, 0, len(h.names)),
          \ a:name .. '() argument names in ' .. a:where)
  endif
  call assert_equal(h.min, a:info.minargs,
        \ a:name .. '() required arguments in ' .. a:where)
  if h.rest
    return
  endif
  if h.max < 0
    call assert_true(a:info.maxargs < 0 || a:info.maxargs > len(h.names),
          \ a:name .. '() takes more arguments in ' .. a:where)
  else
    call assert_equal(h.max, a:info.maxargs,
          \ a:name .. '() maximum number of arguments in ' .. a:where)
  endif
endfunc

" The lines of help file "fname".
func s:HelpLines(fname)
  " "../../doc" is the unpacked MS-Windows zip archive.
  let dir = filereadable('../../doc/tags') ? '../../doc/' : '../../runtime/doc/'
  return readfile(dir .. a:fname)
        \ ->map({_, line -> substitute(line, "\r$", '', '')})
endfunc

" Test that the arguments of global_functions[], as getinfo() reports
" them, are the ones in the help: how many are required and how many there
" are, in the overview and in the entry of the function, and the names in
" the entry.
func Test_function_arguments()
  " The forms in the overview: the lines that start in the first column, up
  " to the end of the section.
  let overview = {}
  let lines = s:HelpLines('builtin.txt')
  let start = match(lines, '^USAGE')
  let end = match(lines, '^==========', start)
  for line in lines[start : end]
    let m = matchlist(line, '^\(\l\w*\)(\(.*\)')
    if !empty(m)
      " The arguments end at the parenthesis that closes the first one; it
      " must be followed by white space or the end of the line.
      let args = matchstr(m[2], '^\%([^()]\|([^()]*)\)*')
      if m[2][len(args) :] !~ '^)\%(\s\|$\)'
        let args ..= ')'
      endif
      if !has_key(overview, m[1])
        let overview[m[1]] = []
      endif
      call add(overview[m[1]], s:ParseHelpArgs(args))
    endif
  endfor

  " The forms in the entry of each function: the line with the "*name()*"
  " tag, or next to it, and every later line starting with "name(".  The tags
  " file gives the help file of the entry.
  let files = {}
  for line in s:HelpLines('tags')
    let m = matchlist(line, '^\(\l\w*\)()\t\(\S*\)')
    if !empty(m)
      let files[m[1]] = m[2]
    endif
  endfor
  let details = {}
  for fname in uniq(sort(values(files)))
    let lines = s:HelpLines(fname)
    let i = 0
    while i < len(lines)
      let name = matchstr(lines[i], '^\l\w*\ze(')
      if name == '' || get(files, name, '') != fname
        let i += 1
        continue
      endif
      let sig = lines[i]
      let j = i
      while count(sig, '(') > count(sig, ')') && j + 1 < len(lines)
        let j += 1
        let sig ..= ' ' .. trim(lines[j])
      endwhile
      let tag = '*' .. name .. '()*'
      let tagged = has_key(details, name)
            \ || stridx(sig, tag) >= 0
            \ || (i > 0 && stridx(lines[i - 1], tag) >= 0)
            \ || (j + 1 < len(lines) && stridx(lines[j + 1], tag) >= 0)
      " After the arguments only tags may follow, not text.
      let pat = '^\l\w*(\zs\%([^()]\|([^()]*)\)*\ze)\s*\%(\*\|$\)'
      if tagged && sig =~ pat
        if !has_key(details, name)
          let details[name] = []
        endif
        call add(details[name], s:ParseHelpArgs(matchstr(sig, pat)))
      endif
      let i = j + 1
    endwhile
  endfor

  for name in sort(keys(overview))
    let info = getinfo('function', name)
    if empty(info)
      call assert_report(name .. '() is not a builtin function')
      continue
    endif
    call s:CheckForms(name, overview[name], info, 'the overview')
    if !has_key(details, name)
      call assert_report(name .. '() has no help entry')
      continue
    endif
    call s:CheckForms(name, details[name], info, files[name])
  endfor
endfunc

" vim: shiftwidth=2 sts=2 expandtab
