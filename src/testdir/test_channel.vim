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

let s:chopt = has('macunix') ? {'waittime' : 1} : {}

" Run "testfunc" after sarting the server and stop the server afterwards.
func s:run_server(testfunc)
  " The Python program writes the port number in Xportnr.
  call delete("Xportnr")

  try
    if has('job')
      let s:job = job_start(s:python . " test_channel.py")
    elseif has('win32')
      exe 'silent !start cmd /c start "test_channel" ' . s:python . ' test_channel.py'
    else
      exe 'silent !' . s:python . ' test_channel.py&'
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

  " Simple string request and reply.
  call assert_equal('got it', ch_sendexpr(handle, 'hello!'))

  " Request that triggers sending two ex commands.  These will usually be
  " handled before getting the response, but it's not guaranteed, thus wait a
  " tiny bit for the commands to get executed.
  call assert_equal('ok', ch_sendexpr(handle, 'make change'))
  sleep 10m
  call assert_equal('added1', getline(line('$') - 1))
  call assert_equal('added2', getline('$'))

  call assert_equal('ok', ch_sendexpr(handle, 'do normal'))
  sleep 10m
  call assert_equal('added more', getline('$'))

  " Send a request with a specific handler.
  call ch_sendexpr(handle, 'hello!', 's:RequestHandler')
  sleep 10m
  if !exists('s:responseHandle')
    call assert_false(1, 's:responseHandle was not set')
  else
    call assert_equal(handle, s:responseHandle)
  endif
  call assert_equal('got it', s:responseMsg)

  unlet s:responseHandle
  let s:responseMsg = ''
  call ch_sendexpr(handle, 'hello!', function('s:RequestHandler'))
  sleep 10m
  if !exists('s:responseHandle')
    call assert_false(1, 's:responseHandle was not set')
  else
    call assert_equal(handle, s:responseHandle)
  endif
  call assert_equal('got it', s:responseMsg)

  " Send an eval request that works.
  call assert_equal('ok', ch_sendexpr(handle, 'eval-works'))
  sleep 10m
  call assert_equal([-1, 'foo123'], ch_sendexpr(handle, 'eval-result'))

  " Send an eval request that fails.
  call assert_equal('ok', ch_sendexpr(handle, 'eval-fails'))
  sleep 10m
  call assert_equal([-2, 'ERROR'], ch_sendexpr(handle, 'eval-result'))

  " Send an eval request that works but can't be encoded.
  call assert_equal('ok', ch_sendexpr(handle, 'eval-error'))
  sleep 10m
  call assert_equal([-3, 'ERROR'], ch_sendexpr(handle, 'eval-result'))

  " Send a bad eval request. There will be no response.
  call assert_equal('ok', ch_sendexpr(handle, 'eval-bad'))
  sleep 10m
  call assert_equal([-3, 'ERROR'], ch_sendexpr(handle, 'eval-result'))

  " Send an expr request
  call assert_equal('ok', ch_sendexpr(handle, 'an expr'))
  sleep 10m
  call assert_equal('one', getline(line('$') - 2))
  call assert_equal('two', getline(line('$') - 1))
  call assert_equal('three', getline('$'))

  " Request a redraw, we don't check for the effect.
  call assert_equal('ok', ch_sendexpr(handle, 'redraw'))
  call assert_equal('ok', ch_sendexpr(handle, 'redraw!'))

  call assert_equal('ok', ch_sendexpr(handle, 'empty-request'))

  " make the server quit, can't check if this works, should not hang.
  call ch_sendexpr(handle, '!quit!', 0)
endfunc

func Test_communicate()
  call s:run_server('s:communicate')
endfunc

" Test that we can open two channels.
func s:two_channels(port)
  let handle = ch_open('localhost:' . a:port, s:chopt)
  if ch_status(handle) == "fail"
    call assert_false(1, "Can't open channel")
    return
  endif

  call assert_equal('got it', ch_sendexpr(handle, 'hello!'))

  let newhandle = ch_open('localhost:' . a:port, s:chopt)
  if ch_status(newhandle) == "fail"
    call assert_false(1, "Can't open second channel")
    return
  endif
  call assert_equal('got it', ch_sendexpr(newhandle, 'hello!'))
  call assert_equal('got it', ch_sendexpr(handle, 'hello!'))

  call ch_close(handle)
  call assert_equal('got it', ch_sendexpr(newhandle, 'hello!'))

  call ch_close(newhandle)
endfunc

func Test_two_channels()
  call s:run_server('s:two_channels')
endfunc

" Test that a server crash is handled gracefully.
func s:server_crash(port)
  let handle = ch_open('localhost:' . a:port, s:chopt)
  if ch_status(handle) == "fail"
    call assert_false(1, "Can't open channel")
    return
  endif

  call ch_sendexpr(handle, '!crash!')

  sleep 10m
endfunc

func Test_server_crash()
  call s:run_server('s:server_crash')
endfunc

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
  call assert_equal('ok', ch_sendexpr(handle, 'call me'))
  sleep 10m
  call assert_equal('we called you', s:reply)

  " Test that it works while not waiting on a numbered message.
  call ch_sendexpr(handle, 'call me again', 0)
  sleep 10m
  call assert_equal('we did call you', s:reply)
endfunc

func Test_channel_handler()
  let s:chopt.callback = 's:Handler'
  call s:run_server('s:channel_handler')
  let s:chopt.callback = function('s:Handler')
  call s:run_server('s:channel_handler')
  unlet s:chopt.callback
endfunc

" Test that trying to connect to a non-existing port fails quickly.
func Test_connect_waittime()
  if !has('unix')
    " TODO: Make this work again for MS-Windows.
    return
  endif
  let start = reltime()
  let handle = ch_open('localhost:9876', s:chopt)
  if ch_status(handle) == "fail"
    " Oops, port does exists.
    call ch_close(handle)
  else
    let elapsed = reltime(start)
    call assert_true(reltimefloat(elapsed) < 1.0)
  endif

  let start = reltime()
  let handle = ch_open('localhost:9867', {'waittime': 2000})
  if ch_status(handle) != "fail"
    " Oops, port does exists.
    call ch_close(handle)
  else
    " Failed connection doesn't wait the full time on Unix.
    " TODO: why is MS-Windows different?
    let elapsed = reltime(start)
    call assert_true(reltimefloat(elapsed) < (has('unix') ? 1.0 : 3.0))
  endif
endfunc

func Test_pipe()
  if !has('job')
    return
  endif
  let job = job_start(s:python . " test_channel_pipe.py")
  call assert_equal("run", job_status(job))
  try
    let handle = job_getchannel(job)
    call ch_sendraw(handle, "echo something\n", 0)
    call assert_equal("something\n", ch_readraw(handle))
    let reply = ch_sendraw(handle, "quit\n")
    call assert_equal("Goodbye!\n", reply)
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
  call ch_sendexpr(s:channelfd, "test", function('s:UnletHandler'))
  sleep 10m
  call assert_equal('what?', s:unletResponse)
endfunc

func Test_unlet_handle()
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
  call ch_sendexpr(s:channelfd, "test", function('s:CloseHandler'))
  sleep 10m
  call assert_equal('what?', s:unletResponse)
endfunc

func Test_close_handle()
  call s:run_server('s:close_handle')
endfunc

""""""""""

func Test_open_fail()
  silent! let ch = ch_open("noserver")
  echo ch
  let d = ch
endfunc
