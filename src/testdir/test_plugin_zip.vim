so check.vim

CheckExecutable unzip

if 0 " Find uncovered line
  profile start zip_profile
  profile! file */zip*.vim
endif

runtime plugin/zipPlugin.vim

def Test_zip_basic()

  ### get our zip file
  if !filecopy("samples/test.zip", "X.zip")
    assert_report("Can't copy samples/test.zip")
    return
  endif
  defer delete("X.zip")

  e X.zip

  ### Check header
  assert_match('^" zip\.vim version v\d\+', getline(1))
  assert_match('^" Browsing zipfile .*/X.zip', getline(2))
  assert_match('^" Select a file with cursor and press ENTER', getline(3))
  assert_match('^$', getline(4))

  ### Check files listing
  assert_equal(["Xzip/", "Xzip/dir/", "Xzip/file.txt"], getline(5, 7))

  ### Check ENTER on header
  :1
  exe ":normal \<cr>"
  assert_equal("X.zip", @%)

  ### Check ENTER on directory
  :1|:/^$//dir/
  assert_match('Please specify a file, not a directory',
               execute("normal \<CR>"))

  ### Check ENTER on file
  :1
  search('file.txt')
  exe ":normal \<cr>"
  assert_match('zipfile://.*/X.zip::Xzip/file.txt', @%)
  assert_equal('one', getline(1))

  ### Check editing file
  if executable("zip")
    s/one/two/
    assert_equal("two", getline(1))
    w
    bw|bw
    e X.zip

    :1|:/^$//file/
    exe "normal \<cr>"
    assert_equal("two", getline(1))
  endif

  only
  e X.zip

  ### Check extracting file
  :1|:/^$//file/
  normal x
  assert_true(filereadable("Xzip/file.txt"))

  ## Check not overwriting existing file
  assert_match('<Xzip/file.txt> .* not overwriting!', execute("normal x"))

  delete("Xzip", "rf")

  ### Check extracting directory
  :1|:/^$//dir/
  assert_match('Please specify a file, not a directory', execute("normal x"))
  assert_equal("X.zip", @%)

  ### Check "x" on header
  :1
  normal x
  assert_equal("X.zip", @%)
  bw

  ### Check opening zip when "unzip" program is missing
  var save_zip_unzipcmd = g:zip_unzipcmd
  g:zip_unzipcmd = "/"
  assert_match('unzip not available on your system', execute("e X.zip"))

  ### Check when "unzip" don't work
  if executable("false")
    g:zip_unzipcmd = "false"
    assert_match('X\.zip is not a zip file', execute("e X.zip"))
  endif
  bw

  g:zip_unzipcmd = save_zip_unzipcmd
  e X.zip

  ### Check opening file when "unzip" is missing
  g:zip_unzipcmd = "/"
  assert_match('sorry, your system doesn''t appear to have the / program',
               execute("normal \<CR>"))

  bw|bw
  g:zip_unzipcmd = save_zip_unzipcmd
  e X.zip

  ### Check :write when "zip" program is missing
  :1|:/^$//file/
  exe "normal \<cr>Goanother\<esc>"
  var save_zip_zipcmd = g:zip_zipcmd
  g:zip_zipcmd = "/"
  assert_match('sorry, your system doesn''t appear to have the / program',
               execute("write"))

  ### Check when "zip" report failure
  if executable("false")
    g:zip_zipcmd = "false"
    assert_match('sorry, unable to update .*/X.zip with Xzip/file.txt',
                  execute("write"))
  endif
  bw!|bw

  g:zip_zipcmd = save_zip_zipcmd

  ### Check opening an no zipfile
  writefile(["qsdf"], "Xcorupt.zip", "D")
  e! Xcorupt.zip
  assert_equal("qsdf", getline(1))

  bw

  ### Check no existing zipfile
  assert_match('File not readable', execute("e Xnot_exists.zip"))

  bw
enddef

def Test_zip_glob_fname()
  CheckNotMSWindows
  # does not work on Windows, why?

  ### copy sample zip file
  if !filecopy("samples/testa.zip", "X.zip")
    assert_report("Can't copy samples/testa.zip")
    return
  endif
  defer delete("X.zip")
  defer delete('zipglob', 'rf')

  e X.zip

  ### 1) Check extracting strange files
  :1
  var fname = 'a[a].txt'
  search('\V' .. fname)
  normal x
  assert_true(filereadable('zipglob/' .. fname))
  delete('zipglob', 'rf')

  :1
  fname = 'a*.txt'
  search('\V' .. fname)
  normal x
  assert_true(filereadable('zipglob/' .. fname))
  delete('zipglob', 'rf')

  :1
  fname = 'a?.txt'
  search('\V' .. fname)
  normal x
  assert_true(filereadable('zipglob/' .. fname))
  delete('zipglob', 'rf')

  :1
  fname = 'a\.txt'
  search('\V' .. escape(fname, '\\'))
  normal x
  assert_true(filereadable('zipglob/' .. fname))
  delete('zipglob', 'rf')

  :1
  fname = 'a\\.txt'
  search('\V' .. escape(fname, '\\'))
  normal x
  assert_true(filereadable('zipglob/' .. fname))
  delete('zipglob', 'rf')

  ### 2) Check entering strange file names
  :1
  fname = 'a[a].txt'
  search('\V' .. fname)
  exe ":normal \<cr>"
  assert_match('zipfile://.*/X.zip::zipglob/a\[a\].txt', @%)
  assert_equal('a test file with []', getline(1))
  bw

  e X.zip
  :1
  fname = 'a*.txt'
  search('\V' .. fname)
  exe ":normal \<cr>"
  assert_match('zipfile://.*/X.zip::zipglob/a\*.txt', @%)
  assert_equal('a test file with a*', getline(1))
  bw

  e X.zip
  :1
  fname = 'a?.txt'
  search('\V' .. fname)
  exe ":normal \<cr>"
  assert_match('zipfile://.*/X.zip::zipglob/a?.txt', @%)
  assert_equal('a test file with a?', getline(1))
  bw

  e X.zip
  :1
  fname = 'a\.txt'
  search('\V' .. escape(fname, '\\'))
  exe ":normal \<cr>"
  assert_match('zipfile://.*/X.zip::zipglob/a\\.txt', @%)
  assert_equal('a test file with a\', getline(1))
  bw

  e X.zip
  :1
  fname = 'a\\.txt'
  search('\V' .. escape(fname, '\\'))
  exe ":normal \<cr>"
  assert_match('zipfile://.*/X.zip::zipglob/a\\\\.txt', @%)
  assert_equal('a test file with a double \', getline(1))
  bw

  bw
enddef
