" Tests for the +clientserver feature.

source util/screendump.vim
CheckFeature job

if !has('clientserver')
  call assert_fails('call remote_startserver("local")', 'E942:')
endif

CheckFeature clientserver

source util/shared.vim

func Check_X11_Connection()
  if has('x11')
    CheckX11
    try
      call remote_send('xxx', '')
    catch
      if v:exception =~ 'E240:'
        throw 'Skipped: no connection to the X server'
      endif
      " ignore other errors
    endtry
  endif
endfunc

func Test_client_server()
  let g:test_is_flaky = 1
  let cmd = GetVimCommand()
  if cmd == ''
    throw 'GetVimCommand() failed'
  endif
  call Check_X11_Connection()

  let name = 'XVIMTEST'
  let cmd .= ' --servername ' . name
  let job = job_start(cmd, {'stoponexit': 'kill', 'out_io': 'null'})
  call WaitForAssert({-> assert_equal("run", job_status(job))})

  " Takes a short while for the server to be active.
  " When using valgrind it takes much longer.
  call WaitForAssert({-> assert_match(name, serverlist())})

  if !has('win32')
    if RunVim([], [], '--serverlist >Xtest_serverlist')
      let lines = readfile('Xtest_serverlist')
      call assert_true(index(lines, 'XVIMTEST') >= 0)
    endif
    call delete('Xtest_serverlist')
  endif

  eval name->remote_foreground()

  call remote_send(name, ":let testvar = 'yes'\<CR>")
  call WaitFor('remote_expr("' . name . '", "exists(\"testvar\") ? testvar : \"\"", "", 1) == "yes"')
  call assert_equal('yes', remote_expr(name, "testvar", "", 2))
  call assert_fails("let x=remote_expr(name, '2+x')", 'E449:')
  call assert_fails("let x=remote_expr('[], '2+2')", 'E116:')

  if has('unix') && has('gui') && !has('gui_running')
    " Running in a terminal and the GUI is available: Tell the server to open
    " the GUI and check that the remote command still works.
    " Need to wait for the GUI to start up, otherwise the send hangs in trying
    " to send to the terminal window.
    if has('gui_motif')
      " For this GUI ignore the 'failed to create input context' error.
      call remote_send(name, ":call test_ignore_error('E285') | gui -f\<CR>")
    else
      call remote_send(name, ":gui -f\<CR>")
    endif
    " Wait for the server to be up and answering requests.
    " When using valgrind this can be very, very slow.
    sleep 1
    call WaitForAssert({-> assert_match('\d', name->remote_expr("v:version", "", 1))}, 10000)

    call remote_send(name, ":let testvar = 'maybe'\<CR>")
    call WaitForAssert({-> assert_equal('maybe', remote_expr(name, "testvar", "", 2))})
  endif

  call assert_fails('call remote_send("XXX", ":let testvar = ''yes''\<CR>")', 'E241:')

  call writefile(['one'], 'Xclientfile')
  let cmd = GetVimProg() .. ' --servername ' .. name .. ' --remote Xclientfile'
  call system(cmd)
  call WaitForAssert({-> assert_equal('Xclientfile', remote_expr(name, "bufname()", "", 2))})
  call WaitForAssert({-> assert_equal('one', remote_expr(name, "getline(1)", "", 2))})
  call writefile(['one', 'two'], 'Xclientfile')
  call system(cmd)
  call WaitForAssert({-> assert_equal('two', remote_expr(name, "getline(2)", "", 2))})
  call delete('Xclientfile')

  " Expression evaluated locally.
  if v:servername == ''
    eval 'MYSELF'->remote_startserver()
    " May get MYSELF1 when running the test again.
    call assert_match('MYSELF', v:servername)
    call assert_fails("call remote_startserver('MYSELF')", 'E941:')
  endif
  let g:testvar = 'myself'
  call assert_equal('myself', remote_expr(v:servername, 'testvar'))
  call remote_send(v:servername, ":let g:testvar2 = 75\<CR>")
  call feedkeys('', 'x')
  call assert_equal(75, g:testvar2)
  call assert_fails('let v = remote_expr(v:servername, "/2")', ['E15:.*/2'])

  call remote_send(name, ":call server2client(expand('<client>'), 'got it')\<CR>", 'g:myserverid')
  call assert_equal('got it', g:myserverid->remote_read(2))

  call remote_send(name, ":eval expand('<client>')->server2client('another')\<CR>", 'g:myserverid')
  let peek_result = 'nothing'
  let r = g:myserverid->remote_peek('peek_result')
  " unpredictable whether the result is already available.
  if r > 0
    call assert_equal('another', peek_result)
  elseif r == 0
    call assert_equal('nothing', peek_result)
  else
    call assert_report('remote_peek() failed')
  endif
  let g:peek_result = 'empty'
  call WaitFor('remote_peek(g:myserverid, "g:peek_result") > 0')
  call assert_equal('another', g:peek_result)
  call assert_equal('another', remote_read(g:myserverid, 2))

  if !has('gui_running')
    " In GUI vim, the following tests display a dialog box

    let cmd = GetVimProg() .. ' --servername ' .. name

    " Run a separate instance to send a command to the server
    call remote_expr(name, 'execute("only")')
    call system(cmd .. ' --remote-send ":new Xclientfile<CR>"')
    call assert_equal('2', remote_expr(name, 'winnr("$")'))
    call assert_equal('Xclientfile', remote_expr(name, 'winbufnr(1)->bufname()'))
    call remote_expr(name, 'execute("only")')

    " Invoke a remote-expr. On MS-Windows, the returned value has a carriage
    " return.
    let l = system(cmd .. ' --remote-expr "2 + 2"')
    call assert_equal(['4'], split(l, "\n"))

    " Edit multiple files using --remote
    call system(cmd .. ' --remote Xclientfile1 Xclientfile2 Xclientfile3')
    call WaitForAssert({-> assert_equal('3', remote_expr(name, 'argc()'))})
    call assert_match(".*Xclientfile1\n.*Xclientfile2\n.*Xclientfile3\n", remote_expr(name, 'argv()'))
    eval name->remote_send(":%bw!\<CR>")

    " Edit files in separate tab pages
    call system(cmd .. ' --remote-tab Xclientfile1 Xclientfile2 Xclientfile3')
    call WaitForAssert({-> assert_equal('3', remote_expr(name, 'tabpagenr("$")'))})
    call assert_match('.*\<Xclientfile2', remote_expr(name, 'bufname(tabpagebuflist(2)[0])'))
    eval name->remote_send(":%bw!\<CR>")

    " Edit a file using --remote-wait
    eval name->remote_send(":source $VIMRUNTIME/plugin/rrhelper.vim\<CR>")
    call system(cmd .. ' --remote-wait +enew Xclientfile1')
    call assert_match('.*\<Xclientfile1', remote_expr(name, 'bufname("#")'))
    eval name->remote_send(":%bw!\<CR>")

    " Edit files using --remote-tab-wait
    call system(cmd .. ' --remote-tabwait +tabonly\|enew Xclientfile1 Xclientfile2')
    call assert_equal('1', remote_expr(name, 'tabpagenr("$")'))
    eval name->remote_send(":%bw!\<CR>")

    " Error cases
    if v:lang == "C" || v:lang =~ '^[Ee]n'
      let l = split(system(cmd .. ' --remote +pwd'), "\n")
      call assert_equal("Argument missing after: \"+pwd\"", l[1])
    endif
    let l = system(cmd .. ' --remote-expr "abcd"')
    call assert_match('^E449: ', l)
  endif

  eval name->remote_send(":%bw!\<CR>")
  eval name->remote_send(":qa!\<CR>")
  try
    call WaitForAssert({-> assert_equal("dead", job_status(job))})
  finally
    if job_status(job) != 'dead'
      call assert_report('Server did not exit')
      call job_stop(job, 'kill')
    endif
  endtry

  call assert_fails('call remote_startserver("")', 'E1175:')
  call assert_fails('call remote_startserver([])', 'E1174:')
  call assert_fails("let x = remote_peek([])", 'E730:')

  " When using socket server, server id is not a number, but the path to the
  " socket.
  if (has('socketserver') && !has('X11') && !has('win32')) || index(v:argv, "socket") != -1
    call assert_fails("let x = remote_read('vim/10')", ['E1564:'])
    call assert_fails("call server2client('x/b/c', 'xyz')", ['E1564:'])
  else
    call assert_fails("let x = remote_read('vim10')",
          \ has('unix') ? ['E573:.*vim10'] : 'E277:')
    call assert_fails("call server2client('abc', 'xyz')",
          \ has('unix') ? ['E573:.*abc'] : 'E258:')
  endif
