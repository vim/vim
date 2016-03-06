" Test for channel functions.
scriptencoding utf-8

if !has('channel')
  finish
endif

" This test requires the Python command to run the test server.
" This most likely only works on Unix and Windows.
if has('unix')
  " We also need the job feature or the pkill command to make sure the server
  " can be stopped.
  if !(executable('python') && (has('job') || executable('pkill')))
    finish
  endif
  let s:python = 'python'
elseif has('win32')
  " Use Python Launcher for Windows (py.exe) if available.
  if executable('py.exe')
    let s:python = 'py.exe'
  elseif executable('python.exe')
    let s:python = 'python.exe'
  else
    finish
  endif
else
  " Can't run this test.
  finish
endif

let s:chopt = {}

" Run "testfunc" after sarting the server and stop the server afterwards.
func s:run_server(testfunc, ...)
  " The Python program writes the port number in Xportnr.
  call delete("Xportnr")

  if a:0 == 1
    let arg = ' ' . a:1
  else
    let arg = ''
  endif
  let cmd = s:python . " test_channel.py" . arg

  try
    if has('job')
      let s:job = job_start(cmd, {"stoponexit": "hup"})
      call job_setoptions(s:job, {"stoponexit": "kill"})
    elseif has('win32')
      exe 'silent !start cmd /c start "test_channel" ' . cmd
    else
      exe 'silent !' . cmd . '&'
    endif

    " Wait for up to 2 seconds for the port number to be there.
    let cnt = 20
    let l = []
    while cnt > 0
      try
        let l = readfile("Xportnr")
      catch
      endtry
      if len(l) >= 1
        break
      endif
      sleep 100m
      let cnt -= 1
    endwhile
    call delete("Xportnr")

    if len(l) == 0
      " Can't make the connection, give up.
      call assert_false(1, "Can't start test_channel.py")
      return -1
    endif
    let port = l[0]

    call call(function(a:testfunc), [port])
  catch
    call assert_false(1, "Caught exception: " . v:exception)
  finally
    call s:kill_server()
  endtry
endfunc

func s:kill_server()
  if has('job')
    if exists('s:job')
      call job_stop(s:job)
      unlet s:job
    endif
  elseif has('win32')
    call system('taskkill /IM ' . s:python . ' /T /F /FI "WINDOWTITLE eq test_channel"')
  else
    call system("pkill -f test_channel.py")
  endif
endfunc

let s:responseMsg = ''
func s:RequestHandler(handle, msg)
  let s:responseHandle = a:handle
  let s:responseMsg = a:msg
endfunc

