" Tests for the terminal window.

if !has('terminal')
  finish
endif

source shared.vim
source screendump.vim

let s:python = PythonProg()

" Open a terminal with a shell, assign the job to g:job and return the buffer
" number.
func Run_shell_in_terminal(options)
  if has('win32')
    let buf = term_start([&shell,'/k'], a:options)
  else
    let buf = term_start(&shell, a:options)
  endif

  let termlist = term_list()
  call assert_equal(1, len(termlist))
  call assert_equal(buf, termlist[0])

  let g:job = term_getjob(buf)
  call assert_equal(v:t_job, type(g:job))

  let string = string({'job': term_getjob(buf)})
  call assert_match("{'job': 'process \\d\\+ run'}", string)

  return buf
endfunc

func Test_terminal_basic()
  au BufWinEnter * if &buftype == 'terminal' | let b:done = 'yes' | endif
  let buf = Run_shell_in_terminal({})

  if has("unix")
    call assert_match('^/dev/', job_info(g:job).tty_out)
    call assert_match('^/dev/', term_gettty(''))
  else
    call assert_match('^\\\\.\\pipe\\', job_info(g:job).tty_out)
    call assert_match('^\\\\.\\pipe\\', term_gettty(''))
  endif
  call assert_equal('t', mode())
  call assert_equal('yes', b:done)
  call assert_match('%aR[^\n]*running]', execute('ls'))

  call Stop_shell_in_terminal(buf)
  call term_wait(buf)
  call assert_equal('n', mode())
  call assert_match('%aF[^\n]*finished]', execute('ls'))

  " closing window wipes out the terminal buffer a with finished job
  close
  call assert_equal("", bufname(buf))

  au! BufWinEnter
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

func Test_terminal_split_quit()
  let buf = Run_shell_in_terminal({})
  call term_wait(buf)
  split
  quit!
  call term_wait(buf)
  sleep 50m
  call assert_equal('run', job_status(g:job))

  quit!
  call WaitFor('job_status(g:job) == "dead"')
  call assert_equal('dead', job_status(g:job))

  exe buf . 'bwipe'
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

func Get_cat_123_cmd()
  if has('win32')
    return 'cmd /c "cls && color 2 && echo 123"'
  else
    call writefile(["\<Esc>[32m123"], 'Xtext')
    return "cat Xtext"
  endif
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

func Test_terminal_scrape_123()
  let cmd = Get_cat_123_cmd()
  let buf = term_start(cmd)

  let termlist = term_list()
  call assert_equal(1, len(termlist))
  call assert_equal(buf, termlist[0])

  " Nothing happens with invalid buffer number
  call term_wait(1234)

  call term_wait(buf)
  " On MS-Windows we first get a startup message of two lines, wait for the
  " "cls" to happen, after that we have one line with three characters.
  call WaitFor({-> len(term_scrape(buf, 1)) == 3})
  call Check_123(buf)

  " Must still work after the job ended.
  let job = term_getjob(buf)
  call WaitFor({-> job_status(job) == "dead"})
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
    let buf = term_start("cmd /K chcp 65001")
    call term_sendkeys(buf, "type Xtext\<CR>")
    call term_sendkeys(buf, "exit\<CR>")
    let line = 4
  else
    let buf = term_start("cat Xtext")
    let line = 1
  endif

  call WaitFor({-> len(term_scrape(buf, line)) >= 7 && term_scrape(buf, line)[0].chars == "l"})
  let l = term_scrape(buf, line)
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

  let job = term_getjob(buf)
  call WaitFor({-> job_status(job) == "dead"})
  call term_wait(buf)

  exe buf . 'bwipe'
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

  let job = term_getjob(buf)
  call WaitFor({-> job_status(job) == "dead"})
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

  call delete('Xtext')
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
  call delete('Xtext')
endfunc

func s:get_sleep_cmd()
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
  return [cmd, waittime]
endfunc

