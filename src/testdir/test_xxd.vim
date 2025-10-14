" Test for the xxd command

source util/screendump.vim

if empty($XXD) && executable('..\xxd\xxd.exe')
  let s:xxd_cmd = '..\xxd\xxd.exe'
elseif empty($XXD) || !executable($XXD)
  throw 'Skipped: xxd program missing'
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
      \ '2e544820585844203120224d6179203230323422',
      \ '20224d616e75616c207061676520666f72207878',
      \ '64220a2e5c220a2e5c222032317374204d617920',
      \ '313939360a2e5c22204d616e2070616765206175',
      \ '74686f723a0a2e5c2220202020546f6e79204e75',
      \ '67656e74203c746f6e79407363746e7567656e2e']
  call assert_equal(expected, getline(1,'$'), s:Mess(s:test))

  " Test 6: Print the date from xxd.1
  let s:test += 1
  for arg in ['-l 13', '-l13', '-len 13']
    %d
    exe '0r! ' . s:xxd_cmd . ' -s 0x33 ' . arg . ' -cols 13 ' . man_copy
    $d
    call assert_equal('00000033: 3231 7374 204d 6179 2031 3939 36  21st May 1996', getline(1), s:Mess(s:test))
  endfor

  " Cleanup after tests 5 and 6
  call delete(man_copy)

  " Test 7: Print C include
  let s:test += 1
  call writefile(['TESTabcd09'], 'XXDfile')
  %d
  exe '0r! ' . s:xxd_cmd . ' -i XXDfile'
  $d
  let expected =<< trim [CODE]
    unsigned char XXDfile[] = {
      0x54, 0x45, 0x53, 0x54, 0x61, 0x62, 0x63, 0x64, 0x30, 0x39, 0x0a
    };
    unsigned int XXDfile_len = 11;
  [CODE]

  call assert_equal(expected, getline(1,'$'), s:Mess(s:test))

  " Test 8: Print C include capitalized
  let s:test += 1
  for arg in ['-C', '-capitalize']
    call writefile(['TESTabcd09'], 'XXDfile')
    %d
    exe '0r! ' . s:xxd_cmd . ' -i ' . arg . ' XXDfile'
    $d
    let expected =<< trim [CODE]
      unsigned char XXDFILE[] = {
        0x54, 0x45, 0x53, 0x54, 0x61, 0x62, 0x63, 0x64, 0x30, 0x39, 0x0a
      };
      unsigned int XXDFILE_LEN = 11;
    [CODE]
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

  " Test 13: simple, decimal offset
  call PrepareBuffer(range(1,30))
  set ff=unix
  w! XXDfile
  let s:test += 1
  exe '%!' . s:xxd_cmd . ' -d %'
  let expected = [
        \ '00000000: 310a 320a 330a 340a 350a 360a 370a 380a  1.2.3.4.5.6.7.8.',
        \ '00000016: 390a 3130 0a31 310a 3132 0a31 330a 3134  9.10.11.12.13.14',
        \ '00000032: 0a31 350a 3136 0a31 370a 3138 0a31 390a  .15.16.17.18.19.',
        \ '00000048: 3230 0a32 310a 3232 0a32 330a 3234 0a32  20.21.22.23.24.2',
        \ '00000064: 350a 3236 0a32 370a 3238 0a32 390a 3330  5.26.27.28.29.30',
        \ '00000080: 0a                                       .']
  call assert_equal(expected, getline(1,'$'), s:Mess(s:test))

  " Test 14: grouping with -d
  let s:test += 1
  let expected = [
        \ '00000000: 310a320a 330a340a 350a360a 370a380a  1.2.3.4.5.6.7.8.',
        \ '00000016: 390a3130 0a31310a 31320a31 330a3134  9.10.11.12.13.14',
        \ '00000032: 0a31350a 31360a31 370a3138 0a31390a  .15.16.17.18.19.',
        \ '00000048: 32300a32 310a3232 0a32330a 32340a32  20.21.22.23.24.2',
        \ '00000064: 350a3236 0a32370a 32380a32 390a3330  5.26.27.28.29.30',
        \ '00000080: 0a                                   .']
  for arg in ['-g 4', '-group 4', '-g4']
    exe '%!' . s:xxd_cmd . ' ' . arg . ' -d %'
    call assert_equal(expected, getline(1,'$'), s:Mess(s:test))
  endfor

  " Test 15: cols with decimal offset: -c 21 -d
  let s:test += 1
  let expected = [
        \ '00000000: 310a 320a 330a 340a 350a 360a 370a 380a 390a 3130 0a  1.2.3.4.5.6.7.8.9.10.',
        \ '00000021: 3131 0a31 320a 3133 0a31 340a 3135 0a31 360a 3137 0a  11.12.13.14.15.16.17.',
        \ '00000042: 3138 0a31 390a 3230 0a32 310a 3232 0a32 330a 3234 0a  18.19.20.21.22.23.24.',
        \ '00000063: 3235 0a32 360a 3237 0a32 380a 3239 0a33 300a          25.26.27.28.29.30.']
  exe '%!' . s:xxd_cmd . ' -c 21 -d %'
  call assert_equal(expected, getline(1,'$'), s:Mess(s:test))

  " Test 16: -o -offset
  let s:test += 1
  let expected = [
        \ '0000000f: 310a 320a 330a 340a 350a 360a 370a 380a  1.2.3.4.5.6.7.8.',
        \ '0000001f: 390a 3130 0a31 310a 3132 0a31 330a 3134  9.10.11.12.13.14',
        \ '0000002f: 0a31 350a 3136 0a31 370a 3138 0a31 390a  .15.16.17.18.19.',
        \ '0000003f: 3230 0a32 310a 3232 0a32 330a 3234 0a32  20.21.22.23.24.2',
        \ '0000004f: 350a 3236 0a32 370a 3238 0a32 390a 3330  5.26.27.28.29.30',
        \ '0000005f: 0a                                       .']
  for arg in ['-o 15', '-offset 15', '-o15']
    exe '%!' . s:xxd_cmd . ' ' . arg . ' %'
    call assert_equal(expected, getline(1,'$'), s:Mess(s:test))
  endfor

  " Test 17: Print C include with custom variable name
  let s:test += 1
  call writefile(['TESTabcd09'], 'XXDfile')
  for arg in ['-nvarName', '-n varName', '-name varName']
    %d
    exe '0r! ' . s:xxd_cmd . ' -i ' . arg . ' XXDfile'
    $d
    let expected =<< trim [CODE]
      unsigned char varName[] = {
        0x54, 0x45, 0x53, 0x54, 0x61, 0x62, 0x63, 0x64, 0x30, 0x39, 0x0a
      };
      unsigned int varName_len = 11;
    [CODE]

    call assert_equal(expected, getline(1,'$'), s:Mess(s:test))
  endfor

  " using "-n name" reading from stdin
  %d
  exe '0r! ' . s:xxd_cmd . ' -i < XXDfile -n StdIn'
  $d
  let expected =<< trim [CODE]
    unsigned char StdIn[] = {
      0x54, 0x45, 0x53, 0x54, 0x61, 0x62, 0x63, 0x64, 0x30, 0x39, 0x0a
    };
    unsigned int StdIn_len = 11;
  [CODE]
  call assert_equal(expected, getline(1,'$'), s:Mess(s:test))


  " Test 18: Print C include: custom variable names can be capitalized
  let s:test += 1
  for arg in ['-C', '-capitalize']
    call writefile(['TESTabcd09'], 'XXDfile')
    %d
    exe '0r! ' . s:xxd_cmd . ' -i ' . arg . ' -n varName XXDfile'
    $d
    let expected =<< trim [CODE]
      unsigned char VARNAME[] = {
        0x54, 0x45, 0x53, 0x54, 0x61, 0x62, 0x63, 0x64, 0x30, 0x39, 0x0a
      };
      unsigned int VARNAME_LEN = 11;
    [CODE]
    call assert_equal(expected, getline(1,'$'), s:Mess(s:test))
  endfor


  " Test 19: Print C include in binary format
  let s:test += 1
  call writefile(['TESTabcd09'], 'XXDfile')
  %d
  exe '0r! ' . s:xxd_cmd . ' -i -b XXDfile'
  $d
  let expected =<< trim [CODE]
    unsigned char XXDfile[] = {
      0b01010100, 0b01000101, 0b01010011, 0b01010100, 0b01100001, 0b01100010,
      0b01100011, 0b01100100, 0b00110000, 0b00111001, 0b00001010
    };
    unsigned int XXDfile_len = 11;
  [CODE]

  call assert_equal(expected, getline(1,'$'), s:Mess(s:test))


  %d
  bwipe!
  call delete('XXDfile')
