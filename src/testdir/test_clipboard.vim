" Tests for clipmethod and selections

source check.vim
source shared.vim
CheckFeature clipboard_working
CheckFeature unix
CheckFeature job
CheckFeature clientserver
CheckFeature x11

if $WAYLAND_DISPLAY == "" || $DISPLAY == ""
  throw "Skipped: Either Wayland or X11 is not available, need both"
endif

let s:xserver_name = ':166151155'

if executable("Xvfb") != 1
  throw "Skipped: Xvfb is not available"
endif

" Used internally by Start_wayland_compositor()
func s:Start_compositor_output(channel, msg)
  let l:display = matchstr(a:msg, 'listening on Wayland socket: \zs.\+$')

  if !empty(l:display)
    let s:wayland_display_name = l:display
  endif
endfunc

" Used internally by Start_wayland_compositor()
func s:Start_compositor_exit(job, status)
    if s:wayland_display_name == ""
      throw "Error: Wayland compositor exiting when starting up"
    endif
endfunc

" Start a separate wayland compositor instance
func s:Start_wayland_compositor()
  let s:wayland_display_name = ""

  " Use niri for now as it supports both wlr and ext data control protocols
  " (since v25.02)
  let s:wayland_compositor_job = job_start(['niri'], {
        \ 'out_io': 'pipe',
        \ 'out_cb': function('s:Start_compositor_output'),
        \ 'out_mode': 'nl',
        \ 'exit_cb': function('s:Start_compositor_exit')
        \ })

  call WaitForAssert({-> assert_equal("run", job_status(s:wayland_compositor_job))})
  call WaitForAssert({-> assert_match('.\+', s:wayland_display_name)})
endfunc

func s:End_wayland_compositor()
  call job_stop(s:wayland_compositor_job, 'term')

  " Block until compositor is actually gone
  call WaitForAssert({-> assert_equal("dead", job_status(s:wayland_compositor_job))})
endfunc

" Start a separate X11 server instance
func s:Start_X11_server()
  let s:xserver_online = v:false
  let s:x11_server_job = job_start(['Xvfb', s:xserver_name], { })

  call WaitForAssert({-> assert_equal("run", job_status(s:x11_server_job))})
  call WaitFor({-> system("DISPLAY=" . s:xserver_name . " xdpyinfo 2> /dev/null") =~? '.\+'})
endfunc

func s:End_X11_server()
  call job_stop(s:x11_server_job)

  " Block until X server is actually gone
  call WaitForAssert({-> assert_equal("dead", job_status(s:x11_server_job))})
endfunc

" Test if no available clipmethod sets v:clipmethod to none and deinits clipboard
func Test_no_clipmethod_sets_v_clipmethod_none()
  set clipmethod=
  call assert_equal("none", v:clipmethod)
  call assert_equal(0, has('clipboard_working'))
  set clipmethod&
endfunc

" Test if method chosen is in line with clipmethod order
func Test_clipmethod_order()
  set cpm=wayland,x11

  call assert_equal("wayland", v:clipmethod)

  :wlrestore 1239

  call assert_equal("x11", v:clipmethod)

  :xrestore 1239

  call assert_equal("none", v:clipmethod)
  call assert_equal(0, has('clipboard_working'))

  exe ":wlrestore " . $WAYLAND_DISPLAY
  exe ":xrestore " . $DISPLAY

  call assert_equal("wayland", v:clipmethod)
  call assert_equal(1, has('clipboard_working'))

  set cpm=x11

  call assert_equal("x11", v:clipmethod)

  set cpm=wayland

  call assert_equal("wayland", v:clipmethod)

  set cpm&
endfunc

" Test if clipmethod is set to 'none' when gui is started
func Test_clipmethod_is_none_when_gui()
  CheckCanRunGui

  let lines =<< trim END
    set cpm=wayland,x11
    call writefile([v:clipmethod], 'Cbdscript')
    gui -f
    call writefile([v:clipmethod], 'Cbdscript', 'a')
    restoreclip
    call writefile([v:clipmethod], 'Cbdscript', 'a')
    quit
  END

  call writefile(lines, 'Cbdscript', 'D')
  call system($'{GetVimCommand()} -S Cbdscript')
  call assert_equal(['wayland', 'none', 'none'], readfile('Cbdscript'))
endfunc

" TODO: add test for when we switch data control protocols
" Test if :restoreclip switches methods when current one doesn't work
func Test_restoreclip_switches()
  call s:Start_wayland_compositor()

  set cpm=wayland,x11
  exe 'wlrestore ' . s:wayland_display_name

  call assert_equal(s:wayland_display_name, v:wayland_display)
  call assert_equal("wayland", v:clipmethod)

  call s:End_wayland_compositor()
  restoreclip

  call assert_equal("", v:wayland_display)
  call assert_equal("x11", v:clipmethod)

  " Do the same but kill a X11 server

  " X11 error handling relies on longjmp magic, but essentially if the X server
  " is killed then it will simply abandon the current commands, making the test
  " hang. This will only happen for commands given from the command line, which
  " is why we cannot just directly call Vim or use the actual Vim instance thats
  " doing all the testing, since main_loop() is never executed. Therefore we
  " should start a separate Vim instance and communicate with it remotely, so we
  " can execute the actual testing stuff with main_loop() running.
  call s:Start_X11_server()

  let lines =<< trim END
    set cpm=x11
    source shared.vim

    func Test()
      restoreclip

      if v:clipmethod ==# 'none'
        return 1
      endif
      return 0
    endfunc

    func DoIt()
      call WaitFor(function('Test'))

      if v:clipmethod == 'none'
        call writefile(['SUCCESS'], 'Xtest')
      else
        call writefile(['FAIL'], 'Xtest')
      endif
      quitall
    endfunc
  END

  call writefile(lines, 'Xtester', 'D')

  let l:name = 'XVIMTEST'
  let l:cmd = GetVimCommand() . ' -S Xtester --servername ' . l:name
  let l:job = job_start(cmd, { 'stoponexit': 'kill', 'out_io': 'null'})

  call WaitForAssert({-> assert_equal("run", job_status(l:job))})
  call WaitForAssert({-> assert_match(name, serverlist())})

  " Change x server to the one will be killing, then block until v:clipmethod is
  " none.
  call remote_send(l:name, ":xrestore " . s:xserver_name . ' | call DoIt()' . "\<CR>")

  call s:End_X11_server()
  call WaitFor({-> filereadable('Xtest')})
  " For some reason readfile sometimes returns an empty list despite the file
  " existing, this why WaitForAssert() is used.
  call WaitForAssert({-> assert_equal(['SUCCESS'], readfile('Xtest'))}, 1000)

  set cpm&
endfunc

" vim: shiftwidth=2 sts=2 expandtab
