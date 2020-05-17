" Tests for the terminal window.

source check.vim
CheckFeature terminal

source shared.vim
source screendump.vim
source mouse.vim

let s:python = PythonProg()
let $PROMPT_COMMAND=''

" Open a terminal with a shell, assign the job to g:job and return the buffer
" number.
func Run_shell_in_terminal(options)
  if has('win32')
    let buf = term_start([&shell,'/k'], a:options)
  else
    let buf = term_start(&shell, a:options)
  endif
  let g:test_is_flaky = 1

  let termlist = term_list()
  call assert_equal(1, len(termlist))
  call assert_equal(buf, termlist[0])

  let g:job = term_getjob(buf)
  call assert_equal(v:t_job, type(g:job))

  let string = string({'job': buf->term_getjob()})
  call assert_match("{'job': 'process \\d\\+ run'}", string)

  return buf
endfunc

func Test_terminal_basic()
  au TerminalOpen * let b:done = 'yes'
  let buf = Run_shell_in_terminal({})

  call assert_equal('t', mode())
  call assert_equal('yes', b:done)
  call assert_match('%aR[^\n]*running]', execute('ls'))
  call assert_match('%aR[^\n]*running]', execute('ls R'))
  call assert_notmatch('%[^\n]*running]', execute('ls F'))
  call assert_notmatch('%[^\n]*running]', execute('ls ?'))
  call assert_fails('set modifiable', 'E946:')

  call StopShellInTerminal(buf)
  call TermWait(buf)
  call assert_equal('n', mode())
  call assert_match('%aF[^\n]*finished]', execute('ls'))
  call assert_match('%aF[^\n]*finished]', execute('ls F'))
  call assert_notmatch('%[^\n]*finished]', execute('ls R'))
  call assert_notmatch('%[^\n]*finished]', execute('ls ?'))

  " closing window wipes out the terminal buffer a with finished job
  close
  call assert_equal("", bufname(buf))

  au! TerminalOpen
  unlet g:job
endfunc

func Test_terminal_TerminalWinOpen()
  au TerminalWinOpen * let b:done = 'yes'
  let buf = Run_shell_in_terminal({})
  call assert_equal('yes', b:done)
  call StopShellInTerminal(buf)
  " closing window wipes out the terminal buffer with the finished job
  close

  if has("unix")
    terminal ++hidden ++open sleep 1
    sleep 1
    call assert_fails("echo b:done", 'E121:')
  endif

  au! TerminalWinOpen
endfunc

func Test_terminal_make_change()
  let buf = Run_shell_in_terminal({})
  call StopShellInTerminal(buf)
  call TermWait(buf)

  setlocal modifiable
  exe "normal Axxx\<Esc>"
  call assert_fails(buf . 'bwipe', 'E517')
  undo

  exe buf . 'bwipe'
  unlet g:job
endfunc

func Test_terminal_paste_register()
  let @" = "text to paste"

  let buf = Run_shell_in_terminal({})
  " Wait for the shell to display a prompt
  call WaitForAssert({-> assert_notequal('', term_getline(buf, 1))})

  call feedkeys("echo \<C-W>\"\" \<C-W>\"=37 + 5\<CR>\<CR>", 'xt')
  call WaitForAssert({-> assert_match("echo text to paste 42$", getline(1))})
  call WaitForAssert({-> assert_equal('text to paste 42',       2->getline())})

  exe buf . 'bwipe!'
  unlet g:job
endfunc

func Test_terminal_wipe_buffer()
  let buf = Run_shell_in_terminal({})
  call assert_fails(buf . 'bwipe', 'E517')
  exe buf . 'bwipe!'
  call WaitForAssert({-> assert_equal('dead', job_status(g:job))})
  call assert_equal("", bufname(buf))

  unlet g:job
endfunc

func Test_terminal_split_quit()
  let buf = Run_shell_in_terminal({})
  call TermWait(buf)
  split
  quit!
  call TermWait(buf)
  sleep 50m
  call assert_equal('run', job_status(g:job))

  quit!
  call WaitForAssert({-> assert_equal('dead', job_status(g:job))})

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
  call StopShellInTerminal(buf)
  exe buf . 'bwipe'

  unlet g:job
endfunc

func s:Nasty_exit_cb(job, st)
  exe g:buf . 'bwipe!'
  let g:buf = 0
endfunc

func Get_cat_123_cmd()
  if has('win32')
    if !has('conpty')
      return 'cmd /c "cls && color 2 && echo 123"'
    else
      " When clearing twice, extra sequence is not output.
      return 'cmd /c "cls && cls && color 2 && echo 123"'
    endif
  else
    call writefile(["\<Esc>[32m123"], 'Xtext')
    return "cat Xtext"
  endif
endfunc

func Test_terminal_nasty_cb()
  let cmd = Get_cat_123_cmd()
  let g:buf = term_start(cmd, {'exit_cb': function('s:Nasty_exit_cb')})
  let g:job = term_getjob(g:buf)

  call WaitForAssert({-> assert_equal("dead", job_status(g:job))})
  call WaitForAssert({-> assert_equal(0, g:buf)})
  unlet g:job
  unlet g:buf
  call delete('Xtext')
endfunc

func Check_123(buf)
  let l = term_scrape(a:buf, 0)
  call assert_true(len(l) == 0)
  let l = term_scrape(a:buf, 999)
  call assert_true(len(l) == 0)
  let l = a:buf->term_scrape(1)
  call assert_true(len(l) > 0)
  call assert_equal('1', l[0].chars)
  call assert_equal('2', l[1].chars)
  call assert_equal('3', l[2].chars)
  call assert_equal('#00e000', l[0].fg)
  call assert_equal(0, term_getattr(l[0].attr, 'bold'))
  call assert_equal(0, l[0].attr->term_getattr('italic'))
  if has('win32')
    " On Windows 'background' always defaults to dark, even though the terminal
    " may use a light background.  Therefore accept both white and black.
    call assert_match('#ffffff\|#000000', l[0].bg)
  else
    if &background == 'light'
      call assert_equal('#ffffff', l[0].bg)
    else
      call assert_equal('#000000', l[0].bg)
    endif
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

  call TermWait(buf)
  " On MS-Windows we first get a startup message of two lines, wait for the
  " "cls" to happen, after that we have one line with three characters.
  call WaitForAssert({-> assert_equal(3, len(term_scrape(buf, 1)))})
  call Check_123(buf)

  " Must still work after the job ended.
  let job = term_getjob(buf)
  call WaitForAssert({-> assert_equal("dead", job_status(job))})
  call TermWait(buf)
  call Check_123(buf)

  exe buf . 'bwipe'
  call delete('Xtext')
endfunc

func Test_terminal_scrape_multibyte()
  call writefile(["léttまrs"], 'Xtext')
  if has('win32')
    " Run cmd with UTF-8 codepage to make the type command print the expected
    " multibyte characters.
    let buf = term_start("cmd /K chcp 65001")
    call term_sendkeys(buf, "type Xtext\<CR>")
    eval buf->term_sendkeys("exit\<CR>")
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
  call WaitForAssert({-> assert_equal("dead", job_status(job))})
  call TermWait(buf)

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
  call WaitForAssert({-> assert_equal("dead", job_status(job))})
  call TermWait(buf)

  " wait until the scrolling stops
  while 1
    let scrolled = buf->term_getscrolled()
    sleep 20m
    if scrolled == buf->term_getscrolled()
      break
    endif
  endwhile

  call assert_equal('1', getline(1))
  call assert_equal('1', term_getline(buf, 1 - scrolled))
  call assert_equal('49', getline(49))
  call assert_equal('49', term_getline(buf, 49 - scrolled))
  call assert_equal('200', getline(200))
  call assert_equal('200', term_getline(buf, 200 - scrolled))

  exe buf . 'bwipe'
  call delete('Xtext')
endfunc

func Test_terminal_scrollback()
  let buf = Run_shell_in_terminal({'term_rows': 15})
  set termwinscroll=100
  call writefile(range(150), 'Xtext')
  if has('win32')
    call term_sendkeys(buf, "type Xtext\<CR>")
  else
    call term_sendkeys(buf, "cat Xtext\<CR>")
  endif
  let rows = term_getsize(buf)[0]
  " On MS-Windows there is an empty line, check both last line and above it.
  call WaitForAssert({-> assert_match( '149', term_getline(buf, rows - 1) . term_getline(buf, rows - 2))})
  let lines = line('$')
  call assert_inrange(91, 100, lines)

  call StopShellInTerminal(buf)
  call TermWait(buf)
  exe buf . 'bwipe'
  set termwinscroll&
  call delete('Xtext')
endfunc

func Test_terminal_postponed_scrollback()
  " tail -f only works on Unix
  CheckUnix

  call writefile(range(50), 'Xtext')
  call writefile([
	\ 'set shell=/bin/sh noruler',
	\ 'terminal',
	\ 'sleep 200m',
	\ 'call feedkeys("tail -n 100 -f Xtext\<CR>", "xt")',
	\ 'sleep 100m',
	\ 'call feedkeys("\<C-W>N", "xt")',
	\ ], 'XTest_postponed')
  let buf = RunVimInTerminal('-S XTest_postponed', {})
  " Check that the Xtext lines are displayed and in Terminal-Normal mode
  call VerifyScreenDump(buf, 'Test_terminal_scrollback_1', {})

  silent !echo 'one more line' >>Xtext
  " Screen will not change, move cursor to get a different dump
  call term_sendkeys(buf, "k")
  call VerifyScreenDump(buf, 'Test_terminal_scrollback_2', {})

  " Back to Terminal-Job mode, text will scroll and show the extra line.
  call term_sendkeys(buf, "a")
  call VerifyScreenDump(buf, 'Test_terminal_scrollback_3', {})

  " stop "tail -f"
  call term_sendkeys(buf, "\<C-C>")
  call TermWait(buf, 25)
  " stop shell
  call term_sendkeys(buf, "exit\<CR>")
  call TermWait(buf, 50)
  " close terminal window
  let tsk_ret = term_sendkeys(buf, ":q\<CR>")

  " check type of term_sendkeys() return value
  echo type(tsk_ret)

  call StopVimInTerminal(buf)
  call delete('XTest_postponed')
  call delete('Xtext')
endfunc

