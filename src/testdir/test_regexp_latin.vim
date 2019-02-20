" Tests for regexp in latin1 encoding
set encoding=latin1
scriptencoding latin1

func s:equivalence_test()
  let str = "AÀÁÂÃÄÅ B C D EÈÉÊË F G H IÌÍÎÏ J K L M NÑ OÒÓÔÕÖØ P Q R S T UÙÚÛÜ V W X YÝ Z aàáâãäå b c d eèéêë f g h iìíîï j k l m nñ oòóôõöø p q r s t uùúûü v w x yýÿ z"
  let groups = split(str)
  for group1 in groups
      for c in split(group1, '\zs')
	" next statement confirms that equivalence class matches every
	" character in group
        call assert_match('^[[=' . c . '=]]*$', group1)
        for group2 in groups
          if group2 != group1
	    " next statement converts that equivalence class doesn't match
	    " a character in any other group
            call assert_equal(-1, match(group2, '[[=' . c . '=]]'))
          endif
        endfor
      endfor
  endfor
endfunc

func Test_equivalence_re1()
  set re=1
  call s:equivalence_test()
endfunc

func Test_equivalence_re2()
  set re=2
  call s:equivalence_test()
endfunc

func Test_recursive_substitute()
  new
  s/^/\=execute("s#^##gn")
  " check we are now not in the sandbox
  call setwinvar(1, 'myvar', 1)
  bwipe!
endfunc

func Test_nested_backrefs()
  " Check example in change.txt.
  new
  for re in range(0, 2)
    exe 'set re=' . re
    call setline(1, 'aa ab x')
    1s/\(\(a[a-d] \)*\)\(x\)/-\1- -\2- -\3-/
    call assert_equal('-aa ab - -ab - -x-', getline(1))

    call assert_equal('-aa ab - -ab - -x-', substitute('aa ab x', '\(\(a[a-d] \)*\)\(x\)', '-\1- -\2- -\3-', ''))
  endfor
  bwipe!
  set re=0
endfunc

func Test_eow_with_optional()
  let expected = ['abc def', 'abc', 'def', '', '', '', '', '', '', '']
  for re in range(0, 2)
    exe 'set re=' . re
    let actual = matchlist('abc def', '\(abc\>\)\?\s*\(def\)')
    call assert_equal(expected, actual)
  endfor
endfunc

func Test_backref()
  new
  call setline(1, ['one', 'two', 'three', 'four', 'five'])
  call assert_equal(3, search('\%#=1\(e\)\1'))
  call assert_equal(3, search('\%#=2\(e\)\1'))
  call assert_fails('call search("\\%#=1\\(e\\1\\)")', 'E65:')
  call assert_fails('call search("\\%#=2\\(e\\1\\)")', 'E65:')
  bwipe!
endfunc

func Test_multi_failure()
  set re=1
  call assert_fails('/a**', 'E61:')
  call assert_fails('/a*\+', 'E62:')
  call assert_fails('/a\{a}', 'E554:')
  set re=2
  call assert_fails('/a**', 'E871:')
  call assert_fails('/a*\+', 'E871:')
  call assert_fails('/a\{a}', 'E870:')
  set re=0
endfunc

func Test_recursive_addstate()
  " This will call addstate() recursively until it runs into the limit.
  let lnum = search('\v((){328}){389}')
  call assert_equal(0, lnum)
endfunc

func Test_out_of_memory()
  new
  s/^/,n
  " This will be slow...
  call assert_fails('call search("\\v((n||<)+);")', 'E363:')
endfunc

func Test_get_equi_class()
  new
  " Incomplete equivalence class caused invalid memory access
  s/^/[[=
  call assert_equal(1, search(getline(1)))
  s/.*/[[.
  call assert_equal(1, search(getline(1)))
endfunc

func Test_rex_init()
  set noincsearch
  set re=1
  new
  setlocal iskeyword=a-z
  call setline(1, ['abc', 'ABC'])
  call assert_equal(1, search('[[:keyword:]]'))
  new
  setlocal iskeyword=A-Z
  call setline(1, ['abc', 'ABC'])
  call assert_equal(2, search('[[:keyword:]]'))
  bwipe!
  bwipe!
  set re=0
endfunc

func Test_range_with_newline()
  new
  call setline(1, "a")
  call assert_equal(0, search("[ -*\\n- ]"))
  call assert_equal(0, search("[ -*\\t-\\n]"))
  bwipe!
endfunc
