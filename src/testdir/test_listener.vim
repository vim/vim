" tests for listener_add() and listener_remove()

func s:StoreList(l)
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
  let id = listener_add({l -> s:StoreList(l)})
  call setline(1, 'one one')
  redraw
  call assert_equal([{'lnum': 1, 'end': 2, 'col': 1, 'added': 0}], s:list)

  " Undo is also a change
  set undolevels&  " start new undo block
  call append(2, 'two two')
  undo
  redraw
  call assert_equal([{'lnum': 3, 'end': 3, 'col': 1, 'added': 1},
	\ {'lnum': 3, 'end': 4, 'col': 1, 'added': -1}, ], s:list)
  1

  " Two listeners, both get called.
  let id2 = listener_add({l -> s:AnotherStoreList(l)})
  let s:list = []
  let s:list2 = []
  exe "normal $asome\<Esc>"
  redraw
  call assert_equal([{'lnum': 1, 'end': 2, 'col': 8, 'added': 0}], s:list)
  call assert_equal([{'lnum': 1, 'end': 2, 'col': 8, 'added': 0}], s:list2)

  call listener_remove(id2)
  let s:list = []
  let s:list2 = []
  call setline(3, 'three')
  redraw
  call assert_equal([{'lnum': 3, 'end': 3, 'col': 1, 'added': 1}], s:list)
  call assert_equal([], s:list2)

  " the "o" command first adds an empty line and then changes it
  let s:list = []
  exe "normal Gofour\<Esc>"
  redraw
  call assert_equal([{'lnum': 4, 'end': 4, 'col': 1, 'added': 1},
	\ {'lnum': 4, 'end': 5, 'col': 1, 'added': 0}], s:list)

  " Remove last listener
  let s:list = []
  call listener_remove(id)
  call setline(1, 'asdfasdf')
  redraw
  call assert_equal([], s:list)

  " Trying to change the list fails
  let id = listener_add({l -> s:EvilStoreList(l)})
  let s:list3 = []
  call setline(1, 'asdfasdf')
  redraw
  call assert_equal([{'lnum': 1, 'end': 2, 'col': 1, 'added': 0}], s:list3)

  call listener_remove(id)
  bwipe!
endfunc

func s:StoreBufList(buf, l)
  let s:bufnr = a:buf
  let s:list = a:l
endfunc

func Test_listening_other_buf()
  new
  call setline(1, ['one', 'two'])
  let bufnr = bufnr('')
  normal ww
  let id = listener_add(function('s:StoreBufList', [bufnr]), bufnr)
  let s:list = []
  call setbufline(bufnr, 1, 'hello')
  redraw
  call assert_equal(bufnr, s:bufnr)
  call assert_equal([{'lnum': 1, 'end': 2, 'col': 1, 'added': 0}], s:list)

  call listener_remove(id)
  exe "buf " .. bufnr
  bwipe!
endfunc