func s:communicate(port)
  let handle = ch_open('localhost:' . a:port, s:chopt)
  if ch_status(handle) == "fail"
    call assert_false(1, "Can't open channel")
    return
  endif
  if has('job')
    " check that no job is handled correctly
    call assert_equal('no process', string(ch_getjob(handle)))
  endif

  " Simple string request and reply.
  call assert_equal('got it', ch_evalexpr(handle, 'hello!'))

  " Request that triggers sending two ex commands.  These will usually be
  " handled before getting the response, but it's not guaranteed, thus wait a
  " tiny bit for the commands to get executed.
  call assert_equal('ok', ch_evalexpr(handle, 'make change'))
  sleep 10m
  call assert_equal('added1', getline(line('$') - 1))
  call assert_equal('added2', getline('$'))

  call assert_equal('ok', ch_evalexpr(handle, 'do normal', {'timeout': 100}))
  sleep 10m
  call assert_equal('added more', getline('$'))

  " Send a request with a specific handler.
  call ch_sendexpr(handle, 'hello!', {'callback': 's:RequestHandler'})
  sleep 10m
  if !exists('s:responseHandle')
    call assert_false(1, 's:responseHandle was not set')
  else
    call assert_equal(handle, s:responseHandle)
    unlet s:responseHandle
  endif
  call assert_equal('got it', s:responseMsg)

  let s:responseMsg = ''
  call ch_sendexpr(handle, 'hello!', {'callback': function('s:RequestHandler')})
  sleep 10m
  if !exists('s:responseHandle')
    call assert_false(1, 's:responseHandle was not set')
  else
    call assert_equal(handle, s:responseHandle)
    unlet s:responseHandle
  endif
  call assert_equal('got it', s:responseMsg)

  " Collect garbage, tests that our handle isn't collected.
  call garbagecollect()

  " check setting options (without testing the effect)
  call ch_setoptions(handle, {'callback': 's:NotUsed'})
  call ch_setoptions(handle, {'timeout': 1111})
  call ch_setoptions(handle, {'mode': 'json'})
  call assert_fails("call ch_setoptions(handle, {'waittime': 111})", "E475")
  call ch_setoptions(handle, {'callback': ''})

  " Send an eval request that works.
  call assert_equal('ok', ch_evalexpr(handle, 'eval-works'))
  sleep 10m
  call assert_equal([-1, 'foo123'], ch_evalexpr(handle, 'eval-result'))

  " Send an eval request that fails.
  call assert_equal('ok', ch_evalexpr(handle, 'eval-fails'))
  sleep 10m
  call assert_equal([-2, 'ERROR'], ch_evalexpr(handle, 'eval-result'))

  " Send an eval request that works but can't be encoded.
  call assert_equal('ok', ch_evalexpr(handle, 'eval-error'))
  sleep 10m
  call assert_equal([-3, 'ERROR'], ch_evalexpr(handle, 'eval-result'))

  " Send a bad eval request. There will be no response.
  call assert_equal('ok', ch_evalexpr(handle, 'eval-bad'))
  sleep 10m
  call assert_equal([-3, 'ERROR'], ch_evalexpr(handle, 'eval-result'))

  " Send an expr request
  call assert_equal('ok', ch_evalexpr(handle, 'an expr'))
  sleep 10m
  call assert_equal('one', getline(line('$') - 2))
  call assert_equal('two', getline(line('$') - 1))
  call assert_equal('three', getline('$'))

  " Request a redraw, we don't check for the effect.
  call assert_equal('ok', ch_evalexpr(handle, 'redraw'))
  call assert_equal('ok', ch_evalexpr(handle, 'redraw!'))

  call assert_equal('ok', ch_evalexpr(handle, 'empty-request'))

  " Reading while there is nothing available.
  call assert_equal(v:none, ch_read(handle, {'timeout': 0}))
  let start = reltime()
  call assert_equal(v:none, ch_read(handle, {'timeout': 333}))
  let elapsed = reltime(start)
  call assert_true(reltimefloat(elapsed) > 0.3)
  call assert_true(reltimefloat(elapsed) < 0.6)

  " Send without waiting for a response, then wait for a response.
  call ch_sendexpr(handle, 'wait a bit')
  let resp = ch_read(handle)
  call assert_equal(type([]), type(resp))
  call assert_equal(type(11), type(resp[0]))
  call assert_equal('waited', resp[1])

  " make the server quit, can't check if this works, should not hang.
  call ch_sendexpr(handle, '!quit!')
endfunc

func Test_communicate()
  call ch_log('Test_communicate()')
  call s:run_server('s:communicate')
endfunc

" Test that we can open two channels.
func s:two_channels(port)
  let handle = ch_open('localhost:' . a:port, s:chopt)
  if ch_status(handle) == "fail"
    call assert_false(1, "Can't open channel")
    return
  endif

  call assert_equal('got it', ch_evalexpr(handle, 'hello!'))

  let newhandle = ch_open('localhost:' . a:port, s:chopt)
  if ch_status(newhandle) == "fail"
    call assert_false(1, "Can't open second channel")
    return
  endif
  call assert_equal('got it', ch_evalexpr(newhandle, 'hello!'))
  call assert_equal('got it', ch_evalexpr(handle, 'hello!'))

  call ch_close(handle)
  call assert_equal('got it', ch_evalexpr(newhandle, 'hello!'))

  call ch_close(newhandle)
