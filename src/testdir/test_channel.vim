" Test for channel functions.
scriptencoding utf-8

" This requires the Python command to run the test server.
" This most likely only works on Unix.
if !has('unix') || !executable('python')
  finish
endif

func Test_communicate()
  " The Python program writes the port number in Xportnr.
  silent !./test_channel.py&

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
    call system("killall test_channel.py")
    return
  endif
  let port = l[0]
  let handle = ch_open('localhost:' . port, 'json')

  " Simple string request and reply.
  call assert_equal('got it', ch_sendexpr(handle, 'hello!'))

  " Request that triggers sending two ex commands.  These will usually be
  " handled before getting the response, but it's not guaranteed, thus wait a
  " tiny bit for the commands to get executed.
  call assert_equal('ok', ch_sendexpr(handle, 'make change'))
  sleep 10m
  call assert_equal('added1', getline(line('$') - 1))
  call assert_equal('added2', getline('$'))

  " make the server quit, can't check if this works, should not hang.
  call ch_sendexpr(handle, '!quit!', 0)

  call system("killall test_channel.py")
endfunc