endfunc

func Test_client_server_stopinsert()
  " test does not work on MS-Windows
  CheckNotMSWindows
  CheckNotMac
  let g:test_is_flaky = 1
  let cmd = GetVimCommand()
  if cmd == ''
    throw 'GetVimCommand() failed'
  endif
  call Check_X11_Connection()
  let fname = 'Xclientserver_stop.txt'
  let name = 'XVIMTEST2'
  call writefile(['one two three'], fname, 'D')

  let cmd .= ' -c "set virtualedit=onemore"'
  let cmd .= ' -c "call cursor(1, 14)"'
  let cmd .= ' -c "startinsert"'
  let cmd .= ' --servername ' . name
  let cmd .= ' ' .. fname
  let job = job_start(cmd, {'stoponexit': 'kill', 'out_io': 'null'})
  call WaitForAssert({-> assert_equal("run", job_status(job))})

  " Takes a short while for the server to be active.
  " When using valgrind it takes much longer.
  call WaitForAssert({-> assert_match(name, serverlist())})

  call remote_send(name, "\<C-\>\<C-N>")

  " Wait for the mode to change to Normal ('n')
  call WaitForAssert({-> assert_equal('n', name->remote_expr("mode(1)"))})
  call WaitForAssert({-> assert_equal('13', name->remote_expr("col('.')"))})

  call remote_send(name, "\<C-\>\<C-N>:qa!\<CR>")
  try
    call WaitForAssert({-> assert_equal("dead", job_status(job))})
  finally
    if job_status(job) != 'dead'
      call assert_report('Server did not exit')
      call job_stop(job, 'kill')
    endif
  endtry