endfunc

func Test_xxd_patch()
  let cmd1 = 'silent !' .. s:xxd_cmd .. ' -r Xxxdin Xxxdfile'
  let cmd2 = 'silent !' .. s:xxd_cmd .. ' -g1 Xxxdfile > Xxxdout'
  call writefile(["2: 41 41", "8: 42 42"], 'Xxxdin', 'D')
  call writefile(['::::::::'], 'Xxxdfile', 'D')
  exe cmd1
  exe cmd2
  call assert_equal(['00000000: 3a 3a 41 41 3a 3a 3a 3a 42 42                    ::AA::::BB'], readfile('Xxxdout'))

  call writefile(["2: 43 43 ", "8: 44 44"], 'Xxxdin')
  exe cmd1
  exe cmd2
  call assert_equal(['00000000: 3a 3a 43 43 3a 3a 3a 3a 44 44                    ::CC::::DD'], readfile('Xxxdout'))

  call writefile(["2: 45 45  ", "8: 46 46"], 'Xxxdin')
  exe cmd1
  exe cmd2
  call assert_equal(['00000000: 3a 3a 45 45 3a 3a 3a 3a 46 46                    ::EE::::FF'], readfile('Xxxdout'))

  call writefile(["2: 41 41", "08: 42 42"], 'Xxxdin')
  call writefile(['::::::::'], 'Xxxdfile')
  exe cmd1
  exe cmd2
  call assert_equal(['00000000: 3a 3a 41 41 3a 3a 3a 3a 42 42                    ::AA::::BB'], readfile('Xxxdout'))

  call writefile(["2: 43 43 ", "09: 44 44"], 'Xxxdin')
  exe cmd1
  exe cmd2
  call assert_equal(['00000000: 3a 3a 43 43 3a 3a 3a 3a 42 44 44                 ::CC::::BDD'], readfile('Xxxdout'))

  call writefile(["2: 45 45  ", "0a: 46 46"], 'Xxxdin')
  exe cmd1
  exe cmd2
  call assert_equal(['00000000: 3a 3a 45 45 3a 3a 3a 3a 42 44 46 46              ::EE::::BDFF'], readfile('Xxxdout'))

  call delete('Xxxdout')