endfunc

func Test_two_channels()
  call ch_log('Test_two_channels()')
  call s:run_server('s:two_channels')
endfunc

" Test that a server crash is handled gracefully.
func s:server_crash(port)
  let handle = ch_open('localhost:' . a:port, s:chopt)
  if ch_status(handle) == "fail"
    call assert_false(1, "Can't open channel")
    return
  endif

  call ch_evalexpr(handle, '!crash!')

  sleep 10m
endfunc

func Test_server_crash()
  call ch_log('Test_server_crash()')
  call s:run_server('s:server_crash')
endfunc

"""""""""

let s:reply = ""
func s:Handler(chan, msg)
  unlet s:reply
  let s:reply = a:msg
endfunc

func s:channel_handler(port)
  let handle = ch_open('localhost:' . a:port, s:chopt)
  if ch_status(handle) == "fail"
    call assert_false(1, "Can't open channel")
    return
  endif

  " Test that it works while waiting on a numbered message.
  call assert_equal('ok', ch_evalexpr(handle, 'call me'))
  sleep 10m
  call assert_equal('we called you', s:reply)

  " Test that it works while not waiting on a numbered message.
  call ch_sendexpr(handle, 'call me again')
  sleep 10m
  call assert_equal('we did call you', s:reply)
endfunc

func Test_channel_handler()
  call ch_log('Test_channel_handler()')
  let s:chopt.callback = 's:Handler'
  call s:run_server('s:channel_handler')
  let s:chopt.callback = function('s:Handler')
  call s:run_server('s:channel_handler')
  unlet s:chopt.callback
endfunc

"""""""""

let s:ch_reply = ''
func s:ChHandler(chan, msg)
  unlet s:ch_reply
  let s:ch_reply = a:msg
endfunc

let s:zero_reply = ''
func s:OneHandler(chan, msg)
  unlet s:zero_reply
  let s:zero_reply = a:msg
endfunc

func s:channel_zero(port)
  let handle = ch_open('localhost:' . a:port, s:chopt)
  if ch_status(handle) == "fail"
    call assert_false(1, "Can't open channel")
    return
  endif

  " Check that eval works.
  call assert_equal('got it', ch_evalexpr(handle, 'hello!'))

  " Check that eval works if a zero id message is sent back.
  let s:ch_reply = ''
  call assert_equal('sent zero', ch_evalexpr(handle, 'send zero'))
  sleep 10m
  if s:has_handler
    call assert_equal('zero index', s:ch_reply)
  else
    call assert_equal('', s:ch_reply)
  endif

  " Check that handler works if a zero id message is sent back.
  let s:ch_reply = ''
  let s:zero_reply = ''
  call ch_sendexpr(handle, 'send zero', {'callback': 's:OneHandler'})
  " Somehow the second message takes a bit of time.
  for i in range(50)
    if s:zero_reply == 'sent zero'
      break
    endif
    sleep 10m
  endfor
  if s:has_handler
    call assert_equal('zero index', s:ch_reply)
  else
    call assert_equal('', s:ch_reply)
  endif
  call assert_equal('sent zero', s:zero_reply)
endfunc

func Test_zero_reply()
  call ch_log('Test_zero_reply()')
  " Run with channel handler
  let s:has_handler = 1
  let s:chopt.callback = 's:ChHandler'
  call s:run_server('s:channel_zero')
  unlet s:chopt.callback

  " Run without channel handler
  let s:has_handler = 0
  call s:run_server('s:channel_zero')
endfunc

