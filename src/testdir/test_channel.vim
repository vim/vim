" Test for channel functions.

if !has('channel')
  finish
endif

source shared.vim

let s:python = PythonProg()
if s:python == ''
  " Can't run this test.
  finish
endif

let s:chopt = {}

" Run "testfunc" after sarting the server and stop the server afterwards.
func s:run_server(testfunc, ...)
  call RunServer('test_channel.py', a:testfunc, a:000)
endfunc

let g:Ch_responseMsg = ''
func Ch_requestHandler(handle, msg)
  let g:Ch_responseHandle = a:handle
  let g:Ch_responseMsg = a:msg
endfunc

func Ch_communicate(port)
  let handle = ch_open('localhost:' . a:port, s:chopt)
  if ch_status(handle) == "fail"
    call assert_false(1, "Can't open channel")
    return
  endif
  if has('job')
    " check that getjob without a job is handled correctly
    call assert_equal('no process', string(ch_getjob(handle)))
  endif
  let dict = ch_info(handle)
  call assert_true(dict.id != 0)
  call assert_equal('open', dict.status)
  call assert_equal(a:port, string(dict.port))
  call assert_equal('open', dict.sock_status)
  call assert_equal('socket', dict.sock_io)

  " Simple string request and reply.
  call assert_equal('got it', ch_evalexpr(handle, 'hello!'))

  " Malformed command should be ignored.
  call assert_equal('ok', ch_evalexpr(handle, 'malformed1'))
  call assert_equal('ok', ch_evalexpr(handle, 'malformed2'))
  call assert_equal('ok', ch_evalexpr(handle, 'malformed3'))

  " split command should work
  call assert_equal('ok', ch_evalexpr(handle, 'split'))
  call WaitFor('exists("g:split")')
  call assert_equal(123, g:split)

  " string with ][ should work
  call assert_equal('this][that', ch_evalexpr(handle, 'echo this][that'))

  " sending three messages quickly then reading should work
  for i in range(3)
    call ch_sendexpr(handle, 'echo hello ' . i)
  endfor
  call assert_equal('hello 0', ch_read(handle)[1])
  call assert_equal('hello 1', ch_read(handle)[1])
  call assert_equal('hello 2', ch_read(handle)[1])

  " Request that triggers sending two ex commands.  These will usually be
  " handled before getting the response, but it's not guaranteed, thus wait a
  " tiny bit for the commands to get executed.
  call assert_equal('ok', ch_evalexpr(handle, 'make change'))
  call WaitFor('"added2" == getline("$")')
  call assert_equal('added1', getline(line('$') - 1))
  call assert_equal('added2', getline('$'))

  " Request command "foo bar", which fails silently.
  call assert_equal('ok', ch_evalexpr(handle, 'bad command'))
  call WaitFor('v:errmsg =~ "E492"')
  call assert_match('E492:.*foo bar', v:errmsg)

  call assert_equal('ok', ch_evalexpr(handle, 'do normal', {'timeout': 100}))
  call WaitFor('"added more" == getline("$")')
  call assert_equal('added more', getline('$'))

  " Send a request with a specific handler.
  call ch_sendexpr(handle, 'hello!', {'callback': 'Ch_requestHandler'})
  call WaitFor('exists("g:Ch_responseHandle")')
  if !exists('g:Ch_responseHandle')
    call assert_false(1, 'g:Ch_responseHandle was not set')
  else
    call assert_equal(handle, g:Ch_responseHandle)
    unlet g:Ch_responseHandle
  endif
  call assert_equal('got it', g:Ch_responseMsg)

  let g:Ch_responseMsg = ''
  call ch_sendexpr(handle, 'hello!', {'callback': function('Ch_requestHandler')})
  call WaitFor('exists("g:Ch_responseHandle")')
  if !exists('g:Ch_responseHandle')
    call assert_false(1, 'g:Ch_responseHandle was not set')
  else
    call assert_equal(handle, g:Ch_responseHandle)
    unlet g:Ch_responseHandle
  endif
  call assert_equal('got it', g:Ch_responseMsg)

  " Using lambda.
  let g:Ch_responseMsg = ''
  call ch_sendexpr(handle, 'hello!', {'callback': {a, b -> Ch_requestHandler(a, b)}})
  call WaitFor('exists("g:Ch_responseHandle")')
  if !exists('g:Ch_responseHandle')
    call assert_false(1, 'g:Ch_responseHandle was not set')
  else
    call assert_equal(handle, g:Ch_responseHandle)
    unlet g:Ch_responseHandle
  endif
  call assert_equal('got it', g:Ch_responseMsg)

  " Collect garbage, tests that our handle isn't collected.
  call test_garbagecollect_now()

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

  " Send an eval request with special characters.
  call assert_equal('ok', ch_evalexpr(handle, 'eval-special'))
  sleep 10m
  call assert_equal([-2, "foo\x7f\x10\x01bar"], ch_evalexpr(handle, 'eval-result'))

  " Send an eval request to get a line with special characters.
  call setline(3, "a\nb\<CR>c\x01d\x7fe")
  call assert_equal('ok', ch_evalexpr(handle, 'eval-getline'))
  sleep 10m
  call assert_equal([-3, "a\nb\<CR>c\x01d\x7fe"], ch_evalexpr(handle, 'eval-result'))

  " Send an eval request that fails.
  call assert_equal('ok', ch_evalexpr(handle, 'eval-fails'))
  sleep 10m
  call assert_equal([-4, 'ERROR'], ch_evalexpr(handle, 'eval-result'))

  " Send an eval request that works but can't be encoded.
  call assert_equal('ok', ch_evalexpr(handle, 'eval-error'))
  sleep 10m
  call assert_equal([-5, 'ERROR'], ch_evalexpr(handle, 'eval-result'))

  " Send a bad eval request. There will be no response.
  call assert_equal('ok', ch_evalexpr(handle, 'eval-bad'))
  sleep 10m
  call assert_equal([-5, 'ERROR'], ch_evalexpr(handle, 'eval-result'))

  " Send an expr request
  call assert_equal('ok', ch_evalexpr(handle, 'an expr'))
  call WaitFor('"three" == getline("$")')
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
  call s:run_server('Ch_communicate')
