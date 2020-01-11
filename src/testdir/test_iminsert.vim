source view_util.vim
source check.vim

let s:imactivatefunc_called = 0
let s:imstatusfunc_called = 0
let s:imstatus_active = 0

func IM_activatefunc(active)
  let s:imactivatefunc_called = 1
let s:imstatus_active = a:active
endfunc

func IM_statusfunc()
  let s:imstatusfunc_called = 1
  return s:imstatus_active
endfunc

func Test_iminsert2()
  let s:imactivatefunc_called = 0
  let s:imstatusfunc_called = 0

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

func Test_getimstatus()
  if has('win32')
    CheckFeature multi_byte_ime
  elseif !has('gui_mac')
    CheckFeature xim
  endif
  if has('gui_running')
    if !has('win32')
      throw 'Skipped: running in the GUI, only works on MS-Windows'
    endif
    set imactivatefunc=
    set imstatusfunc=
  else
    set imactivatefunc=IM_activatefunc
    set imstatusfunc=IM_statusfunc
    let s:imstatus_active = 0
  endif

  new
  set iminsert=2
  call feedkeys("i\<C-R>=getimstatus()\<CR>\<ESC>", 'nx')
  call assert_equal('1', getline(1))
  set iminsert=0
  call feedkeys("o\<C-R>=getimstatus()\<CR>\<ESC>", 'nx')
  call assert_equal('0', getline(2))
  bw!

  set imactivatefunc=
  set imstatusfunc=
endfunc
