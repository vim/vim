" Vim rpgle autoload file
" Language:             Free-Form ILE RPG
" Maintainer:           Andreas Louv <andreas@louv.dk>
" Last Change:          Mar 14, 2023
" Version:              1

function rpgle#NextSection(motion, flags, mode) range abort
  let cnt = v:count1
  let old_pos = line('.')

  if a:mode ==# 'x'
    normal! gv
  endif

  normal! m`0

  while cnt > 0
    call search(a:motion, a:flags . 'W')
    if old_pos == line('.')
      execute 'norm!' a:flags =~# 'b' ? 'gg' : 'G'
    endif
    let old_pos = line('.')
    let cnt = cnt - 1
  endwhile

  normal! ^
endfunction

function rpgle#NextNest(flags) abort
  let flags = a:flags
  let fn = a:flags ==# 'b' ? 'max' : 'min'

  " We can get the list from "b:match_words" and just use first and last of each group
  let poss = split(b:match_words, ',')
           \ ->map({ key, val -> s:nextNestSearch(split(val, ':'), flags) })
           \ ->filter({ key, val -> val > 0 })

  let new_pos = call(fn, [poss])

  if new_pos > 0
    execute 'normal! ' . new_pos . 'G^'
  endif
endfunction

function s:nextNestSearch(kw, flags) abort
  if a:kw[0] =~? 'if'
    let middle = '\<\(else\|elseif\)\>'
  elseif a:kw[0] =~? 'select'
    let middle = '\<\(when\|other\)\>'
  else
    let middle = ''
  endif

  return s:findpair(a:kw[0], middle, a:kw[-1], a:flags)
endfunction

function s:findpair(start, middle, end, flags) abort
  " Find a pair which isn't inside a string nor comment
  return searchpair(a:start, a:middle, a:end, a:flags . 'nW',
                  \ 'synIDattr(synID(line("."), col("."), 1), "name") =~? "string\\|comment"')
endfunction

function rpgle#Operator(ai) abort
  let pairs = split(b:match_words, ',')
            \ ->map({ key, val -> [split(val, ':')[0], split(val, ':')[-1]] })

  " Find a pair which isn't inside a string nor comment
  let poss = pairs
           \ ->map({ key, val -> {"pair": val, "pos": s:findpair(val[0], '', val[1], 'b')} })
           \ ->filter({ key, val -> val.pos > 0 })

  let closest = { "index": 0, "pos": -1 }
  let index = 0
  for pos in poss
    if pos.pos > closest.pos
      let closest.pos = pos.pos
      let closest.index = index
      let closest.pair = pos.pair
    endif
    let index = index + 1
  endfor

  let match_words = b:match_words
  let b:match_words = join(closest.pair, ':')
  execute 'normal! ' . closest.pos . 'G^V'
  normal %
  let b:match_words = match_words

  if a:ai == 'i'
    normal! koj
  endif
endfunction