endfunc

" Test that we can open two channels.
func Ch_two_channels(port)
  let handle = ch_open('localhost:' . a:port, s:chopt)
  call assert_equal(v:t_channel, type(handle))
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
  call s:run_server('Ch_two_channels')
endfunc

" Test that a server crash is handled gracefully.
func Ch_server_crash(port)
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
  call s:run_server('Ch_server_crash')
endfunc

"""""""""

func Ch_handler(chan, msg)
  unlet g:Ch_reply
  let g:Ch_reply = a:msg
endfunc

func Ch_channel_handler(port)
  let handle = ch_open('localhost:' . a:port, s:chopt)
  if ch_status(handle) == "fail"
    call assert_false(1, "Can't open channel")
    return
  endif

  " Test that it works while waiting on a numbered message.
  call assert_equal('ok', ch_evalexpr(handle, 'call me'))
  call WaitFor('"we called you" == g:Ch_reply')
  call assert_equal('we called you', g:Ch_reply)

  " Test that it works while not waiting on a numbered message.
  call ch_sendexpr(handle, 'call me again')
  call WaitFor('"we did call you" == g:Ch_reply')
  call assert_equal('we did call you', g:Ch_reply)
endfunc

func Test_channel_handler()
  call ch_log('Test_channel_handler()')
  let g:Ch_reply = ""
  let s:chopt.callback = 'Ch_handler'
  call s:run_server('Ch_channel_handler')
  let g:Ch_reply = ""
  let s:chopt.callback = function('Ch_handler')
  call s:run_server('Ch_channel_handler')
  unlet s:chopt.callback
endfunc

"""""""""

let g:Ch_reply = ''
func Ch_zeroHandler(chan, msg)
  unlet g:Ch_reply
  let g:Ch_reply = a:msg
endfunc

let g:Ch_zero_reply = ''
func Ch_oneHandler(chan, msg)
  unlet g:Ch_zero_reply
  let g:Ch_zero_reply = a:msg
endfunc

func Ch_channel_zero(port)
  let handle = ch_open('localhost:' . a:port, s:chopt)
  if ch_status(handle) == "fail"
    call assert_false(1, "Can't open channel")
    return
  endif

  " Check that eval works.
  call assert_equal('got it', ch_evalexpr(handle, 'hello!'))

  " Check that eval works if a zero id message is sent back.
  let g:Ch_reply = ''
  call assert_equal('sent zero', ch_evalexpr(handle, 'send zero'))
  if s:has_handler
    call WaitFor('"zero index" == g:Ch_reply')
    call assert_equal('zero index', g:Ch_reply)
  else
    sleep 20m
    call assert_equal('', g:Ch_reply)
  endif

  " Check that handler works if a zero id message is sent back.
  let g:Ch_reply = ''
  let g:Ch_zero_reply = ''
  call ch_sendexpr(handle, 'send zero', {'callback': 'Ch_oneHandler'})
  call WaitFor('"sent zero" == g:Ch_zero_reply')
  if s:has_handler
    call assert_equal('zero index', g:Ch_reply)
  else
    call assert_equal('', g:Ch_reply)
  endif
  call assert_equal('sent zero', g:Ch_zero_reply)
endfunc

func Test_zero_reply()
  call ch_log('Test_zero_reply()')
  " Run with channel handler
  let s:has_handler = 1
  let s:chopt.callback = 'Ch_zeroHandler'
  call s:run_server('Ch_channel_zero')
  unlet s:chopt.callback

  " Run without channel handler
  let s:has_handler = 0
  call s:run_server('Ch_channel_zero')
endfunc

