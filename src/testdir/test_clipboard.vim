" Tests for clipmethod and selections

source check.vim
CheckFeature clipboard_working
CheckFeature unix

if $WAYLAND_DISPLAY == "" || $DISPLAY == ""
  throw "Skipped: Either Wayland or X11 is not available, need both"
endif

" Some random (...) number
let s:xserver_name = ':166151155'

" Should be covered by $DISPLAY environment variable but Xwayland
" might not be running yet so...
if executable("Xwayland") != 1
  throw "Skipped: Xwayland is not available"
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

  if job_status(s:wayland_compositor_job) != "run"
    throw "Error: Wayland compositor not running"
  endif

  " Wait until compositor is fully running
  while empty(s:wayland_display_name)
    sleep 100m " Allow vim to process callbacks
  endwhile
endfunc

func s:End_wayland_compositor()
  call job_stop(s:wayland_compositor_job, 'term')

  " Block until compositor is actually gone
  while job_status(s:wayland_compositor_job) != "dead"
  endwhile
endfunc

" Used internally by Start_X11_server()
func s:Start_xserver_output(channel, msg)
  let s:xserver_online = v:true
endfunc

" Used internally by Start_X11_server()
func s:Start_xserver_exit(job, status)
  if !s:xserver_online
    throw "X11 server exited when starting up"
  endif
endfunc

" Start a separate X11 server instance
func s:Start_X11_server()
  " Use Xwayland for now
  " Xwayland uses stderr for logs
  let s:xserver_online = v:false
  let s:x11_server_job = job_start(['Xwayland', '-verbose', '3', s:xserver_name], {
        \ 'err_io': 'pipe',
        \ 'err_cb': function('s:Start_xserver_output'),
        \ 'err_mode': 'nl',
        \ 'exit_cb': function('s:Start_xserver_exit')
        \ })

  if job_status(s:x11_server_job) != "run"
    throw "Error: X11 server not running"
  endif

  while !s:xserver_online
    sleep 100m " Allow vim to process callbacks
  endwhile
endfunc

func s:End_X11_server()
  call job_stop(s:x11_server_job, 'term')

  " Block until compositor is actually gone
  while job_status(s:x11_server_job) != "dead"
  endwhile
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

" Test if clipmethod is set to none when gui is started
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
" Test if :restoreclip switches methods when old one doesn't work
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
  " For some reason ending the job directly makes vim seg fault?
  " No idea, leaving this out for now...
  " set cpm=x11
  " call s:Start_X11_server()

  " exe 'xrestore ' . s:xserver_name

  " call s:End_X11_server()

  set cpm&
endfunc

" vim: shiftwidth=2 sts=2 expandtab
