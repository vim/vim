" Test the SessionWritePost autocommand

source check.vim

" Ensure `expected` was just recently written as a Vim session
func s:assert_session_path(expected)
  call assert_equal(a:expected, v:this_session)
endfunc

" Check for `expected` after a session is written to-disk.
func s:watch_for_session_path(expected)
  execute 'autocmd SessionWritePost * ++once execute "call s:assert_session_path(\"'
        \ . a:expected
        \ . '\")"'
endfunc

" Ensure v:this_session gets the full session path, if explicitly stated
func Test_explicit_session_absolute_path()
  %bwipeout!

  let l:directory = getcwd()

  let v:this_session = ""
  let l:name = "some_file.vim"
  let l:expected = fnamemodify(l:name, ":p")
  call s:watch_for_session_path(l:expected)
  execute "mksession! " . l:expected

  call delete(l:expected)
endfunc

" Ensure v:this_session gets the full session path, if explicitly stated
func Test_explicit_session_relative_path()
  %bwipeout!

  let l:directory = getcwd()

  let v:this_session = ""
  let l:name = "some_file.vim"
  let l:expected = fnamemodify(l:name, ":p")
  call s:watch_for_session_path(l:expected)
  execute "mksession! " . l:name

  call delete(l:expected)
endfunc

" Ensure v:this_session gets the full session path, if not specified
func Test_implicit_session()
  %bwipeout!

  let l:directory = getcwd()

  let v:this_session = ""
  let l:expected = fnamemodify("Session.vim", ":p")
  call s:watch_for_session_path(l:expected)
  mksession!

  call delete(l:expected)
endfunc
