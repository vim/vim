" Tests for the terminal window.

if !has('terminal')
  finish
endif

source shared.vim

let s:python = PythonProg()

" Open a terminal with a shell, assign the job to g:job and return the buffer
" number.
func Run_shell_in_terminal(options)
  let buf = term_start(&shell, a:options)

  let termlist = term_list()
  call assert_equal(1, len(termlist))
  call assert_equal(buf, termlist[0])

  let g:job = term_getjob(buf)
  call assert_equal(v:t_job, type(g:job))

  let string = string({'job': term_getjob(buf)})
  call assert_match("{'job': 'process \\d\\+ run'}", string)

  return buf
endfunc

" Stops the shell started by Run_shell_in_terminal().
func Stop_shell_in_terminal(buf)
  call term_sendkeys(a:buf, "exit\r")
  call WaitFor('job_status(g:job) == "dead"')
  call assert_equal('dead', job_status(g:job))
endfunc

func Test_terminal_basic()
  let buf = Run_shell_in_terminal({})
  if has("unix")
    call assert_match("^/dev/", job_info(g:job).tty)
    call assert_match("^/dev/", term_gettty(''))
  else
    call assert_match("^winpty://", job_info(g:job).tty)
    call assert_match("^winpty://", term_gettty(''))
  endif
  call assert_equal('t', mode())
  call assert_match('%aR[^\n]*running]', execute('ls'))

  call Stop_shell_in_terminal(buf)
  call term_wait(buf)
  call assert_equal('n', mode())
  call assert_match('%aF[^\n]*finished]', execute('ls'))

  " closing window wipes out the terminal buffer a with finished job
  close
  call assert_equal("", bufname(buf))

  unlet g:job
endfunc

func Test_terminal_make_change()
  let buf = Run_shell_in_terminal({})
  call Stop_shell_in_terminal(buf)
  call term_wait(buf)

  setlocal modifiable
  exe "normal Axxx\<Esc>"
  call assert_fails(buf . 'bwipe', 'E517')
  undo

  exe buf . 'bwipe'
  unlet g:job
endfunc

func Test_terminal_wipe_buffer()
  let buf = Run_shell_in_terminal({})
  call assert_fails(buf . 'bwipe', 'E517')
  exe buf . 'bwipe!'
  call WaitFor('job_status(g:job) == "dead"')
  call assert_equal('dead', job_status(g:job))
  call assert_equal("", bufname(buf))

  unlet g:job
endfunc

func Test_terminal_hide_buffer()
  let buf = Run_shell_in_terminal({})
  setlocal bufhidden=hide
  quit
  for nr in range(1, winnr('$'))
    call assert_notequal(winbufnr(nr), buf)
  endfor
  call assert_true(bufloaded(buf))
  call assert_true(buflisted(buf))

  exe 'split ' . buf . 'buf'
  call Stop_shell_in_terminal(buf)
  exe buf . 'bwipe'

  unlet g:job
endfunc

func! s:Nasty_exit_cb(job, st)
  exe g:buf . 'bwipe!'
  let g:buf = 0
endfunc

func Test_terminal_nasty_cb()
  let cmd = Get_cat_123_cmd()
  let g:buf = term_start(cmd, {'exit_cb': function('s:Nasty_exit_cb')})
  let g:job = term_getjob(g:buf)

  call WaitFor('job_status(g:job) == "dead"')
  call WaitFor('g:buf == 0')
  unlet g:buf
  unlet g:job
  call delete('Xtext')
endfunc

func Check_123(buf)
  let l = term_scrape(a:buf, 0)
  call assert_true(len(l) == 0)
  let l = term_scrape(a:buf, 999)
  call assert_true(len(l) == 0)
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

  let l = term_getline(a:buf, -1)
  call assert_equal('', l)
  let l = term_getline(a:buf, 0)
  call assert_equal('', l)
  let l = term_getline(a:buf, 999)
  call assert_equal('', l)
  let l = term_getline(a:buf, 1)
  call assert_equal('123', l)
endfunc

