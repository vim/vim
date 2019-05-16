" tests for listener_add() and listener_remove()

func s:StoreList(s, l)
  let s:start = a:s
  let s:text = getline(a:s)
  let s:list = a:l
endfunc

func s:AnotherStoreList(l)
  let s:list2 = a:l
endfunc

func s:EvilStoreList(l)
  let s:list3 = a:l
  call assert_fails("call add(a:l, 'myitem')", "E742:")
endfunc

func Test_listening()
  new
  call setline(1, ['one', 'two'])
  let s:list = []
  let id = listener_add({b, s, e, a, l -> s:StoreList(s, l)})
  call setline(1, 'one one')
  call listener_flush()
  call assert_equal([{'lnum': 1, 'end': 2, 'col': 1, 'added': 0}], s:list)

  " Undo is also a change
  set undolevels&  " start new undo block
  call append(2, 'two two')
  undo
  redraw
  " the two changes get merged
  call assert_equal([{'lnum': 3, 'end': 4, 'col': 1, 'added': 0}], s:list)
  1

  " Two listeners, both get called.  Also check column.
  call setline(1, ['one one', 'two'])
  call listener_flush()
  let id2 = listener_add({b, s, e, a, l -> s:AnotherStoreList(l)})
  let s:list = []
  let s:list2 = []
  exe "normal $asome\<Esc>"
  redraw
  call assert_equal([{'lnum': 1, 'end': 2, 'col': 8, 'added': 0}], s:list)
  call assert_equal([{'lnum': 1, 'end': 2, 'col': 8, 'added': 0}], s:list2)

  " removing listener works
  call listener_remove(id2)
  call setline(1, ['one one', 'two'])
  call listener_flush()
  let s:list = []
  let s:list2 = []
  call setline(3, 'three')
  redraw
  call assert_equal([{'lnum': 3, 'end': 3, 'col': 1, 'added': 1}], s:list)
  call assert_equal([], s:list2)

  " a change above a previous change without a line number change is reported
  " together
  call setline(1, ['one one', 'two'])
  call listener_flush()
  call append(2, 'two two')
  call setline(1, 'something')
  call listener_flush()
  call assert_equal([{'lnum': 3, 'end': 3, 'col': 1, 'added': 1},
	\ {'lnum': 1, 'end': 2, 'col': 1, 'added': 0}], s:list)

  " an insert just above a previous change that was the last one gets merged
  call setline(1, ['one one', 'two'])
  call listener_flush()
  let s:list = []
  call setline(2, 'something')
  call append(1, 'two two')
  call assert_equal([], s:list)
  call listener_flush()
  call assert_equal([{'lnum': 2, 'end': 3, 'col': 1, 'added': 1}], s:list)

  " an insert above a previous change causes a flush
  call setline(1, ['one one', 'two'])
  call listener_flush()
  call setline(2, 'something')
  call append(0, 'two two')
  call assert_equal([{'lnum': 2, 'end': 3, 'col': 1, 'added': 0}], s:list)
  call assert_equal('something', s:text)
  call listener_flush()
  call assert_equal([{'lnum': 1, 'end': 1, 'col': 1, 'added': 1}], s:list)
  call assert_equal('two two', s:text)

  " a delete at a previous change that was the last one gets merged
  call setline(1, ['one one', 'two'])
  call listener_flush()
  let s:list = []
  call setline(2, 'something')
  2del
  call assert_equal([], s:list)
  call listener_flush()
  call assert_equal([{'lnum': 2, 'end': 3, 'col': 1, 'added': -1}], s:list)

  " a delete above a previous change causes a flush
  call setline(1, ['one one', 'two'])
  call listener_flush()
  call setline(2, 'another')
  1del
  call assert_equal([{'lnum': 2, 'end': 3, 'col': 1, 'added': 0}], s:list)
  call assert_equal(2, s:start)
  call assert_equal('another', s:text)
  call listener_flush()
  call assert_equal([{'lnum': 1, 'end': 2, 'col': 1, 'added': -1}], s:list)
  call assert_equal('another', s:text)

  " the "o" command first adds an empty line and then changes it
  %del
  call setline(1, ['one one', 'two'])
  call listener_flush()
  let s:list = []
  exe "normal Gofour\<Esc>"
  redraw
  call assert_equal([{'lnum': 3, 'end': 3, 'col': 1, 'added': 1},
	\ {'lnum': 3, 'end': 4, 'col': 1, 'added': 0}], s:list)

  " Remove last listener
  let s:list = []
  call listener_remove(id)
  call setline(1, 'asdfasdf')
  redraw
  call assert_equal([], s:list)

  " Trying to change the list fails
  let id = listener_add({b, s, e, a, l -> s:EvilStoreList(l)})
  let s:list3 = []
  call setline(1, 'asdfasdf')
  redraw
  call assert_equal([{'lnum': 1, 'end': 2, 'col': 1, 'added': 0}], s:list3)

  call listener_remove(id)
  bwipe!
endfunc

func s:StoreListArgs(buf, start, end, added, list)
  let s:buf = a:buf
  let s:start = a:start
  let s:end = a:end
  let s:added = a:added
  let s:list = a:list
endfunc

func Test_listener_args()
  new
  call setline(1, ['one', 'two'])
  let s:list = []
  let id = listener_add('s:StoreListArgs')

  " just one change
  call setline(1, 'one one')
  call listener_flush()
  call assert_equal(bufnr(''), s:buf)
  call assert_equal(1, s:start)
  call assert_equal(2, s:end)
  call assert_equal(0, s:added)
  call assert_equal([{'lnum': 1, 'end': 2, 'col': 1, 'added': 0}], s:list)

  " two disconnected changes
  call setline(1, ['one', 'two', 'three', 'four'])
  call listener_flush()
  call setline(1, 'one one')
  call setline(3, 'three three')
  call listener_flush()
  call assert_equal(bufnr(''), s:buf)
  call assert_equal(1, s:start)
  call assert_equal(4, s:end)
  call assert_equal(0, s:added)
  call assert_equal([{'lnum': 1, 'end': 2, 'col': 1, 'added': 0},
	\ {'lnum': 3, 'end': 4, 'col': 1, 'added': 0}], s:list)

  " add and remove lines
  call setline(1, ['one', 'two', 'three', 'four', 'five', 'six'])
  call listener_flush()
  call append(2, 'two two')
  4del
  call append(5, 'five five')
  call listener_flush()
  call assert_equal(bufnr(''), s:buf)
  call assert_equal(3, s:start)
  call assert_equal(6, s:end)
  call assert_equal(1, s:added)
  call assert_equal([{'lnum': 3, 'end': 3, 'col': 1, 'added': 1},
	\ {'lnum': 4, 'end': 5, 'col': 1, 'added': -1},
	\ {'lnum': 6, 'end': 6, 'col': 1, 'added': 1}], s:list)

  call listener_remove(id)
  bwipe!
endfunc

func s:StoreBufList(buf, start, end, added, list)
  let s:bufnr = a:buf
  let s:list = a:list
endfunc

func Test_listening_other_buf()
  new
  call setline(1, ['one', 'two'])
  let bufnr = bufnr('')
  normal ww
  let id = listener_add(function('s:StoreBufList'), bufnr)
  let s:list = []
  call setbufline(bufnr, 1, 'hello')
  redraw
  call assert_equal(bufnr, s:bufnr)
  call assert_equal([{'lnum': 1, 'end': 2, 'col': 1, 'added': 0}], s:list)

  call listener_remove(id)
  exe "buf " .. bufnr
  bwipe!
endfunc
