" Tests for 'ballooneval' in the GUI.

if !has('gui_running')
  finish
endif

if !has('balloon_eval')
  throw 'Skipped: balloon_eval feature missing'
endif

func Test_balloon_show_gui()
  let msg = 'this this this this'
  call balloon_show(msg)
  call assert_equal(msg, balloon_gettext())
  sleep 10m
  call balloon_show('')

  let msg = 'that that'
  call balloon_show(msg)
  call assert_equal(msg, balloon_gettext())
  sleep 10m
  call balloon_show('')
endfunc
