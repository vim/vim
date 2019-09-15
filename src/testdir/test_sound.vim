" Tests for the sound feature

if !has('sound')
  throw 'Skipped: sound feature not available'
endif

func PlayCallback(id, result)
  let g:id = a:id
  let g:result = a:result
endfunc

func Test_play_event()
  if has('win32')
    throw 'Skipped: Playing event with callback is not supported on Windows'
  endif
  let id = 'bell'->sound_playevent('PlayCallback')
  if id == 0
    throw 'Skipped: bell event not available'
  endif
  " Stop it quickly, avoid annoying the user.
  sleep 20m
  eval id->sound_stop()
  sleep 30m
  call assert_equal(id, g:id)
  call assert_equal(1, g:result)  " sound was aborted
endfunc

func Test_play_silent()
  let fname = fnamemodify('silent.wav', '%p')

  " play without callback
  let id1 = sound_playfile(fname)
  if id1 == 0
    throw 'Skipped: playing a sound is not working'
  endif

  " play until the end
  let id2 = fname->sound_playfile('PlayCallback')
  call assert_true(id2 > 0)
  sleep 500m
  call assert_equal(id2, g:id)
  call assert_equal(0, g:result)

  let id2 = sound_playfile(fname, 'PlayCallback')
  call assert_true(id2 > 0)
  sleep 20m
  call sound_clear()
  sleep 30m
  call assert_equal(id2, g:id)
  call assert_equal(1, g:result)

  " recursive use was causing a crash
  func PlayAgain(id, fname)
    let g:id = a:id
    let g:id_again = sound_playfile(a:fname)
  endfunc

  let id3 = sound_playfile(fname, {id, res -> PlayAgain(id, fname)})
  call assert_true(id3 > 0)
  sleep 50m
  call sound_clear()
  sleep 30m
  call assert_true(g:id_again > 0)
endfunc
