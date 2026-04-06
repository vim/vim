vim9script

CheckExecutable tar
CheckNotMSWindows

runtime plugin/tarPlugin.vim

def CopyFile(source: string)
  if !filecopy($"samples/{source}", "X.tar")
    assert_report($"Can't copy samples/{source}")
  endif
enddef

def g:Test_tar_basic()
  CopyFile("sample.tar")
  defer delete("X.tar")
  defer delete("./testtar", 'rf')
  e X.tar

  ### Check header
  assert_match('^" tar\.vim version v\d\+', getline(1))
  assert_match('^" Browsing tarfile .*/X.tar', getline(2))
  assert_match('^" Select a file with cursor and press ENTER, "x" to extract a file', getline(3))
  assert_match('^$', getline(4))
  assert_match('testtar/', getline(5))
  assert_match('testtar/file1.txt', getline(6))

  ### Check ENTER on header
  :1
  exe ":normal \<cr>"
  assert_equal("X.tar", @%)

  ### Check ENTER on file
  :6
  exe ":normal \<cr>"
  assert_equal("tarfile::testtar/file1.txt", @%)


  ### Check editing file
  ### Note: deleting entries not supported on BSD
  if has("mac")
    return
  endif
  if has("bsd")
    return
  endif
  s/.*/some-content/
  assert_equal("some-content", getline(1))
  w!
  assert_equal("tarfile::testtar/file1.txt", @%)
  bw!
  close
  bw!

  e X.tar
  :6
  exe "normal \<cr>"
  assert_equal("some-content", getline(1))
  bw!
  close

  ### Check extracting file
  :5
  normal x
  assert_true(filereadable("./testtar/file1.txt"))
  bw!
enddef

def g:Test_tar_evil()
  CopyFile("evil.tar")
  defer delete("X.tar")
  defer delete("./etc", 'rf')
  e X.tar

  ### Check header
  assert_match('^" tar\.vim version v\d\+', getline(1))
  assert_match('^" Browsing tarfile .*/X.tar', getline(2))
  assert_match('^" Select a file with cursor and press ENTER, "x" to extract a file', getline(3))
  assert_match('^" Note: Path Traversal Attack detected', getline(4))
  assert_match('^$', getline(5))
  assert_match('/etc/ax-pwn', getline(6))

  ### Check ENTER on header
  :1
  exe ":normal \<cr>"
  assert_equal("X.tar", @%)
  assert_equal(1, b:leading_slash)

  ### Check ENTER on file
  :6
  exe ":normal \<cr>"
  assert_equal(1, b:leading_slash)
  assert_equal("tarfile::/etc/ax-pwn", @%)


  ### Check editing file
  ### Note: deleting entries not supported on BSD
  if has("mac")
    return
  endif
  if has("bsd")
    return
  endif
  s/.*/none/
  assert_equal("none", getline(1))
  w!
  assert_equal(1, b:leading_slash)
  assert_equal("tarfile::/etc/ax-pwn", @%)
  bw!
  close
  bw!

  # Writing was aborted
  e X.tar
  assert_match('^" Note: Path Traversal Attack detected', getline(4))
  :6
  exe "normal \<cr>"
  assert_equal("something", getline(1))
  bw!
  close

  ### Check extracting file
  :5
  normal x
  assert_true(filereadable("./etc/ax-pwn"))

  bw!
enddef

def g:Test_tar_path_traversal_with_nowrapscan()
  CopyFile("evil.tar")
  defer delete("X.tar")
  # Make sure we still find the tar warning (or leading slashes) even when
  # wrapscan is off
  set nowrapscan
  e X.tar

  ### Check header
  assert_match('^" tar\.vim version v\d\+', getline(1))
  assert_match('^" Browsing tarfile .*/X.tar', getline(2))
  assert_match('^" Select a file with cursor and press ENTER, "x" to extract a file', getline(3))
  assert_match('^" Note: Path Traversal Attack detected', getline(4))
  assert_match('^$', getline(5))
  assert_match('/etc/ax-pwn', getline(6))

  assert_equal(1, b:leading_slash)

  bw!
enddef

def g:Test_tar_lz4_extract()
  CheckExecutable lz4

  delete('X.txt')
  delete('Xarchive.tar')
  delete('Xarchive.tar.lz4')
  call writefile(['hello'], 'X.txt')
  call system('tar -cf Xarchive.tar X.txt')
  assert_equal(0, v:shell_error)

  call system('lz4 -z Xarchive.tar Xarchive.tar.lz4')
  assert_equal(0, v:shell_error)

  delete('X.txt')
  delete('Xarchive.tar')
  defer delete('Xarchive.tar.lz4')

  e Xarchive.tar.lz4
  assert_match('X.txt', getline(5))
  :5
  normal x
  assert_true(filereadable('X.txt'))
  assert_equal(['hello'], readfile('X.txt'))
  delete('X.txt')
  bw!
enddef

def g:Test_tlz4_extract()
  CheckExecutable lz4

  delete('X.txt')
  delete('Xarchive.tar')
  delete('Xarchive.tlz4')
  call writefile(['goodbye'], 'X.txt')
  call system('tar -cf Xarchive.tar X.txt')
  assert_equal(0, v:shell_error)

  call system('lz4 -z Xarchive.tar Xarchive.tlz4')
  assert_equal(0, v:shell_error)

  delete('X.txt')
  delete('Xarchive.tar')
  defer delete('Xarchive.tlz4')

  e Xarchive.tlz4
  assert_match('X.txt', getline(5))
  :5
  normal x
  assert_true(filereadable('X.txt'))
  assert_equal(['goodbye'], readfile('X.txt'))
  delete('X.txt')
  bw!
enddef
