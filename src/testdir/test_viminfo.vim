" Test for reading and writing .viminfo

function Test_viminfo_read_and_write()
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
    if line[0] == '|' && line !~ '^|[234],' && line !~ '^|<'
      if done == 0
	call assert_equal('|1,4', line)
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
  let test_blob = 0z00112233445566778899aabbccddeeff
  let g:MY_GLOBAL_BLOB = test_blob
  let test_false = v:false
  let g:MY_GLOBAL_FALSE = test_false
  let test_true = v:true
  let g:MY_GLOBAL_TRUE = test_true
  let test_null = v:null
  let g:MY_GLOBAL_NULL = test_null
  let test_none = v:none
  let g:MY_GLOBAL_NONE = test_none

  set viminfo='100,<50,s10,h,!,nviminfo
  wv! Xviminfo

  unlet g:MY_GLOBAL_DICT
  unlet g:MY_GLOBAL_LIST
  unlet g:MY_GLOBAL_BLOB
  unlet g:MY_GLOBAL_FALSE
  unlet g:MY_GLOBAL_TRUE
  unlet g:MY_GLOBAL_NULL
  unlet g:MY_GLOBAL_NONE

  rv! Xviminfo
  call assert_equal(test_dict, g:MY_GLOBAL_DICT)
  call assert_equal(test_list, g:MY_GLOBAL_LIST)
  call assert_equal(test_blob, g:MY_GLOBAL_BLOB)
  call assert_equal(test_false, g:MY_GLOBAL_FALSE)
  call assert_equal(test_true, g:MY_GLOBAL_TRUE)
  call assert_equal(test_null, g:MY_GLOBAL_NULL)
  call assert_equal(test_none, g:MY_GLOBAL_NONE)

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

  " Length around 440 switches to line continuation.
  let len = 434
  while len < 445
    let s = repeat('a', len)
    call setreg('"', s)
    wviminfo Xviminfo
    call setreg('"', '')
    rviminfo Xviminfo
    call assert_equal(s, getreg('"'), 'wrong register at length: ' . len)

    let len += 1
  endwhile

  call delete('Xviminfo')
endfunc

func Test_viminfo_marks()
  sp bufa
  let bufa = bufnr('%')
  sp bufb
  let bufb = bufnr('%')

  call test_settime(8)
  call setpos("'A", [bufa, 1, 1, 0])
  call test_settime(20)
  call setpos("'B", [bufb, 9, 1, 0])
  call setpos("'C", [bufa, 7, 1, 0])

  delmark 0-9
  call test_settime(25)
  call setpos("'1", [bufb, 12, 1, 0])
  call test_settime(35)
  call setpos("'0", [bufa, 11, 1, 0])

  call test_settime(45)
  wviminfo Xviminfo

  " Writing viminfo inserts the '0 mark.
  call assert_equal([bufb, 1, 1, 0], getpos("'0"))
  call assert_equal([bufa, 11, 1, 0], getpos("'1"))
  call assert_equal([bufb, 12, 1, 0], getpos("'2"))

  call test_settime(4)
  call setpos("'A", [bufa, 9, 1, 0])
  call test_settime(30)
  call setpos("'B", [bufb, 2, 3, 0])
  delmark C

  delmark 0-9
  call test_settime(30)
  call setpos("'1", [bufb, 22, 1, 0])
  call test_settime(55)
  call setpos("'0", [bufa, 21, 1, 0])

  rviminfo Xviminfo

  call assert_equal([bufa, 1, 1, 0], getpos("'A"))
  call assert_equal([bufb, 2, 3, 0], getpos("'B"))
  call assert_equal([bufa, 7, 1, 0], getpos("'C"))

  " numbered marks are merged
  call assert_equal([bufa, 21, 1, 0], getpos("'0"))  " time 55
  call assert_equal([bufb, 1, 1, 0], getpos("'1"))  " time 45
  call assert_equal([bufa, 11, 1, 0], getpos("'2")) " time 35
  call assert_equal([bufb, 22, 1, 0], getpos("'3")) " time 30
  call assert_equal([bufb, 12, 1, 0], getpos("'4")) " time 25

  call delete('Xviminfo')
  exe 'bwipe ' . bufa
  exe 'bwipe ' . bufb
endfunc