endfunc

" Test if socket server, X11, and mswin backends can be chosen and work properly.
func Test_client_server_multiple_backends()
    CheckFeature socketserver

    let g:test_is_flaky = 1
    let cmd = GetVimCommand()

    if cmd == ''
      throw 'GetVimCommand() failed'
    endif
    call Check_X11_Connection()

    let types = [
          \ ['socket', "channel:2000"],
          \ ['x11', "TEST"],
          \ ['mswin', "TEST"],
          \ ]

    for [type, expected] in types
      if (type == 'x11' && (!has('x11') || !empty($WAYLAND_DISPLAY) || empty($DISPLAY))
            \ || (type == 'mswin' && !has('win32')))
        continue
      endif
      if has('win32') && has('gui_running')
        " Windows gVim --remote-expr shows a dialog window, which blocks tests
        " from running. Using --gui-dialog-file does not seem to work either.
        continue
      endif

      let actual_cmd = cmd .. ' --clientserver ' .. type
      let actual_cmd ..= ' --servername ' .. expected
      let job = job_start(actual_cmd, {'stoponexit': 'kill', 'out_io': 'null'})

      call WaitForAssert({-> assert_equal("run", job_status(job))})
      call assert_match(expected, system(actual_cmd .. ' --remote-expr "v:servername"'))

      " On Windows using --remote-expr causes E282, possibly due to some shell
      " escaping quirk? When gtk gui is running, using system() seems to cause a
      " deadlock when using the x11 backend only... don't use it for now...
      if has('win32') || has('gui_running')
        call job_stop(job, 'kill')
      else
        call system(actual_cmd .. " --remote-expr 'execute(\"qa!\")'")
      endif
      try
        call WaitForAssert({-> assert_equal("dead", job_status(job))})
      finally
        if job_status(job) != 'dead'
          call assert_report('Server did not exit')
          call job_stop(job, 'kill')
        endif
      endtry
    endfor
  endfunc