endfunc

func Test_xxd_patch_with_bitdump()
  let cmd1 = 'silent !' .. s:xxd_cmd .. ' -r -b Xxxdin Xxxdfile'
  let cmd2 = 'silent !' .. s:xxd_cmd .. ' -g1 Xxxdfile > Xxxdout'

  call writefile(["2: 01000001 01000001", "8: 01000010 01000010"], 'Xxxdin', 'D')
  call writefile(['::::::::'], 'Xxxdfile', 'D')
  exe cmd1
  exe cmd2
  call assert_equal(['00000000: 3a 3a 41 41 3a 3a 3a 3a 42 42                    ::AA::::BB'], readfile('Xxxdout'))

  call writefile(["1: 01000011 01000011", "4: 01000100 01000100"], 'Xxxdin', 'D')
  call writefile(['::::::::'], 'Xxxdfile', 'D')
  exe cmd1
  exe cmd2
  call assert_equal(['00000000: 3a 43 43 3a 44 44 3a 3a 0a                       :CC:DD::.'], readfile('Xxxdout'))

  call writefile(["02: 01000101 01000101", "08: 01000110 01000110"], 'Xxxdin', 'D')
  call writefile(['::::::::'], 'Xxxdfile', 'D')
  exe cmd1
  exe cmd2
  call assert_equal(['00000000: 3a 3a 45 45 3a 3a 3a 3a 46 46                    ::EE::::FF'], readfile('Xxxdout'))

  call delete('Xxxdout')
endfunc

" Various ways with wrong arguments that trigger the usage output.
func Test_xxd_usage()
  for arg in ['-h', '-c', '-g', '-o', '-s', '-l', '-X', '-R', 'one two three', '----', '---']
    new
    exe 'r! ' . s:xxd_cmd . ' ' . arg
    call assert_match("Usage:", join(getline(1, 3)))
    bwipe!
  endfor
endfunc

func Test_xxd_end_of_options()
  new
  exe 'r! ' . s:xxd_cmd . ' -- random-file-' . rand()
  call assert_match('random-file-.*: No such file or directory', join(getline(1, 3)))
  bwipe!
endfunc

func Test_xxd_ignore_garbage()
  new
  exe 'r! printf "\n\r xxxx 0: 42 42" | ' . s:xxd_cmd . ' -r'
  call assert_match('BB', join(getline(1, 3)))
  bwipe!
endfunc

func Test_xxd_bit_dump()
  new
  exe 'r! printf "123456" | ' . s:xxd_cmd . ' -b1'
  call assert_match('00000000: 00110001 00110010 00110011 00110100 00110101 00110110  123456', join(getline(1, 3)))
  bwipe!
endfunc

func Test_xxd_revert_bit_dump()
  new
  exe 'r! printf "00000000: 01000001 01100010 01000011 01100100 01000101 01100110 01000111 01101000  AbCdEfGh" | ' . s:xxd_cmd . ' -r -b1 -c 8'
  call assert_match('AbCdEfGh', join(getline(1, 3)))
  bwipe!

  new
  exe 'r! printf "00000000: 01000001 01100010 01000011 01100100 01000101 01100110  AbCdEf\n00000006: 01000111 01101000                                      Gh\n" | ' . s:xxd_cmd . ' -r -b1'
  call assert_match('AbCdEfGh', join(getline(1, 3)))
  bwipe!
