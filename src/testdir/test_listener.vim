" tests for listener_add() and listener_remove()

func StoreList(l)
  let g:list = a:l
endfunc

func AnotherStoreList(l)
  let g:list2 = a:l
endfunc

func EvilStoreList(l)
  let g:list3 = a:l
  call assert_fails("call add(a:l, 'myitem')", "E742:")
endfunc

func Test_listening()
  new
  call setline(1, ['one', 'two'])
  let id = listener_add({l -> StoreList(l)})
  call setline(1, 'one one')
  redraw
  call assert_equal([{'lnum': 1, 'end': 2, 'col': 1, 'added': 0}], g:list)

  " Two listeners, both get called.
  let id2 = listener_add({l -> AnotherStoreList(l)})
  let g:list = []
  let g:list2 = []
  exe "normal $asome\<Esc>"
  redraw
  call assert_equal([{'lnum': 1, 'end': 2, 'col': 8, 'added': 0}], g:list)
  call assert_equal([{'lnum': 1, 'end': 2, 'col': 8, 'added': 0}], g:list2)

  call listener_remove(id2)
  let g:list = []
  let g:list2 = []
  call setline(3, 'three')
  redraw
  call assert_equal([{'lnum': 3, 'end': 3, 'col': 1, 'added': 1}], g:list)
  call assert_equal([], g:list2)

  " the "o" command first adds an empty line and then changes it
  let g:list = []
  exe "normal Gofour\<Esc>"
  redraw
  call assert_equal([{'lnum': 4, 'end': 4, 'col': 1, 'added': 1},
	\ {'lnum': 4, 'end': 5, 'col': 1, 'added': 0}], g:list)

  let g:list = []
  call listener_remove(id)
  call setline(1, 'asdfasdf')
  redraw
  call assert_equal([], g:list)

  " Trying to change the list fails
  let id = listener_add({l -> EvilStoreList(l)})
  let g:list3 = []
  call setline(1, 'asdfasdf')
  redraw
  call assert_equal([{'lnum': 1, 'end': 2, 'col': 1, 'added': 0}], g:list3)

  bwipe!
endfunc

func Test_listening_other_buf()
  new
  call setline(1, ['one', 'two'])
  let bufnr = bufnr('')
  normal ww
  let id = listener_add({l -> StoreList(l)}, bufnr)
  let g:list = []
  call setbufline(bufnr, 1, 'hello')
  redraw
  call assert_equal([{'lnum': 1, 'end': 2, 'col': 1, 'added': 0}], g:list)

  exe "buf " .. bufnr
  bwipe!
endfunc
