" Test the netbeans interface.

if !has('netbeans_intg')
  finish
endif

source shared.vim

let s:python = PythonProg()
if s:python == ''
  " Can't run this test.
  finish
endif

" Run "testfunc" after sarting the server and stop the server afterwards.
func s:run_server(testfunc, ...)
  call RunServer('test_netbeans.py', a:testfunc, a:000)
endfunc

func Nb_basic(port)
  call delete("Xnetbeans")
  exe 'nbstart :localhost:' . a:port . ':bunny'
  call assert_true(has("netbeans_enabled"))

  call WaitFor('len(readfile("Xnetbeans")) > 2')
  split +$ README.txt

  " Opening README.txt will result in a setDot command
  call WaitFor('len(readfile("Xnetbeans")) > 4')
  call WaitFor('getcurpos()[1] == 2')
  let pos = getcurpos()
  call assert_equal(3, pos[1])
  call assert_equal(20, pos[2])
  close
  nbclose

  call WaitFor('len(readfile("Xnetbeans")) > 6')
  call assert_false(has("netbeans_enabled"))
  let lines = readfile("Xnetbeans")
  call assert_equal('AUTH bunny', lines[0])
  call assert_equal('0:version=0 "2.5"', lines[1])
  call assert_equal('0:startupDone=0', lines[2])
  call assert_equal('0:fileOpened=0 "README.txt" T F', substitute(lines[3], '".*/', '"', ''))

  call assert_equal('0:disconnect=1', lines[6])

  call delete("Xnetbeans")
endfunc

func Test_nb_basic()
  call ch_log('Test_nb_basic')
  call s:run_server('Nb_basic')
endfunc

func Nb_file_auth(port)
  call assert_fails('nbstart =notexist', 'E660:')
  call writefile(['host=localhost', 'port=' . a:port, 'auth=bunny'], 'Xnbauth')
  if has('unix')
    call setfperm('Xnbauth', "rw-r--r--")
    call assert_fails('nbstart =Xnbauth', 'E668:')
  endif
  call setfperm('Xnbauth', "rw-------")
  exe 'nbstart =Xnbauth'
  call assert_true(has("netbeans_enabled"))

  call WaitFor('len(readfile("Xnetbeans")) > 2')
  nbclose
  let lines = readfile("Xnetbeans")
  call assert_equal('AUTH bunny', lines[0])
  call assert_equal('0:version=0 "2.5"', lines[1])
  call assert_equal('0:startupDone=0', lines[2])

  call delete("Xnbauth")
  call delete("Xnetbeans")
endfunc

func Test_nb_file_auth()
  call ch_log('Test_nb_file_auth')
  call s:run_server('Nb_file_auth')
endfunc
