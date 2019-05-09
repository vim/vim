" Test for the xxd command
if empty($XXD) && executable('..\xxd\xxd.exe')
  let s:xxd_cmd = '..\xxd\xxd.exe'
elseif empty($XXD) || !executable($XXD)
  finish
else
  let s:xxd_cmd = $XXD
endif

func PrepareBuffer(lines)
  new
  call append(0, a:lines)
  $d
endfunc

func s:Mess(counter)
  return printf("Failed xxd test %d:", a:counter)
endfunc

func Test_xxd()
  call PrepareBuffer(range(1,30))
  set ff=unix
  w! XXDfile

  " Test 1: simple, filter the result through xxd
  let s:test = 1
  exe '%!' . s:xxd_cmd . ' %'
  let expected = [
        \ '00000000: 310a 320a 330a 340a 350a 360a 370a 380a  1.2.3.4.5.6.7.8.',
        \ '00000010: 390a 3130 0a31 310a 3132 0a31 330a 3134  9.10.11.12.13.14',
        \ '00000020: 0a31 350a 3136 0a31 370a 3138 0a31 390a  .15.16.17.18.19.',
        \ '00000030: 3230 0a32 310a 3232 0a32 330a 3234 0a32  20.21.22.23.24.2',
        \ '00000040: 350a 3236 0a32 370a 3238 0a32 390a 3330  5.26.27.28.29.30',
        \ '00000050: 0a                                       .']
  call assert_equal(expected, getline(1,'$'), s:Mess(s:test))

  " Test 2: reverse the result
  let s:test += 1
  exe '%!' . s:xxd_cmd . ' -r'
  call assert_equal(map(range(1,30), {v,c -> string(c)}), getline(1,'$'), s:Mess(s:test))

  " Test 3: Skip the first 0x30 bytes
  let s:test += 1
  for arg in ['-s 0x30', '-s0x30', '-s+0x30', '-skip 0x030', '-seek 0x30', '-seek +0x30 --']
    exe '%!' . s:xxd_cmd . ' ' . arg . ' %'
    call assert_equal(expected[3:], getline(1,'$'), s:Mess(s:test))
  endfor

  " Test 4: Skip the first 30 bytes
  let s:test += 1
  for arg in ['-s -0x31', '-s-0x31']
    exe '%!' . s:xxd_cmd . ' ' . arg . ' %'
    call assert_equal(expected[2:], getline(1,'$'), s:Mess(s:test))
  endfor

  " The following tests use the xxd man page.
  " For these tests to pass, the fileformat must be "unix".
  let man_copy = 'Xxd.1'
  let man_page = '../../runtime/doc/xxd.1'
  if has('win32') && !filereadable(man_page)
    let man_page = '../../doc/xxd.1'
  endif
  %d
  exe '0r ' man_page '| set ff=unix | $d | w' man_copy '| bwipe!' man_copy

  " Test 5: Print 120 bytes as continuous hexdump with 20 octets per line
  let s:test += 1
  %d
  exe '0r! ' . s:xxd_cmd . ' -l 120 -ps -c20 ' . man_copy
  $d
  let expected = [
      \ '2e54482058584420312022417567757374203139',
      \ '39362220224d616e75616c207061676520666f72',
      \ '20787864220a2e5c220a2e5c222032317374204d',
      \ '617920313939360a2e5c22204d616e2070616765',
      \ '20617574686f723a0a2e5c2220202020546f6e79',
      \ '204e7567656e74203c746f6e79407363746e7567']
  call assert_equal(expected, getline(1,'$'), s:Mess(s:test))

  " Test 6: Print the date from xxd.1
  let s:test += 1
  for arg in ['-l 13', '-l13', '-len 13']
    %d
    exe '0r! ' . s:xxd_cmd . ' -s 0x36 ' . arg . ' -cols 13 ' . man_copy
    $d
    call assert_equal('00000036: 3231 7374 204d 6179 2031 3939 36  21st May 1996', getline(1), s:Mess(s:test))
  endfor

  " Cleanup after tests 5 and 6
  call delete(man_copy)

  " Test 7: Print C include
  let s:test += 1
  call writefile(['TESTabcd09'], 'XXDfile')
  %d
  exe '0r! ' . s:xxd_cmd . ' -i XXDfile'
  $d
  let expected = ['unsigned char XXDfile[] = {',
        \ '  0x54, 0x45, 0x53, 0x54, 0x61, 0x62, 0x63, 0x64, 0x30, 0x39, 0x0a', '};',
        \ 'unsigned int XXDfile_len = 11;']
  call assert_equal(expected, getline(1,'$'), s:Mess(s:test))

  " Test 8: Print C include capitalized
  let s:test += 1
  for arg in ['-C', '-capitalize']
    call writefile(['TESTabcd09'], 'XXDfile')
    %d
    exe '0r! ' . s:xxd_cmd . ' -i ' . arg . ' XXDfile'
    $d
    let expected = ['unsigned char XXDFILE[] = {',
	  \ '  0x54, 0x45, 0x53, 0x54, 0x61, 0x62, 0x63, 0x64, 0x30, 0x39, 0x0a', '};',
	  \ 'unsigned int XXDFILE_LEN = 11;']
    call assert_equal(expected, getline(1,'$'), s:Mess(s:test))
  endfor

  " Test 9: Create a file with containing a single 'A'
  let s:test += 1
  call delete('XXDfile')
  bwipe! XXDfile
  if has('unix')
    call system('echo "010000: 41"|' . s:xxd_cmd . ' -r -s -0x10000 > XXDfile')
  else
    call writefile(['010000: 41'], 'Xinput')
    silent exe '!' . s:xxd_cmd . ' -r -s -0x10000 < Xinput > XXDfile'
    call delete('Xinput')
  endif
  call PrepareBuffer(readfile('XXDfile')[0])
  call assert_equal('A', getline(1), s:Mess(s:test))
  call delete('XXDfile')

  " Test 10: group with 4 octets
  let s:test += 1
  for arg in ['-g 4', '-group 4', '-g4']
    call writefile(['TESTabcd09'], 'XXDfile')
    %d
    exe '0r! ' . s:xxd_cmd . ' ' . arg . ' XXDfile'
    $d
    let expected = ['00000000: 54455354 61626364 30390a             TESTabcd09.']
    call assert_equal(expected, getline(1,'$'), s:Mess(s:test))
    call delete('XXDfile')
  endfor

  " Test 11: reverse with CR, hex upper, Postscript style with a TAB
  let s:test += 1
  call writefile([" 54455354\t610B6364 30390A             TESTa\0x0bcd09.\r"], 'Xinput')
  silent exe '!' . s:xxd_cmd . ' -r -p < Xinput > XXDfile'
  let blob = readfile('XXDfile', 'B')
  call assert_equal(0z54455354.610B6364.30390A, blob)
  call delete('Xinput')
  call delete('XXDfile')

  " Test 12: reverse with seek
  let s:test += 1
  call writefile(["00000000: 54455354\t610B6364 30390A             TESTa\0x0bcd09.\r"], 'Xinput')
  silent exe '!' . s:xxd_cmd . ' -r -seek 5 < Xinput > XXDfile'
  let blob = readfile('XXDfile', 'B')
  call assert_equal(0z0000000000.54455354.610B6364.30390A, blob)
  call delete('Xinput')
  call delete('XXDfile')

  " TODO:
  " -o -offset

  %d
  bw!
endfunc

" Various ways with wrong arguments that trigger the usage output.
func Test_xxd_usage()
  for arg in ['-c', '-g', '-o', '-s', '-l', '-X', 'one two three']
    new
    exe 'r! ' . s:xxd_cmd . ' ' . arg
    call assert_match("Usage:", join(getline(1, 3)))
    bwipe!
  endfor
endfunc

func Test_xxd_version()
  new
  exe 'r! ' . s:xxd_cmd . ' -v'
  call assert_match("xxd V1.10 .* by Juergen Weigert", join(getline(1, 3)))
  bwipe!
endfunc
