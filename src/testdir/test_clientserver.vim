" Tests for the +clientserver feature.

if !has('job') || !has('clientserver')
  finish
endif

source shared.vim

let s:where = 0
func Abort(id)
  call assert_report('Test timed out at ' . s:where)
  call FinishTesting()
endfunc

func Test_client_server()
  let cmd = GetVimCommand()
  if cmd == ''
    return
  endif

  " Some of these commands may hang when failing.
  call timer_start(10000, 'Abort')

  let s:where = 1
  let name = 'XVIMTEST'
  let cmd .= ' --servername ' . name
  let g:job = job_start(cmd, {'stoponexit': 'kill', 'out_io': 'null'})
  call WaitFor('job_status(g:job) == "run"')
  if job_status(g:job) != 'run'
    call assert_report('Cannot run the Vim server')
    return
  endif
  let s:where = 2

  " Takes a short while for the server to be active.
  call WaitFor('serverlist() =~ "' . name . '"')
  call assert_match(name, serverlist())
  let s:where = 3

  call remote_foreground(name)
  let s:where = 4

  call remote_send(name, ":let testvar = 'yes'\<CR>")
  let s:where = 5
  call WaitFor('remote_expr("' . name . '", "testvar") == "yes"')
  let s:where = 6
  call assert_equal('yes', remote_expr(name, "testvar"))
  let s:where = 7

  if has('unix') && has('gui') && !has('gui_running')
    " Running in a terminal and the GUI is avaiable: Tell the server to open
    " the GUI and check that the remote command still works.
    " Need to wait for the GUI to start up, otherwise the send hangs in trying
    " to send to the terminal window.
    call remote_send(name, ":gui -f\<CR>")
    let s:where = 8
    sleep 500m
    call remote_send(name, ":let testvar = 'maybe'\<CR>")
    let s:where = 9
    call WaitFor('remote_expr("' . name . '", "testvar") == "maybe"')
    let s:where = 10
    call assert_equal('maybe', remote_expr(name, "testvar"))
    let s:where = 11
  endif

  call assert_fails('call remote_send("XXX", ":let testvar = ''yes''\<CR>")', 'E241')
  let s:where = 12

  " Expression evaluated locally.
  if v:servername == ''
    call remote_startserver('MYSELF')
    let s:where = 13
    call assert_equal('MYSELF', v:servername)
  endif
  let g:testvar = 'myself'
  call assert_equal('myself', remote_expr(v:servername, 'testvar'))
  let s:where = 14

  call remote_send(name, ":call server2client(expand('<client>'), 'got it')\<CR>", 'g:myserverid')
  let s:where = 15
  call assert_equal('got it', remote_read(g:myserverid))
  let s:where = 16

  call remote_send(name, ":qa!\<CR>")
  let s:where = 17
  call WaitFor('job_status(g:job) == "dead"')
  let s:where = 18
  if job_status(g:job) != 'dead'
    call assert_report('Server did not exit')
    call job_stop(g:job, 'kill')
  endif
endfunc

" Uncomment this line to get a debugging log
" call ch_logfile('channellog', 'w')
