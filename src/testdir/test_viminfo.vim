" Test for reading and writing .viminfo

function Test_read_and_write()
  call histdel(':')
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
    if line[0] == '|' && line !~ '^|3,'
      if done == 0
	call assert_equal('|1,3', line)
      elseif done == 1
	call assert_equal('|copied as-is', line)
      elseif done == 2
	call assert_equal('|and one more', line)
      endif
      let done += 1
    endif
  endfor
  call assert_equal(3, done)

  call delete('Xviminfo')
endfunc

func Test_global_vars()
  let test_dict = {'foo': 1, 'bar': 0, 'longvarible': 1000}
  let g:MY_GLOBAL_DICT = test_dict
  " store a really long list, so line wrapping will occur in viminfo file
  let test_list = range(1,100)
  let g:MY_GLOBAL_LIST = test_list
  set viminfo='100,<50,s10,h,!,nviminfo
  wv! Xviminfo
  unlet g:MY_GLOBAL_DICT
  unlet g:MY_GLOBAL_LIST

  rv! Xviminfo
  call assert_equal(test_dict, g:MY_GLOBAL_DICT)
  call assert_equal(test_list, g:MY_GLOBAL_LIST)

  call delete('Xviminfo')
  set viminfo-=!
endfunc

func Test_cmdline_history()
  call histdel(':')
  call test_settime(11)
  call histadd(':', "echo 'one'")
  call test_settime(12)
  " split into two lines
  let long800 = repeat(" 'eight'", 100)
  call histadd(':', "echo " . long800)
  call test_settime(13)
  " split into three lines
  let long1400 = repeat(" 'fourteeeeen'", 100)
  call histadd(':', "echo " . long1400)
  wviminfo Xviminfo
  let lines = readfile('Xviminfo')
  let done_colon = 0
  let done_bar = 0
  let lnum = 0
  while lnum < len(lines)
    let line = lines[lnum] | let lnum += 1
    if line[0] == ':'
      if done_colon == 0
	call assert_equal(":\x161408", line)
	let line = lines[lnum] | let lnum += 1
	call assert_equal('<echo ' . long1400, line)
      elseif done_colon == 1
	call assert_equal(":\x16808", line)
	let line = lines[lnum] | let lnum += 1
	call assert_equal("<echo " . long800, line)
      elseif done_colon == 2
	call assert_equal(":echo 'one'", line)
      endif
      let done_colon += 1
    elseif line[0:4] == '|2,0,'
      if done_bar == 0
	call assert_equal("|2,0,13,,>1407", line)
	let line = lines[lnum] | let lnum += 1
	call assert_equal('|<"echo ' . long1400[0:484], line)
	let line = lines[lnum] | let lnum += 1
	call assert_equal('|<' . long1400[485:974], line)
	let line = lines[lnum] | let lnum += 1
	call assert_equal('|<' . long1400[975:] . '"', line)
      elseif done_bar == 1
	call assert_equal('|2,0,12,,>807', line)
	let line = lines[lnum] | let lnum += 1
	call assert_equal('|<"echo ' . long800[0:484], line)
	let line = lines[lnum] | let lnum += 1
	call assert_equal('|<' . long800[485:] . '"', line)
      elseif done_bar == 2
	call assert_equal("|2,0,11,,\"echo 'one'\"", line)
      endif
      let done_bar += 1
    endif
  endwhile
  call assert_equal(3, done_colon)
  call assert_equal(3, done_bar)

  call histdel(':')
  rviminfo Xviminfo
  call assert_equal("echo " . long1400, histget(':', -1))
  call assert_equal("echo " . long800, histget(':', -2))
  call assert_equal("echo 'one'", histget(':', -3))

  call delete('Xviminfo')
endfunc