"""""""""

let s:reply1 = ""
func s:HandleRaw1(chan, msg)
  unlet s:reply1
  let s:reply1 = a:msg
endfunc

let s:reply2 = ""
func s:HandleRaw2(chan, msg)
  unlet s:reply2
  let s:reply2 = a:msg
endfunc

let s:reply3 = ""
func s:HandleRaw3(chan, msg)
  unlet s:reply3
  let s:reply3 = a:msg
endfunc

func s:raw_one_time_callback(port)
  let handle = ch_open('localhost:' . a:port, s:chopt)
  if ch_status(handle) == "fail"
    call assert_false(1, "Can't open channel")
    return
  endif
  call ch_setoptions(handle, {'mode': 'raw'})

  " The message are sent raw, we do our own JSON strings here.
  call ch_sendraw(handle, "[1, \"hello!\"]", {'callback': 's:HandleRaw1'})
  sleep 10m
  call assert_equal("[1, \"got it\"]", s:reply1)
  call ch_sendraw(handle, "[2, \"echo something\"]", {'callback': 's:HandleRaw2'})
  call ch_sendraw(handle, "[3, \"wait a bit\"]", {'callback': 's:HandleRaw3'})
  sleep 10m
  call assert_equal("[2, \"something\"]", s:reply2)
  " wait for up to 500 msec for the 200 msec delayed reply
  for i in range(50)
    sleep 10m
    if s:reply3 != ''
      break
    endif
  endfor
  call assert_equal("[3, \"waited\"]", s:reply3)
endfunc

func Test_raw_one_time_callback()
  call ch_log('Test_raw_one_time_callback()')
  call s:run_server('s:raw_one_time_callback')
endfunc

"""""""""

" Test that trying to connect to a non-existing port fails quickly.
func Test_connect_waittime()
  call ch_log('Test_connect_waittime()')
  let start = reltime()
  let handle = ch_open('localhost:9876', s:chopt)
  if ch_status(handle) != "fail"
    " Oops, port does exists.
    call ch_close(handle)
  else
    let elapsed = reltime(start)
    call assert_true(reltimefloat(elapsed) < 1.0)
  endif

  " We intend to use a socket that doesn't exist and wait for half a second
  " before giving up.  If the socket does exist it can fail in various ways.
  " Check for "Connection reset by peer" to avoid flakyness.
  let start = reltime()
  try
    let handle = ch_open('localhost:9867', {'waittime': 500})
    if ch_status(handle) != "fail"
      " Oops, port does exists.
      call ch_close(handle)
    else
      " Failed connection should wait about 500 msec.
      let elapsed = reltime(start)
      call assert_true(reltimefloat(elapsed) > 0.3)
      call assert_true(reltimefloat(elapsed) < 1.0)
    endif
  catch
    if v:exception !~ 'Connection reset by peer'
      call assert_false(1, "Caught exception: " . v:exception)
    endif
  endtry
endfunc