func Test_terminal_finish_open_close()
  call assert_equal(1, winnr('$'))

  let [cmd, waittime] = s:get_sleep_cmd()

  exe 'terminal ++close ' . cmd
  call assert_equal(2, winnr('$'))
  wincmd p
  call WaitFor("winnr('$') == 1", waittime)

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
  if !executable('pwd')
    return
  endif
  call mkdir('Xdir')
  let buf = term_start('pwd', {'cwd': 'Xdir'})
  call WaitFor('"Xdir" == fnamemodify(getline(1), ":t")')
  call assert_equal('Xdir', fnamemodify(getline(1), ":t"))

  exe buf . 'bwipe'
  call delete('Xdir', 'rf')
endfunc

func Test_terminal_servername()
  if !has('clientserver')
    return
  endif
  let g:buf = Run_shell_in_terminal({})
  " Wait for the shell to display a prompt
  call WaitFor('term_getline(g:buf, 1) != ""')
  if has('win32')
    call term_sendkeys(g:buf, "echo %VIM_SERVERNAME%\r")
  else
    call term_sendkeys(g:buf, "echo $VIM_SERVERNAME\r")
  endif
  call term_wait(g:buf)
  call Stop_shell_in_terminal(g:buf)
  call WaitFor('getline(2) == v:servername')
  call assert_equal(v:servername, getline(2))

  exe g:buf . 'bwipe'
  unlet g:buf
endfunc

func Test_terminal_env()
  let g:buf = Run_shell_in_terminal({'env': {'TESTENV': 'correct'}})
  " Wait for the shell to display a prompt
  call WaitFor('term_getline(g:buf, 1) != ""')
  if has('win32')
    call term_sendkeys(g:buf, "echo %TESTENV%\r")
  else
    call term_sendkeys(g:buf, "echo $TESTENV\r")
  endif
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
  let buf = term_start(&shell)
  if has('mac')
    " The shell or something else has a problem dealing with more than 1000
    " characters at the same time.
    let len = 1000
  else
    let len = 5000
  endif

  for c in ['a','b','c','d','e','f','g','h','i','j','k']
    call term_sendkeys(buf, 'echo ' . repeat(c, len) . "\<cr>")
  endfor
  call term_sendkeys(buf, "echo done\<cr>")

  " On MS-Windows there is an extra empty line below "done".  Find "done" in
  " the last-but-one or the last-but-two line.
  let lnum = term_getsize(buf)[0] - 1
  call WaitFor({-> term_getline(buf, lnum) =~ "done" || term_getline(buf, lnum - 1) =~ "done"}, 10000)
  let line = term_getline(buf, lnum)
  if line !~ 'done'
    let line = term_getline(buf, lnum - 1)
  endif
  call assert_match('done', line)

  let g:job = term_getjob(buf)
  call Stop_shell_in_terminal(buf)
  call term_wait(buf)
  unlet g:job
  bwipe
endfunc

func Test_terminal_write_stdin()
  if !executable('wc')
    throw 'skipped: wc command not available'
  endif
  new
  call setline(1, ['one', 'two', 'three'])
  %term wc
  call WaitFor('getline("$") =~ "3"')
  let nrs = split(getline('$'))
  call assert_equal(['3', '3', '14'], nrs)
  bwipe

  new
  call setline(1, ['one', 'two', 'three', 'four'])
  2,3term wc
  call WaitFor('getline("$") =~ "2"')
  let nrs = split(getline('$'))
  call assert_equal(['2', '2', '10'], nrs)
  bwipe

  if executable('python')
    new
    call setline(1, ['print("hello")'])
    1term ++eof=exit() python
    " MS-Windows echoes the input, Unix doesn't.
    call WaitFor('getline("$") =~ "exit" || getline(1) =~ "hello"')
    if getline(1) =~ 'hello'
      call assert_equal('hello', getline(1))
    else
      call assert_equal('hello', getline(line('$') - 1))
    endif
    bwipe

    if has('win32')
      new
      call setline(1, ['print("hello")'])
      1term ++eof=<C-Z> python
      call WaitFor('getline("$") =~ "Z"')
      call assert_equal('hello', getline(line('$') - 1))
      bwipe
    endif
  endif

  bwipe!
endfunc