endfunc

func Test_xxd_roundtrip_large_bit_dump()
  new
  exe 'r! printf "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789" | ' . s:xxd_cmd . ' -b | ' . s:xxd_cmd . ' -r -b'
  call assert_match('abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ012345678', join(getline(1, 3)))
  bwipe!
endfunc

func Test_xxd_version()
  new
  exe 'r! ' . s:xxd_cmd . ' -v'
  call assert_match('xxd 20\d\d-\d\d-\d\d by Juergen Weigert et al\.', join(getline(1, 3)))
  bwipe!
endfunc

" number of columns must be non-negative
func Test_xxd_min_cols()
  for cols in ['-c-1', '-c -1', '-cols -1']
    for fmt in ['', '-b', '-e', '-i', '-p', ]
      new
      exe 'r! printf "ignored" | ' . s:xxd_cmd . ' ' . cols . ' ' . fmt
      call assert_match("invalid number of columns", join(getline(1, '$')))
      bwipe!
    endfor
  endfor
endfunc

" some hex formats limit columns to 256 (a #define in xxd.c)
func Test_xxd_max_cols()
  for cols in ['-c257', '-c 257', '-cols 257']
    for fmt in ['', '-b', '-e' ]
      new
      exe 'r! printf "ignored" | ' . s:xxd_cmd . ' ' . cols . ' ' . fmt
      call assert_match("invalid number of columns", join(getline(1, '$')))
      bwipe!
    endfor
  endfor
endfunc


" This used to trigger a buffer overflow (#14738)
func Test_xxd_buffer_overflow()
  CheckUnix
  if system('file ' .. s:xxd_cmd) =~ '32-bit'
    throw 'Skipped: test only works on 64-bit architecture'
  endif
  new
  let input = repeat('A', 256)
  call writefile(['-9223372036854775808: ' . repeat("\e[1;32m41\e[0m ", 256) . ' ' . "\e[1;32m" . repeat('A', 256) . "\e[0m"], 'Xxdexpected', 'D')
  exe 'r! printf ' . input . '| ' . s:xxd_cmd . ' -Ralways -g1 -c256 -d -o 9223372036854775808 > Xxdout'
  call assert_equalfile('Xxdexpected', 'Xxdout')
  call delete('Xxdout')
  bwipe!
endfunc

" -c0 selects the format specific default column value, as if no -c was given
" except for -ps, where it disables extra newlines
func Test_xxd_c0_is_def_cols()
  call writefile(["abcdefghijklmnopqrstuvwxyz0123456789"], 'Xxdin', 'D')
  for cols in ['-c0', '-c 0', '-cols 0']
    for fmt in ['', '-b', '-e', '-i']
      exe 'r! ' . s:xxd_cmd . ' ' . fmt ' Xxdin > Xxdout1'
      exe 'r! ' . s:xxd_cmd . ' ' . cols . ' ' . fmt ' Xxdin > Xxdout2'
      call assert_equalfile('Xxdout1', 'Xxdout2')
    endfor
  endfor
  call delete('Xxdout1')
  call delete('Xxdout2')
endfunc

" all output in a single line for -c0 -ps
func Test_xxd_plain_one_line()
  call writefile([
        \ "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",
        \ "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",
        \ "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",
        \ "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",
        \ "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",
        \ "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"],
        \ 'Xxdin', 'D')
  for cols in ['-c0', '-c 0', '-cols 0']
    exe 'r! ' . s:xxd_cmd . ' -ps ' . cols ' Xxdin'
    " output seems to start in line 2
    let out = join(getline(2, '$'))
    bwipe!
    " newlines in xxd output result in spaces in the string variable out
    call assert_notmatch(" ", out)
    " xxd output must be non-empty and comprise only lower case hex digits
    call assert_match("^[0-9a-f][0-9a-f]*$", out)
  endfor
endfunc

func Test_xxd_little_endian_with_cols()
  enew!
  call writefile(["ABCDEF"], 'Xxdin', 'D')
  exe 'r! ' .. s:xxd_cmd .. ' -e -c6 ' .. ' Xxdin'
  call assert_equal('00000000: 44434241     4645  ABCDEF', getline(2))

  enew!
  call writefile(["ABCDEFGHI"], 'Xxdin', 'D')
  exe 'r! ' .. s:xxd_cmd .. ' -e -c9 ' .. ' Xxdin'
  call assert_equal('00000000: 44434241 48474645       49  ABCDEFGHI', getline(2))

  bwipe!
endfunc

func Test_xxd_color()
"Test: color=never
let s:test = 1

