" Tests for executable()

function! Test_Executable()
  if has('win32')
    call assert_equal(1, executable('notepad'))
    call assert_equal(1, executable('notepad.exe'))
    call assert_equal(0, executable('notepad.exe.exe'))
    call assert_equal(0, executable('shell32.dll'))
    call assert_equal(0, executable('win.ini'))
  elseif has('unix')
    call assert_equal(1, executable('cat'))
    call assert_equal(0, executable('dog'))
  endif
endfunction