"""""""""

let g:Ch_reply1 = ""
func Ch_handleRaw1(chan, msg)
  unlet g:Ch_reply1
  let g:Ch_reply1 = a:msg
endfunc

let g:Ch_reply2 = ""
func Ch_handleRaw2(chan, msg)
  unlet g:Ch_reply2
  let g:Ch_reply2 = a:msg
endfunc

let g:Ch_reply3 = ""
func Ch_handleRaw3(chan, msg)
  unlet g:Ch_reply3
  let g:Ch_reply3 = a:msg
endfunc

func Ch_raw_one_time_callback(port)
  let handle = ch_open('localhost:' . a:port, s:chopt)
  if ch_status(handle) == "fail"
    call assert_false(1, "Can't open channel")
    return
  endif
  call ch_setoptions(handle, {'mode': 'raw'})

  " The message are sent raw, we do our own JSON strings here.
  call ch_sendraw(handle, "[1, \"hello!\"]\n", {'callback': 'Ch_handleRaw1'})
  call WaitFor('g:Ch_reply1 != ""')
  call assert_equal("[1, \"got it\"]", g:Ch_reply1)
  call ch_sendraw(handle, "[2, \"echo something\"]\n", {'callback': 'Ch_handleRaw2'})
  call ch_sendraw(handle, "[3, \"wait a bit\"]\n", {'callback': 'Ch_handleRaw3'})
  call WaitFor('g:Ch_reply2 != ""')
  call assert_equal("[2, \"something\"]", g:Ch_reply2)
  " wait for the 200 msec delayed reply
  call WaitFor('g:Ch_reply3 != ""')
  call assert_equal("[3, \"waited\"]", g:Ch_reply3)
endfunc

func Test_raw_one_time_callback()
  call ch_log('Test_raw_one_time_callback()')
  call s:run_server('Ch_raw_one_time_callback')
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
      " Failed connection should wait about 500 msec.  Can be longer if the
      " computer is busy with other things.
      let elapsed = reltime(start)
      call assert_true(reltimefloat(elapsed) > 0.3)
      call assert_true(reltimefloat(elapsed) < 1.5)
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
  call assert_equal(v:t_job, type(job))
  call assert_equal("run", job_status(job))
  try
    " For a change use the job where a channel is expected.
    call ch_sendraw(job, "echo something\n")
    let msg = ch_readraw(job)
    call assert_equal("something\n", substitute(msg, "\r", "", 'g'))

    call ch_sendraw(job, "double this\n")
    let msg = ch_readraw(job)
    call assert_equal("this\nAND this\n", substitute(msg, "\r", "", 'g'))

    let g:Ch_reply = ""
    call ch_sendraw(job, "double this\n", {'callback': 'Ch_handler'})
    call WaitFor('"" != g:Ch_reply')
    call assert_equal("this\nAND this\n", substitute(g:Ch_reply, "\r", "", 'g'))

    let reply = ch_evalraw(job, "quit\n", {'timeout': 100})
    call assert_equal("Goodbye!\n", substitute(reply, "\r", "", 'g'))
  finally
    call job_stop(job)
  endtry

  let g:Ch_job = job
  call WaitFor('"dead" == job_status(g:Ch_job)')
  let info = job_info(job)
  call assert_equal("dead", info.status)
  call assert_equal("term", info.stoponexit)
endfunc

func Test_nl_pipe()
  if !has('job')
    return
  endif
  call ch_log('Test_nl_pipe()')
  let job = job_start([s:python, "test_channel_pipe.py"])
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

    call ch_sendraw(handle, "split this line\n")
    call assert_equal("this linethis linethis line", ch_readraw(handle))

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
  call ch_logfile('Xlog')
  call ch_log('Test_nl_err_to_out_pipe()')
  let job = job_start(s:python . " test_channel_pipe.py", {'err_io': 'out'})
  call assert_equal("run", job_status(job))
  try
    let handle = job_getchannel(job)
    call ch_sendraw(handle, "echo something\n")
    call assert_equal("something", ch_readraw(handle))

    call ch_sendraw(handle, "echoerr wrong\n")
    call assert_equal("wrong", ch_readraw(handle))
  finally
    call job_stop(job)
    call ch_logfile('')
    let loglines = readfile('Xlog')
    call assert_true(len(loglines) > 10)
    let found_test = 0
    let found_send = 0
    let found_recv = 0
    let found_stop = 0
    for l in loglines
      if l =~ 'Test_nl_err_to_out_pipe'
	let found_test = 1
      endif
      if l =~ 'SEND on.*echo something'
	let found_send = 1
      endif
      if l =~ 'RECV on.*something'
	let found_recv = 1
      endif
      if l =~ 'Stopping job with'
	let found_stop = 1
      endif
    endfor
    call assert_equal(1, found_test)
    call assert_equal(1, found_send)
    call assert_equal(1, found_recv)
    call assert_equal(1, found_stop)
    " On MS-Windows need to sleep for a moment to be able to delete the file.
    sleep 10m
    call delete('Xlog')
  endtry
endfunc

func Stop_g_job()
  call job_stop(g:job)
  if has('win32')
    " On MS-Windows the server must close the file handle before we are able
    " to delete the file.
    call WaitFor('job_status(g:job) == "dead"')
    sleep 10m
  endif
endfunc

func Test_nl_read_file()
  if !has('job')
    return
  endif
  call ch_log('Test_nl_read_file()')
  call writefile(['echo something', 'echoerr wrong', 'double this'], 'Xinput')
  let g:job = job_start(s:python . " test_channel_pipe.py",
	\ {'in_io': 'file', 'in_name': 'Xinput'})
  call assert_equal("run", job_status(g:job))
  try
    let handle = job_getchannel(g:job)
    call assert_equal("something", ch_readraw(handle))
    call assert_equal("wrong", ch_readraw(handle, {'part': 'err'}))
    call assert_equal("this", ch_readraw(handle))
    call assert_equal("AND this", ch_readraw(handle))
  finally
    call Stop_g_job()
    call delete('Xinput')
  endtry
endfunc

func Test_nl_write_out_file()
  if !has('job')
    return
  endif
  call ch_log('Test_nl_write_out_file()')
  let g:job = job_start(s:python . " test_channel_pipe.py",
	\ {'out_io': 'file', 'out_name': 'Xoutput'})
  call assert_equal("run", job_status(g:job))
  try
    let handle = job_getchannel(g:job)
    call ch_sendraw(handle, "echo line one\n")
    call ch_sendraw(handle, "echo line two\n")
    call ch_sendraw(handle, "double this\n")
    call WaitFor('len(readfile("Xoutput")) > 2')
    call assert_equal(['line one', 'line two', 'this', 'AND this'], readfile('Xoutput'))
  finally
    call Stop_g_job()
    call delete('Xoutput')
  endtry
endfunc

func Test_nl_write_err_file()
  if !has('job')
    return
  endif
  call ch_log('Test_nl_write_err_file()')
  let g:job = job_start(s:python . " test_channel_pipe.py",
	\ {'err_io': 'file', 'err_name': 'Xoutput'})
  call assert_equal("run", job_status(g:job))
  try
    let handle = job_getchannel(g:job)
    call ch_sendraw(handle, "echoerr line one\n")
    call ch_sendraw(handle, "echoerr line two\n")
    call ch_sendraw(handle, "doubleerr this\n")
    call WaitFor('len(readfile("Xoutput")) > 2')
    call assert_equal(['line one', 'line two', 'this', 'AND this'], readfile('Xoutput'))
  finally
    call Stop_g_job()
    call delete('Xoutput')
  endtry
endfunc

func Test_nl_write_both_file()
  if !has('job')
    return
  endif
  call ch_log('Test_nl_write_both_file()')
  let g:job = job_start(s:python . " test_channel_pipe.py",
	\ {'out_io': 'file', 'out_name': 'Xoutput', 'err_io': 'out'})
  call assert_equal("run", job_status(g:job))
  try
    let handle = job_getchannel(g:job)
    call ch_sendraw(handle, "echoerr line one\n")
    call ch_sendraw(handle, "echo line two\n")
    call ch_sendraw(handle, "double this\n")
    call ch_sendraw(handle, "doubleerr that\n")
    call WaitFor('len(readfile("Xoutput")) > 5')
    call assert_equal(['line one', 'line two', 'this', 'AND this', 'that', 'AND that'], readfile('Xoutput'))
  finally
    call Stop_g_job()
    call delete('Xoutput')
  endtry
endfunc

func BufCloseCb(ch)
  let g:Ch_bufClosed = 'yes'
endfunc

func Run_test_pipe_to_buffer(use_name, nomod, do_msg)
  if !has('job')
    return
  endif
  call ch_log('Test_pipe_to_buffer()')
  let g:Ch_bufClosed = 'no'
  let options = {'out_io': 'buffer', 'close_cb': 'BufCloseCb'}
  let expected = ['', 'line one', 'line two', 'this', 'AND this', 'Goodbye!']
  if a:use_name
    let options['out_name'] = 'pipe-output'
    if a:do_msg
      let expected[0] = 'Reading from channel output...'
    else
      let options['out_msg'] = 0
      call remove(expected, 0)
    endif
  else
    sp pipe-output
    let options['out_buf'] = bufnr('%')
    quit
    call remove(expected, 0)
  endif
  if a:nomod
    let options['out_modifiable'] = 0
  endif
  let job = job_start(s:python . " test_channel_pipe.py", options)
  call assert_equal("run", job_status(job))
  try
    let handle = job_getchannel(job)
    call ch_sendraw(handle, "echo line one\n")
    call ch_sendraw(handle, "echo line two\n")
    call ch_sendraw(handle, "double this\n")
    call ch_sendraw(handle, "quit\n")
    sp pipe-output
    call WaitFor('line("$") >= 6 && g:Ch_bufClosed == "yes"')
    call assert_equal(expected, getline(1, '$'))
    if a:nomod
      call assert_equal(0, &modifiable)
    else
      call assert_equal(1, &modifiable)
    endif
    call assert_equal('yes', g:Ch_bufClosed)
    bwipe!
  finally
    call job_stop(job)
  endtry
endfunc

func Test_pipe_to_buffer_name()
  call Run_test_pipe_to_buffer(1, 0, 1)
endfunc

func Test_pipe_to_buffer_nr()
  call Run_test_pipe_to_buffer(0, 0, 1)
endfunc

func Test_pipe_to_buffer_name_nomod()
  call Run_test_pipe_to_buffer(1, 1, 1)
endfunc

func Test_pipe_to_buffer_name_nomsg()
  call Run_test_pipe_to_buffer(1, 0, 1)
endfunc

func Run_test_pipe_err_to_buffer(use_name, nomod, do_msg)
  if !has('job')
    return
  endif
  call ch_log('Test_pipe_err_to_buffer()')
  let options = {'err_io': 'buffer'}
  let expected = ['', 'line one', 'line two', 'this', 'AND this']
  if a:use_name
    let options['err_name'] = 'pipe-err'
    if a:do_msg
      let expected[0] = 'Reading from channel error...'
    else
      let options['err_msg'] = 0
      call remove(expected, 0)
    endif
  else
    sp pipe-err
    let options['err_buf'] = bufnr('%')
    quit
    call remove(expected, 0)
  endif
  if a:nomod
    let options['err_modifiable'] = 0
  endif
  let job = job_start(s:python . " test_channel_pipe.py", options)
  call assert_equal("run", job_status(job))
  try
    let handle = job_getchannel(job)
    call ch_sendraw(handle, "echoerr line one\n")
    call ch_sendraw(handle, "echoerr line two\n")
    call ch_sendraw(handle, "doubleerr this\n")
    call ch_sendraw(handle, "quit\n")
    sp pipe-err
    call WaitFor('line("$") >= 5')
    call assert_equal(expected, getline(1, '$'))
    if a:nomod
      call assert_equal(0, &modifiable)
    else
      call assert_equal(1, &modifiable)
    endif
    bwipe!
  finally
    call job_stop(job)
  endtry
endfunc

func Test_pipe_err_to_buffer_name()
  call Run_test_pipe_err_to_buffer(1, 0, 1)
endfunc
  
func Test_pipe_err_to_buffer_nr()
  call Run_test_pipe_err_to_buffer(0, 0, 1)
endfunc
  
func Test_pipe_err_to_buffer_name_nomod()
  call Run_test_pipe_err_to_buffer(1, 1, 1)
endfunc
  
func Test_pipe_err_to_buffer_name_nomsg()
  call Run_test_pipe_err_to_buffer(1, 0, 0)
endfunc
  
func Test_pipe_both_to_buffer()
  if !has('job')
    return
  endif
  call ch_log('Test_pipe_both_to_buffer()')
  let job = job_start(s:python . " test_channel_pipe.py",
	\ {'out_io': 'buffer', 'out_name': 'pipe-err', 'err_io': 'out'})
  call assert_equal("run", job_status(job))
  try
    let handle = job_getchannel(job)
    call ch_sendraw(handle, "echo line one\n")
    call ch_sendraw(handle, "echoerr line two\n")
    call ch_sendraw(handle, "double this\n")
    call ch_sendraw(handle, "doubleerr that\n")
    call ch_sendraw(handle, "quit\n")
    sp pipe-err
    call WaitFor('line("$") >= 7')
    call assert_equal(['Reading from channel output...', 'line one', 'line two', 'this', 'AND this', 'that', 'AND that', 'Goodbye!'], getline(1, '$'))
    bwipe!
  finally
    call job_stop(job)
  endtry
endfunc

func Run_test_pipe_from_buffer(use_name)
  if !has('job')
    return
  endif
  call ch_log('Test_pipe_from_buffer()')

  sp pipe-input
  call setline(1, ['echo one', 'echo two', 'echo three'])
  let options = {'in_io': 'buffer', 'block_write': 1}
  if a:use_name
    let options['in_name'] = 'pipe-input'
  else
    let options['in_buf'] = bufnr('%')
  endif

  let job = job_start(s:python . " test_channel_pipe.py", options)
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

func Test_pipe_from_buffer_name()
  call Run_test_pipe_from_buffer(1)
endfunc

func Test_pipe_from_buffer_nr()
  call Run_test_pipe_from_buffer(0)
endfunc

func Run_pipe_through_sort(all, use_buffer)
  if !executable('sort') || !has('job')
    return
  endif
  let options = {'out_io': 'buffer', 'out_name': 'sortout'}
  if a:use_buffer
    split sortin
    call setline(1, ['ccc', 'aaa', 'ddd', 'bbb', 'eee'])
    let options.in_io = 'buffer'
    let options.in_name = 'sortin'
  endif
  if !a:all
    let options.in_top = 2
    let options.in_bot = 4
  endif
  let g:job = job_start('sort', options)
  call assert_equal("run", job_status(g:job))

  if !a:use_buffer
    call ch_sendraw(g:job, "ccc\naaa\nddd\nbbb\neee\n")
    call ch_close_in(g:job)
  endif

  call WaitFor('job_status(g:job) == "dead"')
  call assert_equal("dead", job_status(g:job))

  sp sortout
  call WaitFor('line("$") > 3')
  call assert_equal('Reading from channel output...', getline(1))
  if a:all
    call assert_equal(['aaa', 'bbb', 'ccc', 'ddd', 'eee'], getline(2, 6))
  else
    call assert_equal(['aaa', 'bbb', 'ddd'], getline(2, 4))
  endif

  call job_stop(g:job)
  unlet g:job
  if a:use_buffer
    bwipe! sortin
  endif
  bwipe! sortout
endfunc

func Test_pipe_through_sort_all()
  call ch_log('Test_pipe_through_sort_all()')
  call Run_pipe_through_sort(1, 1)
endfunc

func Test_pipe_through_sort_some()
  call ch_log('Test_pipe_through_sort_some()')
  call Run_pipe_through_sort(0, 1)
endfunc

func Test_pipe_through_sort_feed()
  call ch_log('Test_pipe_through_sort_feed()')
  call Run_pipe_through_sort(1, 0)
endfunc

func Test_pipe_to_nameless_buffer()
  if !has('job')
    return
  endif
  call ch_log('Test_pipe_to_nameless_buffer()')
  let job = job_start(s:python . " test_channel_pipe.py",
	\ {'out_io': 'buffer'})
  call assert_equal("run", job_status(job))
  try
    let handle = job_getchannel(job)
    call ch_sendraw(handle, "echo line one\n")
    call ch_sendraw(handle, "echo line two\n")
    exe ch_getbufnr(handle, "out") . 'sbuf'
    call WaitFor('line("$") >= 3')
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
	\ {'out_io': 'buffer', 'out_mode': 'json'})
  call assert_equal("run", job_status(job))
  try
    let handle = job_getchannel(job)
    call ch_sendraw(handle, "echo [0, \"hello\"]\n")
    call ch_sendraw(handle, "echo [-2, 12.34]\n")
    exe ch_getbufnr(handle, "out") . 'sbuf'
    call WaitFor('line("$") >= 3')
    call assert_equal(['Reading from channel output...', '[0,"hello"]', '[-2,12.34]'], getline(1, '$'))
    bwipe!
  finally
    call job_stop(job)
  endtry
endfunc

" Wait a little while for the last line, minus "offset", to equal "line".
func s:wait_for_last_line(line, offset)
  for i in range(100)
    if getline(line('$') - a:offset) == a:line
      break
    endif
    sleep 10m
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
	\ {'in_io': 'buffer', 'in_name': 'pipe-input', 'in_top': 0,
	\  'out_io': 'buffer', 'out_name': 'pipe-output',
	\  'block_write': 1})
  call assert_equal("run", job_status(job))
  try
    exe "normal Gaecho hello\<CR>"
    exe bufwinnr('pipe-output') . "wincmd w"
    call s:wait_for_last_line('hello', 0)
    call assert_equal('hello', getline('$'))

    exe bufwinnr('pipe-input') . "wincmd w"
    exe "normal Gadouble this\<CR>"
    exe bufwinnr('pipe-output') . "wincmd w"
    call s:wait_for_last_line('AND this', 0)
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
	\ {'in_io': 'buffer', 'in_name': 'pipe-io', 'in_top': 0,
	\  'out_io': 'buffer', 'out_name': 'pipe-io',
	\  'block_write': 1})
  call assert_equal("run", job_status(job))
  try
    exe "normal Goecho hello\<CR>"
    call s:wait_for_last_line('hello', 1)
    call assert_equal('hello', getline(line('$') - 1))

    exe "normal Gadouble this\<CR>"
    call s:wait_for_last_line('AND this', 1)
    call assert_equal('this', getline(line('$') - 2))
    call assert_equal('AND this', getline(line('$') - 1))

    bwipe!
  finally
    call job_stop(job)
  endtry
endfunc

func Test_pipe_null()
  if !has('job')
    return
  endif
  call ch_log('Test_pipe_null()')

  " We cannot check that no I/O works, we only check that the job starts
  " properly.
  let job = job_start(s:python . " test_channel_pipe.py something",
	\ {'in_io': 'null'})
  call assert_equal("run", job_status(job))
  try
    call assert_equal('something', ch_read(job))
  finally
    call job_stop(job)
  endtry

  let job = job_start(s:python . " test_channel_pipe.py err-out",
	\ {'out_io': 'null'})
  call assert_equal("run", job_status(job))
  try
    call assert_equal('err-out', ch_read(job, {"part": "err"}))
  finally
    call job_stop(job)
  endtry

  let job = job_start(s:python . " test_channel_pipe.py something",
	\ {'err_io': 'null'})
  call assert_equal("run", job_status(job))
  try
    call assert_equal('something', ch_read(job))
  finally
    call job_stop(job)
  endtry

  let job = job_start(s:python . " test_channel_pipe.py something",
	\ {'out_io': 'null', 'err_io': 'out'})
  call assert_equal("run", job_status(job))
  call job_stop(job)

  let job = job_start(s:python . " test_channel_pipe.py something",
	\ {'in_io': 'null', 'out_io': 'null', 'err_io': 'null'})
  call assert_equal("run", job_status(job))
  call assert_equal('channel fail', string(job_getchannel(job)))
  call assert_equal('fail', ch_status(job))
  call job_stop(job)
endfunc

func Test_pipe_to_buffer_raw()
  if !has('job')
    return
  endif
  call ch_log('Test_raw_pipe_to_buffer()')
  let options = {'out_mode': 'raw', 'out_io': 'buffer', 'out_name': 'testout'}
  split testout
  let job = job_start([s:python, '-c', 
        \ 'import sys; [sys.stdout.write(".") and sys.stdout.flush() for _ in range(10000)]'], options)
  call assert_equal("run", job_status(job))
  call WaitFor('len(join(getline(2,line("$")),"") >= 10000')
  try
    for line in getline(2, '$')
      let line = substitute(line, '^\.*', '', '')
      call assert_equal('', line)
    endfor
  finally
    call job_stop(job)
    bwipe!
  endtry
endfunc

func Test_reuse_channel()
  if !has('job')
    return
  endif
  call ch_log('Test_reuse_channel()')

  let job = job_start(s:python . " test_channel_pipe.py")
  call assert_equal("run", job_status(job))
  let handle = job_getchannel(job)
  try
    call ch_sendraw(handle, "echo something\n")
    call assert_equal("something", ch_readraw(handle))
  finally
    call job_stop(job)
  endtry

  let job = job_start(s:python . " test_channel_pipe.py", {'channel': handle})
  call assert_equal("run", job_status(job))
  let handle = job_getchannel(job)
  try
    call ch_sendraw(handle, "echo again\n")
    call assert_equal("again", ch_readraw(handle))
  finally
    call job_stop(job)
  endtry
endfunc

func Test_out_cb()
  if !has('job')
    return
  endif
  call ch_log('Test_out_cb()')

  let dict = {'thisis': 'dict: '}
  func dict.outHandler(chan, msg) dict
    let g:Ch_outmsg = self.thisis . a:msg
  endfunc
  func dict.errHandler(chan, msg) dict
    let g:Ch_errmsg = self.thisis . a:msg
  endfunc
  let job = job_start(s:python . " test_channel_pipe.py",
	\ {'out_cb': dict.outHandler,
	\ 'out_mode': 'json',
	\ 'err_cb': dict.errHandler,
	\ 'err_mode': 'json'})
  call assert_equal("run", job_status(job))
  try
    let g:Ch_outmsg = ''
    let g:Ch_errmsg = ''
    call ch_sendraw(job, "echo [0, \"hello\"]\n")
    call ch_sendraw(job, "echoerr [0, \"there\"]\n")
    call WaitFor('g:Ch_outmsg != ""')
    call assert_equal("dict: hello", g:Ch_outmsg)
    call WaitFor('g:Ch_errmsg != ""')
    call assert_equal("dict: there", g:Ch_errmsg)
  finally
    call job_stop(job)
  endtry
endfunc

func Test_out_close_cb()
  if !has('job')
    return
  endif
  call ch_log('Test_out_close_cb()')

  let s:counter = 1
  let g:Ch_msg1 = ''
  let g:Ch_closemsg = 0
  func! OutHandler(chan, msg)
    if s:counter == 1
      let g:Ch_msg1 = a:msg
    endif
    let s:counter += 1
  endfunc
  func! CloseHandler(chan)
    let g:Ch_closemsg = s:counter
    let s:counter += 1
  endfunc
  let job = job_start(s:python . " test_channel_pipe.py quit now",
	\ {'out_cb': 'OutHandler',
	\ 'close_cb': 'CloseHandler'})
  call assert_equal("run", job_status(job))
  try
    call WaitFor('g:Ch_closemsg != 0 && g:Ch_msg1 != ""')
    call assert_equal('quit', g:Ch_msg1)
    call assert_equal(2, g:Ch_closemsg)
  finally
    call job_stop(job)
    delfunc OutHandler
    delfunc CloseHandler
  endtry
endfunc

func Test_read_in_close_cb()
  if !has('job')
    return
  endif
  call ch_log('Test_read_in_close_cb()')

  let g:Ch_received = ''
  func! CloseHandler(chan)
    let g:Ch_received = ch_read(a:chan)
  endfunc
  let job = job_start(s:python . " test_channel_pipe.py quit now",
	\ {'close_cb': 'CloseHandler'})
  call assert_equal("run", job_status(job))
  try
    call WaitFor('g:Ch_received != ""')
    call assert_equal('quit', g:Ch_received)
  finally
    call job_stop(job)
    delfunc CloseHandler
  endtry
endfunc

func Test_out_cb_lambda()
  if !has('job')
    return
  endif
  call ch_log('Test_out_cb_lambda()')

  let job = job_start(s:python . " test_channel_pipe.py",
  \ {'out_cb': {ch, msg -> execute("let g:Ch_outmsg = 'lambda: ' . msg")},
  \ 'out_mode': 'json',
  \ 'err_cb': {ch, msg -> execute(":let g:Ch_errmsg = 'lambda: ' . msg")},
  \ 'err_mode': 'json'})
  call assert_equal("run", job_status(job))
  try
    let g:Ch_outmsg = ''
    let g:Ch_errmsg = ''
    call ch_sendraw(job, "echo [0, \"hello\"]\n")
    call ch_sendraw(job, "echoerr [0, \"there\"]\n")
    call WaitFor('g:Ch_outmsg != ""')
    call assert_equal("lambda: hello", g:Ch_outmsg)
    call WaitFor('g:Ch_errmsg != ""')
    call assert_equal("lambda: there", g:Ch_errmsg)
  finally
    call job_stop(job)
  endtry
endfunc

""""""""""