" Test if custom paths work for socketserver
func Test_client_server_socketserver_custom_path()
  CheckFeature socketserver
  CheckNotMSWindows

  let g:test_is_flaky = 1
  let cmd = GetVimCommand()

  if cmd == ''
    throw 'GetVimCommand() failed'
  endif

  let name = 'VIMTESTSOCKET2'

  let paths = ['./' .. name, '../testdir/' .. name, getcwd(-1) .. '/' .. name]

  for path in paths
    let actual = cmd .. ' --clientserver socket --servername ' .. path

    let job = job_start(actual, {'stoponexit': 'kill', 'out_io': 'null'})

    call WaitForAssert({-> assert_equal("run", job_status(job))})
    call WaitForAssert({-> assert_equal(path, glob(path))})

    call system(actual .. " --remote-expr 'execute(\"qa!\")'")
    try
      call WaitForAssert({-> assert_equal("dead", job_status(job))})
    finally
      if job_status(job) != 'dead'
        call assert_report('Server did not exit')
        call job_stop(job, 'kill')
      endif
    endtry
  endfor
endfunc

" Test if "channel:" prefix works correctly to use channel address for
" socketserver.
func Test_client_server_socketserver_address()
  CheckFeature socketserver

  let g:test_is_flaky = 1
  let cmd = GetVimCommand()

  if cmd == ''
    throw 'GetVimCommand() failed'
  endif

  let actual = cmd .. ' --clientserver socket --servername channel:2000'

  let job = job_start(actual, {'stoponexit': 'kill', 'out_io': 'null'})

  call WaitForAssert({-> assert_equal("run", job_status(job))})

  if !has('win32') || !has('gui_running')
    " Does not work with gVim on Windows because it shows an OK dialog box which
    " blocks tests from running
      call assert_match('channel:2000', system(actual .. ' --remote-expr "v:servername"'))
  endif

  if has('win32')
    call job_stop(job, 'kill')
  else
    call system(actual .. " --remote-expr 'execute(\"qa!\")'")
  endif
  try
    call WaitForAssert({-> assert_equal("dead", job_status(job))})
  finally
    if job_status(job) != 'dead'
      call assert_report('Server did not exit')
      call job_stop(job, 'kill')
    endif
  endtry
endfunc

" A flood of client connections must not crash the socketserver: connections
" beyond the accept cap are refused instead of overflowing the fd_set / pollfd
" sets, and the server keeps working once they are closed again.
func Test_client_server_socketserver_connection_flood()
  CheckFeature socketserver
  CheckNotMSWindows

  let g:test_is_flaky = 1
  let cmd = GetVimCommand()
  if cmd == ''
    throw 'GetVimCommand() failed'
  endif

  let actual = cmd .. ' --clientserver socket --servername channel:2001'
  let job = job_start(actual, {'stoponexit': 'kill', 'out_io': 'null'})
  call WaitForAssert({-> assert_equal("run", job_status(job))})
  call WaitForAssert({-> assert_match('channel:2001',
        \ system(actual .. ' --remote-expr "v:servername"'))})

  " Open many more connections than the server accepts.
  let channels = []
  for i in range(150)
    let ch = ch_open('localhost:2001', {'mode': 'raw', 'waittime': 100})
    if ch_status(ch) == 'open'
      call add(channels, ch)
    endif
  endfor

  " The server must survive, and must have refused the excess connections.
  call assert_equal("run", job_status(job))
  call WaitForAssert({-> assert_inrange(1, 100,
        \ len(filter(copy(channels), {_, c -> ch_status(c) == 'open'})))})

  " Close them again.  Afterwards the server must accept connections once
  " more, which also verifies the client count is decremented.
  for ch in channels
    if ch_status(ch) == 'open'
      call ch_close(ch)
    endif
  endfor
  call WaitForAssert({-> assert_match('channel:2001',
        \ system(actual .. ' --remote-expr "v:servername"'))})

  call system(actual .. " --remote-expr 'execute(\"qa!\")'")
  try
    call WaitForAssert({-> assert_equal("dead", job_status(job))})
  finally
    if job_status(job) != 'dead'
      call assert_report('Server did not exit')
      call job_stop(job, 'kill')
    endif
  endtry
endfunc