func Test_terminal_no_cmd()
  " Todo: make this work in the GUI
  if !has('gui_running')
    return
  endif
  let buf = term_start('NONE', {})
  call assert_notequal(0, buf)

  let pty = job_info(term_getjob(buf))['tty_out']
  call assert_notequal('', pty)
  if has('win32')
    silent exe '!start cmd /c "echo look here > ' . pty . '"'
  else
    call system('echo "look here" > ' . pty)
  endif
  let g:buf = buf
  call WaitFor('term_getline(g:buf, 1) =~ "look here"')

  call assert_match('look here', term_getline(buf, 1))
  bwipe!
endfunc

func Test_terminal_special_chars()
  " this file name only works on Unix
  if !has('unix')
    return
  endif
  call mkdir('Xdir with spaces')
  call writefile(['x'], 'Xdir with spaces/quoted"file')
  term ls Xdir\ with\ spaces/quoted\"file
  call WaitFor('term_getline("", 1) =~ "quoted"')
  call assert_match('quoted"file', term_getline('', 1))
  call term_wait('')

  call delete('Xdir with spaces', 'rf')
  bwipe
endfunc

func Test_terminal_wrong_options()
  call assert_fails('call term_start(&shell, {
	\ "in_io": "file",
	\ "in_name": "xxx",
	\ "out_io": "file",
	\ "out_name": "xxx",
	\ "err_io": "file",
	\ "err_name": "xxx"
	\ })', 'E474:')
  call assert_fails('call term_start(&shell, {
	\ "out_buf": bufnr("%")
	\ })', 'E474:')
  call assert_fails('call term_start(&shell, {
	\ "err_buf": bufnr("%")
	\ })', 'E474:')
endfunc

func Test_terminal_redir_file()
  " TODO: this should work on MS-Window
  if has('unix')
    let cmd = Get_cat_123_cmd()
    let buf = term_start(cmd, {'out_io': 'file', 'out_name': 'Xfile'})
    call term_wait(buf)
    call WaitFor('len(readfile("Xfile")) > 0')
    call assert_match('123', readfile('Xfile')[0])
    let g:job = term_getjob(buf)
    call WaitFor('job_status(g:job) == "dead"')
    call delete('Xfile')
    bwipe
  endif

  if has('unix')
    call writefile(['one line'], 'Xfile')
    let buf = term_start('cat', {'in_io': 'file', 'in_name': 'Xfile'})
    call term_wait(buf)
    call WaitFor('term_getline(' . buf . ', 1) == "one line"')
    call assert_equal('one line', term_getline(buf, 1))
    let g:job = term_getjob(buf)
    call WaitFor('job_status(g:job) == "dead"')
    bwipe
    call delete('Xfile')
  endif
endfunc

func TerminalTmap(remap)
  let buf = Run_shell_in_terminal({})
  call assert_equal('t', mode())

  if a:remap
    tmap 123 456
  else
    tnoremap 123 456
  endif
  " don't use abcde, it's an existing command
  tmap 456 abxde
  call assert_equal('456', maparg('123', 't'))
  call assert_equal('abxde', maparg('456', 't'))
  call feedkeys("123", 'tx')
  let g:buf = buf
  call WaitFor("term_getline(g:buf,term_getcursor(g:buf)[0]) =~ 'abxde\\|456'")
  let lnum = term_getcursor(buf)[0]
  if a:remap
    call assert_match('abxde', term_getline(buf, lnum))
  else
    call assert_match('456', term_getline(buf, lnum))
  endif

  call term_sendkeys(buf, "\r")
  call Stop_shell_in_terminal(buf)
  call term_wait(buf)

  tunmap 123
  tunmap 456
  call assert_equal('', maparg('123', 't'))
  close
  unlet g:job
endfunc

func Test_terminal_tmap()
  call TerminalTmap(1)
  call TerminalTmap(0)
endfunc

func Test_terminal_wall()
  let buf = Run_shell_in_terminal({})
  wall
  call Stop_shell_in_terminal(buf)
  call term_wait(buf)
  exe buf . 'bwipe'
  unlet g:job
