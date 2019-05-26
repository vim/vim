" Tests for search_stats, when "S" is not in 'shortmess'
"
" This test is fragile, it might not work interactively, but it works when run
" as test!

source shared.vim

func! Test_search_stat()
  new
  set shortmess-=S
  " Append 50 lines with text to search for, "foobar" appears 20 times
  call append(0, repeat(['foobar', 'foo', 'fooooobar', 'foba', 'foobar'], 10))

  " match at second line
  call cursor(1, 1)
  let messages_before = execute('messages')
  let @/ = 'fo*\(bar\?\)\?'
  let g:a = execute(':unsilent :norm! n')
  let stat = {'current': 2, 'count': 50, 'wrap': 0}
  let stat_re = '\[2/50\]'
  call assert_equal(stat, v:searchstat)
  let pat = escape(@/, '()*?'). '\s\+'
  call assert_match(pat .. stat_re, g:a)
  " didn't get added to message history
  call assert_equal(messages_before, execute('messages'))

  " Match at last line
  call cursor(line('$')-2, 1)
  let g:a = execute(':unsilent :norm! n')
  let stat = {'current': 50, 'count': 50, 'wrap': 0}
  let stat_re = '\[50/50\]'
  call assert_equal(stat, v:searchstat)
  call assert_match(pat .. stat_re, g:a)

  " No search stat
  set shortmess+=S
  call cursor(1, 1)
  " previous v:searchstat
  let stat = {'current': 50, 'count': 50, 'wrap': 0}
  let stat_re = '\[2/50\]'
  let g:a = execute(':unsilent :norm! n')
  call assert_equal(stat, v:searchstat)
  call assert_notmatch(pat .. stat_re, g:a)
  set shortmess-=S

  " Many matches
  call cursor(line('$')-2, 1)
  let @/ = '.'
  let pat = escape(@/, '()*?'). '\s\+'
  let g:a = execute(':unsilent :norm! n')
  let stat = {'current': 100, 'count': 100, 'wrap': 0}
  let stat_re = '\[>99/>99\]'
  call assert_equal(stat, v:searchstat)
  call assert_match(pat .. stat_re, g:a)
  call cursor(line('$'), 1)
  let g:a = execute(':unsilent :norm! n')
  let stat = {'current': 1, 'count': 100, 'wrap': 1}
  let stat_re = '\[1/>99\] W'
  call assert_equal(stat, v:searchstat)
  call assert_match(pat .. stat_re, g:a)

  " Many matches
  call cursor(1, 1)
  let g:a = execute(':unsilent :norm! n')
  let stat = {'current': 2, 'count': 100, 'wrap': 0}
  let stat_re = '\[2/>99\]'
  call assert_equal(stat, v:searchstat)
  call assert_match(pat .. stat_re, g:a)
  call cursor(1, 1)
  let g:a = execute(':unsilent :norm! N')
  let stat = {'current': 100, 'count': 100, 'wrap': 1}
  let stat_re = '\[>99/>99\] W'
  call assert_equal(stat, v:searchstat)
  call assert_match(pat .. stat_re, g:a)

  " right-left
  if exists("+rightleft")
    set rl
    call cursor(1,1)
    let @/ = 'foobar'
    let pat = 'raboof/\s\+'
    let g:a = execute(':unsilent :norm! n')
    let stat = {'current': 2, 'count': 20, 'wrap': 0}
    let stat_re = '\[20/2\]'
    call assert_equal(stat, v:searchstat)
    call assert_match(pat .. stat_re, g:a)
    set norl
  endif

  " right-left bottom
  if exists("+rightleft")
    set rl
    call cursor('$',1)
    let pat = 'raboof?\s\+'
    let g:a = execute(':unsilent :norm! N')
    let stat = {'current': 20, 'count': 20, 'wrap': 0}
    let stat_re = '\[20/20\]'
    call assert_equal(stat, v:searchstat)
    call assert_match(pat .. stat_re, g:a)
    set norl
  endif

  " right-left back at top
  if exists("+rightleft")
    set rl
    call cursor('$',1)
    let pat = 'raboof/\s\+'
    let g:a = execute(':unsilent :norm! n')
    let stat = {'current': 1, 'count': 20, 'wrap': 1}
    let stat_re = '\[20/1\] W'
    call assert_equal(stat, v:searchstat)
    call assert_match(pat .. stat_re, g:a)
    call assert_match('search hit BOTTOM, continuing at TOP', g:a)
    set norl
  endif

  " normal, back at bottom
  call cursor(1,1)
  let @/ = 'foobar'
  let pat = '?foobar\s\+'
  let g:a = execute(':unsilent :norm! N')
  let stat = {'current': 20, 'count': 20, 'wrap': 1}
  let stat_re = '\[20/20\] W'
  call assert_equal(stat, v:searchstat)
  call assert_match(pat .. stat_re, g:a)
  call assert_match('search hit TOP, continuing at BOTTOM', g:a)
  call assert_match('\[20/20\] W', Screenline(&lines))

  " normal, no match
  call cursor(1,1)
  let @/ = 'zzzzzz'
  let g:a = ''
  try
    let g:a = execute(':unsilent :norm! n')
  catch /^Vim\%((\a\+)\)\=:E486/
    let stat = {}
    let stat_re = ''
    " error message is not redir'ed to g:a, it is empty
    call assert_true(empty(g:a))
  catch
    call assert_false(1)
  endtry

  " with count
  call cursor(1, 1)
  let @/ = 'fo*\(bar\?\)\?'
  let g:a = execute(':unsilent :norm! 2n')
  let stat = '\[3/50\]'
  let pat = escape(@/, '()*?'). '\s\+'
  call assert_match(pat .. stat, g:a)
  let g:a = execute(':unsilent :norm! 2n')
  let stat = '\[5/50\]'
  call assert_match(pat .. stat, g:a)

  " with offset
  call cursor(1, 1)
  call feedkeys("/fo*\\(bar\\?\\)\\?/+1\<cr>", 'tx')
  let g:a = execute(':unsilent :norm! n')
  let stat = '\[5/50\]'
  let pat = escape(@/ .. '/+1', '()*?'). '\s\+'
  call assert_match(pat .. stat, g:a)

  " normal, n comes from a mapping
  "     Need to move over more than 64 lines to trigger char_avail(.
  nnoremap n nzv
  call cursor(1,1)
  call append(50, repeat(['foobar', 'foo', 'fooooobar', 'foba', 'foobar'], 10))
  call setline(2, 'find this')
  call setline(70, 'find this')
  let @/ = 'find this'
  let pat = '/find this\s\+'
  let g:a = execute(':unsilent :norm n')
  " g:a will contain several lines
  let g:b = split(g:a, "\n")[-1]
  let stat = {'current': 1, 'count': 2, 'wrap': 0}
  let stat_re = '\[1/2\]'
  call assert_equal(stat, v:searchstat)
  call assert_match(pat .. stat_re, g:b)
  unmap n

  " normal, but silent
  call cursor(1,1)
  let @/ = 'find this'
  let pat = '/find this\s\+'
  let g:a = execute(':norm! n')
  let stat = {'current': 1, 'count': 2, 'wrap': 0}
  let stat_re = '\[1/2\]'
  " v:searchstat is updated also in silent
  call assert_equal(stat, v:searchstat)
  call assert_notmatch(pat .. stat_re, g:a)

  " close the window
  set shortmess+=S
  bwipe!
endfunc