func Get_cat_123_cmd()
  if has('win32')
    return 'cmd /c "cls && color 2 && echo 123"'
  else
    call writefile(["\<Esc>[32m123"], 'Xtext')
    return "cat Xtext"
  endif
endfunc

func Test_terminal_scrape_123()
  let cmd = Get_cat_123_cmd()
  let buf = term_start(cmd)

  let termlist = term_list()
  call assert_equal(1, len(termlist))
  call assert_equal(buf, termlist[0])

  " Nothing happens with invalid buffer number
  call term_wait(1234)

  call term_wait(buf)
  if has('win32')
    " TODO: this should not be needed
    sleep 100m
  endif
  call Check_123(buf)

  " Must still work after the job ended.
  let g:job = term_getjob(buf)
  call WaitFor('job_status(g:job) == "dead"')
  call term_wait(buf)
  call Check_123(buf)

  exe buf . 'bwipe'
  call delete('Xtext')
endfunc

func Test_terminal_scrape_multibyte()
  if !has('multi_byte')
    return
  endif
  call writefile(["léttまrs"], 'Xtext')
  if has('win32')
    " Run cmd with UTF-8 codepage to make the type command print the expected
    " multibyte characters.
    let g:buf = term_start("cmd /K chcp 65001")
    call term_sendkeys(g:buf, "type Xtext\<CR>")
    call term_sendkeys(g:buf, "exit\<CR>")
    let g:line = 4
  else
    let g:buf = term_start("cat Xtext")
    let g:line = 1
  endif

  call WaitFor('term_scrape(g:buf, g:line)[0].chars == "l"')
  let l = term_scrape(g:buf, g:line)
  call assert_true(len(l) >= 7)
  call assert_equal('l', l[0].chars)
  call assert_equal('é', l[1].chars)
  call assert_equal(1, l[1].width)
  call assert_equal('t', l[2].chars)
  call assert_equal('t', l[3].chars)
  call assert_equal('ま', l[4].chars)
  call assert_equal(2, l[4].width)
  call assert_equal('r', l[5].chars)
  call assert_equal('s', l[6].chars)

  let g:job = term_getjob(g:buf)
  call WaitFor('job_status(g:job) == "dead"')
  call term_wait(g:buf)

  exe g:buf . 'bwipe'
  unlet g:buf
  unlet g:line
  call delete('Xtext')
endfunc

func Test_terminal_scroll()
  call writefile(range(1, 200), 'Xtext')
  if has('win32')
    let cmd = 'cmd /c "type Xtext"'
  else
    let cmd = "cat Xtext"
  endif
  let buf = term_start(cmd)

  let g:job = term_getjob(buf)
  call WaitFor('job_status(g:job) == "dead"')
  call term_wait(buf)
  if has('win32')
    " TODO: this should not be needed
    sleep 100m
  endif

  let scrolled = term_getscrolled(buf)
  call assert_equal('1', getline(1))
  call assert_equal('1', term_getline(buf, 1 - scrolled))
  call assert_equal('49', getline(49))
  call assert_equal('49', term_getline(buf, 49 - scrolled))
  call assert_equal('200', getline(200))
  call assert_equal('200', term_getline(buf, 200 - scrolled))

  exe buf . 'bwipe'
  call delete('Xtext')
endfunc

func Test_terminal_size()
  let cmd = Get_cat_123_cmd()

  exe 'terminal ++rows=5 ' . cmd
  let size = term_getsize('')
  bwipe!
  call assert_equal(5, size[0])

  call term_start(cmd, {'term_rows': 6})
  let size = term_getsize('')
  bwipe!
  call assert_equal(6, size[0])

  vsplit
  exe 'terminal ++rows=5 ++cols=33 ' . cmd
  let size = term_getsize('')
  bwipe!
  call assert_equal([5, 33], size)

  call term_start(cmd, {'term_rows': 6, 'term_cols': 36})
  let size = term_getsize('')
  bwipe!
  call assert_equal([6, 36], size)

  exe 'vertical terminal ++cols=20 ' . cmd
  let size = term_getsize('')
  bwipe!
  call assert_equal(20, size[1])

  call term_start(cmd, {'vertical': 1, 'term_cols': 26})
  let size = term_getsize('')
  bwipe!
  call assert_equal(26, size[1])

  split
  exe 'vertical terminal ++rows=6 ++cols=20 ' . cmd
  let size = term_getsize('')
  bwipe!
  call assert_equal([6, 20], size)

  call term_start(cmd, {'vertical': 1, 'term_rows': 7, 'term_cols': 27})
  let size = term_getsize('')
  bwipe!
  call assert_equal([7, 27], size)
