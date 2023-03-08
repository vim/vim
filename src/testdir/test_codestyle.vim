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

def Test_test_files()
  for fname in glob('*.vim', 0, 1)
    exe 'edit ' .. fname

    # some files intentionally have misplaced white space
    if fname =~ 'test_cindent.vim' || fname =~ 'test_join.vim'
      continue
    endif

    # skip files that are known to have a space before a tab
    if fname !~ 'test_comments.vim'
        && fname !~ 'test_listchars.vim'
        && fname !~ 'test_visual.vim'
      cursor(1, 1)
      var lnum = search(fname =~ "test_regexp_latin" ? '[^á] \t' : ' \t')
      assert_equal(0, lnum, 'testdir/' .. fname .. ': space before tab')
    endif

    # skip files that are known to have trailing white space
    if fname !~ 'test_cmdline.vim'
            && fname !~ 'test_let.vim'
            && fname !~ 'test_tagjump.vim'
            && fname !~ 'test_vim9_cmd.vim'
      cursor(1, 1)
      var lnum = search(
          fname =~ 'test_vim9_assign.vim' ? '[^=]\s$'
          : fname =~ 'test_vim9_class.vim' ? '[^)]\s$'
          : fname =~ 'test_vim9_script.vim' ? '[^,:3]\s$'
          : fname =~ 'test_visual.vim' ? '[^/]\s$'
          : '[^\\]\s$')
      assert_equal(0, lnum, 'testdir/' .. fname .. ': trailing white space')
    endif
  endfor

  bwipe!
enddef

def Test_help_files()
  var lnum: number
  set nowrapscan

  for fpath in glob('../../runtime/doc/*.txt', 0, 1)
    exe 'edit ' .. fpath

    var fname = fnamemodify(fpath, ":t")

    # todo.txt is for developers, it's not need a strictly check
    # version*.txt is a history and large size, so it's not checked
    if fname == 'todo.txt' || fname =~ 'version.*\.txt'
      continue
    endif

    # Check for mixed tabs and spaces
    cursor(1, 1)
    while 1
      lnum = search('[^/] \t')
      if fname == 'visual.txt' && getline(lnum) =~ "STRING  \tjkl"
        || fname == 'usr_27.txt' && getline(lnum) =~ "\[^\? \t\]"
        continue
      endif
      assert_equal(0, lnum, fpath .. ': space before tab')
      if lnum == 0
        break
      endif
    endwhile

    # Check for unnecessary whitespace at the end of a line
    cursor(1, 1)
    while 1
      lnum = search('[^/~\\]\s$')
      # skip line that are known to have trailing white space
      if fname == 'map.txt' && getline(lnum) =~ "unmap @@ $"
        || fname == 'usr_12.txt' && getline(lnum) =~ "^\t/ \t$"
        || fname == 'usr_41.txt' && getline(lnum) =~ "map <F4> o#include  $"
        || fname == 'change.txt' && getline(lnum) =~ "foobar bla $"
        continue
      endif
      assert_equal(0, lnum, fpath .. ': trailing white space')
      if lnum == 0
        break
      endif
    endwhile

    # TODO: Do check and fix help files
#    # Check over 80 columns
#    cursor(1, 1)
#    while 1
#      lnum = search('\%>80v.*$')
#      assert_equal(0, lnum, fpath .. ': line over 80 columns')
#      if lnum == 0
#        break
#      endif
#    endwhile

  endfor

  set wrapscan&vim
  bwipe!
enddef


" vim: shiftwidth=2 sts=2 expandtab