" Run diff on two dumps with different size.
func Test_terminal_dumpdiff_size()
  call assert_equal(1, winnr('$'))
  call term_dumpdiff('dumps/Test_incsearch_search_01.dump', 'dumps/Test_popup_command_01.dump')
  call assert_equal(2, winnr('$'))
  call assert_match('Test_incsearch_search_01.dump', getline(10))
  call assert_match('      +++++$', getline(11))
  call assert_match('Test_popup_command_01.dump', getline(31))
  call assert_equal(repeat('+', 75), getline(30))
  quit
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
  call assert_equal([5, 33], ''->term_getsize())

  call term_setsize('', 6, 0)
  call assert_equal([6, 33], term_getsize(''))

  eval ''->term_setsize(0, 35)
  call assert_equal([6, 35], term_getsize(''))

  call term_setsize('', 7, 30)
  call assert_equal([7, 30], term_getsize(''))

  bwipe!
  call assert_fails("call term_setsize('', 7, 30)", "E955:")

  call term_start(cmd, {'term_rows': 6, 'term_cols': 36})
  let size = term_getsize('')
  bwipe!
  call assert_equal([6, 36], size)

  exe 'vertical terminal ++cols=20 ' . cmd
  let size = term_getsize('')
  bwipe!
  call assert_equal(20, size[1])

  eval cmd->term_start({'vertical': 1, 'term_cols': 26})
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
    " 500 was not enough for Travis
    let waittime = 900
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

  " shell terminal closes automatically
  terminal
  let buf = bufnr('%')
  call assert_equal(2, winnr('$'))
  " Wait for the shell to display a prompt
  call WaitForAssert({-> assert_notequal('', term_getline(buf, 1))})
  call StopShellInTerminal(buf)
  call WaitForAssert({-> assert_equal(1, winnr('$'))}, waittime)

  " shell terminal that does not close automatically
  terminal ++noclose
  let buf = bufnr('%')
  call assert_equal(2, winnr('$'))
  " Wait for the shell to display a prompt
  call WaitForAssert({-> assert_notequal('', term_getline(buf, 1))})
  call StopShellInTerminal(buf)
  call assert_equal(2, winnr('$'))
  quit
  call assert_equal(1, winnr('$'))

  exe 'terminal ++close ' . cmd
  call assert_equal(2, winnr('$'))
  wincmd p
  call WaitForAssert({-> assert_equal(1, winnr('$'))}, waittime)

  call term_start(cmd, {'term_finish': 'close'})
  call assert_equal(2, winnr('$'))
  wincmd p
  call WaitForAssert({-> assert_equal(1, winnr('$'))}, waittime)
  call assert_equal(1, winnr('$'))

  exe 'terminal ++open ' . cmd
  close!
  call WaitForAssert({-> assert_equal(2, winnr('$'))}, waittime)
  bwipe

  call term_start(cmd, {'term_finish': 'open'})
  close!
  call WaitForAssert({-> assert_equal(2, winnr('$'))}, waittime)
  bwipe

  exe 'terminal ++hidden ++open ' . cmd
  call assert_equal(1, winnr('$'))
  call WaitForAssert({-> assert_equal(2, winnr('$'))}, waittime)
  bwipe

  call term_start(cmd, {'term_finish': 'open', 'hidden': 1})
  call assert_equal(1, winnr('$'))
  call WaitForAssert({-> assert_equal(2, winnr('$'))}, waittime)
  bwipe

  call assert_fails("call term_start(cmd, {'term_opencmd': 'open'})", 'E475:')
  call assert_fails("call term_start(cmd, {'term_opencmd': 'split %x'})", 'E475:')
  call assert_fails("call term_start(cmd, {'term_opencmd': 'split %d and %s'})", 'E475:')
  call assert_fails("call term_start(cmd, {'term_opencmd': 'split % and %d'})", 'E475:')

  call term_start(cmd, {'term_finish': 'open', 'term_opencmd': '4split | buffer %d'})
  close!
  call WaitForAssert({-> assert_equal(2, winnr('$'))}, waittime)
  call assert_equal(4, winheight(0))
  bwipe
endfunc

func Test_terminal_cwd()
  if has('win32')
    let cmd = 'cmd /c cd'
  else
    CheckExecutable pwd
    let cmd = 'pwd'
  endif
  call mkdir('Xdir')
  let buf = term_start(cmd, {'cwd': 'Xdir'})
  call WaitForAssert({-> assert_equal('Xdir', fnamemodify(getline(1), ":t"))})

  exe buf . 'bwipe'
  call delete('Xdir', 'rf')
endfunc

func Test_terminal_cwd_failure()
  " Case 1: Provided directory is not actually a directory.  Attempt to make
  " the file executable as well.
  call writefile([], 'Xfile')
  call setfperm('Xfile', 'rwx------')
  call assert_fails("call term_start(&shell, {'cwd': 'Xfile'})", 'E475:')
  call delete('Xfile')

  " Case 2: Directory does not exist.
  call assert_fails("call term_start(&shell, {'cwd': 'Xdir'})", 'E475:')

  " Case 3: Directory exists but is not accessible.
  " Skip this for root, it will be accessible anyway.
  if !IsRoot()
    call mkdir('XdirNoAccess', '', '0600')
    " return early if the directory permissions could not be set properly
    if getfperm('XdirNoAccess')[2] == 'x'
      call delete('XdirNoAccess', 'rf')
      return
    endif
    call assert_fails("call term_start(&shell, {'cwd': 'XdirNoAccess'})", 'E475:')
    call delete('XdirNoAccess', 'rf')
  endif
endfunc

func Test_terminal_servername()
  if !has('clientserver')
    return
  endif
  call s:test_environment("VIM_SERVERNAME", v:servername)
endfunc

func Test_terminal_version()
  call s:test_environment("VIM_TERMINAL", string(v:version))
endfunc

func s:test_environment(name, value)
  let buf = Run_shell_in_terminal({})
  " Wait for the shell to display a prompt
  call WaitForAssert({-> assert_notequal('', term_getline(buf, 1))})
  if has('win32')
    call term_sendkeys(buf, "echo %" . a:name . "%\r")
  else
    call term_sendkeys(buf, "echo $" . a:name . "\r")
  endif
  call TermWait(buf)
  call StopShellInTerminal(buf)
  call WaitForAssert({-> assert_equal(a:value, getline(2))})

  exe buf . 'bwipe'
  unlet buf
endfunc

func Test_terminal_env()
  let buf = Run_shell_in_terminal({'env': {'TESTENV': 'correct'}})
  " Wait for the shell to display a prompt
  call WaitForAssert({-> assert_notequal('', term_getline(buf, 1))})
  if has('win32')
    call term_sendkeys(buf, "echo %TESTENV%\r")
  else
    call term_sendkeys(buf, "echo $TESTENV\r")
  endif
  eval buf->TermWait()
  call StopShellInTerminal(buf)
  call WaitForAssert({-> assert_equal('correct', getline(2))})

  exe buf . 'bwipe'
endfunc

func Test_terminal_list_args()
  let buf = term_start([&shell, &shellcmdflag, 'echo "123"'])
  call assert_fails(buf . 'bwipe', 'E517')
  exe buf . 'bwipe!'
  call assert_equal("", bufname(buf))
endfunction

func Test_terminal_noblock()
  let buf = term_start(&shell)
  let wait_time = 5000
  let letters = 'abcdefghijklmnopqrstuvwxyz'
  if has('bsd') || has('mac') || has('sun')
    " The shell or something else has a problem dealing with more than 1000
    " characters at the same time.  It's very slow too.
    let len = 1000
    let wait_time = 15000
    let letters = 'abcdefghijklm'
  " NPFS is used in Windows, nonblocking mode does not work properly.
  elseif has('win32')
    let len = 1
  else
    let len = 5000
  endif

  " Send a lot of text lines, should be buffered properly.
  for c in split(letters, '\zs')
    call term_sendkeys(buf, 'echo ' . repeat(c, len) . "\<cr>")
  endfor
  call term_sendkeys(buf, "echo done\<cr>")

  " On MS-Windows there is an extra empty line below "done".  Find "done" in
  " the last-but-one or the last-but-two line.
  let lnum = term_getsize(buf)[0] - 1
  call WaitForAssert({-> assert_match('done', term_getline(buf, lnum - 1) .. '//' .. term_getline(buf, lnum))}, wait_time)
  let line = term_getline(buf, lnum)
  if line !~ 'done'
    let line = term_getline(buf, lnum - 1)
  endif
  call assert_match('done', line)

  let g:job = term_getjob(buf)
  call StopShellInTerminal(buf)
  call TermWait(buf)
  unlet g:job
  bwipe
endfunc

func Test_terminal_write_stdin()
  " TODO: enable once writing to stdin works on MS-Windows
  CheckNotMSWindows
  CheckExecutable wc

  call setline(1, ['one', 'two', 'three'])
  %term wc
  call WaitForAssert({-> assert_match('3', getline("$"))})
  let nrs = split(getline('$'))
  call assert_equal(['3', '3', '14'], nrs)
  %bwipe!

  call setline(1, ['one', 'two', 'three', 'four'])
  2,3term wc
  call WaitForAssert({-> assert_match('2', getline("$"))})
  let nrs = split(getline('$'))
  call assert_equal(['2', '2', '10'], nrs)
  %bwipe!
endfunc

func Test_terminal_eof_arg()
  call CheckPython(s:python)

  call setline(1, ['print("hello")'])
  exe '1term ++eof=exit(123) ' .. s:python
  " MS-Windows echoes the input, Unix doesn't.
  if has('win32')
    call WaitFor({-> getline('$') =~ 'exit(123)'})
    call assert_equal('hello', getline(line('$') - 1))
  else
    call WaitFor({-> getline('$') =~ 'hello'})
    call assert_equal('hello', getline('$'))
  endif
  call assert_equal(123, bufnr()->term_getjob()->job_info().exitval)
  %bwipe!
endfunc

func Test_terminal_eof_arg_win32_ctrl_z()
  CheckMSWindows
  call CheckPython(s:python)

  call setline(1, ['print("hello")'])
  exe '1term ++eof=<C-Z> ' .. s:python
  call WaitForAssert({-> assert_match('\^Z', getline(line('$') - 1))})
  call assert_match('\^Z', getline(line('$') - 1))
  %bwipe!
endfunc

func Test_terminal_duplicate_eof_arg()
  call CheckPython(s:python)

  " Check the last specified ++eof arg is used and should not memory leak.
  new
  call setline(1, ['print("hello")'])
  exe '1term ++eof=<C-Z> ++eof=exit(123) ' .. s:python
  " MS-Windows echoes the input, Unix doesn't.
  if has('win32')
    call WaitFor({-> getline('$') =~ 'exit(123)'})
    call assert_equal('hello', getline(line('$') - 1))
  else
    call WaitFor({-> getline('$') =~ 'hello'})
    call assert_equal('hello', getline('$'))
  endif
  call assert_equal(123, bufnr()->term_getjob()->job_info().exitval)
  %bwipe!
endfunc

func Test_terminal_no_cmd()
  let buf = term_start('NONE', {})
  call assert_notequal(0, buf)

  let pty = job_info(term_getjob(buf))['tty_out']
  call assert_notequal('', pty)
  if has('gui_running') && !has('win32')
    " In the GUI job_start() doesn't work, it does not read from the pty.
    call system('echo "look here" > ' . pty)
  else
    " Otherwise using a job works on all systems.
    call job_start([&shell, &shellcmdflag, 'echo "look here" > ' . pty])
  endif
  call WaitForAssert({-> assert_match('look here', term_getline(buf, 1))})

  bwipe!
endfunc

func Test_terminal_special_chars()
  " this file name only works on Unix
  CheckUnix

  call mkdir('Xdir with spaces')
  call writefile(['x'], 'Xdir with spaces/quoted"file')
  term ls Xdir\ with\ spaces/quoted\"file
  call WaitForAssert({-> assert_match('quoted"file', term_getline('', 1))})
  " make sure the job has finished
  call WaitForAssert({-> assert_match('finish', term_getstatus(bufnr()))})

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
  let cmd = Get_cat_123_cmd()
  let buf = term_start(cmd, {'out_io': 'file', 'out_name': 'Xfile'})
  call TermWait(buf)
  " ConPTY may precede escape sequence. There are things that are not so.
  if !has('conpty')
    call WaitForAssert({-> assert_notequal(0, len(readfile("Xfile")))})
    call assert_match('123', readfile('Xfile')[0])
  endif
  let g:job = term_getjob(buf)
  call WaitForAssert({-> assert_equal("dead", job_status(g:job))})
  call delete('Xfile')
  bwipe

  if has('unix')
    call writefile(['one line'], 'Xfile')
    let buf = term_start('cat', {'in_io': 'file', 'in_name': 'Xfile'})
    call TermWait(buf)
    call WaitForAssert({-> assert_equal('one line', term_getline(buf, 1))})
    let g:job = term_getjob(buf)
    call WaitForAssert({-> assert_equal('dead', job_status(g:job))})
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
  call WaitForAssert({-> assert_match('abxde\|456', term_getline(buf, term_getcursor(buf)[0]))})
  let lnum = term_getcursor(buf)[0]
  if a:remap
    call assert_match('abxde', term_getline(buf, lnum))
  else
    call assert_match('456', term_getline(buf, lnum))
  endif

  call term_sendkeys(buf, "\r")
  call StopShellInTerminal(buf)
  call TermWait(buf)

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
  call StopShellInTerminal(buf)
  call TermWait(buf)
  exe buf . 'bwipe'
  unlet g:job
endfunc

