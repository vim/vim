" Test 'statuslineopt' with 'statusline'
"

source util/screendump.vim

func SetUp()
  set laststatus=2
  se ch=1
endfunc

func TearDown()
  set laststatus&
  set statusline&
  set statuslineopt&
  set ch&
endfunc

func s:get_statuslines(height)
  if has('gui_running')
    redraw!
    sleep 1m
  endif
  let lines = [ &lines - &ch - a:height + 1, &lines - &ch]
  return ScreenLines(lines, &columns)
endfunc

func Test_statuslineopt()
  set statuslineopt=maxheight:2
  set statusline=AAA
  let stlh = getwininfo()[0].status_height
  call assert_equal(1, stlh)
  let stls = s:get_statuslines(stlh)
  call assert_match('^AAA *', stls[0])

  set statusline=AAA%@BBB
  let stlh = getwininfo()[0].status_height
  call assert_equal(2, stlh)
  let stls = s:get_statuslines(stlh)
  call assert_match('^AAA *', stls[0])
  call assert_match('^BBB *', stls[1])

  set statusline=AAA%@BBB%@CCC
  let stlh = getwininfo()[0].status_height
  call assert_equal(2, stlh)
  let stls = s:get_statuslines(stlh)

  set statuslineopt=maxheight:3
  let stlh = getwininfo()[0].status_height
  call assert_equal(3, stlh)
  let stls = s:get_statuslines(stlh)
  call assert_match('^AAA *', stls[0])
  call assert_match('^BBB *', stls[1])
  call assert_match('^CCC *', stls[2])
endfunc

func Test_statuslineopt_fixedheight()
  set statuslineopt=maxheight:2,fixedheight
  set statusline=AAA
  let stlh = getwininfo()[0].status_height
  call assert_equal(2, stlh)
  let stls = s:get_statuslines(stlh)
  call assert_match('^AAA *', stls[0])
  call assert_match('^ *', stls[1])

  set statusline=AAA%@BBB
  let stlh = getwininfo()[0].status_height
  call assert_equal(2, stlh)
  let stls = s:get_statuslines(stlh)
  call assert_match('^AAA *', stls[0])
  call assert_match('^BBB *', stls[1])

  set statusline=AAA%@BBB%@CCC
  let stlh = getwininfo()[0].status_height
  call assert_equal(2, stlh)
  let stls = s:get_statuslines(stlh)

  set statuslineopt=maxheight:3,fixedheight
  let stlh = getwininfo()[0].status_height
  call assert_equal(3, stlh)
  let stls = s:get_statuslines(stlh)
  call assert_match('^AAA *', stls[0])
  call assert_match('^BBB *', stls[1])
  call assert_match('^CCC *', stls[2])

  set statusline=AAA%@BBB
  let stlh = getwininfo()[0].status_height
  call assert_equal(3, stlh)
  let stls = s:get_statuslines(stlh)
  call assert_match('^AAA *', stls[0])
  call assert_match('^BBB *', stls[1])
  call assert_match('^ *', stls[2])
endfunc

" vim: shiftwidth=2 sts=2 expandtab
