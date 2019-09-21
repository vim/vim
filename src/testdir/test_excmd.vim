" Tests for various Ex commands.

func Test_ex_delete()
  new
  call setline(1, ['a', 'b', 'c'])
  2
  " :dl is :delete with the "l" flag, not :dlist
  .dl
  call assert_equal(['a', 'c'], getline(1, 2))
endfunc

func Test_range_error()
  call assert_fails(':.echo 1', 'E481:')
  call assert_fails(':$echo 1', 'E481:')
  call assert_fails(':1,2echo 1', 'E481:')
  call assert_fails(':+1echo 1', 'E481:')
  call assert_fails(':/1/echo 1', 'E481:')
  call assert_fails(':\/echo 1', 'E481:')
  normal vv
  call assert_fails(":'<,'>echo 1", 'E481:')
endfunc

func Test_buffers_lastused()
  call test_settime(localtime() - 2000) " middle
  edit bufa
  enew
  call test_settime(localtime() - 10)   " newest
  edit bufb
  enew
  call test_settime(1550010000)	        " oldest
  edit bufc
  enew
  call test_settime(0)
  enew

  let ls = split(execute('buffers t', 'silent!'), '\n')
  let bufs = ls->map({i,v->split(v, '"\s*')[1:2]})
  call assert_equal(['bufb', 'bufa', 'bufc'], bufs[1:]->map({i,v->v[0]}))
  call assert_match('1[0-3] seconds ago', bufs[1][1])
  call assert_match('\d\d:\d\d:\d\d', bufs[2][1])
  call assert_match('2019/02/1\d \d\d:\d\d:00', bufs[3][1])

  bwipeout bufa
  bwipeout bufb
  bwipeout bufc
endfunc
