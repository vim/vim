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

def CreateTar(archivename: string, content: string, outputdir: string)
  var tempdir = tempname()
  mkdir(tempdir, 'R')
  call writefile([content], tempdir .. '/X.txt')
  assert_true(filereadable(tempdir .. '/X.txt'))
  call system('tar -C ' .. tempdir .. ' -cf ' .. outputdir .. '/' .. archivename .. ' X.txt')
  assert_equal(0, v:shell_error)
enddef

def CreateTgz(archivename: string, content: string, outputdir: string)
  var tempdir = tempname()
  mkdir(tempdir, 'R')
  call writefile([content], tempdir .. '/X.txt')
  assert_true(filereadable(tempdir .. '/X.txt'))
  call system('tar -C ' .. tempdir .. ' -czf ' .. outputdir .. '/' .. archivename .. ' X.txt')
  assert_equal(0, v:shell_error)
enddef

def CreateTbz(archivename: string, content: string, outputdir: string)
  var tempdir = tempname()
  mkdir(tempdir, 'R')
  call writefile([content], tempdir .. '/X.txt')
  assert_true(filereadable(tempdir .. '/X.txt'))
  call system('tar -C ' .. tempdir .. ' -cjf ' .. outputdir .. '/' .. archivename .. ' X.txt')
  assert_equal(0, v:shell_error)
enddef

def CreateTxz(archivename: string, content: string, outputdir: string)
  var tempdir = tempname()
  mkdir(tempdir, 'R')
  call writefile([content], tempdir .. '/X.txt')
  assert_true(filereadable(tempdir .. '/X.txt'))
  call system('tar -C ' .. tempdir .. ' -cJf ' .. outputdir .. '/' .. archivename .. ' X.txt')
  assert_equal(0, v:shell_error)
enddef

def CreateTzst(archivename: string, content: string, outputdir: string)
  var tempdir = tempname()
  mkdir(tempdir, 'R')
  call writefile([content], tempdir .. '/X.txt')
  assert_true(filereadable(tempdir .. '/X.txt'))
  call system('tar --zstd -C ' .. tempdir .. ' -cf ' .. outputdir .. '/' .. archivename .. ' X.txt')
  assert_equal(0, v:shell_error)
enddef

def CreateTlz4(archivename: string, content: string, outputdir: string)
  var tempdir = tempname()
  mkdir(tempdir, 'R')
  call writefile([content], tempdir .. '/X.txt')
  assert_true(filereadable(tempdir .. '/X.txt'))
  call system('tar -C ' .. tempdir .. ' -cf ' .. tempdir .. '/Xarchive.tar X.txt')
  assert_equal(0, v:shell_error)
  assert_true(filereadable(tempdir .. '/Xarchive.tar'))
  call system('lz4 -z ' .. tempdir .. '/Xarchive.tar ' .. outputdir .. '/' .. archivename)
  assert_equal(0, v:shell_error)
enddef

# XXX: Add test for .tar.bz3
def g:Test_extraction()
  var control = [
    {create: CreateTar,
     archive: 'Xarchive.tar'},
    {create: CreateTgz,
     archive: 'Xarchive.tgz'},
    {create: CreateTgz,
     archive: 'Xarchive.tar.gz'},
    {create: CreateTbz,
     archive: 'Xarchive.tbz'},
    {create: CreateTbz,
     archive: 'Xarchive.tar.bz2'},
    {create: CreateTxz,
     archive: 'Xarchive.txz'},
    {create: CreateTxz,
     archive: 'Xarchive.tar.xz'},
  ]

  if executable('lz4') == 1
    control->add({
      create: CreateTlz4,
      archive: 'Xarchive.tar.lz4'
    })
    control->add({
      create: CreateTlz4,
      archive: 'Xarchive.tlz4'
    })
  endif
  if executable('zstd') == 1
    control->add({
      create: CreateTzst,
      archive: 'Xarchive.tar.zst'
    })
    control->add({
      create: CreateTzst,
      archive: 'Xarchive.tzst'
    })
  endif

  for c in control
    var dir = tempname()
    mkdir(dir, 'R')
    call(c.create, [c.archive, 'hello', dir])

    delete('X.txt')
    execute 'edit ' .. dir .. '/' .. c.archive
    assert_match('X.txt', getline(5), 'line 5 wrong in archive: ' .. c.archive)
    :5
    normal x
    assert_equal(0, v:shell_error, 'vshell error not 0')
    assert_true(filereadable('X.txt'), 'X.txt not readable for archive: ' .. c.archive)
    assert_equal(['hello'], readfile('X.txt'), 'X.txt wrong contents for archive: ' .. c.archive)
    delete('X.txt')
    delete(dir .. '/' .. c.archive)
    bw!
  endfor
enddef

def g:Test_extract_with_dotted_dir()
  delete('X.txt')
  writefile(['when they kiss they spit white noise'], 'X.txt')

  var dirname = tempname()
  mkdir(dirname, 'R')
  dirname = dirname .. '/foo.bar'
  mkdir(dirname, 'R')
  var tarpath = dirname .. '/Xarchive.tar.gz'
  system('tar -czf ' .. tarpath .. ' X.txt')
  assert_true(filereadable(tarpath))
  assert_equal(0, v:shell_error)

  delete('X.txt')
  defer delete(tarpath)

  execute 'e ' .. tarpath
  assert_match('X.txt', getline(5))
  :5
  normal x
  assert_true(filereadable('X.txt'))
  assert_equal(['when they kiss they spit white noise'], readfile('X.txt'))
  delete('X.txt')
  bw!
enddef

def g:Test_extract_with_dotted_filename()
  delete('X.txt')
  writefile(['holiday inn'], 'X.txt')

  var dirname = tempname()
  mkdir(dirname, 'R')
  var tarpath = dirname .. '/Xarchive.foo.tar.gz'
  system('tar -czf ' .. tarpath .. ' X.txt')
  assert_true(filereadable(tarpath))
  assert_equal(0, v:shell_error)

  delete('X.txt')
  defer delete(tarpath)

  execute 'e ' .. tarpath
  assert_match('X.txt', getline(5))
  :5
  normal x
  assert_true(filereadable('X.txt'))
  assert_equal(['holiday inn'], readfile('X.txt'))
  delete('X.txt')
  bw!
enddef