endfunc

func Test_terminal_curwin()
  let cmd = Get_cat_123_cmd()
  call assert_equal(1, winnr('$'))

  split dummy
  exe 'terminal ++curwin ' . cmd
  call assert_equal(2, winnr('$'))
  bwipe!

  split dummy
  call term_start(cmd, {'curwin': 1})
  call assert_equal(2, winnr('$'))
  bwipe!

  split dummy
  call setline(1, 'change')
  call assert_fails('terminal ++curwin ' . cmd, 'E37:')
  call assert_equal(2, winnr('$'))
  exe 'terminal! ++curwin ' . cmd
  call assert_equal(2, winnr('$'))
  bwipe!

  split dummy
  call setline(1, 'change')
  call assert_fails("call term_start(cmd, {'curwin': 1})", 'E37:')
  call assert_equal(2, winnr('$'))
  bwipe!

  split dummy
  bwipe!

endfunc

func Test_finish_open_close()
  call assert_equal(1, winnr('$'))

  if s:python != ''
    let cmd = s:python . " test_short_sleep.py"
    let waittime = 500
  else
    echo 'This will take five seconds...'
    let waittime = 2000
    if has('win32')
      let cmd = $windir . '\system32\timeout.exe 1'
    else
      let cmd = 'sleep 1'
    endif
  endif

  exe 'terminal ++close ' . cmd
  call assert_equal(2, winnr('$'))
  wincmd p
  call WaitFor("winnr('$') == 1", waittime)
  call assert_equal(1, winnr('$'))

  call term_start(cmd, {'term_finish': 'close'})
  call assert_equal(2, winnr('$'))
  wincmd p
  call WaitFor("winnr('$') == 1", waittime)
  call assert_equal(1, winnr('$'))

  exe 'terminal ++open ' . cmd
  close!
  call WaitFor("winnr('$') == 2", waittime)
  call assert_equal(2, winnr('$'))
  bwipe

  call term_start(cmd, {'term_finish': 'open'})
  close!
  call WaitFor("winnr('$') == 2", waittime)
  call assert_equal(2, winnr('$'))
  bwipe

  exe 'terminal ++hidden ++open ' . cmd
  call assert_equal(1, winnr('$'))
  call WaitFor("winnr('$') == 2", waittime)
  call assert_equal(2, winnr('$'))
  bwipe

  call term_start(cmd, {'term_finish': 'open', 'hidden': 1})
  call assert_equal(1, winnr('$'))
  call WaitFor("winnr('$') == 2", waittime)
  call assert_equal(2, winnr('$'))
  bwipe

  call assert_fails("call term_start(cmd, {'term_opencmd': 'open'})", 'E475:')
  call assert_fails("call term_start(cmd, {'term_opencmd': 'split %x'})", 'E475:')
  call assert_fails("call term_start(cmd, {'term_opencmd': 'split %d and %s'})", 'E475:')
  call assert_fails("call term_start(cmd, {'term_opencmd': 'split % and %d'})", 'E475:')

  call term_start(cmd, {'term_finish': 'open', 'term_opencmd': '4split | buffer %d'})
  close!
  call WaitFor("winnr('$') == 2", waittime)
  call assert_equal(2, winnr('$'))
  call assert_equal(4, winheight(0))
  bwipe

endfunc