"""""""""

func Test_raw_pipe()
  if !has('job')
    return
  endif
  call ch_log('Test_raw_pipe()')
  let job = job_start(s:python . " test_channel_pipe.py", {'mode': 'raw'})
  call assert_equal("run", job_status(job))
  try
    let handle = job_getchannel(job)
    call ch_sendraw(handle, "echo something\n")
    let msg = ch_readraw(handle)
    call assert_equal("something\n", substitute(msg, "\r", "", 'g'))

    call ch_sendraw(handle, "double this\n")
    let msg = ch_readraw(handle)
    call assert_equal("this\nAND this\n", substitute(msg, "\r", "", 'g'))

    let reply = ch_evalraw(handle, "quit\n", {'timeout': 100})
    call assert_equal("Goodbye!\n", substitute(reply, "\r", "", 'g'))
  finally
    call job_stop(job)
  endtry
endfunc

func Test_nl_pipe()
  if !has('job')
    return
  endif
  call ch_log('Test_nl_pipe()')
  let job = job_start(s:python . " test_channel_pipe.py")
  call assert_equal("run", job_status(job))
  try
    let handle = job_getchannel(job)
    call ch_sendraw(handle, "echo something\n")
    call assert_equal("something", ch_readraw(handle))

    call ch_sendraw(handle, "echoerr wrong\n")
    call assert_equal("wrong", ch_readraw(handle, {'part': 'err'}))

    call ch_sendraw(handle, "double this\n")
    call assert_equal("this", ch_readraw(handle))
    call assert_equal("AND this", ch_readraw(handle))

    let reply = ch_evalraw(handle, "quit\n")
    call assert_equal("Goodbye!", reply)
  finally
    call job_stop(job)
  endtry
endfunc

func Test_nl_err_to_out_pipe()
  if !has('job')
    return
  endif
  call ch_log('Test_nl_err_to_out_pipe()')
  let job = job_start(s:python . " test_channel_pipe.py", {'err-io': 'out'})
  call assert_equal("run", job_status(job))
  try
    let handle = job_getchannel(job)
    call ch_sendraw(handle, "echo something\n")
    call assert_equal("something", ch_readraw(handle))

    call ch_sendraw(handle, "echoerr wrong\n")
    call assert_equal("wrong", ch_readraw(handle))
  finally
    call job_stop(job)
  endtry
endfunc

func Test_nl_read_file()
  if !has('job')
    return
  endif
  " TODO: make this work for MS-Windows.
  if !has('unix')
    return
  endif
  call ch_log('Test_nl_read_file()')
  call writefile(['echo something', 'echoerr wrong', 'double this'], 'Xinput')
  let job = job_start(s:python . " test_channel_pipe.py",
	\ {'in-io': 'file', 'in-name': 'Xinput'})
  call assert_equal("run", job_status(job))
  try
    let handle = job_getchannel(job)
    call assert_equal("something", ch_readraw(handle))
    call assert_equal("wrong", ch_readraw(handle, {'part': 'err'}))
    call assert_equal("this", ch_readraw(handle))
    call assert_equal("AND this", ch_readraw(handle))
  finally
    call job_stop(job)
    call delete('Xinput')
  endtry
endfunc

func Test_pipe_to_buffer()
  if !has('job')
    return
  endif
  call ch_log('Test_pipe_to_buffer()')
  let job = job_start(s:python . " test_channel_pipe.py",
	\ {'out-io': 'buffer', 'out-name': 'pipe-output'})
  call assert_equal("run", job_status(job))
  try
    let handle = job_getchannel(job)
    call ch_sendraw(handle, "echo line one\n")
    call ch_sendraw(handle, "echo line two\n")
    call ch_sendraw(handle, "double this\n")
    call ch_sendraw(handle, "quit\n")
    sp pipe-output
    for i in range(100)
      sleep 10m
      if line('$') >= 6
	break
      endif
    endfor
    call assert_equal(['Reading from channel output...', 'line one', 'line two', 'this', 'AND this', 'Goodbye!'], getline(1, '$'))
    bwipe!
  finally
    call job_stop(job)
  endtry
endfunc

func Test_pipe_from_buffer()
  if !has('job')
    return
  endif
  call ch_log('Test_pipe_from_buffer()')

  sp pipe-input
  call setline(1, ['echo one', 'echo two', 'echo three'])

  let job = job_start(s:python . " test_channel_pipe.py",
	\ {'in-io': 'buffer', 'in-name': 'pipe-input'})
  call assert_equal("run", job_status(job))
  try
    let handle = job_getchannel(job)
    call assert_equal('one', ch_read(handle))
    call assert_equal('two', ch_read(handle))
    call assert_equal('three', ch_read(handle))
    bwipe!
  finally
    call job_stop(job)
  endtry
endfunc

func Test_pipe_to_nameless_buffer()
  if !has('job')
    return
  endif
  call ch_log('Test_pipe_to_nameless_buffer()')
  let job = job_start(s:python . " test_channel_pipe.py",
	\ {'out-io': 'buffer'})
  call assert_equal("run", job_status(job))
  try
    let handle = job_getchannel(job)
    call ch_sendraw(handle, "echo line one\n")
    call ch_sendraw(handle, "echo line two\n")
    exe ch_getbufnr(handle, "out") . 'sbuf'
    for i in range(100)
      sleep 10m
      if line('$') >= 3
	break
      endif
    endfor
    call assert_equal(['Reading from channel output...', 'line one', 'line two'], getline(1, '$'))
    bwipe!
  finally
    call job_stop(job)
  endtry
endfunc

func Test_pipe_to_buffer_json()
  if !has('job')
    return
  endif
  call ch_log('Test_pipe_to_buffer_json()')
  let job = job_start(s:python . " test_channel_pipe.py",
	\ {'out-io': 'buffer', 'out-mode': 'json'})
  call assert_equal("run", job_status(job))
  try
    let handle = job_getchannel(job)
    call ch_sendraw(handle, "echo [0, \"hello\"]\n")
    call ch_sendraw(handle, "echo [-2, 12.34]\n")
    exe ch_getbufnr(handle, "out") . 'sbuf'
    for i in range(100)
      sleep 10m
      if line('$') >= 3
	break
      endif
    endfor
    call assert_equal(['Reading from channel output...', '[0,"hello"]', '[-2,12.34]'], getline(1, '$'))
    bwipe!
  finally
    call job_stop(job)
  endtry
endfunc

" Wait a little while for the last line, minus "offset", to equal "line".
func Wait_for_last_line(line, offset)
  for i in range(100)
    sleep 10m
    if getline(line('$') - a:offset) == a:line
      break
    endif
  endfor
endfunc

func Test_pipe_io_two_buffers()
  if !has('job')
    return
  endif
  call ch_log('Test_pipe_io_two_buffers()')

  " Create two buffers, one to read from and one to write to.
  split pipe-output
  set buftype=nofile
  split pipe-input
  set buftype=nofile

  let job = job_start(s:python . " test_channel_pipe.py",
	\ {'in-io': 'buffer', 'in-name': 'pipe-input', 'in-top': 0,
	\  'out-io': 'buffer', 'out-name': 'pipe-output'})
  call assert_equal("run", job_status(job))
  try
    exe "normal Gaecho hello\<CR>"
    exe bufwinnr('pipe-output') . "wincmd w"
    call Wait_for_last_line('hello', 0)
    call assert_equal('hello', getline('$'))

    exe bufwinnr('pipe-input') . "wincmd w"
    exe "normal Gadouble this\<CR>"
    exe bufwinnr('pipe-output') . "wincmd w"
    call Wait_for_last_line('AND this', 0)
    call assert_equal('this', getline(line('$') - 1))
    call assert_equal('AND this', getline('$'))

    bwipe!
    exe bufwinnr('pipe-input') . "wincmd w"
    bwipe!
  finally
    call job_stop(job)
  endtry
endfunc

func Test_pipe_io_one_buffer()
  if !has('job')
    return
  endif
  call ch_log('Test_pipe_io_one_buffer()')

  " Create one buffer to read from and to write to.
  split pipe-io
  set buftype=nofile

  let job = job_start(s:python . " test_channel_pipe.py",
	\ {'in-io': 'buffer', 'in-name': 'pipe-io', 'in-top': 0,
	\  'out-io': 'buffer', 'out-name': 'pipe-io'})
  call assert_equal("run", job_status(job))
  try
    exe "normal Goecho hello\<CR>"
    call Wait_for_last_line('hello', 1)
    call assert_equal('hello', getline(line('$') - 1))

    exe "normal Gadouble this\<CR>"
    call Wait_for_last_line('AND this', 1)
    call assert_equal('this', getline(line('$') - 2))
    call assert_equal('AND this', getline(line('$') - 1))

    bwipe!
  finally
    call job_stop(job)
  endtry
endfunc

""""""""""