endfunc

func Test_terminal_wqall()
  let buf = Run_shell_in_terminal({})
  call assert_fails('wqall', 'E948')
  call Stop_shell_in_terminal(buf)
  call term_wait(buf)
  exe buf . 'bwipe'
  unlet g:job
endfunc

func Test_terminal_composing_unicode()
  let save_enc = &encoding
  set encoding=utf-8

  if has('win32')
    let cmd = "cmd /K chcp 65001"
    let lnum = [3, 6, 9]
  else
    let cmd = &shell
    let lnum = [1, 3, 5]
  endif

  enew
  let buf = term_start(cmd, {'curwin': bufnr('')})
  let g:job = term_getjob(buf)
  call term_wait(buf, 50)

  " ascii + composing
  let txt = "a\u0308bc"
  call term_sendkeys(buf, "echo " . txt . "\r")
  call term_wait(buf, 50)
  call assert_match("echo " . txt, term_getline(buf, lnum[0]))
  call assert_equal(txt, term_getline(buf, lnum[0] + 1))
  let l = term_scrape(buf, lnum[0] + 1)
  call assert_equal("a\u0308", l[0].chars)
  call assert_equal("b", l[1].chars)
  call assert_equal("c", l[2].chars)

  " multibyte + composing
  let txt = "\u304b\u3099\u304e\u304f\u3099\u3052\u3053\u3099"
  call term_sendkeys(buf, "echo " . txt . "\r")
  call term_wait(buf, 50)
  call assert_match("echo " . txt, term_getline(buf, lnum[1]))
  call assert_equal(txt, term_getline(buf, lnum[1] + 1))
  let l = term_scrape(buf, lnum[1] + 1)
  call assert_equal("\u304b\u3099", l[0].chars)
  call assert_equal("\u304e", l[1].chars)
  call assert_equal("\u304f\u3099", l[2].chars)
  call assert_equal("\u3052", l[3].chars)
  call assert_equal("\u3053\u3099", l[4].chars)

  " \u00a0 + composing
  let txt = "abc\u00a0\u0308"
  call term_sendkeys(buf, "echo " . txt . "\r")
  call term_wait(buf, 50)
  call assert_match("echo " . txt, term_getline(buf, lnum[2]))
  call assert_equal(txt, term_getline(buf, lnum[2] + 1))
  let l = term_scrape(buf, lnum[2] + 1)
  call assert_equal("\u00a0\u0308", l[3].chars)

  call term_sendkeys(buf, "exit\r")
  call WaitFor('job_status(g:job) == "dead"')
  call assert_equal('dead', job_status(g:job))
  bwipe!
  unlet g:job
  let &encoding = save_enc
endfunc

func Test_terminal_aucmd_on_close()
  fun Nop()
    let s:called = 1
  endfun

  aug repro
      au!
      au BufWinLeave * call Nop()
  aug END

  let [cmd, waittime] = s:get_sleep_cmd()

  call assert_equal(1, winnr('$'))
  new
  call setline(1, ['one', 'two'])
  exe 'term ++close ' . cmd
  wincmd p
  call WaitFor("winnr('$') == 2", waittime)
  call assert_equal(1, s:called)
  bwipe!

  unlet s:called
  au! repro
  delfunc Nop
endfunc

func Test_terminal_term_start_empty_command()
  let cmd = "call term_start('', {'curwin' : 1, 'term_finish' : 'close'})"
  call assert_fails(cmd, 'E474')
  let cmd = "call term_start('', {'curwin' : 1, 'term_finish' : 'close'})"
  call assert_fails(cmd, 'E474')
  let cmd = "call term_start({}, {'curwin' : 1, 'term_finish' : 'close'})"
  call assert_fails(cmd, 'E474')
  let cmd = "call term_start(0, {'curwin' : 1, 'term_finish' : 'close'})"
  call assert_fails(cmd, 'E474')
endfunc