let g:Ch_unletResponse = ''
func s:UnletHandler(handle, msg)
  let g:Ch_unletResponse = a:msg
  unlet s:channelfd
endfunc

" Test that "unlet handle" in a handler doesn't crash Vim.
func Ch_unlet_handle(port)
  let s:channelfd = ch_open('localhost:' . a:port, s:chopt)
  call ch_sendexpr(s:channelfd, "test", {'callback': function('s:UnletHandler')})
  call WaitFor('"what?" == g:Ch_unletResponse')
  call assert_equal('what?', g:Ch_unletResponse)
endfunc

func Test_unlet_handle()
  call ch_log('Test_unlet_handle()')
  call s:run_server('Ch_unlet_handle')
endfunc

""""""""""

let g:Ch_unletResponse = ''
func Ch_CloseHandler(handle, msg)
  let g:Ch_unletResponse = a:msg
  call ch_close(s:channelfd)
endfunc

" Test that "unlet handle" in a handler doesn't crash Vim.
func Ch_close_handle(port)
  let s:channelfd = ch_open('localhost:' . a:port, s:chopt)
  call ch_sendexpr(s:channelfd, "test", {'callback': function('Ch_CloseHandler')})
  call WaitFor('"what?" == g:Ch_unletResponse')
  call assert_equal('what?', g:Ch_unletResponse)
endfunc

func Test_close_handle()
  call ch_log('Test_close_handle()')
  call s:run_server('Ch_close_handle')
endfunc

""""""""""

