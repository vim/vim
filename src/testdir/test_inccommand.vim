" Test for the 'inccommand' option

source util/screendump.vim

func Test_inccommand_option_values()
  CheckOption inccommand

  set inccommand
  call assert_equal(1, &inccommand)
  set noinccommand
  call assert_equal(0, &inccommand)
  set inccommand&
  call assert_equal(0, &inccommand)
endfunc

func Test_inccommand_substitute_dump()
  CheckOption inccommand
  CheckOption incsearch
  CheckScreendump

  call writefile([
	\ 'set incsearch inccommand hlsearch scrolloff=0',
	\ 'for n in range(1, 10)',
	\ '  call setline(n, "foo " . n)',
	\ 'endfor',
	\ '3',
	\ ], 'Xicm_script', 'D')
  let buf = RunVimInTerminal('-S Xicm_script', {'rows': 14, 'cols': 50})
  sleep 100m

  " Substitute with replacement preview.
  call term_sendkeys(buf, ':%s/foo/bar')
  sleep 100m
  call VerifyScreenDump(buf, 'Test_inccommand_sub_01', {})
  call term_sendkeys(buf, "\<Esc>")
  sleep 100m

  " Verify buffer is restored after Escape.
  call VerifyScreenDump(buf, 'Test_inccommand_sub_02', {})

  " Substitute with third delimiter (no flags yet).
  call term_sendkeys(buf, ':%s/foo/bar/')
  sleep 100m
  call VerifyScreenDump(buf, 'Test_inccommand_sub_03a', {})

  " Add 'g' flag.
  call term_sendkeys(buf, 'g')
  sleep 100m
  call VerifyScreenDump(buf, 'Test_inccommand_sub_03', {})

  " Press Enter to execute - buffer should be changed.
  call term_sendkeys(buf, "\<CR>")
  sleep 100m
  call VerifyScreenDump(buf, 'Test_inccommand_sub_04', {})

  call StopVimInTerminal(buf)
endfunc

func Test_inccommand_substitute_multi_match_dump()
  CheckOption inccommand
  CheckOption incsearch
  CheckScreendump

  call writefile([
	\ 'set incsearch inccommand hlsearch scrolloff=0',
	\ 'call setline(1, ["foo bar foo", "foo baz foo", "foo qux foo"])',
	\ '1',
	\ ], 'Xicm_multi_script', 'D')
  let buf = RunVimInTerminal('-S Xicm_multi_script', {'rows': 9, 'cols': 50})
  sleep 100m

  " Without /g - only first match per line replaced.
  call term_sendkeys(buf, ':%s/foo/XXX')
  sleep 100m
  call VerifyScreenDump(buf, 'Test_inccommand_multi_01', {})

  " Add third delimiter.
  call term_sendkeys(buf, '/')
  sleep 100m
  call VerifyScreenDump(buf, 'Test_inccommand_multi_02', {})

  " Add 'g' flag - all matches replaced.
  call term_sendkeys(buf, 'g')
  sleep 100m
  call VerifyScreenDump(buf, 'Test_inccommand_multi_03', {})

  call term_sendkeys(buf, "\<Esc>")
  call StopVimInTerminal(buf)
endfunc

func Test_inccommand_substitute_range()
  CheckOption inccommand
  CheckOption incsearch

  call test_override("char_avail", 1)
  new
  set incsearch inccommand
  for n in range(1, 10)
    call setline(n, 'foo ' . n)
  endfor
  3
  " Substitute with range and replacement - should execute normally.
  call feedkeys(":.,.+2s/foo/bar\<CR>", 'tx')
  call assert_equal('foo 2', getline(2))
  call assert_equal('bar 3', getline(3))
  call assert_equal('bar 4', getline(4))
  call assert_equal('bar 5', getline(5))
  call assert_equal('foo 6', getline(6))

  " Escape should leave buffer unchanged.
  call feedkeys(":%s/foo/baz\<Esc>", 'tx')
  call assert_equal('foo 1', getline(1))
  call assert_equal('foo 6', getline(6))

  bwipe!
  set noincsearch inccommand&
  call test_override("ALL", 0)