func Test_cmdline_history_order()
  call histdel(':')
  call test_settime(11)
  call histadd(':', "echo '11'")
  call test_settime(22)
  call histadd(':', "echo '22'")
  call test_settime(33)
  call histadd(':', "echo '33'")
  wviminfo Xviminfo

  call histdel(':')
  " items go in between
  call test_settime(15)
  call histadd(':', "echo '15'")
  call test_settime(27)
  call histadd(':', "echo '27'")

  rviminfo Xviminfo
  call assert_equal("echo '33'", histget(':', -1))
  call assert_equal("echo '27'", histget(':', -2))
  call assert_equal("echo '22'", histget(':', -3))
  call assert_equal("echo '15'", histget(':', -4))
  call assert_equal("echo '11'", histget(':', -5))

  call histdel(':')
  " items go before and after
  call test_settime(8)
  call histadd(':', "echo '8'")
  call test_settime(39)
  call histadd(':', "echo '39'")

  rviminfo Xviminfo
  call assert_equal("echo '39'", histget(':', -1))
  call assert_equal("echo '33'", histget(':', -2))
  call assert_equal("echo '22'", histget(':', -3))
  call assert_equal("echo '11'", histget(':', -4))
  call assert_equal("echo '8'", histget(':', -5))

  " Check sorting works when writing with merge.
  call histdel(':')
  call test_settime(8)
  call histadd(':', "echo '8'")
  call test_settime(15)
  call histadd(':', "echo '15'")
  call test_settime(27)
  call histadd(':', "echo '27'")
  call test_settime(39)
  call histadd(':', "echo '39'")
  wviminfo Xviminfo
  
  call histdel(':')
  rviminfo Xviminfo
  call assert_equal("echo '39'", histget(':', -1))
  call assert_equal("echo '33'", histget(':', -2))
  call assert_equal("echo '27'", histget(':', -3))
  call assert_equal("echo '22'", histget(':', -4))
  call assert_equal("echo '15'", histget(':', -5))
  call assert_equal("echo '11'", histget(':', -6))
  call assert_equal("echo '8'", histget(':', -7))

  call delete('Xviminfo')
endfunc

func Test_viminfo_registers()
  call test_settime(8)
  call setreg('a', "eight", 'c')
  call test_settime(20)
  call setreg('b', ["twenty", "again"], 'l')
  call test_settime(40)
  call setreg('c', ["four", "agai"], 'b4')
  let l = []
  set viminfo='100,<600,s10,h,!,nviminfo
  for i in range(500)
    call add(l, 'something')
  endfor
  call setreg('d', l, 'l')
  wviminfo Xviminfo

  call test_settime(10)
  call setreg('a', '', 'b10')
  call test_settime(15)
  call setreg('b', 'drop')
  call test_settime(50)
  call setreg('c', 'keep', 'l')
  call test_settime(30)
  call setreg('d', 'drop', 'l')
  rviminfo Xviminfo

  call assert_equal("", getreg('a'))
  call assert_equal("\<C-V>10", getregtype('a'))
  call assert_equal("twenty\nagain\n", getreg('b'))
  call assert_equal("V", getregtype('b'))
  call assert_equal("keep\n", getreg('c'))
  call assert_equal("V", getregtype('c'))
  call assert_equal(l, getreg('d', 1, 1))
  call assert_equal("V", getregtype('d'))

  call delete('Xviminfo')
endfunc

func Test_viminfo_encoding()
  if !has('multi_byte')
    return
  endif
  set enc=latin1
  call histdel(':')
  call histadd(':', "echo '\xe9'")
  wviminfo Xviminfo

  set fencs=utf-8,latin1
  set enc=utf-8
  sp Xviminfo
  call assert_equal('latin1', &fenc)
  close
  
  call histdel(':')
  rviminfo Xviminfo
  call assert_equal("echo 'é'", histget(':', -1))

  call delete('Xviminfo')
endfunc

func Test_viminfo_bad_syntax()
  let lines = []
  call add(lines, '|<')  " empty continuation line
  call add(lines, '|234234234234234324,nothing')
  call add(lines, '|1+"no comma"')
  call add(lines, '|1,2,3,4,5,6,7')  " too many items
  call add(lines, '|1,"string version"')
  call add(lines, '|1,>x') " bad continuation line
  call add(lines, '|1,"x') " missing quote
  call add(lines, '|1,"x\') " trailing backslash
  call add(lines, '|1,,,,') "trailing comma
  call add(lines, '|1,>234') " trailing continuation line
  call writefile(lines, 'Xviminfo')
  rviminfo Xviminfo

  call delete('Xviminfo')
endfunc