"Note Quotation mark escaped
"Note Aposhpere vaihdettu apostrophe replaced with 0x00
"Note Backslash replaced with 0x00
let data = [
    \ "00000000: 0001 0203 0405 0607 0809 0a0b 0c0d 0e0f  ................",
    \ "00000010: 1011 1213 1415 1617 1819 1a1b 1c1d 1e1f  ................",
    \ "00000020: 2021 2223 2425 2600 2829 2a2b 2c2d 2e2f   !\"#$%&.()*+,-./",
    \ "00000030: 3031 3233 3435 3637 3839 3a3b 3c3d 3e3f  0123456789:;<=>?",
    \ "00000040: 4041 4243 4445 4647 4849 4a4b 4c4d 4e4f  @ABCDEFGHIJKLMNO",
    \ "00000050: 5051 5253 5455 5657 5859 5a5b 005d 5e5f  PQRSTUVWXYZ[.]^_",
    \ "00000060: 6061 6263 6465 6667 6869 6a6b 6c6d 6e6f  `abcdefghijklmno",
    \ "00000070: 7071 7273 7475 7677 7879 7a7b 7c7d 7e7f  pqrstuvwxyz{|}~.",
    \ "00000080: 8081 8283 8485 8687 8889 8a8b 8c8d 8e8f  ................",
    \ "00000090: 9091 9293 9495 9697 9899 9a9b 9c9d 9e9f  ................",
    \ "000000a0: a0a1 a2a3 a4a5 a6a7 a8a9 aaab acad aeaf  ................",
    \ "000000b0: b0b1 b2b3 b4b5 b6b7 b8b9 babb bcbd bebf  ................",
    \ "000000c0: c0c1 c2c3 c4c5 c6c7 c8c9 cacb cccd cecf  ................",
    \ "000000d0: d0d1 d2d3 d4d5 d6d7 d8d9 dadb dcdd dedf  ................",
    \ "000000e0: e0e1 e2e3 e4e5 e6e7 e8e9 eaeb eced eeef  ................",
    \ "000000f0: f0f1 f2f3 f4f5 f6f7 f8f9 fafb fcfd feff  ................"]
