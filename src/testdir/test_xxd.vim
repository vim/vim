" Test for the xxd command

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

" Various ways with wrong arguments that trigger the usage output.
func Test_xxd_usage()
  for arg in ['-h', '-c', '-g', '-o', '-s', '-l', '-X', 'one two three']
    new
    exe 'r! ' . s:xxd_cmd . ' ' . arg
    call assert_match("Usage:", join(getline(1, 3)))
    bwipe!
  endfor
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

" vim: shiftwidth=2 sts=2 expandtab
