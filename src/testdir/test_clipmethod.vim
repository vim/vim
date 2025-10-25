" Tests for clipmethod

source util/window_manager.vim

CheckFeature clipboard_working
CheckFeature xterm_clipboard
CheckFeature wayland_clipboard
CheckUnix

" Test if no available clipmethod sets v:clipmethod to none and deinits clipboard
func Test_no_clipmethod_sets_v_clipmethod_none()
  CheckNotGui

  set clipmethod=
  call assert_equal("none", v:clipmethod)
  call assert_equal(0, has('clipboard_working'))
endfunc

" Test if method chosen is in line with clipmethod order
func Test_clipmethod_order()
  CheckNotGui

  set cpm=wayland,x11

  let l:wayland_display = StartWaylandCompositor()

  let $WAYLAND_DISPLAY = l:wayland_display
  exe 'wlrestore ' .. l:wayland_display

  call assert_equal("wayland", v:clipmethod)

  :wlrestore 1239
  clipreset

  if exists("$DISPLAY")
    call assert_equal("x11", v:clipmethod)
  endif

  :xrestore 1239
  clipreset

  call assert_equal("none", v:clipmethod)
  call assert_equal(0, has('clipboard_working'))

  exe ":wlrestore " . $WAYLAND_DISPLAY
  exe ":xrestore " . $DISPLAY
  clipreset

  call assert_equal("wayland", v:clipmethod)
  call assert_equal(1, has('clipboard_working'))

  if exists("$DISPLAY")
    set cpm=x11

    call assert_equal("x11", v:clipmethod)
  endif

  set cpm=wayland

  call assert_equal("wayland", v:clipmethod)

  call EndWaylandCompositor(l:wayland_display)
endfunc

" Test if clipmethod is set to 'none' when gui is started
func Test_clipmethod_is_none_when_gui()
  CheckCanRunGui

  let lines =<< trim END
    set cpm=wayland,x11
    call writefile([v:clipmethod != ""], 'Cbdscript')
    gui -f
    call writefile([v:clipmethod], 'Cbdscript', 'a')
    clipreset
    call writefile([v:clipmethod], 'Cbdscript', 'a')
    quit
  END

  call writefile(lines, 'Cbdscript', 'D')
  call system($'{GetVimCommand()} -S Cbdscript')
  call assert_equal(['1', 'none', 'none'], readfile('Cbdscript'))
endfunc

" Test if :clipreset switches methods when current one doesn't work
func Test_clipreset_switches()
  CheckNotGui
  CheckFeature clientserver
  CheckXServer
  CheckWaylandCompositor

  let l:wayland_display = StartWaylandCompositor()

  set cpm=wayland,x11

  exe 'wlrestore ' .. l:wayland_display

  call assert_equal(l:wayland_display, v:wayland_display)
  call assert_equal("wayland", v:clipmethod)

  call EndWaylandCompositor(l:wayland_display)

  " wlrestore updates clipmethod as well
  wlrestore!

  call assert_equal("", v:wayland_display)
  if exists("$DISPLAY")
    call assert_equal("x11", v:clipmethod)
  endif

  " Do the same but kill a X11 server

  " X11 error handling relies on longjmp magic, but essentially if the X server
  " is killed then it will simply abandon the current commands, making the test
  " hang.

  " This will only happen for commands given from the command line, which
  " is why we cannot just directly call Vim or use the actual Vim instance thats
  " doing all the testing, since main_loop() is never executed.

  " Therefore we should start a separate Vim instance and communicate with it
  " remotely, so we can execute the actual testing stuff with main_loop()
  " running.

  let l:lines =<< trim END
    set cpm=x11
    source util/shared.vim

    func Test()
      clipreset

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
  call writefile(l:lines, 'Xtester', 'D')

  let l:xdisplay = StartXServer()

  let l:name = 'XVIMTEST'
  let l:cmd = GetVimCommand() .. ' -S Xtester --servername ' .. l:name
  let l:job = job_start(l:cmd, { 'stoponexit': 'kill', 'out_io': 'null'})

  call WaitForAssert({-> assert_equal("run", job_status(l:job))})
  if exists("$DISPLAY")
    call WaitForAssert({-> assert_match(l:name, serverlist())})
  endif

  " Change x server to the one that will be killed, then block until
  " v:clipmethod is none.
  if exists("$DISPLAY")
    call remote_send(l:name, ":xrestore " .. l:xdisplay ..
        \ ' | call DoIt()' .. "\<CR>")

    call EndXServer(l:xdisplay)
    call WaitFor({-> filereadable('Xtest')})

    " For some reason readfile sometimes returns an empty list despite the file
    " existing, this why WaitForAssert() is used.
    call WaitForAssert({-> assert_equal(['SUCCESS'], readfile('Xtest'))}, 1000)
  endif
endfunc

" vim: shiftwidth=2 sts=2 expandtab
