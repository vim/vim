" Test for checking the source code style.

def Test_source_files()
  for fname in glob('../*.[ch]', 0, 1)
    exe 'edit ' .. fname

    cursor(1, 1)
    var lnum = search(' \t')
    assert_equal(0, lnum, fname .. ': space before tab')

    cursor(1, 1)
    lnum = search('\s$')
    assert_equal(0, lnum, fname .. ': trailing white space')

    # some files don't stick to the Vim style rules
    if fname =~ 'iscygpty.c'
      continue
    endif

    # Examples in comments use "condition) {", skip them.
    # Skip if a double quote or digit comes after the "{".
    # Skip specific string used in os_unix.c.
    # Also skip fold markers.
    var skip = 'getline(".") =~ "condition) {" || getline(".") =~ "vimglob_func" || getline(".") =~ "{\"" || getline(".") =~ "{\\d" || getline(".") =~ "{{{"'
    cursor(1, 1)
    lnum = search(')\s*{', '', 0, 0, skip)
    assert_equal(0, lnum, fname .. ': curly after closing paren')

    cursor(1, 1)
    # Examples in comments use double quotes.
    skip = "getline('.') =~ '\"'"
    # Avoid examples that contain: "} else
    lnum = search('[^"]}\s*else', '', 0, 0, skip)
    assert_equal(0, lnum, fname .. ': curly before "else"')

    cursor(1, 1)
    lnum = search('else\s*{', '', 0, 0, skip)
    assert_equal(0, lnum, fname .. ': curly after "else"')
  endfor

  bwipe!
enddef


" vim: shiftwidth=2 sts=2 expandtab