func Test_viminfo_jumplist()
  split testbuf
  clearjumps
  call setline(1, ['time 05', 'time 10', 'time 15', 'time 20', 'time 30', 'last pos'])
  call cursor(2, 1)
  call test_settime(10)
  exe "normal /20\r"
  call test_settime(20)
  exe "normal /30\r"
  call test_settime(30)
  exe "normal /last pos\r"
  wviminfo Xviminfo

  clearjumps
  call cursor(1, 1)
  call test_settime(5)
  exe "normal /15\r"
  call test_settime(15)
  exe "normal /last pos\r"
  call test_settime(40)
  exe "normal ?30\r"
  rviminfo Xviminfo

  call assert_equal('time 30', getline('.'))
  exe "normal \<C-O>"
  call assert_equal('last pos', getline('.'))
  exe "normal \<C-O>"
  " duplicate for 'time 30' was removed
  call assert_equal('time 20', getline('.'))
  exe "normal \<C-O>"
  call assert_equal('time 15', getline('.'))
  exe "normal \<C-O>"
  call assert_equal('time 10', getline('.'))
  exe "normal \<C-O>"
  call assert_equal('time 05', getline('.'))

  clearjumps
  call cursor(1, 1)
  call test_settime(5)
  exe "normal /15\r"
  call test_settime(15)
  exe "normal /last pos\r"
  call test_settime(40)
  exe "normal ?30\r"
  " Test merge when writing
  wviminfo Xviminfo
  clearjumps
  rviminfo Xviminfo

  let last_line = line('.')
  exe "normal \<C-O>"
  call assert_equal('time 30', getline('.'))
  exe "normal \<C-O>"
  call assert_equal('last pos', getline('.'))
  exe "normal \<C-O>"
  " duplicate for 'time 30' was removed
  call assert_equal('time 20', getline('.'))
  exe "normal \<C-O>"
  call assert_equal('time 15', getline('.'))
  exe "normal \<C-O>"
  call assert_equal('time 10', getline('.'))
  exe "normal \<C-O>"
  call assert_equal('time 05', getline('.'))

  " Test with jumplist full.
  clearjumps
  call setline(1, repeat(['match here'], 101))
  call cursor(1, 1)
  call test_settime(10)
  for i in range(100)
    exe "normal /here\r"
  endfor
  rviminfo Xviminfo

  " must be newest mark that comes from viminfo.
  exe "normal \<C-O>"
  call assert_equal(last_line, line('.'))

  bwipe!
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

func Test_viminfo_file_marks()
  silent! bwipe test_viminfo.vim
  silent! bwipe Xviminfo

  call test_settime(10)
  edit ten
  call test_settime(25)
  edit again
  call test_settime(30)
  edit thirty
  wviminfo Xviminfo

  call test_settime(20)
  edit twenty
  call test_settime(35)
  edit again
  call test_settime(40)
  edit fourty
  wviminfo Xviminfo

  sp Xviminfo
  1
  for name in ['fourty', 'again', 'thirty', 'twenty', 'ten']
    /^>
    call assert_equal(name, substitute(getline('.'), '.*/', '', ''))
  endfor
  close

  call delete('Xviminfo')
endfunc

func Test_viminfo_file_mark_tabclose()
  tabnew Xtestfileintab
  call setline(1, ['a','b','c','d','e'])
  4
  q!
  wviminfo Xviminfo
  sp Xviminfo
  /^> .*Xtestfileintab
  let lnum = line('.')
  while 1
    if lnum == line('$')
      call assert_report('mark not found in Xtestfileintab')
      break
    endif
    let lnum += 1
    let line = getline(lnum)
    if line == ''
      call assert_report('mark not found in Xtestfileintab')
      break
    endif
    if line =~ "^\t\""
      call assert_equal('4', substitute(line, ".*\"\t\\(\\d\\).*", '\1', ''))
      break
    endif
  endwhile

  call delete('Xviminfo')
  silent! bwipe Xtestfileintab
endfunc

func Test_viminfo_file_mark_zero_time()
  let lines = [
	\ '# Viminfo version',
	\ '|1,4',
	\ '',
	\ '*encoding=utf-8',
	\ '',
	\ '# File marks:',
	\ "'B  1  0  /tmp/nothing",
	\ '|4,66,1,0,0,"/tmp/nothing"',
	\ "",
	\ ]
  call writefile(lines, 'Xviminfo')
  delmark B
  rviminfo Xviminfo
  call delete('Xviminfo')
  call assert_equal(1, line("'B"))
  delmark B
endfunc

func Test_viminfo_oldfiles()
  let v:oldfiles = []
  let lines = [
	\ '# comment line',
	\ '*encoding=utf-8',
	\ '',
	\ "> /tmp/file_one.txt",
	\ "\t\"\t11\t0",
	\ "",
	\ "> /tmp/file_two.txt",
	\ "\t\"\t11\t0",
	\ "",
	\ "> /tmp/another.txt",
	\ "\t\"\t11\t0",
	\ "",
	\ ]
  call writefile(lines, 'Xviminfo')
  rviminfo! Xviminfo
  call delete('Xviminfo')

  call assert_equal(['1: /tmp/file_one.txt', '2: /tmp/file_two.txt', '3: /tmp/another.txt'], filter(split(execute('oldfiles'), "\n"), {i, v -> v =~ '/tmp/'}))
  call assert_equal(['1: /tmp/file_one.txt', '2: /tmp/file_two.txt'], filter(split(execute('filter file_ oldfiles'), "\n"), {i, v -> v =~ '/tmp/'}))
  call assert_equal(['3: /tmp/another.txt'], filter(split(execute('filter /another/ oldfiles'), "\n"), {i, v -> v =~ '/tmp/'}))
endfunc