" Test if --remote-wait works properly with multiple files
func Test_client_server_multiple_remote_wait()
  CheckRunVimInTerminal

  call Check_X11_Connection()

  let buf = RunVimInTerminal('--servername TEST', {'rows': 8})
  call TermWait(buf)

  call writefile(["1"], 'XRemoteOne', 'D')
  call writefile(["2"], 'XRemoteTwo', 'D')

  let cmd = GetVimCommand()

  if cmd == ''
    throw 'GetVimCommand() failed'
  endif

  let actual = cmd .. ' -n --servername TEST  --remote-wait ./XRemoteOne ./XRemoteTwo'
  let job = job_start(actual, {'stoponexit': 'kill', 'out_io': 'null'})

  sleep 500m " Wait for server to receive request
  call assert_equal("run", job_status(job))

  call term_sendkeys(buf, "\<Esc>:next\<CR>")
  call TermWait(buf)
  call assert_equal("run", job_status(job))

  call term_sendkeys(buf, "\<Esc>:q!\<CR>") " Don't use qa! because we only want to quit one file
  call WaitForAssert({-> assert_equal("dead", job_status(job))})
endfunc

" Check if socket is removed even if Vim changes directory
func Test_client_server_socketserver_chdir()
  CheckFeature socketserver
  CheckRunVimInTerminal
  CheckNotMSWindows

  let buf = RunVimInTerminal('--clientserver socket --servername ./TEST',
        \ {'rows': 8})
  call TermWait(buf)

  call term_sendkeys(buf, "\<Esc>:cd ../\<CR>")
  call StopVimInTerminal(buf)

  call assert_equal("", glob("./TEST"))
endfunc