func Test_open_fail()
  call ch_log('Test_open_fail()')
  silent! let ch = ch_open("noserver")
  echo ch
  let d = ch
endfunc

""""""""""

func Ch_open_delay(port)
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
  call s:run_server('Ch_open_delay', 'delay')
endfunc

"""""""""

function MyFunction(a,b,c)
  let g:Ch_call_ret = [a:a, a:b, a:c]
endfunc

function Ch_test_call(port)
  let handle = ch_open('localhost:' . a:port, s:chopt)
  if ch_status(handle) == "fail"
    call assert_false(1, "Can't open channel")
    return
  endif

  let g:Ch_call_ret = []
  call assert_equal('ok', ch_evalexpr(handle, 'call-func'))
  call WaitFor('len(g:Ch_call_ret) > 0')
  call assert_equal([1, 2, 3], g:Ch_call_ret)
endfunc

func Test_call()
  call ch_log('Test_call()')
  call s:run_server('Ch_test_call')
endfunc

"""""""""

let g:Ch_job_exit_ret = 'not yet'
function MyExitCb(job, status)
  let g:Ch_job_exit_ret = 'done'
endfunc

function Ch_test_exit_callback(port)
  call job_setoptions(g:currentJob, {'exit_cb': 'MyExitCb'})
  let g:Ch_exit_job = g:currentJob
  call assert_equal('MyExitCb', job_info(g:currentJob)['exit_cb'])
endfunc

func Test_exit_callback()
  if has('job')
    call ch_log('Test_exit_callback()')
    call s:run_server('Ch_test_exit_callback')

    " wait up to a second for the job to exit
    for i in range(100)
      if g:Ch_job_exit_ret == 'done'
	break
      endif
      sleep 10m
      " calling job_status() triggers the callback
      call job_status(g:Ch_exit_job)
    endfor

    call assert_equal('done', g:Ch_job_exit_ret)
    call assert_equal('dead', job_info(g:Ch_exit_job).status)
    unlet g:Ch_exit_job
  endif
endfunc

"""""""""

