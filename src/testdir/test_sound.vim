" Tests for the sound feature

if !has('sound')
  throw 'Skipped: sound feature not available'
endif

func PlayCallback(id, result)
  let g:id = a:id
  let g:result = a:result
endfunc

func Test_play_event()
  let id = sound_playevent('bell', 'PlayCallback')
  if id == 0
    throw 'Skipped: bell event not available'
  endif
  " Stop it quickly, avoid annoying the user.
  sleep 20m
  call sound_stop(id)
  sleep 20m
  call assert_equal(id, g:id)
  call assert_equal(1, g:result)  " sound was aborted
endfunc

func Test_play_silent()
  let fname = fnamemodify('silent.wav', '%p')

  " play without callback
  let id1 = sound_playfile(fname)
  call assert_true(id1 > 0)

  " play until the end
  let id2 = sound_playfile(fname, 'PlayCallback')
  call assert_true(id2 > 0)
  sleep 500m
  call assert_equal(id2, g:id)
  call assert_equal(0, g:result)

  let id2 = sound_playfile(fname, 'PlayCallback')
  call assert_true(id2 > 0)
  sleep 20m
  call sound_stopall()
  call assert_equal(id2, g:id)
  call assert_equal(1, g:result)
endfunc
