" Tests for the :set command

source check.vim

function Test_set_backslash()
  let isk_save = &isk

  set isk=a,b,c
  set isk+=d
  call assert_equal('a,b,c,d', &isk)
  set isk+=\\,e
  call assert_equal('a,b,c,d,\,e', &isk)
  set isk-=e
  call assert_equal('a,b,c,d,\', &isk)
  set isk-=\\
  call assert_equal('a,b,c,d', &isk)

  let &isk = isk_save
endfunction

" :set, :setlocal, :setglobal without arguments show values of options.
func Test_set_no_arg()
  set textwidth=79
  let a = execute('set')
  call assert_match("^\n--- Options ---\n.*textwidth=79\\>", a)
  set textwidth&

  setlocal textwidth=78
  let a = execute('setlocal')
  call assert_match("^\n--- Local option values ---\n.*textwidth=78\\>", a)
  setlocal textwidth&

  setglobal textwidth=77
  let a = execute('setglobal')
  call assert_match("^\n--- Global option values ---\n.*textwidth=77\\>", a)
  setglobal textwidth&
endfunc

" Test for :set all
func Test_set_all()
  set tw=75
  set iskeyword=a-z,A-Z
  set nosplitbelow
  let out = execute('set all')
  call assert_match('textwidth=75', out)
  call assert_match('iskeyword=a-z,A-Z', out)
  call assert_match('nosplitbelow', out)
  set tw& iskeyword& splitbelow&
endfunc

" Test for :set! all
func Test_set_all_one_column()
  let out_mult = execute('set all')->split("\n")
  let out_one = execute('set! all')->split("\n")
  call assert_true(len(out_mult) < len(out_one))
  call assert_equal(out_one[0], '--- Options ---')
  let options = out_one[1:]->mapnew({_, line -> line[2:]})
  call assert_equal(sort(copy(options)), options)
endfunc

" Test for :set termcap
func Test_set_termcap()
  CheckNotGui

  let lines = split(execute('set termcap'), "\n")
  call assert_match('--- Terminal codes ---', lines[0])
  " four columns
  call assert_match('t_..=.*t_..=.*t_..=.*t_..=', lines[1])

  for keys_idx in range(len(lines))
    if lines[keys_idx] =~ '--- Terminal keys ---'
      break
    endif
  endfor
  call assert_true(keys_idx < len(lines))
  " three columns
  call assert_match('<[^>]*> .*<[^>]*> .*<[^>]*> ', lines[keys_idx + 1])

  let more_lines = split(execute('set! termcap'), "\n")
  for i in range(len(more_lines))
    if more_lines[i] =~ '--- Terminal keys ---'
      break
    endif
  endfor
  call assert_true(i < len(more_lines))
  call assert_true(i > keys_idx)
  call assert_true(len(more_lines) - i > len(lines) - keys_idx)
endfunc

" Test for setting string option value
func Test_set_string_option()
  " :set {option}=
  set makeprg=
  call assert_equal('', &mp)
  set makeprg=abc
  call assert_equal('abc', &mp)

  " :set {option}:
  set makeprg:
  call assert_equal('', &mp)
  set makeprg:abc
  call assert_equal('abc', &mp)

  " Let string
  let &makeprg = ''
  call assert_equal('', &mp)
  let &makeprg = 'abc'
  call assert_equal('abc', &mp)

  " Let number
  let &makeprg = 42
  call assert_equal('42', &mp)

  " Appending
  set makeprg=abc
  set makeprg+=def
  call assert_equal('abcdef', &mp)
  set makeprg+=def
  call assert_equal('abcdefdef', &mp, ':set+= appends a value even if it already contained')
  let &makeprg .= 'gh'
  call assert_equal('abcdefdefgh', &mp)
  let &makeprg ..= 'ij'
  call assert_equal('abcdefdefghij', &mp)

  " Removing
  set makeprg=abcdefghi
  set makeprg-=def
  call assert_equal('abcghi', &mp)
  set makeprg-=def
  call assert_equal('abcghi', &mp, ':set-= does not remove a value if it is not contained')

  " Prepending
  set makeprg=abc
  set makeprg^=def
  call assert_equal('defabc', &mp)
  set makeprg^=def
  call assert_equal('defdefabc', &mp, ':set+= prepends a value even if it already contained')

  set makeprg&
endfunc

" Test for setting comma-separated list option value
func Test_set_list_option()
  " :set {option}=
  set tags=
  call assert_equal('', &tags)
  set tags=abc
  call assert_equal('abc', &tags)

  " :set {option}:
  set tags:
  call assert_equal('', &tags)
  set tags:abc
  call assert_equal('abc', &tags)

  " Let string
  let &tags = ''
  call assert_equal('', &tags)
  let &tags = 'abc'
  call assert_equal('abc', &tags)

  " Let number
  let &tags = 42
  call assert_equal('42', &tags)

  " Appending
  set tags=abc
  set tags+=def
  call assert_equal('abc,def', &tags, ':set+= prepends a comma to append a value')
  set tags+=def
  call assert_equal('abc,def', &tags, ':set+= does not append a value if it already contained')
  set tags+=ef
  call assert_equal('abc,def,ef', &tags, ':set+= prepends a comma to append a value if it is not exactly match to item')
  let &tags .= 'gh'
  call assert_equal('abc,def,efgh', &tags, ':let-& .= appends a value without a comma')
  let &tags ..= 'ij'
  call assert_equal('abc,def,efghij', &tags, ':let-& ..= appends a value without a comma')

  " Removing
  set tags=abc,def,ghi
  set tags-=def
  call assert_equal('abc,ghi', &tags)
  set tags-=def
  call assert_equal('abc,ghi', &tags, ':set-= does not remove a value if it is not contained')
  set tags-=bc
  call assert_equal('abc,ghi', &tags, ':set-= does not remove a value if it is not exactly match to item')

  " Prepending
  set tags=abc
  set tags^=def
  call assert_equal('def,abc', &tags)
  set tags^=def
  call assert_equal('def,abc', &tags, ':set+= does not prepend a value if it already contained')
  set tags^=ef
  call assert_equal('ef,def,abc', &tags, ':set+= prepend a value if it is not exactly match to item')

  set tags&
endfunc

" Test for setting flags option value
func Test_set_flags_option()
  " :set {option}=
  set formatoptions=
  call assert_equal('', &fo)
  set formatoptions=abc
  call assert_equal('abc', &fo)

  " :set {option}:
  set formatoptions:
  call assert_equal('', &fo)
  set formatoptions:abc
  call assert_equal('abc', &fo)

  " Let string
  let &formatoptions = ''
  call assert_equal('', &fo)
  let &formatoptions = 'abc'
  call assert_equal('abc', &fo)

  " Let number
  let &formatoptions = 12
  call assert_equal('12', &fo)

  " Appending
  set formatoptions=abc
  set formatoptions+=pqr
  call assert_equal('abcpqr', &fo)
  set formatoptions+=pqr
  call assert_equal('abcpqr', &fo, ':set+= does not append a value if it already contained')
  let &formatoptions .= 'r'
  call assert_equal('abcpqrr', &fo, ':let-& .= appends a value even if it already contained')
  let &formatoptions ..= 'r'
  call assert_equal('abcpqrrr', &fo, ':let-& ..= appends a value even if it already contained')

  " Removing
  set formatoptions=abcpqr
  set formatoptions-=cp
  call assert_equal('abqr', &fo)
  set formatoptions-=cp
  call assert_equal('abqr', &fo, ':set-= does not remove a value if it is not contained')
  set formatoptions-=ar
  call assert_equal('abqr', &fo, ':set-= does not remove a value if it is not exactly match')

  " Prepending
  set formatoptions=abc
  set formatoptions^=pqr
  call assert_equal('pqrabc', &fo)
  set formatoptions^=qr
  call assert_equal('pqrabc', &fo, ':set+= does not prepend a value if it already contained')

  set formatoptions&
endfunc

" Test for setting number option value
func Test_set_number_option()
  " :set {option}=
  set scrolljump=5
  call assert_equal(5, &sj)
  set scrolljump=-3
  call assert_equal(-3, &sj)

  " :set {option}:
  set scrolljump:7
  call assert_equal(7, &sj)
  set scrolljump:-5
  call assert_equal(-5, &sj)

  " Set hex
  set scrolljump=0x10
  call assert_equal(16, &sj)
  set scrolljump=-0x10
  call assert_equal(-16, &sj)
  set scrolljump=0X12
  call assert_equal(18, &sj)
  set scrolljump=-0X12
  call assert_equal(-18, &sj)

  " Set octal
  set scrolljump=010
  call assert_equal(8, &sj)
  set scrolljump=-010
  call assert_equal(-8, &sj)
  set scrolljump=0o12
  call assert_equal(10, &sj)
  set scrolljump=-0o12
  call assert_equal(-10, &sj)
  set scrolljump=0O15
  call assert_equal(13, &sj)
  set scrolljump=-0O15
  call assert_equal(-13, &sj)

  " Let number
  let &scrolljump = 4
  call assert_equal(4, &sj)
  let &scrolljump = -6
  call assert_equal(-6, &sj)

  " Let string
  let &scrolljump = '7'
  call assert_equal(7, &sj)
  let &scrolljump = '-9'
  call assert_equal(-9, &sj)

  " Incrementing
  set shiftwidth=4
  set shiftwidth+=2
  call assert_equal(6, &sw)
  let &shiftwidth += 2
  call assert_equal(8, &sw)

  " Decrementing
  set shiftwidth=6
  set shiftwidth-=2
  call assert_equal(4, &sw)
  let &shiftwidth -= 2
  call assert_equal(2, &sw)

  " Multiplying
  set shiftwidth=4
  set shiftwidth^=2
  call assert_equal(8, &sw)
  let &shiftwidth *= 2
  call assert_equal(16, &sw)

  set scrolljump&
  set shiftwidth&
endfunc

" Test for setting boolean option value
func Test_set_boolean_option()
  set number&

  " :set {option}
  set number
  call assert_equal(1, &nu)

  " :set no{option}
  set nonu
  call assert_equal(0, &nu)

  " :set {option}!
  set number!
  call assert_equal(1, &nu)
  set number!
  call assert_equal(0, &nu)

  " :set inv{option}
  set invnumber
  call assert_equal(1, &nu)
  set invnumber
  call assert_equal(0, &nu)

  " Let number
  let &number = 1
  call assert_equal(1, &nu)
  let &number = 0
  call assert_equal(0, &nu)

  " Let string
  let &number = '1'
  call assert_equal(1, &nu)
  let &number = '0'
  call assert_equal(0, &nu)

  " Let v:true and v:false
  let &number = v:true
  call assert_equal(1, &nu)
  let &number = v:false
  call assert_equal(0, &nu)

  set number&
endfunc

" Test for setting string option errors
func Test_set_string_option_errors()
  " :set no{option}
  call assert_fails('set notabstop', 'E474:')
  call assert_fails('setlocal notabstop', 'E474:')
  call assert_fails('setglobal notabstop', 'E474:')

  " :set inv{option}
  call assert_fails('set invtabstop', 'E474:')
  call assert_fails('setlocal invtabstop', 'E474:')
  call assert_fails('setglobal invtabstop', 'E474:')

  " :set {option}!
  call assert_fails('set makeprg!', 'E488:')
  call assert_fails('setlocal makeprg!', 'E488:')
  call assert_fails('setglobal makeprg!', 'E488:')

  " Invalid trailing chars
  call assert_fails('set makeprg??', 'E488:')
  call assert_fails('setlocal makeprg??', 'E488:')
  call assert_fails('setglobal makeprg??', 'E488:')
  call assert_fails('set makeprg&&', 'E488:')
  call assert_fails('setlocal makeprg&&', 'E488:')
  call assert_fails('setglobal makeprg&&', 'E488:')
  call assert_fails('set makeprg<<', 'E488:')
  call assert_fails('setlocal makeprg<<', 'E488:')
  call assert_fails('setglobal makeprg<<', 'E488:')
  call assert_fails('set makeprg@', 'E488:')
  call assert_fails('setlocal makeprg@', 'E488:')
  call assert_fails('setglobal makeprg@', 'E488:')
endfunc

" Test for setting number option errors
func Test_set_number_option_errors()
  " :set no{option}
  call assert_fails('set notabstop', 'E474:')
  call assert_fails('setlocal notabstop', 'E474:')
  call assert_fails('setglobal notabstop', 'E474:')

  " :set inv{option}
  call assert_fails('set invtabstop', 'E474:')
  call assert_fails('setlocal invtabstop', 'E474:')
  call assert_fails('setglobal invtabstop', 'E474:')

  " :set {option}!
  call assert_fails('set tabstop!', 'E488:')
  call assert_fails('setlocal tabstop!', 'E488:')
  call assert_fails('setglobal tabstop!', 'E488:')

  " Invalid trailing chars
  call assert_fails('set tabstop??', 'E488:')
  call assert_fails('setlocal tabstop??', 'E488:')
  call assert_fails('setglobal tabstop??', 'E488:')
  call assert_fails('set tabstop&&', 'E488:')
  call assert_fails('setlocal tabstop&&', 'E488:')
  call assert_fails('setglobal tabstop&&', 'E488:')
  call assert_fails('set tabstop<<', 'E488:')
  call assert_fails('setlocal tabstop<<', 'E488:')
  call assert_fails('setglobal tabstop<<', 'E488:')
  call assert_fails('set tabstop@', 'E488:')
  call assert_fails('setlocal tabstop@', 'E488:')
  call assert_fails('setglobal tabstop@', 'E488:')

  " Not a number
  call assert_fails('set tabstop=', 'E521:')
  call assert_fails('setlocal tabstop=', 'E521:')
  call assert_fails('setglobal tabstop=', 'E521:')
  call assert_fails('set tabstop=x', 'E521:')
  call assert_fails('setlocal tabstop=x', 'E521:')
  call assert_fails('setglobal tabstop=x', 'E521:')
  call assert_fails('set tabstop=1x', 'E521:')
  call assert_fails('setlocal tabstop=1x', 'E521:')
  call assert_fails('setglobal tabstop=1x', 'E521:')
  call assert_fails('set tabstop=-x', 'E521:')
  call assert_fails('setlocal tabstop=-x', 'E521:')
  call assert_fails('setglobal tabstop=-x', 'E521:')
  call assert_fails('set tabstop=0x', 'E521:')
  call assert_fails('setlocal tabstop=0x', 'E521:')
  call assert_fails('setglobal tabstop=0x', 'E521:')
  call assert_fails('set tabstop=0o', 'E521:')
  call assert_fails('setlocal tabstop=0o', 'E521:')
  call assert_fails('setglobal tabstop=0o', 'E521:')
  call assert_fails("let &tabstop = 'x'", 'E521:')
  call assert_fails("let &g:tabstop = 'x'", 'E521:')
  call assert_fails("let &l:tabstop = 'x'", 'E521:')
endfunc

" Test for setting boolean option errors
func Test_set_boolean_option_errors()
  " :set {option}=
  call assert_fails('set number=', 'E474:')
  call assert_fails('setlocal number=', 'E474:')
  call assert_fails('setglobal number=', 'E474:')
  call assert_fails('set number=1', 'E474:')
  call assert_fails('setlocal number=1', 'E474:')
  call assert_fails('setglobal number=1', 'E474:')

  " :set {option}:
  call assert_fails('set number:', 'E474:')
  call assert_fails('setlocal number:', 'E474:')
  call assert_fails('setglobal number:', 'E474:')
  call assert_fails('set number:1', 'E474:')
  call assert_fails('setlocal number:1', 'E474:')
  call assert_fails('setglobal number:1', 'E474:')

  " :set {option}+=
  call assert_fails('set number+=1', 'E474:')
  call assert_fails('setlocal number+=1', 'E474:')
  call assert_fails('setglobal number+=1', 'E474:')

  " :set {option}^=
  call assert_fails('set number^=1', 'E474:')
  call assert_fails('setlocal number^=1', 'E474:')
  call assert_fails('setglobal number^=1', 'E474:')

  " :set {option}-=
  call assert_fails('set number-=1', 'E474:')
  call assert_fails('setlocal number-=1', 'E474:')
  call assert_fails('setglobal number-=1', 'E474:')

  " Invalid trailing chars
  call assert_fails('set number!!', 'E488:')
  call assert_fails('setlocal number!!', 'E488:')
  call assert_fails('setglobal number!!', 'E488:')
  call assert_fails('set number??', 'E488:')
  call assert_fails('setlocal number??', 'E488:')
  call assert_fails('setglobal number??', 'E488:')
  call assert_fails('set number&&', 'E488:')
  call assert_fails('setlocal number&&', 'E488:')
  call assert_fails('setglobal number&&', 'E488:')
  call assert_fails('set number<<', 'E488:')
  call assert_fails('setlocal number<<', 'E488:')
  call assert_fails('setglobal number<<', 'E488:')
  call assert_fails('set number@', 'E488:')
  call assert_fails('setlocal number@', 'E488:')
  call assert_fails('setglobal number@', 'E488:')
endfunc

" Test for setting unknown option errors
func Test_set_unknown_option_error()
  call assert_fails('set xxx', 'E518:')
  call assert_fails('setlocal xxx', 'E518:')
  call assert_fails('setglobal xxx', 'E518:')
  call assert_fails('set xxx=', 'E518:')
  call assert_fails('setlocal xxx=', 'E518:')
  call assert_fails('setglobal xxx=', 'E518:')
  call assert_fails('set xxx:', 'E518:')
  call assert_fails('setlocal xxx:', 'E518:')
  call assert_fails('setglobal xxx:', 'E518:')
  call assert_fails('set xxx!', 'E518:')
  call assert_fails('setlocal xxx!', 'E518:')
  call assert_fails('setglobal xxx!', 'E518:')
  call assert_fails('set xxx?', 'E518:')
  call assert_fails('setlocal xxx?', 'E518:')
  call assert_fails('setglobal xxx?', 'E518:')
  call assert_fails('set xxx&', 'E518:')
  call assert_fails('setlocal xxx&', 'E518:')
  call assert_fails('setglobal xxx&', 'E518:')
  call assert_fails('set xxx<', 'E518:')
  call assert_fails('setlocal xxx<', 'E518:')
  call assert_fails('setglobal xxx<', 'E518:')
endfunc

" Test for setting an option to a Vi or Vim default
func Test_set_default()
  set formatoptions&vi
  call assert_equal('vt', &formatoptions)
  set formatoptions&vim
  call assert_equal('tcq', &formatoptions)

  call assert_equal('ucs-bom,utf-8,default,latin1', &fencs)
  set fencs=latin1
  set fencs&
  call assert_equal('ucs-bom,utf-8,default,latin1', &fencs)
  set fencs=latin1
  set all&
  call assert_equal('ucs-bom,utf-8,default,latin1', &fencs)
endfunc

" Test for setting options in sandbox
func Test_set_in_sandbox()
  " Some boolean options cannot be set in sandbox, some can.
  call assert_fails('sandbox set modelineexpr', 'E48:')
  sandbox set number
  call assert_true(&number)
  set number&

  " Some number options cannot be set in sandbox, some can.
  if has('python') || has('python3')
    call assert_fails('sandbox set pyxversion=3', 'E48:')
  endif
  sandbox set tabstop=4
  call assert_equal(4, &tabstop)
  set tabstop&

  " Some string options cannot be set in sandbox, some can.
  call assert_fails('sandbox set backupdir=/tmp', 'E48:')
  sandbox set filetype=perl
  call assert_equal('perl', &filetype)
  set filetype&
endfunc

" Test for setting keycodes using set
func Test_set_keycode()
  call assert_fails('set <t_k1=l', 'E474:')
  call assert_fails('set <Home=l', 'E474:')
  set <t_k9>=abcd
  call assert_equal('abcd', &t_k9)
  set <t_k9>&
  set <F9>=xyz
  call assert_equal('xyz', &t_k9)
  set <t_k9>&

  " Not found in termcap
  call assert_fails('set t_#-&', 'E522:')

  " Keycode not set
  call assert_fails('set t_foo=', 'E846:')

  " should we test all of them?
  set t_Ce=testCe
  set t_Cs=testCs
  set t_Us=testUs
  set t_ds=testds
  set t_Ds=testDs
  call assert_equal('testCe', &t_Ce)
  call assert_equal('testCs', &t_Cs)
  call assert_equal('testUs', &t_Us)
  call assert_equal('testds', &t_ds)
  call assert_equal('testDs', &t_Ds)
endfunc

" vim: shiftwidth=2 sts=2 expandtab
