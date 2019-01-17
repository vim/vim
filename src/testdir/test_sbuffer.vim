func Test_sbuffer()
  enew!
  norm ibuffer1
  sbuffer
  new
  norm ibuffer2
  wincmd j

  call assert_equal("buffer1", getline(1))
  wincmd j
  call assert_equal("buffer1", getline(1))
  wincmd t
  call assert_equal("buffer2", getline(1))
endfunc

func Test_vsbuffer()
  enew!
  norm ibuffer1
  vsbuffer
  new
  norm ibuffer2
  wincmd j

  call assert_equal("buffer1", getline(1))
  wincmd l
  call assert_equal("buffer1", getline(1))
  wincmd t
  call assert_equal("buffer2", getline(1))
endfunc
