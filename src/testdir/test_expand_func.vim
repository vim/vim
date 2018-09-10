" Tests for expand()

let s:sfile = expand('<sfile>')
let s:slnum = str2nr(expand('<slnum>'))
let s:flnum = str2nr(expand('<flnum>'))

func s:expand_sfile()
  return expand('<sfile>')  
endfunc

func s:expand_slnum()
  return str2nr(expand('<slnum>'))  
endfunc

func s:expand_flnum()
  return str2nr(expand('<flnum>'))  
endfunc

func Test_expand_sfile()
  call assert_match('test_expand_func\.vim$', s:sfile)
  call assert_match('^function .*\.\.Test_expand_sfile$', expand('<sfile>'))

  " Call in script-local function
  call assert_match('^function .*\.\.Test_expand_sfile\[5\]\.\.<SNR>\d\+_expand_sfile$', s:expand_sfile())

  " Call in command
  command Sfile echo expand('<sfile>')
  call assert_match('^function .*\.\.Test_expand_sfile$', trim(execute('Sfile')))
  delcommand Sfile
endfunc

func Test_expand_slnum()
  call assert_equal(4, s:slnum)
  call assert_equal(2, str2nr(expand('<slnum>')))

  " Line-continuation
  call assert_equal(
        \ 5,
        \ str2nr(expand('<slnum>')))

  " Call in script-local function
  call assert_equal(1, s:expand_slnum())

  " Call in command
  command Slnum echo expand('<slnum>')
  call assert_equal(14, str2nr(trim(execute('Slnum'))))
  delcommand Slnum
endfunc

func Test_expand_flnum()
  call assert_equal(5, s:flnum)
  call assert_equal(52, str2nr(expand('<flnum>')))

  " Line-continuation
  call assert_equal(
        \ 55,
        \ str2nr(expand('<flnum>')))

  " Call in script-local function
  call assert_equal(16, s:expand_flnum())

  " Call in command
  command Flnum echo expand('<flnum>')
  call assert_equal(64, str2nr(trim(execute('Flnum'))))
  delcommand Flnum
endfunc