let s:unletResponse = ''
func s:UnletHandler(handle, msg)
  let s:unletResponse = a:msg
  unlet s:channelfd
endfunc

" Test that "unlet handle" in a handler doesn't crash Vim.
func s:unlet_handle(port)
  let s:channelfd = ch_open('localhost:' . a:port, s:chopt)
  call ch_sendexpr(s:channelfd, "test", {'callback': function('s:UnletHandler')})
  sleep 10m
  call assert_equal('what?', s:unletResponse)
endfunc

func Test_unlet_handle()
  call ch_log('Test_unlet_handle()')
  call s:run_server('s:unlet_handle')
endfunc

""""""""""

let s:unletResponse = ''
func s:CloseHandler(handle, msg)
  let s:unletResponse = a:msg
  call ch_close(s:channelfd)
endfunc

" Test that "unlet handle" in a handler doesn't crash Vim.
func s:close_handle(port)
  let s:channelfd = ch_open('localhost:' . a:port, s:chopt)
  call ch_sendexpr(s:channelfd, "test", {'callback': function('s:CloseHandler')})
  sleep 10m
  call assert_equal('what?', s:unletResponse)
endfunc

func Test_close_handle()
  call ch_log('Test_close_handle()')
  call s:run_server('s:close_handle')
endfunc

""""""""""

