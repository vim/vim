source check.vim
source shared.vim

CheckFeature job

let g:xdisplay_num = 100

command -nargs=0 CheckXServer call CheckXServer()
func CheckXServer()
  CheckFeature x11

  if executable("Xvfb") != 1
    throw "Skipped: Xvfb is not available"
  endif
  if executable("xdpyinfo") != 1
    throw "Skipped: xdpyinfo is not available"
  endif
  if $DISPLAY == ""
    throw "Skipped: $DISPLAY not set"
  endif
endfunc

command -nargs=0 CheckWaylandCompositor call CheckWaylandCompositor()
func CheckWaylandCompositor()
  CheckFeature wayland

  if executable("sway") != 1
    throw "Skipped: sway is not available"
  endif
  if $WAYLAND_DISPLAY == ""
    throw "Skipped: $WAYLAND_DISPLAY not set"
  endif
  if v:wayland_display == ""
    throw "Skipped: v:wayland_display is empty"
  endif
endfunc

" Used internally by Start_wayland_compositor()
func s:Start_compositor_output(channel, msg)
  let l:display = matchstr(a:msg, 'Running compositor on wayland display ''\zs[^'']\+\ze''')

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
func Start_wayland_compositor()
  let s:wayland_display_name = ""

  " When using systemd, sway sets some environment variables like
  " WAYLAND_DISPLAY, this is because of /etc/sway/config.d/50-systemd-user.conf,
  " but there isn't a way for sway to not use the system config, so I guess just
  " save the environment then restore it.

  if system("ps --no-headers -o comm 1") =~? "systemd"
    call system("systemctl show-environment --user --no-pager > wlsaveenv.txt")
  endif

  let l:wayland_compositor_job = job_start(['sway', '--verbose'], {
        \ 'err_io': 'pipe',
        \ 'err_cb': function('s:Start_compositor_output'),
        \ 'err_mode': 'nl',
        \ 'exit_cb': function('s:Start_compositor_exit'),
        \ 'env': { 'WLR_BACKENDS': 'headless' }
        \ })

  call WaitForAssert({-> assert_equal("run", job_status(l:wayland_compositor_job))})
  call WaitForAssert({-> assert_match('.\+', s:wayland_display_name)})

  return (s:wayland_display_name, l:wayland_compositor_job)
endfunc

func End_wayland_compositor(job)
  call job_stop(a:job, 'term')

  " Block until compositor is actually gone
  call WaitForAssert({-> assert_equal("dead", job_status(a:job))})

  " Restore environment
  if system("ps --no-headers -o comm 1") =~? "systemd"
    for line in readfile("wlsaveenv.txt")
      call system("systemctl set-environment --user " .. line)
    endfor
    call delete("wlsaveenv.txt")
  endif
endfunc

" Start a separate X11 server instance
func Start_X11_server()
  let l:xdisplay = ':' .. g:xdisplay_num

  let l:x11_server_job = job_start(['Xvfb', l:xdisplay], {})

  call WaitForAssert({-> assert_equal("run", job_status(l:x11_server_job))})
  " Check if server is ready. Not sure if this is the best way though...
  call WaitFor({-> system("DISPLAY=" .. l:xdisplay .. " xdpyinfo 2> /dev/null")
        \ =~? '.\+'})

  g:xdisplay_num += 1
  return (l:xdisplay, l:x11_server_job)
endfunc

func End_X11_server(job)
  call job_stop(a:job)

  " Block until X server is actually gone
  call WaitForAssert({-> assert_equal("dead", job_status(a:job))})
endfunc