call writefile(data,'Xinput')

  silent exe '!' . s:xxd_cmd . ' -r < Xinput > XXDfile'

  %d
  exe '0r! ' . s:xxd_cmd . ' -R never ' . ' XXDfile'
  $d
  let expected = [
      \ "00000000: 0001 0203 0405 0607 0809 0a0b 0c0d 0e0f  ................",
      \ "00000010: 1011 1213 1415 1617 1819 1a1b 1c1d 1e1f  ................",
      \ "00000020: 2021 2223 2425 2600 2829 2a2b 2c2d 2e2f   !\"#$%&.()*+,-./",
      \ "00000030: 3031 3233 3435 3637 3839 3a3b 3c3d 3e3f  0123456789:;<=>?",
      \ "00000040: 4041 4243 4445 4647 4849 4a4b 4c4d 4e4f  @ABCDEFGHIJKLMNO",
      \ "00000050: 5051 5253 5455 5657 5859 5a5b 005d 5e5f  PQRSTUVWXYZ[.]^_",
      \ "00000060: 6061 6263 6465 6667 6869 6a6b 6c6d 6e6f  `abcdefghijklmno",
      \ "00000070: 7071 7273 7475 7677 7879 7a7b 7c7d 7e7f  pqrstuvwxyz{|}~.",
      \ "00000080: 8081 8283 8485 8687 8889 8a8b 8c8d 8e8f  ................",
      \ "00000090: 9091 9293 9495 9697 9899 9a9b 9c9d 9e9f  ................",
      \ "000000a0: a0a1 a2a3 a4a5 a6a7 a8a9 aaab acad aeaf  ................",
      \ "000000b0: b0b1 b2b3 b4b5 b6b7 b8b9 babb bcbd bebf  ................",
      \ "000000c0: c0c1 c2c3 c4c5 c6c7 c8c9 cacb cccd cecf  ................",
      \ "000000d0: d0d1 d2d3 d4d5 d6d7 d8d9 dadb dcdd dedf  ................",
      \ "000000e0: e0e1 e2e3 e4e5 e6e7 e8e9 eaeb eced eeef  ................",
      \ "000000f0: f0f1 f2f3 f4f5 f6f7 f8f9 fafb fcfd feff  ................"]

  call assert_equal(expected, getline(1,'$'), s:Mess(s:test))

  "Test: color=always
  let s:test += 1

  %d
  exe '0r! ' . s:xxd_cmd . ' -R always -c 4 ' . ' XXDfile'
  $d
  let expected = [
      \ "00000000: \e[1;37m00\e[0m\e[1;31m01\e[0m \e[1;31m0203\e[0m  \e[1;37m.\e[0m\e[1;31m...\e[0m",
      \ "00000004: \e[1;31m0405\e[0m \e[1;31m0607\e[0m  \e[1;31m....\e[0m",
      \ "00000008: \e[1;31m08\e[0m\e[1;33m09\e[0m \e[1;33m0a\e[0m\e[1;31m0b\e[0m  \e[1;31m.\e[0m\e[1;33m..\e[0m\e[1;31m.\e[0m",
      \ "0000000c: \e[1;31m0c\e[0m\e[1;33m0d\e[0m \e[1;31m0e0f\e[0m  \e[1;31m.\e[0m\e[1;33m.\e[0m\e[1;31m..\e[0m",
      \ "00000010: \e[1;31m1011\e[0m \e[1;31m1213\e[0m  \e[1;31m....\e[0m",
      \ "00000014: \e[1;31m1415\e[0m \e[1;31m1617\e[0m  \e[1;31m....\e[0m",
      \ "00000018: \e[1;31m1819\e[0m \e[1;31m1a1b\e[0m  \e[1;31m....\e[0m",
      \ "0000001c: \e[1;31m1c1d\e[0m \e[1;31m1e1f\e[0m  \e[1;31m....\e[0m",
      \ "00000020: \e[1;32m2021\e[0m \e[1;32m2223\e[0m  \e[1;32m !\"#\e[0m",
      \ "00000024: \e[1;32m2425\e[0m \e[1;32m26\e[0m\e[1;37m00\e[0m  \e[1;32m$%&\e[0m\e[1;37m.\e[0m",
      \ "00000028: \e[1;32m2829\e[0m \e[1;32m2a2b\e[0m  \e[1;32m()*+\e[0m",
      \ "0000002c: \e[1;32m2c2d\e[0m \e[1;32m2e2f\e[0m  \e[1;32m,-./\e[0m",
      \ "00000030: \e[1;32m3031\e[0m \e[1;32m3233\e[0m  \e[1;32m0123\e[0m",
      \ "00000034: \e[1;32m3435\e[0m \e[1;32m3637\e[0m  \e[1;32m4567\e[0m",
      \ "00000038: \e[1;32m3839\e[0m \e[1;32m3a3b\e[0m  \e[1;32m89:;\e[0m",
      \ "0000003c: \e[1;32m3c3d\e[0m \e[1;32m3e3f\e[0m  \e[1;32m<=>?\e[0m",
      \ "00000040: \e[1;32m4041\e[0m \e[1;32m4243\e[0m  \e[1;32m@ABC\e[0m",
      \ "00000044: \e[1;32m4445\e[0m \e[1;32m4647\e[0m  \e[1;32mDEFG\e[0m",
      \ "00000048: \e[1;32m4849\e[0m \e[1;32m4a4b\e[0m  \e[1;32mHIJK\e[0m",
      \ "0000004c: \e[1;32m4c4d\e[0m \e[1;32m4e4f\e[0m  \e[1;32mLMNO\e[0m",
      \ "00000050: \e[1;32m5051\e[0m \e[1;32m5253\e[0m  \e[1;32mPQRS\e[0m",
      \ "00000054: \e[1;32m5455\e[0m \e[1;32m5657\e[0m  \e[1;32mTUVW\e[0m",
      \ "00000058: \e[1;32m5859\e[0m \e[1;32m5a5b\e[0m  \e[1;32mXYZ[\e[0m",
      \ "0000005c: \e[1;37m00\e[0m\e[1;32m5d\e[0m \e[1;32m5e5f\e[0m  \e[1;37m.\e[0m\e[1;32m]^_\e[0m",
      \ "00000060: \e[1;32m6061\e[0m \e[1;32m6263\e[0m  \e[1;32m`abc\e[0m",
      \ "00000064: \e[1;32m6465\e[0m \e[1;32m6667\e[0m  \e[1;32mdefg\e[0m",
      \ "00000068: \e[1;32m6869\e[0m \e[1;32m6a6b\e[0m  \e[1;32mhijk\e[0m",
      \ "0000006c: \e[1;32m6c6d\e[0m \e[1;32m6e6f\e[0m  \e[1;32mlmno\e[0m",
      \ "00000070: \e[1;32m7071\e[0m \e[1;32m7273\e[0m  \e[1;32mpqrs\e[0m",
      \ "00000074: \e[1;32m7475\e[0m \e[1;32m7677\e[0m  \e[1;32mtuvw\e[0m",
      \ "00000078: \e[1;32m7879\e[0m \e[1;32m7a7b\e[0m  \e[1;32mxyz{\e[0m",
      \ "0000007c: \e[1;32m7c7d\e[0m \e[1;32m7e\e[0m\e[1;31m7f\e[0m  \e[1;32m|}~\e[0m\e[1;31m.\e[0m",
      \ "00000080: \e[1;31m8081\e[0m \e[1;31m8283\e[0m  \e[1;31m....\e[0m",
      \ "00000084: \e[1;31m8485\e[0m \e[1;31m8687\e[0m  \e[1;31m....\e[0m",
      \ "00000088: \e[1;31m8889\e[0m \e[1;31m8a8b\e[0m  \e[1;31m....\e[0m",
      \ "0000008c: \e[1;31m8c8d\e[0m \e[1;31m8e8f\e[0m  \e[1;31m....\e[0m",
      \ "00000090: \e[1;31m9091\e[0m \e[1;31m9293\e[0m  \e[1;31m....\e[0m",
      \ "00000094: \e[1;31m9495\e[0m \e[1;31m9697\e[0m  \e[1;31m....\e[0m",
      \ "00000098: \e[1;31m9899\e[0m \e[1;31m9a9b\e[0m  \e[1;31m....\e[0m",
      \ "0000009c: \e[1;31m9c9d\e[0m \e[1;31m9e9f\e[0m  \e[1;31m....\e[0m",
      \ "000000a0: \e[1;31ma0a1\e[0m \e[1;31ma2a3\e[0m  \e[1;31m....\e[0m",
      \ "000000a4: \e[1;31ma4a5\e[0m \e[1;31ma6a7\e[0m  \e[1;31m....\e[0m",
      \ "000000a8: \e[1;31ma8a9\e[0m \e[1;31maaab\e[0m  \e[1;31m....\e[0m",
      \ "000000ac: \e[1;31macad\e[0m \e[1;31maeaf\e[0m  \e[1;31m....\e[0m",
      \ "000000b0: \e[1;31mb0b1\e[0m \e[1;31mb2b3\e[0m  \e[1;31m....\e[0m",
      \ "000000b4: \e[1;31mb4b5\e[0m \e[1;31mb6b7\e[0m  \e[1;31m....\e[0m",
      \ "000000b8: \e[1;31mb8b9\e[0m \e[1;31mbabb\e[0m  \e[1;31m....\e[0m",
      \ "000000bc: \e[1;31mbcbd\e[0m \e[1;31mbebf\e[0m  \e[1;31m....\e[0m",
      \ "000000c0: \e[1;31mc0c1\e[0m \e[1;31mc2c3\e[0m  \e[1;31m....\e[0m",
      \ "000000c4: \e[1;31mc4c5\e[0m \e[1;31mc6c7\e[0m  \e[1;31m....\e[0m",
      \ "000000c8: \e[1;31mc8c9\e[0m \e[1;31mcacb\e[0m  \e[1;31m....\e[0m",
      \ "000000cc: \e[1;31mcccd\e[0m \e[1;31mcecf\e[0m  \e[1;31m....\e[0m",
      \ "000000d0: \e[1;31md0d1\e[0m \e[1;31md2d3\e[0m  \e[1;31m....\e[0m",
      \ "000000d4: \e[1;31md4d5\e[0m \e[1;31md6d7\e[0m  \e[1;31m....\e[0m",
      \ "000000d8: \e[1;31md8d9\e[0m \e[1;31mdadb\e[0m  \e[1;31m....\e[0m",
      \ "000000dc: \e[1;31mdcdd\e[0m \e[1;31mdedf\e[0m  \e[1;31m....\e[0m",
      \ "000000e0: \e[1;31me0e1\e[0m \e[1;31me2e3\e[0m  \e[1;31m....\e[0m",
      \ "000000e4: \e[1;31me4e5\e[0m \e[1;31me6e7\e[0m  \e[1;31m....\e[0m",
      \ "000000e8: \e[1;31me8e9\e[0m \e[1;31meaeb\e[0m  \e[1;31m....\e[0m",
      \ "000000ec: \e[1;31meced\e[0m \e[1;31meeef\e[0m  \e[1;31m....\e[0m",
      \ "000000f0: \e[1;31mf0f1\e[0m \e[1;31mf2f3\e[0m  \e[1;31m....\e[0m",
      \ "000000f4: \e[1;31mf4f5\e[0m \e[1;31mf6f7\e[0m  \e[1;31m....\e[0m",
      \ "000000f8: \e[1;31mf8f9\e[0m \e[1;31mfafb\e[0m  \e[1;31m....\e[0m",
      \ "000000fc: \e[1;31mfcfd\e[0m \e[1;31mfe\e[0m\e[1;34mff\e[0m  \e[1;31m...\e[0m\e[1;34m.\e[0m"]
  call assert_equal(expected, getline(1,'$'), s:Mess(s:test))

  call delete('Xinput')
  call delete('XXDfile')