func Test_terminal_wqall()
  let buf = Run_shell_in_terminal({})
  call assert_fails('wqall', 'E948')
  call StopShellInTerminal(buf)
  call TermWait(buf)
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
  call WaitFor({-> term_getline(buf, 1) !=# ''}, 1000)

  if has('win32')
    call assert_equal('cmd', job_info(g:job).cmd[0])
  else
    call assert_equal(&shell, job_info(g:job).cmd[0])
  endif

  " ascii + composing
  let txt = "a\u0308bc"
  call term_sendkeys(buf, "echo " . txt)
  call TermWait(buf, 25)
  call assert_match("echo " . txt, term_getline(buf, lnum[0]))
  call term_sendkeys(buf, "\<cr>")
  call WaitForAssert({-> assert_equal(txt, term_getline(buf, lnum[0] + 1))}, 1000)
  let l = term_scrape(buf, lnum[0] + 1)
  call assert_equal("a\u0308", l[0].chars)
  call assert_equal("b", l[1].chars)
  call assert_equal("c", l[2].chars)

  " multibyte + composing
  let txt = "\u304b\u3099\u304e\u304f\u3099\u3052\u3053\u3099"
  call term_sendkeys(buf, "echo " . txt)
  call TermWait(buf, 25)
  call assert_match("echo " . txt, term_getline(buf, lnum[1]))
  call term_sendkeys(buf, "\<cr>")
  call WaitForAssert({-> assert_equal(txt, term_getline(buf, lnum[1] + 1))}, 1000)
  let l = term_scrape(buf, lnum[1] + 1)
  call assert_equal("\u304b\u3099", l[0].chars)
  call assert_equal("\u304e", l[2].chars)
  call assert_equal("\u304f\u3099", l[3].chars)
  call assert_equal("\u3052", l[5].chars)
  call assert_equal("\u3053\u3099", l[6].chars)

  " \u00a0 + composing
  let txt = "abc\u00a0\u0308"
  call term_sendkeys(buf, "echo " . txt)
  call TermWait(buf, 25)
  call assert_match("echo " . txt, term_getline(buf, lnum[2]))
  call term_sendkeys(buf, "\<cr>")
  call WaitForAssert({-> assert_equal(txt, term_getline(buf, lnum[2] + 1))}, 1000)
  let l = term_scrape(buf, lnum[2] + 1)
  call assert_equal("\u00a0\u0308", l[3].chars)

  call term_sendkeys(buf, "exit\r")
  call WaitForAssert({-> assert_equal('dead', job_status(g:job))})
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
  call WaitForAssert({-> assert_equal(2, winnr('$'))}, waittime)
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
  let cmd = "call term_start('', {'term_name' : []})"
  call assert_fails(cmd, 'E475')
  let cmd = "call term_start('', {'term_finish' : 'axby'})"
  call assert_fails(cmd, 'E475')
  let cmd = "call term_start('', {'eof_chars' : []})"
  call assert_fails(cmd, 'E475:')
  let cmd = "call term_start('', {'term_kill' : []})"
  call assert_fails(cmd, 'E475:')
  let cmd = "call term_start('', {'tty_type' : []})"
  call assert_fails(cmd, 'E475:')
  let cmd = "call term_start('', {'tty_type' : 'abc'})"
  call assert_fails(cmd, 'E475:')
  let cmd = "call term_start('', {'term_highlight' : []})"
  call assert_fails(cmd, 'E475:')
  if has('gui')
    let cmd = "call term_start('', {'ansi_colors' : 'abc'})"
    call assert_fails(cmd, 'E475:')
    let cmd = "call term_start('', {'ansi_colors' : [[]]})"
    call assert_fails(cmd, 'E730:')
    let cmd = "call term_start('', {'ansi_colors' : repeat(['blue'], 18)})"
    call assert_fails(cmd, 'E475:')
  endif
endfunc

func Test_terminal_response_to_control_sequence()
  CheckUnix

  let buf = Run_shell_in_terminal({})
  call WaitForAssert({-> assert_notequal('', term_getline(buf, 1))})

  call term_sendkeys(buf, "cat\<CR>")
  call WaitForAssert({-> assert_match('cat', term_getline(buf, 1))})

  " Request the cursor position.
  call term_sendkeys(buf, "\x1b[6n\<CR>")

  " Wait for output from tty to display, below an empty line.
  call WaitForAssert({-> assert_match('3;1R', term_getline(buf, 4))})

  " End "cat" gently.
  call term_sendkeys(buf, "\<CR>\<C-D>")

  call StopShellInTerminal(buf)
  exe buf . 'bwipe'
  unlet g:job
endfunc

" Run Vim, start a terminal in that Vim with the kill argument,
" :qall works.
func Run_terminal_qall_kill(line1, line2)
  " 1. Open a terminal window and wait for the prompt to appear
  " 2. set kill using term_setkill()
  " 3. make Vim exit, it will kill the shell
  let after = [
	\ a:line1,
	\ 'let buf = bufnr("%")',
	\ 'while term_getline(buf, 1) =~ "^\\s*$"',
	\ '  sleep 10m',
	\ 'endwhile',
	\ a:line2,
	\ 'au VimLeavePre * call writefile(["done"], "Xdone")',
	\ 'qall',
	\ ]
  if !RunVim([], after, '')
    return
  endif
  call assert_equal("done", readfile("Xdone")[0])
  call delete("Xdone")
endfunc

" Run Vim in a terminal, then start a terminal in that Vim with a kill
" argument, check that :qall works.
func Test_terminal_qall_kill_arg()
  call Run_terminal_qall_kill('term ++kill=kill', '')
endfunc

" Run Vim, start a terminal in that Vim, set the kill argument with
" term_setkill(), check that :qall works.
func Test_terminal_qall_kill_func()
  call Run_terminal_qall_kill('term', 'eval buf->term_setkill("kill")')
endfunc

" Run Vim, start a terminal in that Vim without the kill argument,
" check that :qall does not exit, :qall! does.
func Test_terminal_qall_exit()
  let after =<< trim [CODE]
    term
    let buf = bufnr("%")
    while term_getline(buf, 1) =~ "^\\s*$"
      sleep 10m
    endwhile
    set nomore
    au VimLeavePre * call writefile(["too early"], "Xdone")
    qall
    au! VimLeavePre * exe buf . "bwipe!" | call writefile(["done"], "Xdone")
    cquit
  [CODE]

  if !RunVim([], after, '')
    return
  endif
  call assert_equal("done", readfile("Xdone")[0])
  call delete("Xdone")
endfunc

" Run Vim in a terminal, then start a terminal in that Vim without a kill
" argument, check that :confirm qall works.
func Test_terminal_qall_prompt()
  CheckRunVimInTerminal
  let buf = RunVimInTerminal('', {})

  " Open a terminal window and wait for the prompt to appear
  call term_sendkeys(buf, ":term\<CR>")
  call WaitForAssert({-> assert_match('\[running]', term_getline(buf, 10))})
  call WaitForAssert({-> assert_notmatch('^\s*$', term_getline(buf, 1))})

  " make Vim exit, it will prompt to kill the shell
  call term_sendkeys(buf, "\<C-W>:confirm qall\<CR>")
  call WaitForAssert({-> assert_match('ancel:', term_getline(buf, 20))})
  call term_sendkeys(buf, "y")
  call WaitForAssert({-> assert_equal('finished', term_getstatus(buf))})

  " close the terminal window where Vim was running
  quit
endfunc

" Run Vim in a terminal, then start a terminal window with a shell and check
" that Vim exits if it is closed.
func Test_terminal_exit()
  CheckRunVimInTerminal

  let lines =<< trim END
     let winid = win_getid()
     help
     term
     let termid = win_getid()
     call win_gotoid(winid)
     close
     call win_gotoid(termid)
  END
  call writefile(lines, 'XtermExit')
  let buf = RunVimInTerminal('-S XtermExit', #{rows: 10})
  let job = term_getjob(buf)
  call WaitForAssert({-> assert_equal("run", job_status(job))})

  " quit the shell, it will make Vim exit
  call term_sendkeys(buf, "exit\<CR>")
  call WaitForAssert({-> assert_equal("dead", job_status(job))})

  call delete('XtermExit')
endfunc

func Test_terminal_open_autocmd()
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

func Check_dump01(off)
  call assert_equal('one two three four five', trim(getline(a:off + 1)))
  call assert_equal('~           Select Word', trim(getline(a:off + 7)))
  call assert_equal(':popup PopUp', trim(getline(a:off + 20)))
endfunc

func Test_terminal_dumpwrite_composing()
  CheckRunVimInTerminal
  let save_enc = &encoding
  set encoding=utf-8
  call assert_equal(1, winnr('$'))

  let text = " a\u0300 e\u0302 o\u0308"
  call writefile([text], 'Xcomposing')
  let buf = RunVimInTerminal('--cmd "set encoding=utf-8" Xcomposing', {})
  call WaitForAssert({-> assert_match(text, term_getline(buf, 1))})
  eval 'Xdump'->term_dumpwrite(buf)
  let dumpline = readfile('Xdump')[0]
  call assert_match('|à| |ê| |ö', dumpline)

  call StopVimInTerminal(buf)
  call delete('Xcomposing')
  call delete('Xdump')
  let &encoding = save_enc
endfunc

" Tests for failures in the term_dumpwrite() function
func Test_terminal_dumpwrite_errors()
  CheckRunVimInTerminal
  call assert_fails("call term_dumpwrite({}, 'Xtest.dump')", 'E728:')
  let buf = RunVimInTerminal('', {})
  call term_wait(buf)
  call assert_fails("call term_dumpwrite(buf, 'Xtest.dump', '')", 'E715:')
  call assert_fails("call term_dumpwrite(buf, [])", 'E730:')
  call writefile([], 'Xtest.dump')
  call assert_fails("call term_dumpwrite(buf, 'Xtest.dump')", 'E953:')
  call delete('Xtest.dump')
  call assert_fails("call term_dumpwrite(buf, '')", 'E482:')
  call assert_fails("call term_dumpwrite(buf, test_null_string())", 'E482:')
  call StopVimInTerminal(buf)
  call term_wait(buf)
  call assert_fails("call term_dumpwrite(buf, 'Xtest.dump')", 'E958:')
  call assert_fails('call term_sendkeys([], ":q\<CR>")', 'E745:')
  call assert_equal(0, term_sendkeys(buf, ":q\<CR>"))
endfunc

" just testing basic functionality.
func Test_terminal_dumpload()
  let curbuf = winbufnr('')
  call assert_equal(1, winnr('$'))
  let buf = term_dumpload('dumps/Test_popup_command_01.dump')
  call assert_equal(2, winnr('$'))
  call assert_equal(20, line('$'))
  call Check_dump01(0)

  " Load another dump in the same window
  let buf2 = 'dumps/Test_diff_01.dump'->term_dumpload({'bufnr': buf})
  call assert_equal(buf, buf2)
  call assert_notequal('one two three four five', trim(getline(1)))

  " Load the first dump again in the same window
  let buf2 = term_dumpload('dumps/Test_popup_command_01.dump', {'bufnr': buf})
  call assert_equal(buf, buf2)
  call Check_dump01(0)

  call assert_fails("call term_dumpload('dumps/Test_popup_command_01.dump', {'bufnr': curbuf})", 'E475:')
  call assert_fails("call term_dumpload('dumps/Test_popup_command_01.dump', {'bufnr': 9999})", 'E86:')
  new
  let closedbuf = winbufnr('')
  quit
  call assert_fails("call term_dumpload('dumps/Test_popup_command_01.dump', {'bufnr': closedbuf})", 'E475:')
  call assert_fails('call term_dumpload([])', 'E474:')
  call assert_fails('call term_dumpload("xabcy.dump")', 'E485:')

  quit
endfunc

func Test_terminal_dumpload_dump()
  CheckRunVimInTerminal

  let lines =<< trim END
     call term_dumpload('dumps/Test_popupwin_22.dump', #{term_rows: 12})
  END
  call writefile(lines, 'XtermDumpload')
  let buf = RunVimInTerminal('-S XtermDumpload', #{rows: 15})
  call VerifyScreenDump(buf, 'Test_terminal_dumpload', {})

  call StopVimInTerminal(buf)
  call delete('XtermDumpload')
endfunc

func Test_terminal_dumpdiff()
  call assert_equal(1, winnr('$'))
  eval 'dumps/Test_popup_command_01.dump'->term_dumpdiff('dumps/Test_popup_command_02.dump')
  call assert_equal(2, winnr('$'))
  call assert_equal(62, line('$'))
  call Check_dump01(0)
  call Check_dump01(42)
  call assert_equal('           bbbbbbbbbbbbbbbbbb ', getline(26)[0:29])
  quit

  call assert_fails('call term_dumpdiff("X1.dump", [])', 'E474:')
  call assert_fails('call term_dumpdiff("X1.dump", "X2.dump")', 'E485:')
  call writefile([], 'X1.dump')
  call assert_fails('call term_dumpdiff("X1.dump", "X2.dump")', 'E485:')
  call delete('X1.dump')
endfunc

func Test_terminal_dumpdiff_swap()
  call assert_equal(1, winnr('$'))
  call term_dumpdiff('dumps/Test_popup_command_01.dump', 'dumps/Test_popup_command_03.dump')
  call assert_equal(2, winnr('$'))
  call assert_equal(62, line('$'))
  call assert_match('Test_popup_command_01.dump', getline(21))
  call assert_match('Test_popup_command_03.dump', getline(42))
  call assert_match('Undo', getline(3))
  call assert_match('three four five', getline(45))

  normal s
  call assert_match('Test_popup_command_03.dump', getline(21))
  call assert_match('Test_popup_command_01.dump', getline(42))
  call assert_match('three four five', getline(3))
  call assert_match('Undo', getline(45))
  quit
endfunc

func Test_terminal_dumpdiff_options()
  set laststatus=0
  call assert_equal(1, winnr('$'))
  let height = winheight(0)
  call term_dumpdiff('dumps/Test_popup_command_01.dump', 'dumps/Test_popup_command_02.dump', {'vertical': 1, 'term_cols': 33})
  call assert_equal(2, winnr('$'))
  call assert_equal(height, winheight(winnr()))
  call assert_equal(33, winwidth(winnr()))
  call assert_equal('dump diff dumps/Test_popup_command_01.dump', bufname('%'))
  quit

  call assert_equal(1, winnr('$'))
  call term_dumpdiff('dumps/Test_popup_command_01.dump', 'dumps/Test_popup_command_02.dump', {'vertical': 0, 'term_rows': 13, 'term_name': 'something else'})
  call assert_equal(2, winnr('$'))
  call assert_equal(&columns, winwidth(0))
  call assert_equal(13, winheight(0))
  call assert_equal('something else', bufname('%'))
  quit

  call assert_equal(1, winnr('$'))
  call term_dumpdiff('dumps/Test_popup_command_01.dump', 'dumps/Test_popup_command_02.dump', {'curwin': 1})
  call assert_equal(1, winnr('$'))
  call assert_fails("call term_dumpdiff('dumps/Test_popup_command_01.dump', 'dumps/Test_popup_command_02.dump', {'bufnr': -1})", 'E475:')
  bwipe

  set laststatus&
endfunc

func Api_drop_common(options)
  call assert_equal(1, winnr('$'))

  " Use the title termcap entries to output the escape sequence.
  call writefile([
	\ 'set title',
	\ 'exe "set t_ts=\<Esc>]51; t_fs=\x07"',
	\ 'let &titlestring = ''["drop","Xtextfile"' . a:options . ']''',
	\ 'redraw',
	\ "set t_ts=",
	\ ], 'Xscript')
  let buf = RunVimInTerminal('-S Xscript', {})
  call WaitFor({-> bufnr('Xtextfile') > 0})
  call assert_equal('Xtextfile', expand('%:t'))
  call assert_true(winnr('$') >= 3)
  return buf
endfunc

func Test_terminal_api_drop_newwin()
  CheckRunVimInTerminal
  let buf = Api_drop_common('')
  call assert_equal(0, &bin)
  call assert_equal('', &fenc)

  call StopVimInTerminal(buf)
  call delete('Xscript')
  bwipe Xtextfile
endfunc

func Test_terminal_api_drop_newwin_bin()
  CheckRunVimInTerminal
  let buf = Api_drop_common(',{"bin":1}')
  call assert_equal(1, &bin)

  call StopVimInTerminal(buf)
  call delete('Xscript')
  bwipe Xtextfile
endfunc

func Test_terminal_api_drop_newwin_binary()
  CheckRunVimInTerminal
  let buf = Api_drop_common(',{"binary":1}')
  call assert_equal(1, &bin)

  call StopVimInTerminal(buf)
  call delete('Xscript')
  bwipe Xtextfile
endfunc

func Test_terminal_api_drop_newwin_nobin()
  CheckRunVimInTerminal
  set binary
  let buf = Api_drop_common(',{"nobin":1}')
  call assert_equal(0, &bin)

  call StopVimInTerminal(buf)
  call delete('Xscript')
  bwipe Xtextfile
  set nobinary
endfunc

func Test_terminal_api_drop_newwin_nobinary()
  CheckRunVimInTerminal
  set binary
  let buf = Api_drop_common(',{"nobinary":1}')
  call assert_equal(0, &bin)

  call StopVimInTerminal(buf)
  call delete('Xscript')
  bwipe Xtextfile
  set nobinary
endfunc

func Test_terminal_api_drop_newwin_ff()
  CheckRunVimInTerminal
  let buf = Api_drop_common(',{"ff":"dos"}')
  call assert_equal("dos", &ff)

  call StopVimInTerminal(buf)
  call delete('Xscript')
  bwipe Xtextfile
endfunc

func Test_terminal_api_drop_newwin_fileformat()
  CheckRunVimInTerminal
  let buf = Api_drop_common(',{"fileformat":"dos"}')
  call assert_equal("dos", &ff)

  call StopVimInTerminal(buf)
  call delete('Xscript')
  bwipe Xtextfile
endfunc

func Test_terminal_api_drop_newwin_enc()
  CheckRunVimInTerminal
  let buf = Api_drop_common(',{"enc":"utf-16"}')
  call assert_equal("utf-16", &fenc)

  call StopVimInTerminal(buf)
  call delete('Xscript')
  bwipe Xtextfile
endfunc

func Test_terminal_api_drop_newwin_encoding()
  CheckRunVimInTerminal
  let buf = Api_drop_common(',{"encoding":"utf-16"}')
  call assert_equal("utf-16", &fenc)

  call StopVimInTerminal(buf)
  call delete('Xscript')
  bwipe Xtextfile
endfunc

func Test_terminal_api_drop_oldwin()
  CheckRunVimInTerminal
  let firstwinid = win_getid()
  split Xtextfile
  let textfile_winid = win_getid()
  call assert_equal(2, winnr('$'))
  call win_gotoid(firstwinid)

  " Use the title termcap entries to output the escape sequence.
  call writefile([
	\ 'set title',
	\ 'exe "set t_ts=\<Esc>]51; t_fs=\x07"',
	\ 'let &titlestring = ''["drop","Xtextfile"]''',
	\ 'redraw',
	\ "set t_ts=",
	\ ], 'Xscript')
  let buf = RunVimInTerminal('-S Xscript', {'rows': 10})
  call WaitForAssert({-> assert_equal('Xtextfile', expand('%:t'))})
  call assert_equal(textfile_winid, win_getid())

  call StopVimInTerminal(buf)
  call delete('Xscript')
  bwipe Xtextfile
endfunc

func Tapi_TryThis(bufnum, arg)
  let g:called_bufnum = a:bufnum
  let g:called_arg = a:arg
endfunc

func WriteApiCall(funcname)
  " Use the title termcap entries to output the escape sequence.
  call writefile([
	\ 'set title',
	\ 'exe "set t_ts=\<Esc>]51; t_fs=\x07"',
	\ 'let &titlestring = ''["call","' . a:funcname . '",["hello",123]]''',
	\ 'redraw',
	\ "set t_ts=",
	\ ], 'Xscript')
endfunc

func Test_terminal_api_call()
  CheckRunVimInTerminal

  unlet! g:called_bufnum
  unlet! g:called_arg

  call WriteApiCall('Tapi_TryThis')

  " Default
  let buf = RunVimInTerminal('-S Xscript', {})
  call WaitFor({-> exists('g:called_bufnum')})
  call assert_equal(buf, g:called_bufnum)
  call assert_equal(['hello', 123], g:called_arg)
  call StopVimInTerminal(buf)

  unlet! g:called_bufnum
  unlet! g:called_arg

  " Enable explicitly
  let buf = RunVimInTerminal('-S Xscript', {'term_api': 'Tapi_Try'})
  call WaitFor({-> exists('g:called_bufnum')})
  call assert_equal(buf, g:called_bufnum)
  call assert_equal(['hello', 123], g:called_arg)
  call StopVimInTerminal(buf)

  unlet! g:called_bufnum
  unlet! g:called_arg

  func! ApiCall_TryThis(bufnum, arg)
    let g:called_bufnum2 = a:bufnum
    let g:called_arg2 = a:arg
  endfunc

  call WriteApiCall('ApiCall_TryThis')

  " Use prefix match
  let buf = RunVimInTerminal('-S Xscript', {'term_api': 'ApiCall_'})
  call WaitFor({-> exists('g:called_bufnum2')})
  call assert_equal(buf, g:called_bufnum2)
  call assert_equal(['hello', 123], g:called_arg2)
  call StopVimInTerminal(buf)

  call assert_fails("call term_start('ls', {'term_api' : []})", 'E475:')

  unlet! g:called_bufnum2
  unlet! g:called_arg2

  call delete('Xscript')
  delfunction! ApiCall_TryThis
  unlet! g:called_bufnum2
  unlet! g:called_arg2
endfunc

func Test_terminal_api_call_fails()
  CheckRunVimInTerminal

  func! TryThis(bufnum, arg)
    let g:called_bufnum3 = a:bufnum
    let g:called_arg3 = a:arg
  endfunc

  call WriteApiCall('TryThis')

  unlet! g:called_bufnum3
  unlet! g:called_arg3

  " Not permitted
  call ch_logfile('Xlog', 'w')
  let buf = RunVimInTerminal('-S Xscript', {'term_api': ''})
  call WaitForAssert({-> assert_match('Unpermitted function: TryThis', string(readfile('Xlog')))})
  call assert_false(exists('g:called_bufnum3'))
  call assert_false(exists('g:called_arg3'))
  call StopVimInTerminal(buf)

  " No match
  call ch_logfile('Xlog', 'w')
  let buf = RunVimInTerminal('-S Xscript', {'term_api': 'TryThat'})
  call WaitFor({-> string(readfile('Xlog')) =~ 'Unpermitted function: TryThis'})
  call assert_false(exists('g:called_bufnum3'))
  call assert_false(exists('g:called_arg3'))
  call StopVimInTerminal(buf)

  call delete('Xscript')
  call ch_logfile('')
  call delete('Xlog')
  delfunction! TryThis
  unlet! g:called_bufnum3
  unlet! g:called_arg3
endfunc

let s:caught_e937 = 0

func Tapi_Delete(bufnum, arg)
  try
    execute 'bdelete!' a:bufnum
  catch /E937:/
    let s:caught_e937 = 1
  endtry
endfunc

func Test_terminal_api_call_fail_delete()
  CheckRunVimInTerminal

  call WriteApiCall('Tapi_Delete')
  let buf = RunVimInTerminal('-S Xscript', {})
  call WaitForAssert({-> assert_equal(1, s:caught_e937)})

  call StopVimInTerminal(buf)
  call delete('Xscript')
  call ch_logfile('', '')
endfunc

func Test_terminal_ansicolors_default()
  CheckFunction term_getansicolors

  let colors = [
	\ '#000000', '#e00000',
	\ '#00e000', '#e0e000',
	\ '#0000e0', '#e000e0',
	\ '#00e0e0', '#e0e0e0',
	\ '#808080', '#ff4040',
	\ '#40ff40', '#ffff40',
	\ '#4040ff', '#ff40ff',
	\ '#40ffff', '#ffffff',
	\]

  let buf = Run_shell_in_terminal({})
  call assert_equal(colors, term_getansicolors(buf))
  call StopShellInTerminal(buf)
  call TermWait(buf)

  exe buf . 'bwipe'
endfunc

let s:test_colors = [
	\ '#616e64', '#0d0a79',
	\ '#6d610d', '#0a7373',
	\ '#690d0a', '#6d696e',
	\ '#0d0a6f', '#616e0d',
	\ '#0a6479', '#6d0d0a',
	\ '#617373', '#0d0a69',
	\ '#6d690d', '#0a6e6f',
	\ '#610d0a', '#6e6479',
	\]

func Test_terminal_ansicolors_global()
  CheckFeature termguicolors
  CheckFunction term_getansicolors

  let g:terminal_ansi_colors = reverse(copy(s:test_colors))
  let buf = Run_shell_in_terminal({})
  call assert_equal(g:terminal_ansi_colors, term_getansicolors(buf))
  call StopShellInTerminal(buf)
  call TermWait(buf)

  exe buf . 'bwipe'
  unlet g:terminal_ansi_colors
endfunc

func Test_terminal_ansicolors_func()
  CheckFeature termguicolors
  CheckFunction term_getansicolors

  let g:terminal_ansi_colors = reverse(copy(s:test_colors))
  let buf = Run_shell_in_terminal({'ansi_colors': s:test_colors})
  call assert_equal(s:test_colors, term_getansicolors(buf))

  call term_setansicolors(buf, g:terminal_ansi_colors)
  call assert_equal(g:terminal_ansi_colors, buf->term_getansicolors())

  let colors = [
	\ 'ivory', 'AliceBlue',
	\ 'grey67', 'dark goldenrod',
	\ 'SteelBlue3', 'PaleVioletRed4',
	\ 'MediumPurple2', 'yellow2',
	\ 'RosyBrown3', 'OrangeRed2',
	\ 'white smoke', 'navy blue',
	\ 'grey47', 'gray97',
	\ 'MistyRose2', 'DodgerBlue4',
	\]
  eval buf->term_setansicolors(colors)

  let colors[4] = 'Invalid'
  call assert_fails('call term_setansicolors(buf, colors)', 'E474:')

  call StopShellInTerminal(buf)
  call TermWait(buf)
  exe buf . 'bwipe'
endfunc

func Test_terminal_all_ansi_colors()
  CheckRunVimInTerminal

  " Use all the ANSI colors.
  call writefile([
	\ 'call setline(1, "AABBCCDDEEFFGGHHIIJJKKLLMMNNOOPP XXYYZZ")',
	\ 'hi Tblack ctermfg=0 ctermbg=8',
	\ 'hi Tdarkred ctermfg=1 ctermbg=9',
	\ 'hi Tdarkgreen ctermfg=2 ctermbg=10',
	\ 'hi Tbrown ctermfg=3 ctermbg=11',
	\ 'hi Tdarkblue ctermfg=4 ctermbg=12',
	\ 'hi Tdarkmagenta ctermfg=5 ctermbg=13',
	\ 'hi Tdarkcyan ctermfg=6 ctermbg=14',
	\ 'hi Tlightgrey ctermfg=7 ctermbg=15',
	\ 'hi Tdarkgrey ctermfg=8 ctermbg=0',
	\ 'hi Tred ctermfg=9 ctermbg=1',
	\ 'hi Tgreen ctermfg=10 ctermbg=2',
	\ 'hi Tyellow ctermfg=11 ctermbg=3',
	\ 'hi Tblue ctermfg=12 ctermbg=4',
	\ 'hi Tmagenta ctermfg=13 ctermbg=5',
	\ 'hi Tcyan ctermfg=14 ctermbg=6',
	\ 'hi Twhite ctermfg=15 ctermbg=7',
	\ 'hi TdarkredBold ctermfg=1 cterm=bold',
	\ 'hi TgreenBold ctermfg=10 cterm=bold',
	\ 'hi TmagentaBold ctermfg=13 cterm=bold ctermbg=5',
	\ '',
	\ 'call  matchadd("Tblack", "A")',
	\ 'call  matchadd("Tdarkred", "B")',
	\ 'call  matchadd("Tdarkgreen", "C")',
	\ 'call  matchadd("Tbrown", "D")',
	\ 'call  matchadd("Tdarkblue", "E")',
	\ 'call  matchadd("Tdarkmagenta", "F")',
	\ 'call  matchadd("Tdarkcyan", "G")',
	\ 'call  matchadd("Tlightgrey", "H")',
	\ 'call  matchadd("Tdarkgrey", "I")',
	\ 'call  matchadd("Tred", "J")',
	\ 'call  matchadd("Tgreen", "K")',
	\ 'call  matchadd("Tyellow", "L")',
	\ 'call  matchadd("Tblue", "M")',
	\ 'call  matchadd("Tmagenta", "N")',
	\ 'call  matchadd("Tcyan", "O")',
	\ 'call  matchadd("Twhite", "P")',
	\ 'call  matchadd("TdarkredBold", "X")',
	\ 'call  matchadd("TgreenBold", "Y")',
	\ 'call  matchadd("TmagentaBold", "Z")',
	\ 'redraw',
	\ ], 'Xcolorscript')
  let buf = RunVimInTerminal('-S Xcolorscript', {'rows': 10})
  call VerifyScreenDump(buf, 'Test_terminal_all_ansi_colors', {})

  call term_sendkeys(buf, ":q\<CR>")
  call StopVimInTerminal(buf)
  call delete('Xcolorscript')
endfunc

func Test_terminal_termwinsize_option_fixed()
  CheckRunVimInTerminal
  set termwinsize=6x40
  let text = []
  for n in range(10)
    call add(text, repeat(n, 50))
  endfor
  call writefile(text, 'Xwinsize')
  let buf = RunVimInTerminal('Xwinsize', {})
  let win = bufwinid(buf)
  call assert_equal([6, 40], term_getsize(buf))
  call assert_equal(6, winheight(win))
  call assert_equal(40, winwidth(win))

  " resizing the window doesn't resize the terminal.
  resize 10
  vertical resize 60
  call assert_equal([6, 40], term_getsize(buf))
  call assert_equal(10, winheight(win))
  call assert_equal(60, winwidth(win))

  call StopVimInTerminal(buf)
  call delete('Xwinsize')

  call assert_fails('set termwinsize=40', 'E474')
  call assert_fails('set termwinsize=10+40', 'E474')
  call assert_fails('set termwinsize=abc', 'E474')

  set termwinsize=
endfunc

func Test_terminal_termwinsize_option_zero()
  set termwinsize=0x0
  let buf = Run_shell_in_terminal({})
  let win = bufwinid(buf)
  call assert_equal([winheight(win), winwidth(win)], term_getsize(buf))
  call StopShellInTerminal(buf)
  call TermWait(buf)
  exe buf . 'bwipe'

  set termwinsize=7x0
  let buf = Run_shell_in_terminal({})
  let win = bufwinid(buf)
  call assert_equal([7, winwidth(win)], term_getsize(buf))
  call StopShellInTerminal(buf)
  call TermWait(buf)
  exe buf . 'bwipe'

  set termwinsize=0x33
  let buf = Run_shell_in_terminal({})
  let win = bufwinid(buf)
  call assert_equal([winheight(win), 33], term_getsize(buf))
  call StopShellInTerminal(buf)
  call TermWait(buf)
  exe buf . 'bwipe'

  set termwinsize=
endfunc

func Test_terminal_termwinsize_minimum()
  set termwinsize=10*50
  vsplit
  let buf = Run_shell_in_terminal({})
  let win = bufwinid(buf)
  call assert_inrange(10, 1000, winheight(win))
  call assert_inrange(50, 1000, winwidth(win))
  call assert_equal([winheight(win), winwidth(win)], term_getsize(buf))

  resize 15
  vertical resize 60
  redraw
  call assert_equal([15, 60], term_getsize(buf))
  call assert_equal(15, winheight(win))
  call assert_equal(60, winwidth(win))

  resize 7
  vertical resize 30
  redraw
  call assert_equal([10, 50], term_getsize(buf))
  call assert_equal(7, winheight(win))
  call assert_equal(30, winwidth(win))

  call StopShellInTerminal(buf)
  call TermWait(buf)
  exe buf . 'bwipe'

  set termwinsize=0*0
  let buf = Run_shell_in_terminal({})
  let win = bufwinid(buf)
  call assert_equal([winheight(win), winwidth(win)], term_getsize(buf))
  call StopShellInTerminal(buf)
  call TermWait(buf)
  exe buf . 'bwipe'

  set termwinsize=
endfunc

func Test_terminal_termwinkey()
  " make three tabpages, terminal in the middle
  0tabnew
  tabnext
  tabnew
  tabprev
  call assert_equal(1, winnr('$'))
  call assert_equal(2, tabpagenr())
  let thiswin = win_getid()

  let buf = Run_shell_in_terminal({})
  let termwin = bufwinid(buf)
  set termwinkey=<C-L>
  call feedkeys("\<C-L>w", 'tx')
  call assert_equal(thiswin, win_getid())
  call feedkeys("\<C-W>w", 'tx')
  call assert_equal(termwin, win_getid())

  if has('langmap')
    set langmap=xjyk
    call feedkeys("\<C-L>x", 'tx')
    call assert_equal(thiswin, win_getid())
    call feedkeys("\<C-W>y", 'tx')
    call assert_equal(termwin, win_getid())
    set langmap=
  endif

  call feedkeys("\<C-L>gt", "xt")
  call assert_equal(3, tabpagenr())
  tabprev
  call assert_equal(2, tabpagenr())
  call assert_equal(termwin, win_getid())

  call feedkeys("\<C-L>gT", "xt")
  call assert_equal(1, tabpagenr())
  tabnext
  call assert_equal(2, tabpagenr())
  call assert_equal(termwin, win_getid())

  let job = term_getjob(buf)
  call feedkeys("\<C-L>\<C-C>", 'tx')
  call WaitForAssert({-> assert_equal("dead", job_status(job))})

  set termwinkey&
  tabnext
  tabclose
  tabprev
  tabclose
endfunc

func Test_terminal_out_err()
  CheckUnix

  call writefile([
	\ '#!/bin/sh',
	\ 'echo "this is standard error" >&2',
	\ 'echo "this is standard out" >&1',
	\ ], 'Xechoerrout.sh')
  call setfperm('Xechoerrout.sh', 'rwxrwx---')

  let outfile = 'Xtermstdout'
  let buf = term_start(['./Xechoerrout.sh'], {'out_io': 'file', 'out_name': outfile})

  call WaitFor({-> !empty(readfile(outfile)) && !empty(term_getline(buf, 1))})
  call assert_equal(['this is standard out'], readfile(outfile))
  call assert_equal('this is standard error', term_getline(buf, 1))

  call WaitForAssert({-> assert_equal('dead', job_status(term_getjob(buf)))})
  exe buf . 'bwipe'
  call delete('Xechoerrout.sh')
  call delete(outfile)
endfunc

func Test_termwinscroll()
  CheckUnix

  " Let the terminal output more than 'termwinscroll' lines, some at the start
  " will be dropped.
  exe 'set termwinscroll=' . &lines
  let buf = term_start('/bin/sh')
  for i in range(1, &lines)
    call feedkeys("echo " . i . "\<CR>", 'xt')
    call WaitForAssert({-> assert_match(string(i), term_getline(buf, term_getcursor(buf)[0] - 1))})
  endfor
  " Go to Terminal-Normal mode to update the buffer.
  call feedkeys("\<C-W>N", 'xt')
  call assert_inrange(&lines, &lines * 110 / 100 + winheight(0), line('$'))

  " Every "echo nr" must only appear once
  let lines = getline(1, line('$'))
  for i in range(&lines - len(lines) / 2 + 2, &lines)
    let filtered = filter(copy(lines), {idx, val -> val =~ 'echo ' . i . '\>'})
    call assert_equal(1, len(filtered), 'for "echo ' . i . '"')
  endfor

  exe buf . 'bwipe!'
endfunc

" Resizing the terminal window caused an ml_get error.
" TODO: This does not reproduce the original problem.
func Test_terminal_resize()
  set statusline=x
  terminal
  call assert_equal(2, winnr('$'))

  " Fill the terminal with text.
  if has('win32')
    call feedkeys("dir\<CR>", 'xt')
  else
    call feedkeys("ls\<CR>", 'xt')
  endif
  " Go to Terminal-Normal mode for a moment.
  call feedkeys("\<C-W>N", 'xt')
  " Open a new window
  call feedkeys("i\<C-W>n", 'xt')
  call assert_equal(3, winnr('$'))
  redraw

  close
  call assert_equal(2, winnr('$'))
  call feedkeys("exit\<CR>", 'xt')
  set statusline&
endfunc

" must be nearly the last, we can't go back from GUI to terminal
func Test_zz1_terminal_in_gui()
  CheckCanRunGui

  " Ignore the "failed to create input context" error.
  call test_ignore_error('E285:')

  gui -f

  call assert_equal(1, winnr('$'))
  let buf = Run_shell_in_terminal({'term_finish': 'close'})
  call StopShellInTerminal(buf)
  call TermWait(buf)

  " closing window wipes out the terminal buffer a with finished job
  call WaitForAssert({-> assert_equal(1, winnr('$'))})
  call assert_equal("", bufname(buf))

  unlet g:job
endfunc

func Test_zz2_terminal_guioptions_bang()
  CheckGui
  set guioptions+=!

  let filename = 'Xtestscript'
  if has('win32')
    let filename .= '.bat'
    let prefix = ''
    let contents = ['@echo off', 'exit %1']
  else
    let filename .= '.sh'
    let prefix = './'
    let contents = ['#!/bin/sh', 'exit $1']
  endif
  call writefile(contents, filename)
  call setfperm(filename, 'rwxrwx---')

  " Check if v:shell_error is equal to the exit status.
  let exitval = 0
  execute printf(':!%s%s %d', prefix, filename, exitval)
  call assert_equal(exitval, v:shell_error)

  let exitval = 9
  execute printf(':!%s%s %d', prefix, filename, exitval)
  call assert_equal(exitval, v:shell_error)

  set guioptions&
  call delete(filename)
endfunc

func Test_terminal_hidden()
  CheckUnix

  term ++hidden cat
  let bnr = bufnr('$')
  call assert_equal('terminal', getbufvar(bnr, '&buftype'))
  exe 'sbuf ' . bnr
  call assert_equal('terminal', &buftype)
  call term_sendkeys(bnr, "asdf\<CR>")
  call WaitForAssert({-> assert_match('asdf', term_getline(bnr, 2))})
  call term_sendkeys(bnr, "\<C-D>")
  call WaitForAssert({-> assert_equal('finished', bnr->term_getstatus())})
  bwipe!
endfunc

func Test_terminal_switch_mode()
  term
  let bnr = bufnr('$')
  call WaitForAssert({-> assert_equal('running', term_getstatus(bnr))})
  call feedkeys("\<C-W>N", 'xt')
  call WaitForAssert({-> assert_equal('running,normal', term_getstatus(bnr))})
  call feedkeys("A", 'xt')
  call WaitForAssert({-> assert_equal('running', term_getstatus(bnr))})
  call feedkeys("\<C-W>N", 'xt')
  call WaitForAssert({-> assert_equal('running,normal', term_getstatus(bnr))})
  call feedkeys("I", 'xt')
  call WaitForAssert({-> assert_equal('running', term_getstatus(bnr))})
  call feedkeys("\<C-W>Nv", 'xt')
  call WaitForAssert({-> assert_equal('running,normal', term_getstatus(bnr))})
  call feedkeys("I", 'xt')
  call WaitForAssert({-> assert_equal('running', term_getstatus(bnr))})
  call feedkeys("\<C-W>Nv", 'xt')
  call WaitForAssert({-> assert_equal('running,normal', term_getstatus(bnr))})
  call feedkeys("A", 'xt')
  call WaitForAssert({-> assert_equal('running', term_getstatus(bnr))})
  bwipe!
endfunc

func Test_terminal_normal_mode()
  CheckRunVimInTerminal

  " Run Vim in a terminal and open a terminal window to run Vim in.
  let lines =<< trim END
    call setline(1, range(11111, 11122))
    3
  END
  call writefile(lines, 'XtermNormal')
  let buf = RunVimInTerminal('-S XtermNormal', {'rows': 8})
  call TermWait(buf)

  call term_sendkeys(buf, "\<C-W>N")
  call term_sendkeys(buf, ":set number cursorline culopt=both\r")
  call VerifyScreenDump(buf, 'Test_terminal_normal_1', {})

  call term_sendkeys(buf, ":set culopt=number\r")
  call VerifyScreenDump(buf, 'Test_terminal_normal_2', {})

  call term_sendkeys(buf, ":set culopt=line\r")
  call VerifyScreenDump(buf, 'Test_terminal_normal_3', {})

  call term_sendkeys(buf, "a:q!\<CR>:q\<CR>:q\<CR>")
  call StopVimInTerminal(buf)
  call delete('XtermNormal')
endfunc

func Test_terminal_hidden_and_close()
  CheckUnix

  call assert_equal(1, winnr('$'))
  term ++hidden ++close ls
  let bnr = bufnr('$')
  call assert_equal('terminal', getbufvar(bnr, '&buftype'))
  call WaitForAssert({-> assert_false(bufexists(bnr))})
  call assert_equal(1, winnr('$'))
endfunc

func Test_terminal_does_not_truncate_last_newlines()
  " This test does not pass through ConPTY.
  if has('conpty')
    return
  endif
  let contents = [
  \   [ 'One', '', 'X' ],
  \   [ 'Two', '', '' ],
  \   [ 'Three' ] + repeat([''], 30)
  \ ]

  for c in contents
    call writefile(c, 'Xfile')
    if has('win32')
      term cmd /c type Xfile
    else
      term cat Xfile
    endif
    let bnr = bufnr('$')
    call assert_equal('terminal', getbufvar(bnr, '&buftype'))
    call WaitForAssert({-> assert_equal('finished', term_getstatus(bnr))})
    sleep 100m
    call assert_equal(c, getline(1, line('$')))
    quit
  endfor

  call delete('Xfile')
endfunc

func Test_terminal_no_job()
  if has('win32')
    let cmd = 'cmd /c ""'
  else
    CheckExecutable false
    let cmd = 'false'
  endif
  let term = term_start(cmd, {'term_finish': 'close'})
  call WaitForAssert({-> assert_equal(v:null, term_getjob(term)) })
endfunc

func Test_term_getcursor()
  CheckUnix

  let buf = Run_shell_in_terminal({})

  " Wait for the shell to display a prompt.
  call WaitForAssert({-> assert_notequal('', term_getline(buf, 1))})

  " Hide the cursor.
  call term_sendkeys(buf, "echo -e '\\033[?25l'\r")
  call WaitForAssert({-> assert_equal(0, term_getcursor(buf)[2].visible)})

  " Show the cursor.
  call term_sendkeys(buf, "echo -e '\\033[?25h'\r")
  call WaitForAssert({-> assert_equal(1, buf->term_getcursor()[2].visible)})

  " Change color of cursor.
  call WaitForAssert({-> assert_equal('', term_getcursor(buf)[2].color)})
  call term_sendkeys(buf, "echo -e '\\033]12;blue\\007'\r")
  call WaitForAssert({-> assert_equal('blue', term_getcursor(buf)[2].color)})
  call term_sendkeys(buf, "echo -e '\\033]12;green\\007'\r")
  call WaitForAssert({-> assert_equal('green', term_getcursor(buf)[2].color)})

  " Make cursor a blinking block.
  call term_sendkeys(buf, "echo -e '\\033[1 q'\r")
  call WaitForAssert({-> assert_equal([1, 1],
  \ [term_getcursor(buf)[2].blink, term_getcursor(buf)[2].shape])})

  " Make cursor a steady block.
  call term_sendkeys(buf, "echo -e '\\033[2 q'\r")
  call WaitForAssert({-> assert_equal([0, 1],
  \ [term_getcursor(buf)[2].blink, term_getcursor(buf)[2].shape])})

  " Make cursor a blinking underline.
  call term_sendkeys(buf, "echo -e '\\033[3 q'\r")
  call WaitForAssert({-> assert_equal([1, 2],
  \ [term_getcursor(buf)[2].blink, term_getcursor(buf)[2].shape])})

  " Make cursor a steady underline.
  call term_sendkeys(buf, "echo -e '\\033[4 q'\r")
  call WaitForAssert({-> assert_equal([0, 2],
  \ [term_getcursor(buf)[2].blink, term_getcursor(buf)[2].shape])})

  " Make cursor a blinking vertical bar.
  call term_sendkeys(buf, "echo -e '\\033[5 q'\r")
  call WaitForAssert({-> assert_equal([1, 3],
  \ [term_getcursor(buf)[2].blink, term_getcursor(buf)[2].shape])})

  " Make cursor a steady vertical bar.
  call term_sendkeys(buf, "echo -e '\\033[6 q'\r")
  call WaitForAssert({-> assert_equal([0, 3],
  \ [term_getcursor(buf)[2].blink, term_getcursor(buf)[2].shape])})

  call StopShellInTerminal(buf)
endfunc

func Test_term_gettitle()
  " term_gettitle() returns an empty string for a non-terminal buffer
  " and for a non-existing buffer.
  call assert_equal('', bufnr('%')->term_gettitle())
  call assert_equal('', term_gettitle(bufnr('$') + 1))

  if !has('title') || &title == 0 || empty(&t_ts)
    throw "Skipped: can't get/set title"
  endif

  let term = term_start([GetVimProg(), '--clean', '-c', 'set noswapfile'])
  if has('autoservername')
    call WaitForAssert({-> assert_match('^\[No Name\] - VIM\d\+$', term_gettitle(term)) })
    call term_sendkeys(term, ":e Xfoo\r")
    call WaitForAssert({-> assert_match('^Xfoo (.*[/\\]testdir) - VIM\d\+$', term_gettitle(term)) })
  else
    call WaitForAssert({-> assert_equal('[No Name] - VIM', term_gettitle(term)) })
    call term_sendkeys(term, ":e Xfoo\r")
    call WaitForAssert({-> assert_match('^Xfoo (.*[/\\]testdir) - VIM$', term_gettitle(term)) })
  endif

  call term_sendkeys(term, ":set titlestring=foo\r")
  call WaitForAssert({-> assert_equal('foo', term_gettitle(term)) })

  exe term . 'bwipe!'
endfunc

func Test_term_gettty()
  let buf = Run_shell_in_terminal({})
  let gettty = term_gettty(buf)

  if has('unix') && executable('tty')
    " Find tty using the tty shell command.
    call WaitForAssert({-> assert_notequal('', term_getline(buf, 1))})
    call term_sendkeys(buf, "tty\r")
    call WaitForAssert({-> assert_notequal('', term_getline(buf, 3))})
    let tty = term_getline(buf, 2)
    call assert_equal(tty, gettty)
  endif

  let gettty0 = term_gettty(buf, 0)
  let gettty1 = term_gettty(buf, 1)

  call assert_equal(gettty, gettty0)
  call assert_equal(job_info(g:job).tty_out, gettty0)
  call assert_equal(job_info(g:job).tty_in,  gettty1)

  if has('unix')
    " For unix, term_gettty(..., 0) and term_gettty(..., 1)
    " are identical according to :help term_gettty()
    call assert_equal(gettty0, gettty1)
    call assert_match('^/dev/', gettty)
  else
    " ConPTY works on anonymous pipe.
    if !has('conpty')
      call assert_match('^\\\\.\\pipe\\', gettty0)
      call assert_match('^\\\\.\\pipe\\', gettty1)
    endif
  endif

  call assert_fails('call term_gettty(buf, 2)', 'E475:')
  call assert_fails('call term_gettty(buf, -1)', 'E475:')

  call assert_equal('', term_gettty(buf + 1))

  call StopShellInTerminal(buf)
  call TermWait(buf)
  exe buf . 'bwipe'
endfunc

" When drawing the statusline the cursor position may not have been updated
" yet.
" 1. create a terminal, make it show 2 lines
" 2. 0.5 sec later: leave terminal window, execute "i"
" 3. 0.5 sec later: clear terminal window, now it's 1 line
" 4. 0.5 sec later: redraw, including statusline (used to trigger bug)
" 4. 0.5 sec later: should be done, clean up
func Test_terminal_statusline()
  CheckUnix

  set statusline=x
  terminal
  let tbuf = bufnr('')
  call term_sendkeys(tbuf, "clear; echo a; echo b; sleep 1; clear\n")
  call timer_start(500, { tid -> feedkeys("\<C-w>j", 'tx') })
  call timer_start(1500, { tid -> feedkeys("\<C-l>", 'tx') })
  au BufLeave * if &buftype == 'terminal' | silent! normal i | endif

  sleep 2
  exe tbuf . 'bwipe!'
  au! BufLeave
  set statusline=
endfunc

func Test_terminal_getwinpos()
  CheckRunVimInTerminal

  " split, go to the bottom-right window
  split
  wincmd j
  set splitright

  call writefile([
	\ 'echo getwinpos()',
	\ ], 'XTest_getwinpos')
  let buf = RunVimInTerminal('-S XTest_getwinpos', {'cols': 60})
  call TermWait(buf)

  " Find the output of getwinpos() in the bottom line.
  let rows = term_getsize(buf)[0]
  call WaitForAssert({-> assert_match('\[\d\+, \d\+\]', term_getline(buf, rows))})
  let line = term_getline(buf, rows)
  let xpos = str2nr(substitute(line, '\[\(\d\+\), \d\+\]', '\1', ''))
  let ypos = str2nr(substitute(line, '\[\d\+, \(\d\+\)\]', '\1', ''))

  " Position must be bigger than the getwinpos() result of Vim itself.
  " The calculation in the console assumes a 10 x 7 character cell.
  " In the GUI it can be more, let's assume a 20 x 14 cell.
  " And then add 100 / 200 tolerance.
  let [xroot, yroot] = getwinpos()
  let winpos = 50->getwinpos()
  call assert_equal(xroot, winpos[0])
  call assert_equal(yroot, winpos[1])
  let [winrow, wincol] = win_screenpos('.')
  let xoff = wincol * (has('gui_running') ? 14 : 7) + 100
  let yoff = winrow * (has('gui_running') ? 20 : 10) + 200
  call assert_inrange(xroot + 2, xroot + xoff, xpos)
  call assert_inrange(yroot + 2, yroot + yoff, ypos)

  call TermWait(buf)
  call term_sendkeys(buf, ":q\<CR>")
  call StopVimInTerminal(buf)
  call delete('XTest_getwinpos')
  exe buf . 'bwipe!'
  set splitright&
  only!
endfunc

func Test_terminal_altscreen()
  " somehow doesn't work on MS-Windows
  CheckUnix
  let cmd = "cat Xtext\<CR>"

  let buf = term_start(&shell, {})
  call writefile(["\<Esc>[?1047h"], 'Xtext')
  call term_sendkeys(buf, cmd)
  call WaitForAssert({-> assert_equal(1, term_getaltscreen(buf))})

  call writefile(["\<Esc>[?1047l"], 'Xtext')
  call term_sendkeys(buf, cmd)
  call WaitForAssert({-> assert_equal(0, term_getaltscreen(buf))})

  call term_sendkeys(buf, "exit\r")
  exe buf . "bwipe!"
  call delete('Xtext')
endfunc

func Test_terminal_shell_option()
  if has('unix')
    " exec is a shell builtin command, should fail without a shell.
    term exec ls runtest.vim
    call WaitForAssert({-> assert_match('job failed', term_getline(bufnr(), 1))})
    bwipe!

    term ++shell exec ls runtest.vim
    call WaitForAssert({-> assert_match('runtest.vim', term_getline(bufnr(), 1))})
    bwipe!
  elseif has('win32')
    " dir is a shell builtin command, should fail without a shell.
    try
      term dir /b runtest.vim
      call WaitForAssert({-> assert_match('job failed\|cannot access .*: No such file or directory', term_getline(bufnr(), 1))})
    catch /CreateProcess/
      " ignore
    endtry
    bwipe!

    term ++shell dir /b runtest.vim
    call WaitForAssert({-> assert_match('runtest.vim', term_getline(bufnr(), 1))})
    bwipe!
  endif
endfunc

func Test_terminal_setapi_and_call()
  CheckRunVimInTerminal

  call WriteApiCall('Tapi_TryThis')
  call ch_logfile('Xlog', 'w')

  unlet! g:called_bufnum
  unlet! g:called_arg

  let buf = RunVimInTerminal('-S Xscript', {'term_api': ''})
  call WaitForAssert({-> assert_match('Unpermitted function: Tapi_TryThis', string(readfile('Xlog')))})
  call assert_false(exists('g:called_bufnum'))
  call assert_false(exists('g:called_arg'))

  eval buf->term_setapi('Tapi_')
  call term_sendkeys(buf, ":set notitle\<CR>")
  call term_sendkeys(buf, ":source Xscript\<CR>")
  call WaitFor({-> exists('g:called_bufnum')})
  call assert_equal(buf, g:called_bufnum)
  call assert_equal(['hello', 123], g:called_arg)

  call StopVimInTerminal(buf)

  call delete('Xscript')
  call ch_logfile('')
  call delete('Xlog')
  unlet! g:called_bufnum
  unlet! g:called_arg
endfunc

func Test_terminal_api_arg()
  CheckRunVimInTerminal

  call WriteApiCall('Tapi_TryThis')
  call ch_logfile('Xlog', 'w')

  unlet! g:called_bufnum
  unlet! g:called_arg

  execute 'term ++api= ' .. GetVimCommandCleanTerm() .. '-S Xscript'
  let buf = bufnr('%')
  call WaitForAssert({-> assert_match('Unpermitted function: Tapi_TryThis', string(readfile('Xlog')))})
  call assert_false(exists('g:called_bufnum'))
  call assert_false(exists('g:called_arg'))

  call StopVimInTerminal(buf)

  call ch_logfile('Xlog', 'w')

  execute 'term ++api=Tapi_ ' .. GetVimCommandCleanTerm() .. '-S Xscript'
  let buf = bufnr('%')
  call WaitFor({-> exists('g:called_bufnum')})
  call assert_equal(buf, g:called_bufnum)
  call assert_equal(['hello', 123], g:called_arg)

  call StopVimInTerminal(buf)

  call delete('Xscript')
  call ch_logfile('')
  call delete('Xlog')
  unlet! g:called_bufnum
  unlet! g:called_arg
endfunc

func Test_terminal_invalid_arg()
  call assert_fails('terminal ++xyz', 'E181:')
endfunc

func Test_terminal_in_popup()
  CheckRunVimInTerminal

  let text =<< trim END
    some text
    to edit
    in a popup window
  END
  call writefile(text, 'Xtext')
  let cmd = GetVimCommandCleanTerm()
  let lines = [
	\ 'set t_u7=',
	\ 'call setline(1, range(20))',
	\ 'hi PopTerm ctermbg=grey',
	\ 'func OpenTerm(setColor)',
	\ "  set noruler",
	\ "  let s:buf = term_start('" .. cmd .. " Xtext', #{hidden: 1, term_finish: 'close'})",
	\ '  let g:winid = popup_create(s:buf, #{minwidth: 45, minheight: 7, border: [], drag: 1, resize: 1})',
	\ '  if a:setColor',
	\ '    call win_execute(g:winid, "set wincolor=PopTerm")',
	\ '  endif',
	\ 'endfunc',
	\ 'func HidePopup()',
	\ '  call popup_hide(g:winid)',
	\ 'endfunc',
	\ 'func ClosePopup()',
	\ '  call popup_close(g:winid)',
	\ 'endfunc',
	\ 'func ReopenPopup()',
	\ '  call popup_create(s:buf, #{minwidth: 40, minheight: 6, border: []})',
	\ 'endfunc',
	\ ]
  call writefile(lines, 'XtermPopup')
  let buf = RunVimInTerminal('-S XtermPopup', #{rows: 15})
  call TermWait(buf, 100)
  call term_sendkeys(buf, ":call OpenTerm(0)\<CR>")
  call TermWait(buf, 100)
  call term_sendkeys(buf, ":\<CR>")
  call TermWait(buf, 100)
  call term_sendkeys(buf, "\<C-W>:echo getwinvar(g:winid, \"&buftype\") win_gettype(g:winid)\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_1', {})

  call term_sendkeys(buf, ":q\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_2', {})
 
  call term_sendkeys(buf, ":call OpenTerm(1)\<CR>")
  call TermWait(buf, 150)
  call term_sendkeys(buf, ":set hlsearch\<CR>")
  call term_sendkeys(buf, "/edit\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_3', {})
 
  call term_sendkeys(buf, "\<C-W>:call HidePopup()\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_4', {})
  call term_sendkeys(buf, "\<CR>")
  call TermWait(buf, 50)

  call term_sendkeys(buf, "\<C-W>:call ClosePopup()\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_5', {})

  call term_sendkeys(buf, "\<C-W>:call ReopenPopup()\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_6', {})

  " Go to terminal-Normal mode and visually select text.
  call term_sendkeys(buf, "\<C-W>Ngg/in\<CR>vww")
  call VerifyScreenDump(buf, 'Test_terminal_popup_7', {})

  " Back to job mode, redraws
  call term_sendkeys(buf, "A")
  call VerifyScreenDump(buf, 'Test_terminal_popup_8', {})

  call TermWait(buf, 50)
  call term_sendkeys(buf, ":q\<CR>")
  call TermWait(buf, 100)  " wait for terminal to vanish

  call StopVimInTerminal(buf)
  call delete('Xtext')
  call delete('XtermPopup')
endfunc

" Check a terminal in popup window uses the default mininum size.
func Test_terminal_in_popup_min_size()
  CheckRunVimInTerminal

  let text =<< trim END
    another text
    to show
    in a popup window
  END
  call writefile(text, 'Xtext')
  let lines = [
	\ 'set t_u7=',
	\ 'call setline(1, range(20))',
	\ 'func OpenTerm()',
	\ "  let s:buf = term_start('cat Xtext', #{hidden: 1})",
	\ '  let g:winid = popup_create(s:buf, #{ border: []})',
	\ 'endfunc',
	\ ]
  call writefile(lines, 'XtermPopup')
  let buf = RunVimInTerminal('-S XtermPopup', #{rows: 15})
  call TermWait(buf, 100)
  call term_sendkeys(buf, ":set noruler\<CR>")
  call term_sendkeys(buf, ":call OpenTerm()\<CR>")
  call TermWait(buf, 50)
  call term_sendkeys(buf, ":\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_m1', {})

  call TermWait(buf, 50)
  call term_sendkeys(buf, ":q\<CR>")
  call TermWait(buf, 50)  " wait for terminal to vanish
  call StopVimInTerminal(buf)
  call delete('Xtext')
  call delete('XtermPopup')
endfunc

" Check a terminal in popup window with different colors
func Terminal_in_popup_colored(group_name, highlight_cmd, highlight_opt)
  CheckRunVimInTerminal
  CheckUnix

  let lines = [
	\ 'set t_u7=',
	\ 'call setline(1, range(20))',
	\ 'func OpenTerm()',
	\ "  let s:buf = term_start('cat', #{hidden: 1, "
	\ .. a:highlight_opt .. "})",
	\ '  let g:winid = popup_create(s:buf, #{ border: []})',
	\ 'endfunc',
	\ a:highlight_cmd,
	\ ]
  call writefile(lines, 'XtermPopup')
  let buf = RunVimInTerminal('-S XtermPopup', #{rows: 15})
  call TermWait(buf, 100)
  call term_sendkeys(buf, ":set noruler\<CR>")
  call term_sendkeys(buf, ":call OpenTerm()\<CR>")
  call TermWait(buf, 50)
  call term_sendkeys(buf, "hello\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_' .. a:group_name, {})

  call term_sendkeys(buf, "\<C-D>")
  call TermWait(buf, 50)
  call term_sendkeys(buf, ":q\<CR>")
  call TermWait(buf, 50)  " wait for terminal to vanish
  call StopVimInTerminal(buf)
  call delete('XtermPopup')
endfunc

func Test_terminal_in_popup_colored_Terminal()
  call Terminal_in_popup_colored("Terminal", "highlight Terminal ctermfg=blue ctermbg=yellow", "")
endfunc

func Test_terminal_in_popup_colored_group()
  call Terminal_in_popup_colored("MyTermCol", "highlight MyTermCol ctermfg=darkgreen ctermbg=lightblue", "term_highlight: 'MyTermCol',")
endfunc

func Test_double_popup_terminal()
  let buf1 = term_start(&shell, #{hidden: 1})
  let win1 = popup_create(buf1, {})
  let buf2 = term_start(&shell, #{hidden: 1})
  let win2 = popup_create(buf2, {})
  call popup_close(win1)
  call popup_close(win2)
  exe buf1 .. 'bwipe!'
  exe buf2 .. 'bwipe!'
endfunc

func Test_issue_5607()
  let wincount = winnr('$')
  exe 'terminal' &shell &shellcmdflag 'exit'
  let job = term_getjob(bufnr())
  call WaitForAssert({-> assert_equal("dead", job_status(job))})

  let old_wincolor = &wincolor
  try
    set wincolor=
  finally
    let &wincolor = old_wincolor
    bw!
  endtry
endfunc

func Test_hidden_terminal()
  let buf = term_start(&shell, #{hidden: 1})
  call assert_equal('', bufname('^$'))
  call StopShellInTerminal(buf)
endfunc

func Test_term_nasty_callback()
  CheckExecutable sh

  set hidden
  let g:buf0 = term_start('sh', #{hidden: 1})
  call popup_create(g:buf0, {})
  let g:buf1 = term_start('sh', #{hidden: 1, term_finish: 'close'})
  call popup_create(g:buf1, {})
  call assert_fails("call term_start(['sh', '-c'], #{curwin: 1})", 'E863:')

  call popup_clear(1)
  set hidden&
endfunc

func Test_term_and_startinsert()
  CheckRunVimInTerminal
  CheckUnix

  let lines =<< trim EOL
     put='some text'
     term
     startinsert
  EOL
  call writefile(lines, 'XTest_startinsert')
  let buf = RunVimInTerminal('-S XTest_startinsert', {})

  call term_sendkeys(buf, "exit\r")
  call WaitForAssert({-> assert_equal("some text", term_getline(buf, 1))})
  call term_sendkeys(buf, "0l")
  call term_sendkeys(buf, "A<\<Esc>")
  call WaitForAssert({-> assert_equal("some text<", term_getline(buf, 1))})

  call StopVimInTerminal(buf)
  call delete('XTest_startinsert')
endfunc

" Test for passing invalid arguments to terminal functions
func Test_term_func_invalid_arg()
  call assert_fails('let b = term_getaltscreen([])', 'E745:')
  call assert_fails('let p = term_getansicolors([])', 'E745:')
  call assert_fails('let a = term_getattr(1, [])', 'E730:')
  call assert_fails('let c = term_getcursor([])', 'E745:')
  call assert_fails('let l = term_getline([], 1)', 'E745:')
  call assert_fails('let l = term_getscrolled([])', 'E745:')
  call assert_fails('let s = term_getsize([])', 'E745:')
  call assert_fails('let s = term_getstatus([])', 'E745:')
  call assert_fails('let s = term_scrape([], 1)', 'E745:')
  call assert_fails('call term_sendkeys([], "a")', 'E745:')
  call assert_fails('call term_setansicolors([], [])', 'E745:')
  call assert_fails('call term_setapi([], "")', 'E745:')
  call assert_fails('call term_setrestore([], "")', 'E745:')
  call assert_fails('call term_setkill([], "")', 'E745:')
endfunc

" Test for sending various special keycodes to a terminal
func Test_term_keycode_translation()
  CheckRunVimInTerminal

  let buf = RunVimInTerminal('', {})
  call term_sendkeys(buf, ":set nocompatible\<CR>")

  let keys = ["\<F1>", "\<F2>", "\<F3>", "\<F4>", "\<F5>", "\<F6>", "\<F7>",
        \ "\<F8>", "\<F9>", "\<F10>", "\<F11>", "\<F12>", "\<Home>",
        \ "\<S-Home>", "\<C-Home>", "\<End>", "\<S-End>", "\<C-End>",
	\ "\<Ins>", "\<Del>", "\<Left>", "\<S-Left>", "\<C-Left>", "\<Right>",
        \ "\<S-Right>", "\<C-Right>", "\<Up>", "\<S-Up>", "\<Down>",
        \ "\<S-Down>"]
  let output = ['<F1>', '<F2>', '<F3>', '<F4>', '<F5>', '<F6>', '<F7>',
        \ '<F8>', '<F9>', '<F10>', '<F11>', '<F12>', '<Home>', '<S-Home>',
        \ '<C-Home>', '<End>', '<S-End>', '<C-End>', '<Insert>', '<Del>',
        \ '<Left>', '<S-Left>', '<C-Left>', '<Right>', '<S-Right>',
        \ '<C-Right>', '<Up>', '<S-Up>', '<Down>', '<S-Down>',
        \ '0123456789', "\t\t.+-*/"]

  for k in keys
    call term_sendkeys(buf, "i\<C-K>" .. k .. "\<CR>\<C-\>\<C-N>")
  endfor
  call term_sendkeys(buf, "i\<K0>\<K1>\<K2>\<K3>\<K4>\<K5>\<K6>\<K7>")
  call term_sendkeys(buf, "\<K8>\<K9>\<kEnter>\<kPoint>\<kPlus>")
  call term_sendkeys(buf, "\<kMinus>\<kMultiply>\<kDivide>\<C-\>\<C-N>")
  call term_sendkeys(buf, "\<Home>\<Ins>\<Tab>\<S-Tab>\<C-\>\<C-N>")

  call term_sendkeys(buf, ":write Xkeycodes\<CR>")
  call term_wait(buf)
  call StopVimInTerminal(buf)
  call assert_equal(output, readfile('Xkeycodes'))
  call delete('Xkeycodes')
endfunc

" Test for using the mouse in a terminal
func Test_term_mouse()
  CheckNotGui
  CheckRunVimInTerminal

  let save_mouse = &mouse
  let save_term = &term
  let save_ttymouse = &ttymouse
  let save_clipboard = &clipboard
  call test_override('no_query_mouse', 1)
  set mouse=a term=xterm ttymouse=sgr mousetime=200 clipboard=

  let lines =<< trim END
    one two three four five
    red green yellow red blue
    vim emacs sublime nano
  END
  call writefile(lines, 'Xtest_mouse')

  let buf = RunVimInTerminal('Xtest_mouse -n', {})
  call term_sendkeys(buf, ":set nocompatible\<CR>")
  call term_sendkeys(buf, ":set mouse=a term=xterm ttymouse=sgr\<CR>")
  call term_sendkeys(buf, ":set clipboard=\<CR>")
  call term_sendkeys(buf, ":set mousemodel=extend\<CR>")
  call term_wait(buf)
  redraw!

  " Test for <LeftMouse> click/release
  call test_setmouse(2, 5)
  call feedkeys("\<LeftMouse>\<LeftRelease>", 'xt')
  call test_setmouse(3, 8)
  call term_sendkeys(buf, "\<LeftMouse>\<LeftRelease>")
  call term_wait(buf, 50)
  call term_sendkeys(buf, ":call writefile([json_encode(getpos('.'))], 'Xbuf')\<CR>")
  call term_wait(buf, 50)
  let pos = json_decode(readfile('Xbuf')[0])
  call assert_equal([3, 8], pos[1:2])

  " Test for selecting text using mouse
  call delete('Xbuf')
  call test_setmouse(2, 11)
  call term_sendkeys(buf, "\<LeftMouse>")
  call test_setmouse(2, 16)
  call term_sendkeys(buf, "\<LeftRelease>y")
  call term_wait(buf, 50)
  call term_sendkeys(buf, ":call writefile([@\"], 'Xbuf')\<CR>")
  call term_wait(buf, 50)
  call assert_equal('yellow', readfile('Xbuf')[0])

  " Test for selecting text using doubleclick
  call delete('Xbuf')
  call test_setmouse(1, 11)
  call term_sendkeys(buf, "\<LeftMouse>\<LeftRelease>\<LeftMouse>")
  call test_setmouse(1, 17)
  call term_sendkeys(buf, "\<LeftRelease>y")
  call term_wait(buf, 50)
  call term_sendkeys(buf, ":call writefile([@\"], 'Xbuf')\<CR>")
  call term_wait(buf, 50)
  call assert_equal('three four', readfile('Xbuf')[0])

  " Test for selecting a line using triple click
  call delete('Xbuf')
  call test_setmouse(3, 2)
  call term_sendkeys(buf, "\<LeftMouse>\<LeftRelease>\<LeftMouse>\<LeftRelease>\<LeftMouse>\<LeftRelease>y")
  call term_wait(buf, 50)
  call term_sendkeys(buf, ":call writefile([@\"], 'Xbuf')\<CR>")
  call term_wait(buf, 50)
  call assert_equal("vim emacs sublime nano\n", readfile('Xbuf')[0])

  " Test for selecting a block using qudraple click
  call delete('Xbuf')
  call test_setmouse(1, 11)
  call term_sendkeys(buf, "\<LeftMouse>\<LeftRelease>\<LeftMouse>\<LeftRelease>\<LeftMouse>\<LeftRelease>\<LeftMouse>")
  call test_setmouse(3, 13)
  call term_sendkeys(buf, "\<LeftRelease>y")
  call term_wait(buf, 50)
  call term_sendkeys(buf, ":call writefile([@\"], 'Xbuf')\<CR>")
  call term_wait(buf, 50)
  call assert_equal("ree\nyel\nsub", readfile('Xbuf')[0])

  " Test for extending a selection using right click
  call delete('Xbuf')
  call test_setmouse(2, 9)
  call term_sendkeys(buf, "\<LeftMouse>\<LeftRelease>")
  call test_setmouse(2, 16)
  call term_sendkeys(buf, "\<RightMouse>\<RightRelease>y")
  call term_wait(buf, 50)
  call term_sendkeys(buf, ":call writefile([@\"], 'Xbuf')\<CR>")
  call term_wait(buf, 50)
  call assert_equal("n yellow", readfile('Xbuf')[0])

  " Test for pasting text using middle click
  call delete('Xbuf')
  call term_sendkeys(buf, ":let @r='bright '\<CR>")
  call test_setmouse(2, 22)
  call term_sendkeys(buf, "\"r\<MiddleMouse>\<MiddleRelease>")
  call term_wait(buf, 50)
  call term_sendkeys(buf, ":call writefile([getline(2)], 'Xbuf')\<CR>")
  call term_wait(buf, 50)
  call assert_equal("red bright blue", readfile('Xbuf')[0][-15:])

  " cleanup
  call term_wait(buf)
  call StopVimInTerminal(buf)
  let &mouse = save_mouse
  let &term = save_term
  let &ttymouse = save_ttymouse
  let &clipboard = save_clipboard
  set mousetime&
  call test_override('no_query_mouse', 0)
  call delete('Xtest_mouse')
  call delete('Xbuf')
endfunc

" Test for modeless selection in a terminal
func Test_term_modeless_selection()
  CheckUnix
  CheckNotGui
  CheckRunVimInTerminal
  CheckFeature clipboard_working

  let save_mouse = &mouse
  let save_term = &term
  let save_ttymouse = &ttymouse
  call test_override('no_query_mouse', 1)
  set mouse=a term=xterm ttymouse=sgr mousetime=200
  set clipboard=autoselectml

  let lines =<< trim END
    one two three four five
    red green yellow red blue
    vim emacs sublime nano
  END
  call writefile(lines, 'Xtest_modeless')

  let buf = RunVimInTerminal('Xtest_modeless -n', {})
  call term_sendkeys(buf, ":set nocompatible\<CR>")
  call term_sendkeys(buf, ":set mouse=\<CR>")
  call term_wait(buf)
  redraw!

  " Test for copying a modeless selection to clipboard
  let @* = 'clean'
  " communicating with X server may take a little time
  sleep 100m
  call feedkeys(MouseLeftClickCode(2, 3), 'x')
  call feedkeys(MouseLeftDragCode(2, 11), 'x')
  call feedkeys(MouseLeftReleaseCode(2, 11), 'x')
  call assert_equal("d green y", @*)

  " cleanup
  call term_wait(buf)
  call StopVimInTerminal(buf)
  let &mouse = save_mouse
  let &term = save_term
  let &ttymouse = save_ttymouse
  set mousetime& clipboard&
  call test_override('no_query_mouse', 0)
  call delete('Xtest_modeless')
endfunc

" vim: shiftwidth=2 sts=2 expandtab
