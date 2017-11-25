if !has('multi_byte')
  finish
endif

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
  if has('gui_win32')
    return
  endif
  set imactivatefunc=IM_activatefunc
  set imstatusfunc=IM_statusfunc
  set iminsert=2
  normal! i
  set iminsert=0
  set imactivatefunc=
  set imstatusfunc=
  call assert_equal(1, s:imactivatefunc_called)
  call assert_equal(1, s:imstatusfunc_called)
endfunc
