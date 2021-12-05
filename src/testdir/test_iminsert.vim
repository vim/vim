" Test for 'iminsert'

source view_util.vim
source check.vim
source vim9.vim

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
  else
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
  else
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

" Test for different ways of setting the 'imactivatefunc' and 'imstatusfunc'
" options
func Test_imactivatefunc_imstatusfunc_callback()
  CheckNotMSWindows
  func IMactivatefunc1(active)
    let g:IMactivatefunc_called += 1
  endfunc
  func IMstatusfunc1()
    let g:IMstatusfunc_called += 1
    return 1
  endfunc
  let g:IMactivatefunc_called = 0
  let g:IMstatusfunc_called = 0
  set iminsert=2

  " Test for using a function()
  set imactivatefunc=function('IMactivatefunc1')
  set imstatusfunc=function('IMstatusfunc1')
  normal! i

  " Using a funcref variable to set 'completefunc'
  let Fn1 = function('IMactivatefunc1')
  let &imactivatefunc = Fn1
  let Fn2 = function('IMstatusfunc1')
  let &imstatusfunc = Fn2
  normal! i

  " Using a string(funcref variable) to set 'completefunc'
  let &imactivatefunc = string(Fn1)
  let &imstatusfunc = string(Fn2)
  normal! i

  " Test for using a funcref()
  set imactivatefunc=funcref('IMactivatefunc1')
  set imstatusfunc=funcref('IMstatusfunc1')
  normal! i

  " Using a funcref variable to set 'imactivatefunc'
  let Fn1 = funcref('IMactivatefunc1')
  let &imactivatefunc = Fn1
  let Fn2 = funcref('IMstatusfunc1')
  let &imstatusfunc = Fn2
  normal! i

  " Using a string(funcref variable) to set 'imactivatefunc'
  let &imactivatefunc = string(Fn1)
  let &imstatusfunc = string(Fn2)
  normal! i

  " Test for using a lambda function
  set imactivatefunc={a\ ->\ IMactivatefunc1(a)}
  set imstatusfunc={\ ->\ IMstatusfunc1()}
  normal! i

  " Set 'imactivatefunc' and 'imstatusfunc' to a lambda expression
  let &imactivatefunc = {a -> IMactivatefunc1(a)}
  let &imstatusfunc = { -> IMstatusfunc1()}
  normal! i

  " Set 'imactivatefunc' and 'imstatusfunc' to a string(lambda expression)
  let &imactivatefunc = '{a -> IMactivatefunc1(a)}'
  let &imstatusfunc = '{ -> IMstatusfunc1()}'
  normal! i

  " Set 'imactivatefunc' 'imstatusfunc' to a variable with a lambda expression
  let Lambda1 = {a -> IMactivatefunc1(a)}
  let Lambda2 = { -> IMstatusfunc1()}
  let &imactivatefunc = Lambda1
  let &imstatusfunc = Lambda2
  normal! i

  " Set 'imactivatefunc' 'imstatusfunc' to a string(variable with a lambda
  " expression)
  let &imactivatefunc = string(Lambda1)
  let &imstatusfunc = string(Lambda2)
  normal! i

  " Test for clearing the 'completefunc' option
  set imactivatefunc='' imstatusfunc=''
  set imactivatefunc& imstatusfunc&

  call assert_fails("set imactivatefunc=function('abc')", "E700:")
  call assert_fails("set imstatusfunc=function('abc')", "E700:")
  call assert_fails("set imactivatefunc=funcref('abc')", "E700:")
  call assert_fails("set imstatusfunc=funcref('abc')", "E700:")

  call assert_equal(11, g:IMactivatefunc_called)
  call assert_equal(22, g:IMstatusfunc_called)

  " Vim9 tests
  let lines =<< trim END
    vim9script

    # Test for using function()
    def IMactivatefunc1(active: number): any
      g:IMactivatefunc_called += 1
      return 1
    enddef
    def IMstatusfunc1(): number
      g:IMstatusfunc_called += 1
      return 1
    enddef
    g:IMactivatefunc_called = 0
    g:IMstatusfunc_called = 0
    set iminsert=2
    set imactivatefunc=function('IMactivatefunc1')
    set imstatusfunc=function('IMstatusfunc1')
    normal! i

    # Test for using a lambda
    &imactivatefunc = '(a) => IMactivatefunc1(a)'
    &imstatusfunc = '() => IMstatusfunc1()'
    normal! i

    # Test for using a variable with a lambda expression
    var Fn1: func = (active) => {
           g:IMactivatefunc_called += 1
           return 1
        }
    var Fn2: func = () => {
           g:IMstatusfunc_called += 1
           return 1
        }
    &imactivatefunc = Fn1
    &imstatusfunc = Fn2
    normal! i

    # Test for using a string(variable with a lambda expression)
    &imactivatefunc = string(Fn1)
    &imstatusfunc = string(Fn2)
    normal! i

    assert_equal(4, g:IMactivatefunc_called)
    assert_equal(8, g:IMstatusfunc_called)

    set iminsert=0
    set imactivatefunc=
    set imstatusfunc=
  END
  call CheckScriptSuccess(lines)

  " Using Vim9 lambda expression in legacy context should fail
  set imactivatefunc=(a)\ =>\ IMactivatefunc1(a)
  set imstatusfunc=IMstatusfunc1
  call assert_fails('normal! i', 'E117:')
  set imactivatefunc=IMactivatefunc1
  set imstatusfunc=()\ =>\ IMstatusfunc1(a)
  call assert_fails('normal! i', 'E117:')

  " cleanup
  delfunc IMactivatefunc1
  delfunc IMstatusfunc1
  set iminsert=0
  set imactivatefunc=
  set imstatusfunc=

  %bw!
endfunc

" vim: shiftwidth=2 sts=2 expandtab