func Test_terminal_response_to_control_sequence()
  if !has('unix')
    return
  endif

  let buf = Run_shell_in_terminal({})
  call term_wait(buf)

  new
  call setline(1, "\x1b[6n")
  write! Xescape
  bwipe
  call term_sendkeys(buf, "cat Xescape\<cr>")

  " wait for the response of control sequence from libvterm (and send it to tty)
  sleep 200m
  call term_wait(buf)

  " Wait for output from tty to display, below an empty line.
  " It should show \e3;1R, but only 1R may show up
  call assert_match('\<\d\+R', term_getline(buf, 3))

  call term_sendkeys(buf, "\<c-c>")
  call term_wait(buf)
  call Stop_shell_in_terminal(buf)

  exe buf . 'bwipe'
  call delete('Xescape')
  unlet g:job
endfunc

" Run Vim in a terminal, then start a terminal in that Vim with a kill
" argument, check that :qall works.
func Test_terminal_qall_kill_arg()
  if !CanRunVimInTerminal()
    return
  endif
  let buf = RunVimInTerminal('', {})

  " Open a terminal window and wait for the prompt to appear
  call term_sendkeys(buf, ":term ++kill=kill\<CR>")
  call WaitFor({-> term_getline(buf, 10) =~ '\[running]'})
  call WaitFor({-> term_getline(buf, 1) !~ '^\s*$'})

  " make Vim exit, it will kill the shell
  call term_sendkeys(buf, "\<C-W>:qall\<CR>")
  call WaitFor({-> term_getstatus(buf) == "finished"})

  " close the terminal window where Vim was running
  quit
endfunc

" Run Vim in a terminal, then start a terminal in that Vim with a kill
" argument, check that :qall works.
func Test_terminal_qall_kill_func()
  if !CanRunVimInTerminal()
    return
  endif
  let buf = RunVimInTerminal('', {})

  " Open a terminal window and wait for the prompt to appear
  call term_sendkeys(buf, ":term\<CR>")
  call WaitFor({-> term_getline(buf, 10) =~ '\[running]'})
  call WaitFor({-> term_getline(buf, 1) !~ '^\s*$'})

  " set kill using term_setkill()
  call term_sendkeys(buf, "\<C-W>:call term_setkill(bufnr('%'), 'kill')\<CR>")

  " make Vim exit, it will kill the shell
  call term_sendkeys(buf, "\<C-W>:qall\<CR>")
  call WaitFor({-> term_getstatus(buf) == "finished"})

  " close the terminal window where Vim was running
  quit
endfunc

" Run Vim in a terminal, then start a terminal in that Vim without a kill
" argument, check that :confirm qall works.
func Test_terminal_qall_prompt()
  if !CanRunVimInTerminal()
    return
  endif
  let buf = RunVimInTerminal('', {})

  " Open a terminal window and wait for the prompt to appear
  call term_sendkeys(buf, ":term\<CR>")
  call WaitFor({-> term_getline(buf, 10) =~ '\[running]'})
  call WaitFor({-> term_getline(buf, 1) !~ '^\s*$'})

  " make Vim exit, it will prompt to kill the shell
  call term_sendkeys(buf, "\<C-W>:confirm qall\<CR>")
  call WaitFor({-> term_getline(buf, 20) =~ 'ancel:'})
  call term_sendkeys(buf, "y")
  call WaitFor({-> term_getstatus(buf) == "finished"})

  " close the terminal window where Vim was running
  quit
endfunc

func Test_terminalopen_autocmd()
  augroup repro
    au!
    au TerminalOpen * let s:called += 1
  augroup END

  let s:called = 0

  " Open a terminal window with :terminal
  terminal
  call assert_equal(1, s:called)
  bwipe!

  " Open a terminal window with term_start()
  call term_start(&shell)
  call assert_equal(2, s:called)
  bwipe!

  " Open a hidden terminal buffer with :terminal
  terminal ++hidden
  call assert_equal(3, s:called)
  for buf in term_list()
    exe buf . "bwipe!"
  endfor

  " Open a hidden terminal buffer with term_start()
  let buf = term_start(&shell, {'hidden': 1})
  call assert_equal(4, s:called)
  exe buf . "bwipe!"

  unlet s:called
  au! repro
endfunction
