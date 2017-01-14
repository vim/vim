" Test Vim profiler
if !has('profile')
  finish
endif

func Test_profile()
  let lines = [
    \ "func! Foo1()",
    \ "endfunc",
    \ "func! Foo2()",
    \ "  let count = 10000",
    \ "  while count > 0",
    \ "    let count = count - 1",
    \ "  endwhile",
    \ "endfunc",
    \ "func! Bar()",
    \ "endfunc",
    \ "call Foo1()",
    \ "call Foo1()",
    \ "profile pause",
    \ "call Foo1()",
    \ "profile continue",
    \ "call Foo2()",
    \ "call Bar()",
    \ ]

  call writefile(lines, 'Xprofile.vim')
  let a = system(v:progpath
    \ . " -u NONE -i NONE --noplugin "
    \ . "-c 'profile start Xprofile.log' "
    \ . "-c 'profile func Foo*' "
    \ . "-c 'so Xprofile.vim'")
  let lines = readfile('Xprofile.log')

  call assert_equal('FUNCTION  Foo1()', lines[0])
  call assert_equal('Called 2 times',   lines[1])
  call assert_equal('FUNCTION  Foo2()', lines[7])
  call assert_equal('Called 1 time',    lines[8])

  " - Foo2() should come before Foo1() since Foo1() does much more work.
  " - Foo1() is called 3 times but should be reported as called twice
  "   since one call is in between "profile pause" .. "profile continue".
  " - Function Bar() which is called is not expected to be profiled since
  "   it does not match "profile func Foo*".
  call assert_equal('FUNCTIONS SORTED ON TOTAL TIME',        lines[18])
  call assert_equal('count  total (s)   self (s)  function', lines[19])
  call assert_match('^\s*1\s\+\d\+\.\d\+\s\+Foo2()$',        lines[20])
  call assert_match('^\s*2\s\+\d\+\.\d\+\s\+Foo1()$',        lines[21])
  call assert_equal('',                                      lines[22])
  call assert_equal('FUNCTIONS SORTED ON SELF TIME',         lines[23])
  call assert_equal('count  total (s)   self (s)  function', lines[24])
  call assert_match('^\s*1\s\+\d\+\.\d\+\s\+Foo2()$',        lines[25])
  call assert_match('^\s*2\s\+\d\+\.\d\+\s\+Foo1()$',        lines[26])
  call assert_equal('',                                      lines[27])

  call delete('Xprofile.vim')
  call delete('Xprofile.log')
endfunc

func Test_profile_completion()
  call feedkeys(":profile \<C-A>\<C-B>\"\<CR>", 'tx')
  call assert_equal('"profile continue file func pause start', @:)

  call feedkeys(":profile start \<C-A>\<C-B>\"\<CR>", 'tx')
  call assert_match('^"profile start .* test_profile\.vim ', @:)
endfunc

func Test_profile_errors()
  call assert_fails("profile func Foo", 'E750:')
  call assert_fails("profile pause", 'E750:')
  call assert_fails("profile continue", 'E750:')
endfunc
