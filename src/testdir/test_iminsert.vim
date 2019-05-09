source view_util.vim

let s:imactivatefunc_called = 0
let s:imstatusfunc_called = 0

func IM_activatefunc(active)
  let s:imactivatefunc_called = 1
endfunc

func IM_statusfunc()
  let s:imstatusfunc_called = 1
  return 0
endfunc

func Test_iminsert2()
  set imactivatefunc=IM_activatefunc
  set imstatusfunc=IM_statusfunc
  set iminsert=2
  normal! i
  set iminsert=0
  set imactivatefunc=
  set imstatusfunc=

  let expected = has('gui_running') ? 0 : 1
  call assert_equal(expected, s:imactivatefunc_called)
  call assert_equal(expected, s:imstatusfunc_called)
endfunc
