" Test the :filter command modifier

func Test_filter()
  edit Xdoesnotmatch
  edit Xwillmatch
  call assert_equal('"Xwillmatch"', substitute(execute('filter willma ls'), '[^"]*\(".*"\)[^"]*', '\1', ''))
  bwipe Xdoesnotmatch
  bwipe Xwillmatch

  new
  call setline(1, ['foo1', 'foo2', 'foo3', 'foo4', 'foo5'])
  call assert_equal("\nfoo2\nfoo4", execute('filter /foo[24]/ 1,$print'))
  call assert_equal("\n  2 foo2\n  4 foo4", execute('filter /foo[24]/ 1,$number'))
  call assert_equal("\nfoo2$\nfoo4$", execute('filter /foo[24]/ 1,$list'))

  call assert_equal("\nfoo1$\nfoo3$\nfoo5$", execute('filter! /foo[24]/ 1,$list'))
  bwipe!

  command XTryThis echo 'this'
  command XTryThat echo 'that'
  command XDoThat echo 'that'
  let lines = split(execute('filter XTry command'), "\n")
  call assert_equal(3, len(lines))
  call assert_match("XTryThat", lines[1])
  call assert_match("XTryThis", lines[2])
  delcommand XTryThis
  delcommand XTryThat
  delcommand XDoThat

  map f1 the first key
  map f2 the second key
  map f3 not a key
  let lines = split(execute('filter the map f'), "\n")
  call assert_equal(2, len(lines))
  call assert_match("f2", lines[0])
  call assert_match("f1", lines[1])
  unmap f1
  unmap f2
  unmap f3
endfunc

func Test_filter_fails()
  call assert_fails('filter', 'E471:')
  call assert_fails('filter pat', 'E476:')
  call assert_fails('filter /pat', 'E476:')
  call assert_fails('filter /pat/', 'E476:')
  call assert_fails('filter /pat/ asdf', 'E492:')

  call assert_fails('filter!', 'E471:')
  call assert_fails('filter! pat', 'E476:')
  call assert_fails('filter! /pat', 'E476:')
  call assert_fails('filter! /pat/', 'E476:')
  call assert_fails('filter! /pat/ asdf', 'E492:')
endfunc

function s:complete_filter_cmd(filtcmd)
  let keystroke = "\<TAB>\<C-R>=execute('let cmdline = getcmdline()')\<CR>\<C-C>"
  let cmdline = ''
  call feedkeys(':' . a:filtcmd . keystroke, 'ntx')
  return cmdline
endfunction

func Test_filter_cmd_completion()
  " Do not complete pattern
  call assert_equal("filter \t", s:complete_filter_cmd('filter '))
  call assert_equal("filter pat\t", s:complete_filter_cmd('filter pat'))
  call assert_equal("filter /pat\t", s:complete_filter_cmd('filter /pat'))
  call assert_equal("filter /pat/\t", s:complete_filter_cmd('filter /pat/'))

  " Complete after string pattern
  call assert_equal('filter pat print', s:complete_filter_cmd('filter pat pri'))

  " Complete after regexp pattern
  call assert_equal('filter /pat/ print', s:complete_filter_cmd('filter /pat/ pri'))
  call assert_equal('filter #pat# print', s:complete_filter_cmd('filter #pat# pri'))
endfunc

func Test_filter_commands()
  let g:test_filter_a = 1
  let b:test_filter_b = 2
  let test_filter_c = 3

  " Test filtering :let command
  let redi = ""
  redi => redi
  filter /^test_filter/ let
  redi END
  let res = split(redi, "\n")
  call assert_equal(["test_filter_a         #1"], res)

  redi => redi
  filter /^\(b:\|\)test_filter/ let
  redi END
  let res = split(redi, "\n")
  call assert_equal(["test_filter_a         #1", "b:test_filter_b       #2"], res)

  unlet g:test_filter_a
  unlet b:test_filter_b
  unlet test_filter_c

  " Test filtering :set command
  redi => redi
  filter /^help/ set
  redi END
  let res = split(redi, "\n")[1:]
  call assert_equal(["  helplang=en"], res)

  " Test filtering :llist command
  call setloclist(0, [{"filename": "/path/vim.c"}, {"filename": "/path/vim.h"}])
  redi => redi
  filter /\.c$/ llist
  redi END
  let res = split(redi, "\n")
  call assert_equal([" 1 /path/vim.c:  "], res)

  " Test filtering :jump command
  e file.c
  e file.h
  e file.hs
  redi => redi
  filter /\.c$/ jumps
  redi END
  let res = split(redi, "\n")[1:]
  call assert_equal(["   2     1    0 file.c", ">"], res)
endfunc