endfunc

func Test_xxd_color2()
  CheckScreendump
  CheckUnix
  CheckNotMac
  CheckNotBSD
  CheckExecutable dash

  "Note Quotation mark escaped
  "Note Aposhpere vaihdettu apostrophe replaced with 0x00
  "Note Backslash replaced with 0x00
  let data = [
      \ "00000000: 0001 0203 0405 0607 0809 0a0b 0c0d 0e0f  ................",
      \ "00000010: 1011 1213 1415 1617 1819 1a1b 1c1d 1e1f  ................",
      \ "00000020: 2021 2223 2425 2600 2829 2a2b 2c2d 2e2f   !\"#$%&.()*+,-./",
      \ "00000030: 3031 3233 3435 3637 3839 3a3b 3c3d 3e3f  0123456789:;<=>?",
      \ "00000040: 4041 4243 4445 4647 4849 4a4b 4c4d 4e4f  @ABCDEFGHIJKLMNO",
      \ "00000050: 5051 5253 5455 5657 5859 5a5b 005d 5e5f  PQRSTUVWXYZ[.]^_",
      \ "00000060: 6061 6263 6465 6667 6869 6a6b 6c6d 6e6f  `abcdefghijklmno",
      \ "00000070: 7071 7273 7475 7677 7879 7a7b 7c7d 7e7f  pqrstuvwxyz{|}~.",
      \ "00000080: 8081 8283 8485 8687 8889 8a8b 8c8d 8e8f  ................",
      \ "00000090: 9091 9293 9495 9697 9899 9a9b 9c9d 9e9f  ................",
      \ "000000a0: a0a1 a2a3 a4a5 a6a7 a8a9 aaab acad aeaf  ................",
      \ "000000b0: b0b1 b2b3 b4b5 b6b7 b8b9 babb bcbd bebf  ................",
      \ "000000c0: c0c1 c2c3 c4c5 c6c7 c8c9 cacb cccd cecf  ................",
      \ "000000d0: d0d1 d2d3 d4d5 d6d7 d8d9 dadb dcdd dedf  ................",
      \ "000000e0: e0e1 e2e3 e4e5 e6e7 e8e9 eaeb eced eeef  ................",
      \ "000000f0: f0f1 f2f3 f4f5 f6f7 f8f9 fafb fcfd feff  ................"]
  call writefile(data, 'Xinput', 'D')

  call system(s:xxd_cmd .. ' -r < Xinput > XXDfile_colors')

  let $PS1='$ '
  " This needs dash, plain bashs sh does not seem to work :(
  let buf = RunVimInTerminal('', #{rows: 20, cmd: 'sh'})
  call term_sendkeys(buf,  s:xxd_cmd .. " -R never  < XXDfile_colors\<cr>")
  call TermWait(buf)
  redraw
  call VerifyScreenDump(buf, 'Test_xxd_color_0', {})

  call TermWait(buf)
  call term_sendkeys(buf,  "clear\<CR>")
  call term_sendkeys(buf,  s:xxd_cmd .. " -R always  < XXDfile_colors\<cr>")
  call TermWait(buf)
  call VerifyScreenDump(buf, 'Test_xxd_color_1', {})

  call term_sendkeys(buf,  "exit\<CR>")

  call delete('XXDfile_colors')
  unlet! $PS1
endfunc

" this caused a buffer overflow
func Test_xxd_overflow()
  CheckUnix
  CheckExecutable /bin/true
  new
  " we are only checking, that there are addresses in the first 5 lines
  let expected = [
        \ '00000000: ',
        \ '00000080: ',
        \ '00000100: ',
        \ '00000180: ',
        \ '00000200: ']
  exe "0r! " s:xxd_cmd "-b -E -c 128 -g 256 /bin/true 2>&1"
  " there should not be an ASAN error message
  call getline(1, '$')->join('\n')->assert_notmatch('runtime error')
  6,$d
  %s/^\x\+: \zs.*//g
  call assert_equal(expected, getline(1, 5))
  bw!
endfunc

" this caused a NULL dereference
func Test_xxd_null_dereference()
  CheckUnix
  CheckExecutable /bin/true
  new
  " we are only checking, that there are addresses in the first 5 lines
  let expected = [
        \ '00000000: ',
        \ '00000010: ',
        \ '00000020: ',
        \ '00000030: ',
        \ '00000040: ']
  exe "0r! " s:xxd_cmd "-a -R never /bin/true 2>&1"
  " there should be more than 6 lines
  call assert_true(line('$') > 5)
  " there should not be an ASAN error message
  call getline(1, '$')->join('\n')->assert_notmatch('runtime error')
  6,$d
  %s/^\x\+: \zs.*//g
  call assert_equal(expected, getline(1, 5))
  bw!
endfunc

" vim: shiftwidth=2 sts=2 expandtab
