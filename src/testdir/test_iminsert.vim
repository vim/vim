" Test for 'iminsert'

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

  let expected = (has('win32') && has('gui_running')) ? 0 : 1
  call assert_equal(expected, s:imactivatefunc_called)
  call assert_equal(expected, s:imstatusfunc_called)
endfunc

func Test_getimstatus()
  if has('win32')
    CheckFeature multi_byte_ime
  elseif !has('gui_mac')
    CheckFeature xim
  endif
  if has('win32') && has('gui_running')
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

" Test for using an lmap in insert mode
func Test_lmap_in_insert_mode()
  new
  call setline(1, 'abc')
  lmap { w
  set iminsert=1
  call feedkeys('r{', 'xt')
  call assert_equal('wbc', getline(1))
  set iminsert=2
  call feedkeys('$r{', 'xt')
  call assert_equal('wb{', getline(1))
  call setline(1, 'vim web')
  set iminsert=1
  call feedkeys('0f{', 'xt')
  call assert_equal(5, col('.'))
  set iminsert&
  lunmap {
  close!
endfunc

" Test for using CTRL-^ to toggle iminsert in insert mode
func Test_iminsert_toggle()
  CheckGui
  if has('win32')
    CheckFeature multi_byte_ime
  elseif !has('gui_mac')
    CheckFeature xim
  endif
  if has('gui_running') && !has('win32')
    throw 'Skipped: works only in Win32 GUI version (for some reason)'
  endif
  new
  let save_imdisable = &imdisable
  let save_iminsert = &iminsert
  set noimdisable
  set iminsert=0
  exe "normal i\<C-^>"
  call assert_equal(2, &iminsert)
  exe "normal i\<C-^>"
  call assert_equal(0, &iminsert)
  let &iminsert = save_iminsert
  let &imdisable = save_imdisable
  close!
endfunc

" vim: shiftwidth=2 sts=2 expandtab
