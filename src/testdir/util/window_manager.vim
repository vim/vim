CheckFeature job
CheckUnix

let g:xdisplay_num = 100

" Each key is the display name and its value is the compositor/wm job
let s:wayland_displays = {}
let s:x11_displays = {}

command -nargs=0 CheckWaylandCompositor call CheckWaylandCompositor()
command -nargs=0 CheckXServer call CheckXServer()

func CheckWaylandCompositor()
  CheckFeature wayland

  if executable("labwc") != 1
    throw "Skipped: labwc is not available"
  endif
endfunc

func CheckXServer()
  CheckFeature x11

  if executable("Xvfb") != 1
    throw "Skipped: Xvfb is not available"
  endif
  if executable("xdpyinfo") != 1
    throw "Skipped: xdpyinfo is not available"
  endif
endfunc

func s:StartCompositorOutput(channel, msg)
  let l:display = matchstr(a:msg, 'WAYLAND_DISPLAY=\zs.\+')

  if !empty(l:display)
    let s:wayland_display_name = l:display
  endif
endfunc

func s:StartCompositorExit(job, status)
    if s:wayland_display_name == ""
      throw "Skipped: Error: Wayland compositor exited when starting up"
    endif
endfunc

func StartWaylandCompositor()
  let s:wayland_display_name = ""

  let l:wayland_compositor_job = job_start(
        \ ['labwc', '-c', 'NONE', '-d'], {
        \ 'err_io': 'pipe',
        \ 'err_cb': function('s:StartCompositorOutput'),
        \ 'err_mode': 'nl',
        \ 'exit_cb': function('s:StartCompositorExit'),
        \ 'env': { 'WLR_BACKENDS': 'headless' }
        \ })

  call WaitForAssert({-> assert_equal("run",
        \ job_status(l:wayland_compositor_job))})
  call WaitForAssert({-> assert_match('.\+', s:wayland_display_name)})

  let s:wayland_displays[s:wayland_display_name] = l:wayland_compositor_job

  return s:wayland_display_name
endfunc

func EndWaylandCompositor(display)
  let l:job = s:wayland_displays[a:display]

  call job_stop(l:job, 'term')

  " Block until compositor is actually gone
  call WaitForAssert({-> assert_equal("dead", job_status(l:job))})

  unlet s:wayland_displays[a:display]
endfunc

" Start a separate X11 server instance
func StartXServer()
  let l:xdisplay = ':' .. g:xdisplay_num

  let l:x11_server_job = job_start(['Xvfb', l:xdisplay], {})

  call WaitForAssert({-> assert_equal("run", job_status(l:x11_server_job))})
  " Check if server is ready. Not sure if this is the best way though...
  call WaitFor({-> system("DISPLAY=" .. l:xdisplay .. " xdpyinfo 2> /dev/null")
        \ =~? '.\+'})

  g:xdisplay_num += 1

  let s:x11_displays[l:xdisplay] = l:x11_server_job

  return l:xdisplay
endfunc

func EndXServer(display)
  let l:job = s:x11_displays[a:display]

  call job_stop(l:job)

  " Block until X server is actually gone
  call WaitForAssert({-> assert_equal("dead", job_status(l:job))})

  unlet s:x11_displays[a:display]
endfunc

" vim: shiftwidth=2 sts=2 expandtab
