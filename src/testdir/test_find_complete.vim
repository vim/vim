" Tests for the 'find' command completion.

" Do all the tests in a separate window to avoid E211 when we recursively
" delete the Xfind directory during cleanup
func Test_find_complete()
  set belloff=all

  " On windows a stale "Xfind" directory may exist, remove it so that
  " we start from a clean state.
  call delete("Xfind", "rf")
  let cwd = getcwd()
  let test_out = cwd . '/test.out'
  call mkdir('Xfind', 'R')
  cd Xfind

  new
  set path=
  call assert_fails('call feedkeys(":find \t\n", "xt")', 'E471:')
  close

  new
  set path=.
  call assert_fails('call feedkeys(":find \t\n", "xt")', 'E471:')
  close

  new
  set path=.,,
  call assert_fails('call feedkeys(":find \t\n", "xt")', 'E471:')
  close

  new
  set path=./**
  call assert_fails('call feedkeys(":find \t\n", "xt")', 'E471:')
  close

  " We shouldn't find any file till this point

  call mkdir('in/path', 'p')
  call chdir(cwd)
  call writefile(['Holy Grail'], 'Xfind/file.txt')
  call writefile(['Jimmy Hoffa'], 'Xfind/in/file.txt')
  call writefile(['Another Holy Grail'], 'Xfind/in/stuff.txt')
  call writefile(['E.T.'], 'Xfind/in/path/file.txt')

  new
  set path=Xfind/**
  call feedkeys(":find file\t\n", "xt")
  call assert_equal('Holy Grail', getline(1))
  call feedkeys(":find file\t\t\n", "xt")
  call assert_equal('Jimmy Hoffa', getline(1))
  call feedkeys(":find file\t\t\t\n", "xt")
  call assert_equal('E.T.', getline(1))

  " Rerun the previous three find completions, using fullpath in 'path'
  exec "set path=" . cwd . "/Xfind/**"

  call feedkeys(":find file\t\n", "xt")
  call assert_equal('Holy Grail', getline(1))
  call feedkeys(":find file\t\t\n", "xt")
  call assert_equal('Jimmy Hoffa', getline(1))
  call feedkeys(":find file\t\t\t\n", "xt")
  call assert_equal('E.T.', getline(1))

  " Same steps again, using relative and fullpath items that point to the same
  " recursive location.
  " This is to test that there are no duplicates in the completion list.
  set path+=Xfind/**
  call feedkeys(":find file\t\n", "xt")
  call assert_equal('Holy Grail', getline(1))
  call feedkeys(":find file\t\t\n", "xt")
  call assert_equal('Jimmy Hoffa', getline(1))
  call feedkeys(":find file\t\t\t\n", "xt")
  call assert_equal('E.T.', getline(1))
  call feedkeys(":find file\t\t\n", "xt")

  " Test find completion for directory of current buffer, which at this point
  " is Xfind/in/file.txt.
  set path=.
  call feedkeys(":find st\t\n", "xt")
  call assert_equal('Another Holy Grail', getline(1))

  " Test find completion for empty path item ",," which is the current
  " directory
  cd Xfind
  set path=,,
  call feedkeys(":find f\t\n", "xt")
  call assert_equal('Holy Grail', getline(1))

  " Test that find completion on directory appends a slash
  call feedkeys(":find in/pa\tfile.txt\n", "xt")
  call assert_equal('E.T.', getline(1))
  call feedkeys(":find ./i\tstuff.txt\n", "xt")
  call assert_equal('Another Holy Grail', getline(1))

  " Test shortening of
  "
  "    foo/x/bar/voyager.txt
  "    foo/y/bar/voyager.txt
  "
  " When current directory is above foo/ they should be shortened to (in order
  " of appearance):
  "
  "    x/bar/voyager.txt
  "    y/bar/voyager.txt
  call mkdir('foo/x/bar', 'p')
  call mkdir('foo/y/bar', 'p')
  call writefile(['Voyager 1'], 'foo/x/bar/voyager.txt')
  call writefile(['Voyager 2'], 'foo/y/bar/voyager.txt')

  exec "set path=" . cwd . "/Xfind/**"
  call feedkeys(":find voyager\t\n", "xt")
  call assert_equal('Voyager 1', getline(1))
  call feedkeys(":find voyager\t\t\n", "xt")
  call assert_equal('Voyager 2', getline(1))

  "
  " When current directory is .../foo/y/bar they should be shortened to (in
  " order of appearance):
  "
  "    ./voyager.txt
  "    x/bar/voyager.txt
  cd foo/y/bar
  call feedkeys(":find voyager\t\n", "xt")
  call assert_equal('Voyager 2', getline(1))
  call feedkeys(":find voyager\t\t\n", "xt")
  call assert_equal('Voyager 1', getline(1))

  " Check the opposite too:
  cd ../../x/bar
  call feedkeys(":find voyager\t\n", "xt")
  call assert_equal('Voyager 1', getline(1))
  call feedkeys(":find voyager\t\t\n", "xt")
  call assert_equal('Voyager 2', getline(1))

  " Check for correct handling of shorten_fname()'s behavior on windows
  call chdir(cwd .. "/Xfind/in")
  call feedkeys(":find file\t\n", "xt")
  call assert_equal('Jimmy Hoffa', getline(1))

  " Test for relative to current buffer 'path' item
  call chdir(cwd . "/Xfind/")
  set path=./path
  " Open the file where Jimmy Hoffa is found
  e in/file.txt
  " Find the file containing 'E.T.' in the Xfind/in/path directory
  call feedkeys(":find file\t\n", "xt")
  call assert_equal('E.T.', getline(1))

  " Test that completion works when path=.,,
  set path=.,,
  " Open Jimmy Hoffa file
  e in/file.txt
  call assert_equal('Jimmy Hoffa', getline(1))

  " Search for the file containing Holy Grail in same directory as in/path.txt
  call feedkeys(":find stu\t\n", "xt")
  call assert_equal('Another Holy Grail', getline(1))

  enew | only
  call chdir(cwd)
  set path&
endfunc

" Completion with the depth limiter "**N" in 'path' and 'cdpath'
func Test_find_complete_path_limiter()
  " Xlimit/**/f{0,1,2,...}.txt
  call mkdir('Xlimit/a/b/c/d', 'pR')
  let files = ['f0.txt', 'f1.txt', 'f2.txt', 'f3.txt', 'f4.txt']
  let dir = 'Xlimit'
  for i in range(5)
    call writefile([], dir .. '/' .. files[i])
    " don't append anything for the final loop iter
    let dir ..= '/' .. 'abcd'[i]
  endfor
  let cwd = getcwd()

  let tests = [
        "\ [<path to complete>, <depths to find a file>, <whether it matches findfile()>]
        \ ['Xlimit/**0', [0], 1],
        \ ['Xlimit/**1', [0, 1], 1],
        \ ['Xlimit/**2', [0, 1, 2], 1],
        \ ['Xlimit/**3', [0, 1, 2, 3], 1],
        \ ['Xlimit/**4', [0, 1, 2, 3, 4], 1],
        \ ['Xlimit/**99', [0, 1, 2, 3, 4], 1],
        \ [cwd .. '/Xlimit/**2', [0, 1, 2], 1],
        \ ['Xlimit/a/**1', [1, 2], 1],
        \ ['Xlimit/**0/a', [1], 1],
        \ ['Xlimit/**0/**0/a/**0', [1], 1],
        \ ['Xlimit/**1/b', [2], 1],
        \ ['Xlimit/**3/c', [3], 1],
        \ ['Xlimit/*/**1/c', [3], 1],
        \ ['Xlimit/**1/b/**1', [2, 3], 1],
        \ ['Xlimit/**1/**1', [0, 1, 2], 1],
        \ ['Xlimit/**1,Xlimit/a/b/**1', [0, 1, 2, 3], 1],
        "\ no limit is used when the number is out of range
        \ ['Xlimit/**100', [0, 1, 2, 3, 4], 1],
        \ ['Xlimit/**-1', [0, 1, 2, 3, 4], 1],
        \ ['Xlimit/**1000', [0, 1, 2, 3, 4], 1],
        "\ no limit is used when there would be too many paths, this may find
        "\ more files than findfile() does
        \ ['Xlimit/**9/**9', [0, 1, 2, 3, 4], 1],
        \ ['Xlimit/**10/**10', [0, 1, 2, 3, 4], 1],
        \ ['Xlimit/**60/b/**1', [2, 3, 4], 0],
        "\ not a valid limit
        \ ['Xlimit/**2x', [], 0],
        \ ['Xlimit/a**2', [], 0],
        \ ]

  for [path, depths, check_findfile] in tests
    let &path = path
    let expected = mapnew(depths, {_, v -> files[v]})
    call assert_equal(expected, getcompletion('f', 'file_in_path'), path)
    if check_findfile
      let found = filter(copy(files), {_, v -> findfile(v, path) != ''})
      call assert_equal(expected, found, path)
    endif
  endfor

  " relative to the directory of the current file
  new Xlimit/a/f1.txt
  set path=./**1
  call assert_equal(['f1.txt', 'f2.txt'], getcompletion('f', 'file_in_path'))
  set path=./**0/b/**1
  call assert_equal(['f2.txt', 'f3.txt'], getcompletion('f', 'file_in_path'))

  " relative to the current directory
  cd Xlimit/a
  set path=**1
  call assert_equal(['f1.txt', 'f2.txt'], getcompletion('f', 'file_in_path'))
  set path=**1/c
  call assert_equal(['f3.txt'], getcompletion('f', 'file_in_path'))
  call feedkeys(":find f\<Tab>\<CR>", "xt")
  call assert_equal('f3.txt', expand('%:t'))
  call chdir(cwd)
  %bwipe!

  " directories in 'cdpath'
  let dirs = ['a', 'b', 'c', 'd']
  for [cdpath, depths] in [
        \ ['Xlimit/**0', [0]],
        \ ['Xlimit/**1', [0, 1]],
        \ ['Xlimit/**2', [0, 1, 2]],
        \ ['Xlimit/**1/b', [2]],
        \ ['Xlimit/**1/b/**1', [2, 3]],
        \ ['Xlimit/**100', [0, 1, 2, 3]],
        \ ]

    let &cdpath = cdpath
    let expected = mapnew(depths, {_, v -> dirs[v] .. '/'})
    let found = getcompletion('', 'dir_in_path')
    call map(found, {_, v -> substitute(v, '\\', '/', 'g')})
    call assert_equal(expected, found, cdpath)
  endfor

  set path& cdpath&
endfunc

" Verify that backticks in 'path' are not executed
func Test_find_completion_backtick_in_path()
  CheckUnix
  CheckExecutable id
  set nomodelinestrict modeline

  let lines =<< trim END
    // vim: set path+=`id>Xrce_marker` :
  END

  call writefile(lines, 'Xpoc.c', 'D')
  new Xpoc.c
  call assert_match('`id>Xrce_marker`', &path)
  " Triggering completion must not execute the backtick command.
  call getcompletion('', 'file_in_path')
  call assert_false(filereadable('Xrce_marker'))
  call feedkeys(":find \t\n", "xt")
  call assert_false(filereadable('Xrce_marker'))

  bwipe!
  call delete('Xrce_marker')
  set modelinestrict& modeline&
endfunc

" vim: shiftwidth=2 sts=2 expandtab
