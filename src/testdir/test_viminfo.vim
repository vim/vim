" Test for reading and writing .viminfo

function Test_read_and_write()
  let lines = [
	\ '# comment line',
	\ '*encoding=utf-8',
	\ '~MSle0~/asdf',
	\ '|copied as-is',
	\ '|and one more',
	\ ]
  call writefile(lines, 'Xviminfo')
  rviminfo Xviminfo
  call assert_equal('asdf', @/)

  wviminfo Xviminfo
  let lines = readfile('Xviminfo')
  let done = 0
  for line in lines
    if line[0] == '|'
      if done == 0
	call assert_equal('|copied as-is', line)
      elseif done == 1
	call assert_equal('|and one more', line)
      endif
      let done += 1
    endif
  endfor
  call assert_equal(2, done)

  call delete('Xviminfo')
endfunc

func Test_global_vars()
  let test_dict = {'foo': 1, 'bar': 0, 'longvarible': 1000}
  let g:MY_GLOBAL_DICT = test_dict
  " store a really long list, so line wrapping will occur in viminfo file
  let test_list = range(1,100)
  let g:MY_GLOBAL_LIST = test_list
  set viminfo='100,<50,s10,h,!
  wv! Xviminfo
  unlet g:MY_GLOBAL_DICT
  unlet g:MY_GLOBAL_LIST

  rv! Xviminfo
  call assert_equal(test_dict, g:MY_GLOBAL_DICT)
  call assert_equal(test_list, g:MY_GLOBAL_LIST)

  call delete('Xviminfo')
  set viminfo-=!
endfunc
