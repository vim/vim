" Tests for :bvimgrep

func Test_bvimgrep()
  set hidden
  " Test1: only grep in current unsaved buffer foo
  new
  :f foo
  $put ='buffer: foo'
  vimgrep <buffer> /foo/j foo
  call assert_equal([{'lnum': 2, 'bufnr': 2, 'col': 9, 'valid': 1, 'vcol': 0, 'nr': 0, 'type': '', 'pattern': '', 'text': 'buffer: foo'}], getqflist())

  " Test2: buffer foo twice + buffer foobar
  new
  f foobar
  $put = 'buffer: foobar'
  vimgrepadd <buffer> /foo/j foo*
  call assert_equal([
  \ {'lnum': 2, 'bufnr': 2, 'col': 9, 'valid': 1, 'vcol': 0, 'nr': 0, 'type': '', 'pattern': '', 'text': 'buffer: foo'},
  \ {'lnum': 2, 'bufnr': 2, 'col': 9, 'valid': 1, 'vcol': 0, 'nr': 0, 'type': '', 'pattern': '', 'text': 'buffer: foo'},
  \ {'lnum': 2, 'bufnr': 3, 'col': 9, 'valid': 1, 'vcol': 0, 'nr': 0, 'type': '', 'pattern': '', 'text': 'buffer: foobar'}
  \ ], getqflist())

  " Test3: Only search current buffer: testfile
  new
  f testfile
  call append('$', ['test_1', 'test_2', 'test_3', 'test_4', 'test_5'])
  try
    vimgrep <buffer> /test_/j
  catch
  endtry
  call assert_equal([
  \ {'lnum': 2, 'bufnr': 4, 'col': 1, 'valid': 1, 'vcol': 0, 'nr': 0, 'type': '', 'pattern': '', 'text': 'test_1'},
  \ {'lnum': 3, 'bufnr': 4, 'col': 1, 'valid': 1, 'vcol': 0, 'nr': 0, 'type': '', 'pattern': '', 'text': 'test_2'},
  \ {'lnum': 4, 'bufnr': 4, 'col': 1, 'valid': 1, 'vcol': 0, 'nr': 0, 'type': '', 'pattern': '', 'text': 'test_3'},
  \ {'lnum': 5, 'bufnr': 4, 'col': 1, 'valid': 1, 'vcol': 0, 'nr': 0, 'type': '', 'pattern': '', 'text': 'test_4'},
  \ {'lnum': 6, 'bufnr': 4, 'col': 1, 'valid': 1, 'vcol': 0, 'nr': 0, 'type': '', 'pattern': '', 'text': 'test_5'}
  \ ], getqflist())

  " Test4: Search all 4 buffers
  vimgrep <buffer> /^\(test_\d\+\|buffer: foo\)/j *
  call assert_equal([
  \ {'lnum': 2, 'bufnr': 2, 'col': 1, 'valid': 1, 'vcol': 0, 'nr': 0, 'type': '', 'pattern': '', 'text': 'buffer: foo'},
  \ {'lnum': 2, 'bufnr': 3, 'col': 1, 'valid': 1, 'vcol': 0, 'nr': 0, 'type': '', 'pattern': '', 'text': 'buffer: foobar'},
  \ {'lnum': 2, 'bufnr': 4, 'col': 1, 'valid': 1, 'vcol': 0, 'nr': 0, 'type': '', 'pattern': '', 'text': 'test_1'},
  \ {'lnum': 3, 'bufnr': 4, 'col': 1, 'valid': 1, 'vcol': 0, 'nr': 0, 'type': '', 'pattern': '', 'text': 'test_2'},
  \ {'lnum': 4, 'bufnr': 4, 'col': 1, 'valid': 1, 'vcol': 0, 'nr': 0, 'type': '', 'pattern': '', 'text': 'test_3'},
  \ {'lnum': 5, 'bufnr': 4, 'col': 1, 'valid': 1, 'vcol': 0, 'nr': 0, 'type': '', 'pattern': '', 'text': 'test_4'},
  \ {'lnum': 6, 'bufnr': 4, 'col': 1, 'valid': 1, 'vcol': 0, 'nr': 0, 'type': '', 'pattern': '', 'text': 'test_5'},
  \ ], getqflist())

  " Test5: bvimgrep aborts, clears qflist
  set nohidden
  try
    vimgrep <buffer> /\(test_\|foo\)/j *
    call assert_false(1, 'bvimgrep should have failed')
  catch
    call assert_exception('E37:')
    call assert_equal([], getqflist())
  endtry
endfunc