let g:Ch_close_ret = 'alive'
function MyCloseCb(ch)
  let g:Ch_close_ret = 'closed'
endfunc

function Ch_test_close_callback(port)
  let handle = ch_open('localhost:' . a:port, s:chopt)
  if ch_status(handle) == "fail"
    call assert_false(1, "Can't open channel")
    return
  endif
  call ch_setoptions(handle, {'close_cb': 'MyCloseCb'})

  call assert_equal('', ch_evalexpr(handle, 'close me'))
  call WaitFor('"closed" == g:Ch_close_ret')
  call assert_equal('closed', g:Ch_close_ret)
endfunc

func Test_close_callback()
  call ch_log('Test_close_callback()')
  call s:run_server('Ch_test_close_callback')
endfunc

function Ch_test_close_partial(port)
  let handle = ch_open('localhost:' . a:port, s:chopt)
  if ch_status(handle) == "fail"
    call assert_false(1, "Can't open channel")
    return
  endif
  let g:Ch_d = {}
  func g:Ch_d.closeCb(ch) dict
    let self.close_ret = 'closed'
  endfunc
  call ch_setoptions(handle, {'close_cb': g:Ch_d.closeCb})

  call assert_equal('', ch_evalexpr(handle, 'close me'))
  call WaitFor('"closed" == g:Ch_d.close_ret')
  call assert_equal('closed', g:Ch_d.close_ret)
  unlet g:Ch_d