endfunc

func Test_inccommand_substitute_empty_pattern()
  CheckOption inccommand
  CheckOption incsearch

  call test_override("char_avail", 1)
  new
  set incsearch inccommand
  call setline(1, ['hello world', 'hello vim'])
  " Set a previous search pattern.
  call feedkeys("/hello\<CR>", 'tx')
  " Use empty pattern - should reuse previous.
  call feedkeys(":%s//goodbye\<CR>", 'tx')
  call assert_equal('goodbye world', getline(1))
  call assert_equal('goodbye vim', getline(2))

  bwipe!
  set noincsearch inccommand&
  call test_override("ALL", 0)
endfunc

func Test_inccommand_substitute_global_flag()
  CheckOption inccommand
  CheckOption incsearch

  call test_override("char_avail", 1)
  new
  set incsearch inccommand
  call setline(1, ['aaa bbb aaa', 'ccc aaa ccc'])
  call feedkeys(":%s/aaa/xxx/g\<CR>", 'tx')
  call assert_equal('xxx bbb xxx', getline(1))
  call assert_equal('ccc xxx ccc', getline(2))

  bwipe!
  set noincsearch inccommand&
  call test_override("ALL", 0)
endfunc

func Test_inccommand_substitute_expr_skipped()
  CheckOption inccommand
  CheckOption incsearch

  " Substitution with \= should be skipped for preview but still execute.
  call test_override("char_avail", 1)
  new
  set incsearch inccommand
  call setline(1, ['foo bar', 'foo baz'])
  call feedkeys(":%s/foo/\\=substitute(submatch(0),'f','F','')\<CR>", 'tx')
  call assert_equal('Foo bar', getline(1))
  call assert_equal('Foo baz', getline(2))

  bwipe!
  set noincsearch inccommand&
  call test_override("ALL", 0)
endfunc

func Test_inccommand_requires_incsearch()
  CheckOption inccommand

  " inccommand without incsearch should not preview.
  call test_override("char_avail", 1)
  new
  set noincsearch inccommand
  call setline(1, ['foo bar'])
  " This should just work as a normal substitute with no preview.
  call feedkeys(":%s/foo/baz\<CR>", 'tx')
  call assert_equal('baz bar', getline(1))

  bwipe!
  set inccommand&
  call test_override("ALL", 0)
endfunc

func Test_inccommand_global_dump()
  CheckOption inccommand
  CheckOption incsearch
  CheckScreendump

  call writefile([
	\ 'set incsearch inccommand scrolloff=0',
	\ 'call setline(1, ["foo one", "bar two", "foo three", "bar four"])',
	\ '1',
	\ ], 'Xicm_global_script', 'D')
  let buf = RunVimInTerminal('-S Xicm_global_script', {'rows': 9, 'cols': 50})
  sleep 100m

  " :g/pattern/ should highlight all matching lines.
  call term_sendkeys(buf, ':g/foo')
  sleep 100m
  call VerifyScreenDump(buf, 'Test_inccommand_global_01', {})

  call term_sendkeys(buf, "\<Esc>")
  call StopVimInTerminal(buf)
endfunc

func Test_inccommand_substitute_empty_replacement()
  CheckOption inccommand
  CheckOption incsearch

  call test_override("char_avail", 1)
  new
  set incsearch inccommand
  call setline(1, ['foo bar foo'])
  call feedkeys(":%s/foo//g\<CR>", 'tx')
  call assert_equal(' bar ', getline(1))

  bwipe!
  set noincsearch inccommand&
  call test_override("ALL", 0)
endfunc

func Test_inccommand_substitute_special_replacement()
  CheckOption inccommand
  CheckOption incsearch

  call test_override("char_avail", 1)
  new
  set incsearch inccommand
  " & in replacement refers to matched text.
  call setline(1, ['foo bar'])
  call feedkeys(":%s/foo/[&]\<CR>", 'tx')
  call assert_equal('[foo] bar', getline(1))

  bwipe!
  set noincsearch inccommand&
  call test_override("ALL", 0)
endfunc

" vim: shiftwidth=2 sts=2 expandtab