func Test_open_fail()
  call ch_log('Test_open_fail()')
  silent! let ch = ch_open("noserver")
  echo ch
  let d = ch
endfunc

""""""""""

func s:open_delay(port)
  " Wait up to a second for the port to open.
  let s:chopt.waittime = 1000
  let channel = ch_open('localhost:' . a:port, s:chopt)
  unlet s:chopt.waittime
  if ch_status(channel) == "fail"
    call assert_false(1, "Can't open channel")
    return
  endif
  call assert_equal('got it', ch_evalexpr(channel, 'hello!'))
  call ch_close(channel)
endfunc

func Test_open_delay()
  call ch_log('Test_open_delay()')
  " The server will wait half a second before creating the port.
  call s:run_server('s:open_delay', 'delay')
endfunc

"""""""""

function MyFunction(a,b,c)
  let s:call_ret = [a:a, a:b, a:c]
endfunc

function s:test_call(port)
  let handle = ch_open('localhost:' . a:port, s:chopt)
  if ch_status(handle) == "fail"
    call assert_false(1, "Can't open channel")
    return
  endif

  call assert_equal('ok', ch_evalexpr(handle, 'call-func'))
  sleep 20m
  call assert_equal([1, 2, 3], s:call_ret)
endfunc

func Test_call()
  call ch_log('Test_call()')
  call s:run_server('s:test_call')
endfunc

"""""""""

let s:job_exit_ret = 'not yet'
function MyExitCb(job, status)
  let s:job_exit_ret = 'done'
endfunc

function s:test_exit_callback(port)
  call job_setoptions(s:job, {'exit-cb': 'MyExitCb'})
  let s:exit_job = s:job
endfunc

func Test_exit_callback()
  if has('job')
    call ch_log('Test_exit_callback()')
    call s:run_server('s:test_exit_callback')

    " wait up to a second for the job to exit
    for i in range(100)
      if s:job_exit_ret == 'done'
	break
      endif
      sleep 10m
      " calling job_status() triggers the callback
      call job_status(s:exit_job)
    endfor

    call assert_equal('done', s:job_exit_ret)
    unlet s:exit_job
  endif
endfunc

"""""""""

let s:ch_close_ret = 'alive'
function MyCloseCb(ch)
  let s:ch_close_ret = 'closed'
endfunc

function s:test_close_callback(port)
  let handle = ch_open('localhost:' . a:port, s:chopt)
  if ch_status(handle) == "fail"
    call assert_false(1, "Can't open channel")
    return
  endif
  call ch_setoptions(handle, {'close-cb': 'MyCloseCb'})

  call assert_equal('', ch_evalexpr(handle, 'close me'))
  sleep 20m
  call assert_equal('closed', s:ch_close_ret)
endfunc

func Test_close_callback()
  call ch_log('Test_close_callback()')
  call s:run_server('s:test_close_callback')
endfunc

" Uncomment this to see what happens, output is in src/testdir/channellog.
" call ch_logfile('channellog', 'w')