endfunc

func Test_close_partial()
  call ch_log('Test_close_partial()')
  call s:run_server('Ch_test_close_partial')
endfunc

func Test_job_start_invalid()
  call assert_fails('call job_start($x)', 'E474:')
  call assert_fails('call job_start("")', 'E474:')
endfunc

" This was leaking memory.
func Test_partial_in_channel_cycle()
  let d = {}
  let d.a = function('string', [d])
  try
    let d.b = ch_open('nowhere:123', {'close_cb': d.a})
  catch
    call assert_exception('E901:')
  endtry
  unlet d
endfunc

func Test_using_freed_memory()
  let g:a = job_start(['ls'])
  sleep 10m
  call test_garbagecollect_now()
endfunc

func Test_collapse_buffers()
  if !executable('cat') || !has('job')
    return
  endif
  sp test_channel.vim
  let g:linecount = line('$')
  close
  split testout
  1,$delete
  call job_start('cat test_channel.vim', {'out_io': 'buffer', 'out_name': 'testout'})
  call WaitFor('line("$") > g:linecount')
  call assert_inrange(g:linecount, g:linecount + 1, line('$'))
  bwipe!
endfunc

func Test_raw_passes_nul()
  if !executable('cat') || !has('job')
    return
  endif

  " Test lines from the job containing NUL are stored correctly in a buffer.
  new
  call setline(1, ["asdf\nasdf", "xxx\n", "\nyyy"])
  w! Xtestread
  bwipe!
  split testout
  1,$delete
  call job_start('cat Xtestread', {'out_io': 'buffer', 'out_name': 'testout'})
  call WaitFor('line("$") > 2')
  call assert_equal("asdf\nasdf", getline(1))
  call assert_equal("xxx\n", getline(2))
  call assert_equal("\nyyy", getline(3))

  call delete('Xtestread')
  bwipe!

  " Test lines from a buffer with NUL bytes are written correctly to the job.
  new mybuffer
  call setline(1, ["asdf\nasdf", "xxx\n", "\nyyy"])
  let g:Ch_job = job_start('cat', {'in_io': 'buffer', 'in_name': 'mybuffer', 'out_io': 'file', 'out_name': 'Xtestwrite'})
  call WaitFor('"dead" == job_status(g:Ch_job)')
  bwipe!
  split Xtestwrite
  call assert_equal("asdf\nasdf", getline(1))
  call assert_equal("xxx\n", getline(2))
  call assert_equal("\nyyy", getline(3))

  call delete('Xtestwrite')
  bwipe!
endfunc

function Ch_test_close_lambda(port)
  let handle = ch_open('localhost:' . a:port, s:chopt)
  if ch_status(handle) == "fail"
    call assert_false(1, "Can't open channel")
    return
  endif
  let g:Ch_close_ret = ''
  call ch_setoptions(handle, {'close_cb': {ch -> execute("let g:Ch_close_ret = 'closed'")}})

  call assert_equal('', ch_evalexpr(handle, 'close me'))
  call WaitFor('"closed" == g:Ch_close_ret')
  call assert_equal('closed', g:Ch_close_ret)
endfunc

func Test_close_lambda()
  call ch_log('Test_close_lambda()')
  call s:run_server('Ch_test_close_lambda')
endfunc

" Uncomment this to see what happens, output is in src/testdir/channellog.
" call ch_logfile('channellog', 'w')