func Test_terminal_cwd()
  if !has('unix')
    return
  endif
  call mkdir('Xdir')
  let buf = term_start('pwd', {'cwd': 'Xdir'})
  sleep 100m
  call term_wait(buf)
  call assert_equal(getcwd() . '/Xdir', getline(1))

  exe buf . 'bwipe'
  call delete('Xdir', 'rf')
endfunc

func Test_terminal_env()
  if !has('unix')
    return
  endif
  let g:buf = Run_shell_in_terminal({'env': {'TESTENV': 'correct'}})
  " Wait for the shell to display a prompt
  call WaitFor('term_getline(g:buf, 1) != ""')
  call term_sendkeys(g:buf, "echo $TESTENV\r")
  call term_wait(g:buf)
  call Stop_shell_in_terminal(g:buf)
  call WaitFor('getline(2) == "correct"')
  call assert_equal('correct', getline(2))

  exe g:buf . 'bwipe'
  unlet g:buf
endfunc

" must be last, we can't go back from GUI to terminal
func Test_zz_terminal_in_gui()
  if !CanRunGui()
    return
  endif

  " Ignore the "failed to create input context" error.
  call test_ignore_error('E285:')

  gui -f

  call assert_equal(1, winnr('$'))
  let buf = Run_shell_in_terminal({'term_finish': 'close'})
  call Stop_shell_in_terminal(buf)
  call term_wait(buf)

  " closing window wipes out the terminal buffer a with finished job
  call WaitFor("winnr('$') == 1")
  call assert_equal(1, winnr('$'))
  call assert_equal("", bufname(buf))

  unlet g:job
endfunc

func Test_terminal_list_args()
  let buf = term_start([&shell, &shellcmdflag, 'echo "123"'])
  call assert_fails(buf . 'bwipe', 'E517')
  exe buf . 'bwipe!'
  call assert_equal("", bufname(buf))
endfunction

func Test_terminal_noblock()
  let g:buf = term_start(&shell)

  for c in ['a','b','c','d','e','f','g','h','i','j','k']
    call term_sendkeys(g:buf, 'echo ' . repeat(c, 5000) . "\<cr>")
    if has('mac')
      " TODO: this should not be needed, but without it sending keys blocks
      " after 8000 chars or so.
      sleep 100m
    endif
  endfor
  call term_sendkeys(g:buf, "echo done\<cr>")

  " On MS-Windows there is an extra empty line below "done".  Find "done" in
  " the last-but-one or the last-but-two line.
  let g:lnum = term_getsize(g:buf)[0] - 1
  call WaitFor('term_getline(g:buf, g:lnum) =~ "done" || term_getline(g:buf, g:lnum - 1) =~ "done"', 3000)
  let line = term_getline(g:buf, g:lnum)
  if line !~ 'done'
    let line = term_getline(g:buf, g:lnum - 1)
  endif
  call assert_match('done', line)

  let g:job = term_getjob(g:buf)
  call Stop_shell_in_terminal(g:buf)
  call term_wait(g:buf)
  unlet g:buf
  unlet g:job
  unlet g:lnum
  bwipe
endfunc

func Test_terminal_write_stdin()
  if !executable('wc')
    return
  endif
  new
  call setline(1, ['one', 'two', 'three'])
  %term wc
  call WaitFor('getline(1) != ""')
  let nrs = split(getline('$'))
  call assert_equal(['3', '3', '14'], nrs)
  bwipe

  call setline(1, ['one', 'two', 'three', 'four'])
  2,3term wc
  call WaitFor('getline(1) != ""')
  let nrs = split(getline('$'))
  call assert_equal(['2', '2', '10'], nrs)
  bwipe

  bwipe!
endfunc

func Test_terminal_no_cmd()
  " Todo: make this work on all systems.
  if !has('unix')
    return
  endif
  " Todo: make this work in the GUI
  if !has('gui_running')
    return
  endif
  let buf = term_start('NONE', {})
  call assert_notequal(0, buf)

  let pty = job_info(term_getjob(buf))['tty']
  call assert_notequal('', pty)
  call system('echo "look here" > ' . pty)
  call term_wait(buf)
  call assert_equal('look here', term_getline(buf, 1))
  bwipe!
endfunc
