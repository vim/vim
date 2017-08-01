" Tests for the terminal window.

if !exists('*term_start')
  finish
endif

source shared.vim

func Run_shell_in_terminal()
  let buf = term_start(&shell)

  let termlist = term_list()
  call assert_equal(1, len(termlist))
  call assert_equal(buf, termlist[0])

  let g:job = term_getjob(buf)
  call assert_equal(v:t_job, type(g:job))

  call term_sendkeys(buf, "exit\r")
  call WaitFor('job_status(g:job) == "dead"')
  call assert_equal('dead', job_status(g:job))

  return buf
endfunc

func Test_terminal_basic()
  let buf = Run_shell_in_terminal()

  exe buf . 'bwipe'
  unlet g:job
endfunc

func Test_terminal_make_change()
  let buf = Run_shell_in_terminal()
  call term_wait(buf)

  setlocal modifiable
  exe "normal Axxx\<Esc>"
  call assert_fails(buf . 'bwipe', 'E517')
  undo

  exe buf . 'bwipe'
  unlet g:job
endfunc

func Check_123(buf)
  let l = term_scrape(a:buf, 1)
  call assert_true(len(l) > 0)
  call assert_equal('1', l[0].chars)
  call assert_equal('2', l[1].chars)
  call assert_equal('3', l[2].chars)
  call assert_equal('#00e000', l[0].fg)
  if &background == 'light'
    call assert_equal('#ffffff', l[0].bg)
  else
    call assert_equal('#000000', l[0].bg)
  endif

  let l = term_getline(a:buf, 1)
  call assert_equal('123', l)
endfunc

func Test_terminal_scrape()
  if has('win32')
    let cmd = 'cmd /c "cls && color 2 && echo 123"'
  else
    call writefile(["\<Esc>[32m123"], 'Xtext')
    let cmd = "cat Xtext"
  endif
  let buf = term_start(cmd)

  let termlist = term_list()
  call assert_equal(1, len(termlist))
  call assert_equal(buf, termlist[0])

  " Nothing happens with invalid buffer number
  call term_wait(1234)

  call term_wait(buf)
  call Check_123(buf)

  " Must still work after the job ended.
  let g:job = term_getjob(buf)
  call WaitFor('job_status(g:job) == "dead"')
  call term_wait(buf)
  call Check_123(buf)

  exe buf . 'bwipe'
  call delete('Xtext')
endfunc