func DummyServerCallback(ch, addr)
  let msg = json_encode(#{type: "ver"}) .. "\n"
  call ch_sendraw(a:ch, msg)
endfunc

" Test if commands with different version are ignored and handled properly.
func Test_client_server_socketserver_version_mismatch()
  CheckFeature socketserver
  CheckRunVimInTerminal
  CheckScreendump

  let cmd = GetVimCommand()

  if cmd == ''
    throw 'GetVimCommand() failed'
  endif

  let buf = RunVimInTerminal('--clientserver socket --servername
        \ channel:2000', {'rows': 8})
  call TermWait(buf)

  let ch = ch_open('localhost:2000', #{mode: 'nl'})
  let msg = json_encode(#{
        \ type: "expr",
        \ str: "v:servername",
        \ version: 999999999
        \ }) .. "\n"

  call ch_sendraw(ch, msg)
  let resp = ch_readraw(ch)

  call assert_equal(#{type: "ver"}, json_decode(resp))

  call StopVimInTerminal(buf)

  let ch = ch_listen("2000", #{
        \ mode: 'nl',
        \ callback: function('DummyServerCallback')
        \ })

  let buf = RunVimInTerminal('--clientserver socket --servername channel:3000', {'rows': 8})
  call TermWait(buf)

  call term_sendkeys(buf, "\<Esc>:echo remote_expr('channel:2000', 'v:servername', '', 1)\<CR>")
  call TermWait(buf)

  call VerifyScreenDump(buf, 'Test_clientserver_1', #{wait: 3000})

  call StopVimInTerminal(buf)
endfunc

" Test if invalid messages do not crash Vim socketserver.
func Test_clientserver_socketserver_invalid_msg()
  CheckFeature socketserver
  CheckRunVimInTerminal

  let cmd = GetVimCommand()

  if cmd == ''
    throw 'GetVimCommand() failed'
  endif

  let buf = RunVimInTerminal('--clientserver socket --servername
        \ channel:3000', {'rows': 8})
  call TermWait(buf)

  let ch = ch_open('localhost:3000', #{mode: 'nl'})

  call ch_sendraw(ch, "wjdaljdsjalsj\n")
  call ch_sendraw(ch, "{\"type\": \"unknown\"}\n")

  call assert_match("channel:3000", system(cmd .. " --clientserver socket --servername channel:3000 --remote-expr 'v:servername'"))
  call assert_equal("running", term_getstatus(buf))

  call StopVimInTerminal(buf)
endfunc

" Test that $VIM_CLIENTSERVER env var works properly
func Test_clientserver_env_method()
  CheckFeature socketserver

  let g:test_is_flaky = 1
  let cmd = GetVimCommand()

  if cmd == ''
    throw 'GetVimCommand() failed'
  endif

  " Don't use channel:2000, because previous tests use that and it may take a
  " while for the channel to fully close.
  let actual = cmd .. ' --servername channel:4000'
  let save_vim_clientserver = $VIM_CLIENTSERVER
  let $VIM_CLIENTSERVER = 'socket'

  let job = job_start(actual, {'stoponexit': 'kill', 'out_io': 'null'})

  call WaitForAssert({-> assert_equal("run", job_status(job))})

  if !has('win32') || !has('gui_running')
      call assert_match('channel:4000',
            \ system(actual .. ' --remote-expr "v:servername"'))
  endif

  if has('win32')
    call job_stop(job, 'kill')
  else
    call system(actual .. " --remote-expr 'execute(\"qa!\")'")
  endif
  if save_vim_clientserver == ''
    unlet $VIM_CLIENTSERVER
  else
    let $VIM_CLIENTSERVER = save_vim_clientserver
  endif

  try
    call WaitForAssert({-> assert_equal("dead", job_status(job))})
  finally
    if job_status(job) != 'dead'
      call assert_report('Server did not exit')
      call job_stop(job, 'kill')
    endif
  endtry
endfunc

" Test if serverlist() can return a list of strings
func Test_clientserver_serverlist_list()
  CheckNotGui

  " CheckNotGui has already confirmed gvim is not being used to run this test.
  " However, if this is a GUI _build_ of vim, then the running Vim process
  " will already have selected _not_ to use socket clientserver. Therefore, we
  " either need the ability to use the GUI clientserver or to skip the test.
  call Check_X11_Connection()

  let g:test_is_flaky = 1
  let cmd = GetVimCommand()

  if cmd == ''
    throw 'GetVimCommand() failed'
  endif

  " Don't use channel:2000, because previous tests use that and it may take a
  " while for the channel to fully close.
  let actual = cmd .. ' --servername XVIMTEST'

  let job = job_start(actual, {'stoponexit': 'kill', 'out_io': 'null'})

  call WaitForAssert({-> assert_match('XVIMTEST', serverlist())})

  call assert_equal('list<string>', typename(serverlist(#{list: v:true})))
  call assert_true(serverlist(#{list: v:true})->index('XVIMTEST') != -1)

  if has('win32')
    call job_stop(job, 'kill')
  else
    call system(actual .. " --remote-expr 'execute(\"qa!\")'")
  endif
  try
    call WaitForAssert({-> assert_equal("dead", job_status(job))})
  finally
    if job_status(job) != 'dead'
      call assert_report('Server did not exit')
      call job_stop(job, 'kill')
    endif
  endtry
endfunc

func Test_clientserver_serverlist_without_x11()
  CheckNotGui
  CheckFeature x11

  let cmd = GetVimCommand()
  if cmd == ''
    throw 'GetVimCommand() failed'
  endif

  " This test verifies that serverlist() fails with error E240 when a
  " connection to X11 cannot be established. It must be executed with the
  " CLIENTSERVER backend set to x11 and in a state where the X11 server is
  " unreachable.
  "
  " To achieve this, the `VIM_CLIENTSERVER` and `DISPLAY` environment
  " variables must be unset before running Vim as a child process. Within the
  " child process, `assert_fails()` and `v:errors` are used to confirm that
  " E240 occurred; if E240 is raised as expected, `v:errors` remains empty,
  " whereas if the call succeeds or a different error occurs, `v:errors` will
  " contain one or more errors.

  call writefile([
        \ "call assert_fails('let x = serverlist()', 'E240:')",
        \ "execute 'cq! ' .. len(v:errors)"
        \ ], 'Xtest', 'D')

  let save_vim_clientserver = $VIM_CLIENTSERVER
  unlet $VIM_CLIENTSERVER
  let save_display = $DISPLAY
  unlet $DISPLAY

  try
    call system(cmd .. ' -S Xtest')
    call assert_equal(0, v:shell_error)
  finally
    if save_display != ''
      let $DISPLAY = save_display
    endif
    if save_vim_clientserver != ''
      let $VIM_CLIENTSERVER = save_vim_clientserver
    endif
  endtry
endfunc

" Uncomment this line to get a debugging log
" call ch_logfile('channellog', 'w')

" vim: shiftwidth=2 sts=2 expandtab
