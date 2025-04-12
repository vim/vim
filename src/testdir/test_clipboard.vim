" Tests for clipmethod and selections

source check.vim
CheckFeature clipboard_working

if $WAYLAND_DISPLAY == "" || $DISPLAY == ""
  throw "Skipped: Either Wayland or X11 is not available, need both"
endif

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

" Test if :restoreclip switches methods when old one doesn't work
func Test_restoreclip_switches()
  " TODO: How to do this (possibly run a separate wayland compositor?)

  " Creating a symlink to the current wayland display and then removing it
  " wont work because display file descriptor is still valid
endfunc

" vim: shiftwidth=2 sts=2 expandtab
