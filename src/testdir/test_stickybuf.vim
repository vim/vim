" Test 'stickybuf'

source check.vim

" Find the number of open windows in the current tab
func s:get_windows_count()
  return tabpagewinnr(tabpagenr(), '$')
endfunc

" Create some unnamed buffers.
func s:make_buffers_list()
  enew
  file first
  let l:first = bufnr()

  enew
  file middle
  let l:middle = bufnr()

  enew
  file last
  let l:last = bufnr()
  set stickybuf

  return [l:first, l:last]
endfunc

" Create some unnamed buffers and add them to an args list
func s:make_args_list()
  let [l:first, l:last] = s:make_buffers_list()

  args! first middle last

  return [l:first, l:last]
endfunc

" Create two buffers and then set the window to 'stickybuf'
func s:make_buffer_pairs(...)
  let l:reversed = get(a:, 1, 0)

  if l:reversed == 1
    enew
    file original
    set stickybuf

    enew!
    file other
    let l:other = bufnr()

    return l:other
  endif

  enew
  file other
  let l:other = bufnr()

  enew
  file current
  set stickybuf

  return l:other
endfunc

" Create 3 quick buffers and set the window to 'stickybuf'
func s:make_buffer_trio()
  edit first
  let l:first = bufnr()
  edit second
  let l:second = bufnr()
  set stickybuf
  edit! third
  let l:third = bufnr()

  execute ":buffer! " . l:second

  return [l:first, l:second, l:third]
endfunc

" Create a location list with at least 2 entries + a 'stickybuf' window.
func s:make_simple_location_list()
  enew
  file middle
  let l:middle = bufnr()
  call append(0, ["sticky search-term", "another line"])

  enew!
  file first
  let l:first = bufnr()
  call append(0, "first search-term")

  enew!
  file last
  let l:last = bufnr()
  call append(0, "last search-term")

  call setloclist(
  \  0,
  \  [
  \    {
  \      "filename": "first",
  \      "bufnr": l:first,
  \      "lnum": 1,
  \    },
  \    {
  \      "filename": "middle",
  \      "bufnr": l:middle,
  \      "lnum": 1,
  \    },
  \    {
  \      "filename": "middle",
  \      "bufnr": l:middle,
  \      "lnum": 2,
  \    },
  \    {
  \      "filename": "last",
  \      "bufnr": l:last,
  \      "lnum": 1,
  \    },
  \  ]
  \)

  set stickybuf

  return [l:first, l:middle, l:last]
endfunc

" Create a quickfix with at least 2 entries that are in the current 'stickybuf' window.
func s:make_simple_quickfix()
  enew
  file current
  let l:current = bufnr()
  call append(0, ["sticky search-term", "another line"])

  enew!
  file first
  let l:first = bufnr()
  call append(0, "first search-term")

  enew!
  file last
  let l:last = bufnr()
  call append(0, "last search-term")

  call setqflist(
  \  [
  \    {
  \      "filename": "first",
  \      "bufnr": l:first,
  \      "lnum": 1,
  \    },
  \    {
  \      "filename": "current",
  \      "bufnr": l:current,
  \      "lnum": 1,
  \    },
  \    {
  \      "filename": "current",
  \      "bufnr": l:current,
  \      "lnum": 2,
  \    },
  \    {
  \      "filename": "last",
  \      "bufnr": l:last,
  \      "lnum": 1,
  \    },
  \  ]
  \)

  set stickybuf

  return [l:current, l:last]
endfunc

" Create a quickfix with at least 2 entries that are in the current 'stickybuf' window.
func s:make_quickfix_windows()
  let [l:current, _] = s:make_simple_quickfix()
  execute "buffer! " . l:current

  split
  let l:first_window = win_getid()
  execute "normal \<C-w>j"
  let l:sticky_window = win_getid()

  " Open the quickfix in a separate split and go to it
  copen
  let l:quickfix_window = win_getid()

  return [l:first_window, l:sticky_window, l:quickfix_window]
endfunc

" Revert all changes that occurred in any past test
func s:reset_all_buffers()
  %bwipeout!
  set nostickybuf

  call setqflist([])

  for l:window_info in getwininfo()
    call setloclist(l:window_info["winid"], [])
  endfor

  delmarks A-Z0-9
endfunc

" Try to run `command` and remember if the command raises a known error code
func s:execute_try_catch(command)
  let l:caught = 0

  try
    execute a:command
  catch /E1513:/
    return 1
  catch /E1514:/
    return 1
  endtry

  return 0
endfunc

" Find and set the first quickfix entry that points to `buffer`
func s:set_quickfix_by_buffer(buffer)
  let l:index = 1  " quickfix indices start at 1
  for l:entry in getqflist()
    if l:entry["bufnr"] == a:buffer
      execute l:index . "cc"

      return
    endif

    let l:index += 1
  endfor

  echoerr 'No quickfix entry matching "' . a:buffer . '" could be found.'
endfunc

" Fail to call :Next on a 'stickybuf' window unless :Next! is used.
func Test_Next()
  CheckFeature quickfix
  call s:reset_all_buffers()

  let [l:first, _] = s:make_args_list()
  next!

  let l:caught = s:execute_try_catch("Next")
  call assert_equal(1, l:caught)
  call assert_notequal(l:first, bufnr())

  let l:caught = s:execute_try_catch("Next!")
  call assert_equal(0, l:caught)
  call assert_equal(l:first, bufnr())
endfunc

" Call :argdo and choose the next available 'nostickybuf' window.
func Test_argdo_choose_available_window()
  call s:reset_all_buffers()
  let [_, l:last] = s:make_args_list()

  " Make a split window that is 'nostickybuf' but make it the second-to-last
  " window so that :argdo will first try the 'stickybuf' window, pass over it,
  " and prefer the other 'nostickybuf' window, instead.
  "
  " +-------------------+
  " |   'nostickybuf'   |
  " +-------------------+
  " |    'stickybuf'    |  <-- Cursor is here
  " +-------------------+
  split
  let l:nostickybuf_window = win_getid()
  " Move to the 'stickybuf' window now
  execute "normal \<C-w>j"
  let l:stickybuf_window = win_getid()

  let l:caught = s:execute_try_catch("argdo echo ''")
  call assert_equal(0, l:caught)
  call assert_equal(l:nostickybuf_window, win_getid())
  call assert_equal(l:last, bufnr())
endfunc

" Call :argdo and create a new split window if all available windows are 'stickybuf'.
func Test_argdo_make_new_window()
  call s:reset_all_buffers()
  let [l:first, l:last] = s:make_args_list()

  let l:current = win_getid()

  let l:caught = s:execute_try_catch("argdo echo ''")
  call assert_equal(0, l:caught)
  call assert_notequal(l:current, win_getid())
  call assert_equal(l:last, bufnr())
  execute "normal \<C-w>j"
  call assert_equal(l:first, bufnr())
endfunc

" Fail :argedit but :argedit! is allowed
func Test_argedit()
  call s:reset_all_buffers()

  args! first middle last
  enew
  file first
  let l:first = bufnr()

  enew
  file middle
  let l:middle = bufnr()

  enew
  file last
  let l:last = bufnr()

  set stickybuf

  let l:current = bufnr()
  let l:caught = s:execute_try_catch("argedit first middle last")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("argedit! first middle last")
  call assert_equal(0, l:caught)
  call assert_equal(l:first, bufnr())
endfunc

" Fail :arglocal but :arglocal! is allowed
func Test_arglocal()
  call s:reset_all_buffers()

  let l:other = s:make_buffer_pairs()
  let l:current = bufnr()
  argglobal! other
  execute "buffer! " . l:current

  let l:caught = s:execute_try_catch("arglocal other")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("arglocal! other")
  call assert_equal(0, l:caught)
  call assert_equal(l:other, bufnr())
endfunc

" Fail :argglobal but :argglobal! is allowed
func Test_argglobal()
  call s:reset_all_buffers()

  let l:other = s:make_buffer_pairs()
  let l:current = bufnr()

  let l:caught = s:execute_try_catch("argglobal other")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("argglobal! other")
  call assert_equal(0, l:caught)
  call assert_equal(l:other, bufnr())
endfunc

" Fail :args but :args! is allowed
func Test_args()
  call s:reset_all_buffers()

  let [l:first, _] = s:make_buffers_list()

  let l:current = bufnr()
  let l:caught = s:execute_try_catch("args first middle last")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("args! first middle last")
  call assert_equal(0, l:caught)
  call assert_equal(l:first, bufnr())
endfunc

" Fail :bNext but :bNext! is allowed
func Test_bNext()
  call s:reset_all_buffers()

  let l:other = s:make_buffer_pairs()

  let l:caught = s:execute_try_catch("bNext")
  let l:current = bufnr()
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("bNext!")
  call assert_equal(0, l:caught)
  call assert_equal(l:other, bufnr())
endfunc

" Allow :badd because it doesn't actually change the current window's buffer
func Test_badd()
  call s:reset_all_buffers()

  call s:make_buffer_pairs()
  let l:current = bufnr()

  let l:caught = s:execute_try_catch("badd other")
  call assert_equal(0, l:caught)
  call assert_equal(l:current, bufnr())
endfunc

" Allow :balt because it doesn't actually change the current window's buffer
func Test_balt()
  call s:reset_all_buffers()

  call s:make_buffer_pairs()

  let l:current = bufnr()

  let l:caught = s:execute_try_catch("balt other")
  call assert_equal(0, l:caught)
  call assert_equal(l:current, bufnr())
endfunc

" Fail :bfirst but :bfirst! is allowed
func Test_bfirst()
  call s:reset_all_buffers()

  let l:other = s:make_buffer_pairs()
  let l:current = bufnr()

  let l:caught = s:execute_try_catch("bfirst")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("bfirst!")
  call assert_equal(0, l:caught)
  call assert_equal(l:other, bufnr())
endfunc

" Fail :blast but :blast! is allowed
func Test_blast()
  call s:reset_all_buffers()

  let l:other = s:make_buffer_pairs(1)
  bfirst!
  let l:current = bufnr()

  let l:caught = s:execute_try_catch("blast")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("blast!")
  call assert_equal(0, l:caught)
  call assert_equal(l:other, bufnr())
endfunc

" Fail :bmodified but :bmodified! is allowed
func Test_bmodified()
  call s:reset_all_buffers()

  let l:other = s:make_buffer_pairs()
  let l:current = bufnr()

  execute "buffer! " . l:other
  set modified
  execute "buffer! " . l:current

  let l:caught = s:execute_try_catch("bmodified")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("bmodified!")
  call assert_equal(0, l:caught)
  call assert_equal(l:other, bufnr())
endfunc

" Fail :bnext but :bnext! is allowed
func Test_bnext()
  call s:reset_all_buffers()

  let l:other = s:make_buffer_pairs()
  let l:current = bufnr()

  let l:caught = s:execute_try_catch("bnext")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("bnext!")
  call assert_equal(0, l:caught)
  call assert_equal(l:other, bufnr())
endfunc

" Fail :bprevious but :bprevious! is allowed
func Test_bprevious()
  call s:reset_all_buffers()

  let l:other = s:make_buffer_pairs()
  let l:current = bufnr()

  let l:caught = s:execute_try_catch("bprevious")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("bprevious!")
  call assert_equal(0, l:caught)
  call assert_equal(l:other, bufnr())
endfunc

" Fail :brewind but :brewind! is allowed
func Test_brewind()
  call s:reset_all_buffers()

  let l:other = s:make_buffer_pairs()
  let l:current = bufnr()

  let l:caught = s:execute_try_catch("brewind")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("brewind!")
  call assert_equal(0, l:caught)
  call assert_equal(l:other, bufnr())
endfunc

" Call :bufdo and choose the next available 'nostickybuf' window.
func Test_bufdo_choose_available_window()
  call s:reset_all_buffers()

  let l:other = s:make_buffer_pairs()

  " Make a split window that is 'nostickybuf' but make it the second-to-last
  " window so that :bufdo will first try the 'stickybuf' window, pass over it,
  " and prefer the other 'nostickybuf' window, instead.
  "
  " +-------------------+
  " |   'nostickybuf'   |
  " +-------------------+
  " |    'stickybuf'    |  <-- Cursor is here
  " +-------------------+
  split
  let l:nostickybuf_window = win_getid()
  " Move to the 'stickybuf' window now
  execute "normal \<C-w>j"
  let l:stickybuf_window = win_getid()

  let l:current = bufnr()
  call assert_notequal(l:current, l:other)

  let l:caught = s:execute_try_catch("bufdo echo ''")
  call assert_equal(0, l:caught)
  call assert_equal(l:nostickybuf_window, win_getid())
  call assert_notequal(l:other, bufnr())
endfunc

" Call :bufdo and create a new split window if all available windows are 'stickybuf'.
func Test_bufdo_make_new_window()
  call s:reset_all_buffers()
  let [l:first, l:last] = s:make_buffers_list()
  execute "buffer! " . l:first

  let l:current = win_getid()

  let l:caught = s:execute_try_catch("bufdo echo ''")
  call assert_equal(0, l:caught)
  call assert_notequal(l:current, win_getid())
  call assert_equal(l:last, bufnr())
  execute "normal \<C-w>j"
  call assert_equal(l:first, bufnr())
endfunc

" Fail :buffer but :buffer! is allowed
func Test_buffer()
  call s:reset_all_buffers()

  let l:other = s:make_buffer_pairs()
  let l:current = bufnr()

  let l:caught = s:execute_try_catch("buffer " . l:other)
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("buffer! " . l:other)
  call assert_equal(0, l:caught)
  call assert_equal(l:other, bufnr())
endfunc

" Allow :cNext but the 'nostickybuf' window is selected, instead
func Test_cNext()
  CheckFeature quickfix
  call s:reset_all_buffers()

  let [l:first_window, l:sticky_window, l:quickfix_window] = s:make_quickfix_windows()

  " The call to `:cNext` succeeds but it selects the window with 'nostickybuf' instead
  call s:set_quickfix_by_buffer(winbufnr(l:sticky_window))

  " Make sure the previous window has 'stickybuf' so we can test that our
  " "skip 'stickybuf' window" logic works.
  call win_gotoid(l:sticky_window)
  call win_gotoid(l:quickfix_window)

  let l:caught = s:execute_try_catch("cNext")
  call assert_equal(0, l:caught)
  call assert_equal(l:first_window, win_getid())
endfunc

" Allow :cNfile but the 'nostickybuf' window is selected, instead
func Test_cNfile()
  CheckFeature quickfix
  call s:reset_all_buffers()

  let [l:first_window, l:sticky_window, l:quickfix_window] = s:make_quickfix_windows()

  " The call to `:cNfile` succeeds but it selects the window with 'nostickybuf' instead
  call s:set_quickfix_by_buffer(winbufnr(l:sticky_window))
  cnext!

  " Make sure the previous window has 'stickybuf' so we can test that our
  " "skip 'stickybuf' window" logic works.
  call win_gotoid(l:sticky_window)
  call win_gotoid(l:quickfix_window)

  let l:caught = s:execute_try_catch("cNfile")
  call assert_equal(0, l:caught)
  call assert_equal(l:first_window, win_getid())
endfunc

" Allow :cbuffer but don't switch to the first much unless :cbuffer! is used
func Test_cbuffer()
  CheckFeature quickfix
  call s:reset_all_buffers()

  let l:file_path = tempname()
  call writefile(["first.unittest:1:Error - bad-thing-found"], l:file_path)
  execute "edit " . l:file_path
  let l:file_buffer = bufnr()
  let l:current = bufnr()

  edit first.unittest
  call append(0, ["some-search-term bad-thing-found"])

  edit! other.unittest
  set stickybuf

  execute "buffer! " . l:file_buffer

  let l:caught = s:execute_try_catch("cbuffer " . l:file_buffer)
  call assert_equal(0, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("cbuffer! " . l:file_buffer)
  call assert_equal(0, l:caught)
  call assert_equal("first.unittest", expand("%:t"))
endfunc

" Allow :cc but the 'nostickybuf' window is selected, instead
func Test_cc()
  CheckFeature quickfix
  call s:reset_all_buffers()

  let [l:first_window, l:sticky_window, l:quickfix_window] = s:make_quickfix_windows()

  " The call to `:cnext` succeeds but it selects the window with 'nostickybuf' instead
  call s:set_quickfix_by_buffer(winbufnr(l:sticky_window))

  " Make sure the previous window has 'stickybuf' so we can test that our
  " "skip 'stickybuf' window" logic works.
  call win_gotoid(l:sticky_window)
  call win_gotoid(l:quickfix_window)
  " Go up one line in the quickfix window to an quickfix entry that doesn't
  " point to a stickybuf buffer
  normal k
  " Attempt to make the previous window, stickbuf buffer, to go to the
  " non-stickybuf quickfix entry
  .cc

  " Confirm that :.cc did not change the stickbuf-enabled window
  call assert_equal(l:first_window, win_getid())
endfunc

" Call :cdo and choose the next available 'nostickybuf' window.
func Test_cdo_choose_available_window()
  CheckFeature quickfix
  call s:reset_all_buffers()
  let [l:current, l:last] = s:make_simple_quickfix()
  execute "buffer! " . l:current

  " Make a split window that is 'nostickybuf' but make it the second-to-last
  " window so that :cdo will first try the 'stickybuf' window, pass over it,
  " and prefer the other 'nostickybuf' window, instead.
  "
  " +-------------------+
  " |   'nostickybuf'   |
  " +-------------------+
  " |    'stickybuf'    |  <-- Cursor is here
  " +-------------------+
  split
  let l:nostickybuf_window = win_getid()
  " Move to the 'stickybuf' window now
  execute "normal \<C-w>j"
  let l:stickybuf_window = win_getid()

  let l:caught = s:execute_try_catch("cdo echo ''")
  call assert_equal(0, l:caught)
  call assert_equal(l:nostickybuf_window, win_getid())
  call assert_equal(l:last, bufnr())
  execute "normal \<C-w>j"
  call assert_equal(l:current, bufnr())
endfunc

" Call :cdo and create a new split window if all available windows are 'stickybuf'.
func Test_cdo_make_new_window()
  CheckFeature quickfix
  call s:reset_all_buffers()
  let [l:current_buffer, l:last] = s:make_simple_quickfix()
  execute "buffer! " . l:current_buffer

  let l:current_window = win_getid()

  let l:caught = s:execute_try_catch("cdo echo ''")
  call assert_equal(0, l:caught)
  call assert_notequal(l:current_window, win_getid())
  call assert_equal(l:last, bufnr())
  execute "normal \<C-w>j"
  call assert_equal(l:current_buffer, bufnr())
endfunc

" Allow :cexpr but don't switch to the first much unless :cexpr! is used
func Test_cexpr()
  CheckFeature quickfix
  call s:reset_all_buffers()


  let l:file = tempname()
  let l:entry = '["' . l:file . ':1:bar"]'
  let l:current = bufnr()

  set stickybuf

  let l:caught = s:execute_try_catch('cexpr ' . l:entry)
  call assert_equal(0, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch('cexpr! ' . l:entry)
  call assert_equal(0, l:caught)
  call assert_equal(fnamemodify(l:file, ":t"), expand("%:t"))
endfunc

" Allow :cfile but don't swap to the first file unless ':cfile! is used
func Test_cfile()
  CheckFeature quickfix
  call s:reset_all_buffers()

  edit first.unittest
  call append(0, ["some-search-term bad-thing-found"])
  write
  let l:first = bufnr()

  edit! second.unittest
  call append(0, ["some-search-term"])
  write

  let l:file = tempname()
  call writefile(["first.unittest:1:Error - bad-thing-found was detected"], l:file)

  let l:current = bufnr()
  set stickybuf

  let l:caught = s:execute_try_catch(":cfile " . l:file)
  call assert_equal(0, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch(":cfile! " . l:file)
  call assert_equal(0, l:caught)
  call assert_equal(l:first, bufnr())

  call delete(l:file)
  call delete("first.unittest")
  call delete("second.unittest")
endfunc

" Allow :cfirst but the 'nostickybuf' window is selected, instead
func Test_cfirst()
  CheckFeature quickfix
  call s:reset_all_buffers()

  let [l:first_window, l:sticky_window, l:quickfix_window] = s:make_quickfix_windows()

  " The call to `:cfirst` succeeds but it selects the window with 'nostickybuf' instead
  call s:set_quickfix_by_buffer(winbufnr(l:sticky_window))

  " Make sure the previous window has 'stickybuf' so we can test that our
  " "skip 'stickybuf' window" logic works.
  call win_gotoid(l:sticky_window)
  call win_gotoid(l:quickfix_window)

  let l:caught = s:execute_try_catch("cfirst")
  call assert_equal(0, l:caught)
  call assert_equal(l:first_window, win_getid())
endfunc

" Allow :clast but the 'nostickybuf' window is selected, instead
func Test_clast()
  CheckFeature quickfix
  call s:reset_all_buffers()

  let [l:first_window, l:sticky_window, l:quickfix_window] = s:make_quickfix_windows()

  " The call to `:clast` succeeds but it selects the window with 'nostickybuf' instead
  call s:set_quickfix_by_buffer(winbufnr(l:sticky_window))

  " Make sure the previous window has 'stickybuf' so we can test that our
  " "skip 'stickybuf' window" logic works.
  call win_gotoid(l:sticky_window)
  call win_gotoid(l:quickfix_window)

  let l:caught = s:execute_try_catch("clast")
  call assert_equal(0, l:caught)
  call assert_equal(l:first_window, win_getid())
endfunc

" Allow :cnext but the 'nostickybuf' window is selected, instead
func Test_cnext()
  CheckFeature quickfix
  call s:reset_all_buffers()

  let [l:first_window, l:sticky_window, l:quickfix_window] = s:make_quickfix_windows()

  " The call to `:cnext` succeeds but it selects the window with 'nostickybuf' instead
  call s:set_quickfix_by_buffer(winbufnr(l:sticky_window))
  cnext!

  " Make sure the previous window has 'stickybuf' so we can test that our
  " "skip 'stickybuf' window" logic works.
  call win_gotoid(l:sticky_window)
  call win_gotoid(l:quickfix_window)

  let l:caught = s:execute_try_catch("cnext")
  call assert_equal(0, l:caught)
  call assert_equal(l:first_window, win_getid())
endfunc

" Allow :cnext and create a 'nostickybuf' window if none exists
func Test_cnext_make_new_window()
  CheckFeature quickfix
  call s:reset_all_buffers()

  let [l:current, _] = s:make_simple_quickfix()
  let l:current = win_getid()

  cfirst!

  let l:windows = s:get_windows_count()
  let l:expected = l:windows + 1  " We're about to create a new split window

  let l:caught = s:execute_try_catch("cnext")
  call assert_equal(0, l:caught)
  call assert_equal(l:expected, s:get_windows_count())

  let l:caught = s:execute_try_catch("cnext!")
  call assert_equal(0, l:caught)
  call assert_equal(l:expected, s:get_windows_count())
endfunc

" Allow :cprevious but the 'nostickybuf' window is selected, instead
func Test_cprevious()
  CheckFeature quickfix
  call s:reset_all_buffers()

  let [l:first_window, l:sticky_window, l:quickfix_window] = s:make_quickfix_windows()

  " The call to `:cprevious` succeeds but it selects the window with 'nostickybuf' instead
  call s:set_quickfix_by_buffer(winbufnr(l:sticky_window))

  " Make sure the previous window has 'stickybuf' so we can test that our
  " "skip 'stickybuf' window" logic works.
  call win_gotoid(l:sticky_window)
  call win_gotoid(l:quickfix_window)

  let l:caught = s:execute_try_catch("cprevious")
  call assert_equal(0, l:caught)
  call assert_equal(l:first_window, win_getid())
endfunc

" Allow :cnfile but the 'nostickybuf' window is selected, instead
func Test_cnfile()
  CheckFeature quickfix
  call s:reset_all_buffers()

  let [l:first_window, l:sticky_window, l:quickfix_window] = s:make_quickfix_windows()

  " The call to `:cnfile` succeeds but it selects the window with 'nostickybuf' instead
  call s:set_quickfix_by_buffer(winbufnr(l:sticky_window))
  cnext!

  " Make sure the previous window has 'stickybuf' so we can test that our
  " "skip 'stickybuf' window" logic works.
  call win_gotoid(l:sticky_window)
  call win_gotoid(l:quickfix_window)

  let l:caught = s:execute_try_catch("cnfile")
  call assert_equal(0, l:caught)
  call assert_equal(l:first_window, win_getid())
endfunc

" Allow :cpfile but the 'nostickybuf' window is selected, instead
func Test_cpfile()
  CheckFeature quickfix
  call s:reset_all_buffers()

  let [l:first_window, l:sticky_window, l:quickfix_window] = s:make_quickfix_windows()

  " The call to `:cpfile` succeeds but it selects the window with 'nostickybuf' instead
  call s:set_quickfix_by_buffer(winbufnr(l:sticky_window))
  cnext!

  " Make sure the previous window has 'stickybuf' so we can test that our
  " "skip 'stickybuf' window" logic works.
  call win_gotoid(l:sticky_window)
  call win_gotoid(l:quickfix_window)

  let l:caught = s:execute_try_catch("cpfile")
  call assert_equal(0, l:caught)
  call assert_equal(l:first_window, win_getid())
endfunc

" Allow :crewind but the 'nostickybuf' window is selected, instead
func Test_crewind()
  CheckFeature quickfix
  call s:reset_all_buffers()

  let [l:first_window, l:sticky_window, l:quickfix_window] = s:make_quickfix_windows()

  " The call to `:crewind` succeeds but it selects the window with 'nostickybuf' instead
  call s:set_quickfix_by_buffer(winbufnr(l:sticky_window))
  cnext!

  " Make sure the previous window has 'stickybuf' so we can test that our
  " "skip 'stickybuf' window" logic works.
  call win_gotoid(l:sticky_window)
  call win_gotoid(l:quickfix_window)

  let l:caught = s:execute_try_catch("crewind")
  call assert_equal(0, l:caught)
  call assert_equal(l:first_window, win_getid())
endfunc

" Fail :djump but :djump! is allowed
func Test_djump()
  call s:reset_all_buffers()

  let l:include_file = tempname() . ".h"
  call writefile(["min(1, 12);",
        \ '#include "' . l:include_file . '"'
        \ ],
        \ "main.c")
  call writefile(["#define min(X, Y)  ((X) < (Y) ? (X) : (Y))"], l:include_file)
  edit main.c
  set stickybuf

  let l:current = bufnr()

  let l:caught = s:execute_try_catch("djump 1 /min/")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("djump! 1 /min/")
  call assert_equal(0, l:caught)
  call assert_notequal(l:current, bufnr())

  set tags&
  call delete("main.c")
  call delete(l:include_file)
endfunc

" Fail :drop
func Test_drop()
  call s:reset_all_buffers()

  let l:other = s:make_buffer_pairs()
  let l:current = bufnr()

  let l:caught = s:execute_try_catch("drop other")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())
endfunc

" Fail :edit but :edit! is allowed
func Test_edit()
  call s:reset_all_buffers()

  let l:other = s:make_buffer_pairs()
  let l:current = bufnr()

  let l:caught = s:execute_try_catch("edit other")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("edit! other")
  call assert_equal(0, l:caught)
  call assert_equal(l:other, bufnr())
endfunc

" Fail :enew but :enew! is allowed
func Test_enew()
  call s:reset_all_buffers()

  let l:other = s:make_buffer_pairs()
  let l:current = bufnr()

  let l:caught = s:execute_try_catch("enew")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("enew!")
  call assert_equal(0, l:caught)
  call assert_notequal(l:other, bufnr())
  call assert_notequal(3, bufnr())
endfunc

" Fail :ex but :ex! is allowed
func Test_ex()
  call s:reset_all_buffers()

  let l:other = s:make_buffer_pairs()
  let l:current = bufnr()

  let l:caught = s:execute_try_catch("ex other")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("ex! other")
  call assert_equal(0, l:caught)
  call assert_equal(l:other, bufnr())
endfunc

" Fail :find but :find! is allowed
func Test_find()
  call s:reset_all_buffers()

  let l:current = bufnr()
  let l:file = tempname()
  call writefile([], l:file)
  let l:directory = fnamemodify(l:file, ":p:h")
  let l:name = fnamemodify(l:file, ":p:t")

  let l:original_path = &path
  execute "set path=" . l:directory

  set stickybuf

  let l:caught = s:execute_try_catch("find " . l:name)
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("find! " . l:name)
  call assert_equal(0, l:caught)
  call assert_equal(l:file, expand("%:p"))

  execute "set path=" . l:original_path
  call delete(l:file)
endfunc

" Fail :first but :first! is allowed
func Test_first()
  call s:reset_all_buffers()

  let [l:first, _] = s:make_args_list()
  next!

  let l:caught = s:execute_try_catch("first")
  call assert_equal(1, l:caught)
  call assert_notequal(l:first, bufnr())

  let l:caught = s:execute_try_catch("first!")
  call assert_equal(0, l:caught)
  call assert_equal(l:first, bufnr())
endfunc

" Allow :grep but don't change the 'stickybuf' window to another buffer
func Test_grep()
  CheckFeature quickfix
  call s:reset_all_buffers()

  edit first.unittest
  call append(0, ["some-search-term"])
  write

  edit current.unittest
  call append(0, ["some-search-term"])
  write
  let l:current = bufnr()

  set stickybuf

  edit! last.unittest
  call append(0, ["some-search-term"])
  write
  let l:last = bufnr()

  buffer! current.unittest

  " Don't error but don't swap to the first match because the current window
  " has 'stickybuf' enabled
  let l:caught = s:execute_try_catch("silent! grep some-search-term *.unittest")
  call assert_equal(0, l:caught)
  call assert_equal(l:current, bufnr())

  " Don't error and also do not swap to the first match because ! was included
  let l:caught = s:execute_try_catch("silent! grep! some-search-term *.unittest")
  call assert_equal(0, l:caught)
  call assert_equal(l:current, bufnr())

  call delete("first.unittest")
  call delete("current.unittest")
  call delete("last.unittest")
endfunc

" Fail :ijump but :ijump! is allowed
func Test_ijump()
  call s:reset_all_buffers()

  let l:include_file = tempname() . ".h"
  call writefile([
        \ '#include "' . l:include_file . '"'
        \ ],
        \ "main.c")
  call writefile(["#define min(X, Y)  ((X) < (Y) ? (X) : (Y))"], l:include_file)
  edit main.c
  set stickybuf

  let l:current = bufnr()

  set define=^\\s*#\\s*define
  set include=^\\s*#\\s*include
  set path=.,/usr/include,,

  let l:caught = s:execute_try_catch("ijump /min/")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  set nostickybuf
  let l:caught = s:execute_try_catch("ijump! /min/")
  call assert_equal(0, l:caught)
  call assert_notequal(l:current, bufnr())

  set tags&
  set define&
  set include&
  set path&
  call delete("main.c")
  call delete(l:include_file)
endfunc

" Fail :lNext but :lNext! is allowed
func Test_lNext()
  CheckFeature quickfix
  call s:reset_all_buffers()
  let [l:first, l:middle, _] = s:make_simple_location_list()

  lnext!

  let l:caught = s:execute_try_catch("lNext")
  call assert_equal(1, l:caught)
  call assert_equal(l:middle, bufnr())

  lnext!  " Reset for the next test

  let l:caught = s:execute_try_catch("lNext!")
  call assert_equal(0, l:caught)
  call assert_equal(l:first, bufnr())
endfunc

" Fail :lNfile but :lNfile! is allowed
func Test_lNfile()
  CheckFeature quickfix
  call s:reset_all_buffers()
  let [l:first, l:current, _] = s:make_simple_location_list()

  lnext!

  let l:caught = s:execute_try_catch("lNfile")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  lnext!  " Reset for the next test

  let l:caught = s:execute_try_catch("lNfile!")
  call assert_equal(0, l:caught)
  call assert_equal(l:first, bufnr())
endfunc

" Fail :last but :last! is allowed
func Test_last()
  call s:reset_all_buffers()

  let [_, l:last] = s:make_args_list()
  next!

  let l:caught = s:execute_try_catch("last")
  call assert_equal(1, l:caught)
  call assert_notequal(l:last, bufnr())

  let l:caught = s:execute_try_catch("last!")
  call assert_equal(0, l:caught)
  call assert_equal(l:last, bufnr())
endfunc

" Allow :lbuffer but don't switch to the first much unless :lbuffer! is used
func Test_lbuffer()
  CheckFeature quickfix
  call s:reset_all_buffers()

  let l:file_path = tempname()
  call writefile(["first.unittest:1:Error - bad-thing-found"], l:file_path)
  execute "edit " . l:file_path
  let l:file_buffer = bufnr()
  let l:current = bufnr()

  edit first.unittest
  call append(0, ["some-search-term bad-thing-found"])

  edit! other.unittest
  set stickybuf

  execute "buffer! " . l:file_buffer

  let l:caught = s:execute_try_catch("lbuffer " . l:file_buffer)
  call assert_equal(0, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("lbuffer! " . l:file_buffer)
  call assert_equal(0, l:caught)
  call assert_equal("first.unittest", expand("%:t"))
endfunc

" Fail :ldo but :ldo! is allowed
func Test_ldo()
  CheckFeature quickfix
  call s:reset_all_buffers()
  let [l:first, l:middle, l:last] = s:make_simple_location_list()

  lnext!

  let l:caught = s:execute_try_catch("ldo buffer " . l:first)
  call assert_equal(1, l:caught)
  call assert_equal(l:middle, bufnr())
  let l:caught = s:execute_try_catch("ldo! buffer " . l:first)
  call assert_equal(0, l:caught)
  call assert_notequal(l:last, bufnr())
endfunc

" Allow :lfile but don't swap to the first file unless ':lfile! is used
func Test_lfile()
  CheckFeature quickfix
  call s:reset_all_buffers()

  edit first.unittest
  call append(0, ["some-search-term bad-thing-found"])
  write
  let l:first = bufnr()

  edit! second.unittest
  call append(0, ["some-search-term"])
  write

  let l:file = tempname()
  call writefile(["first.unittest:1:Error - bad-thing-found was detected"], l:file)

  let l:current = bufnr()
  set stickybuf

  let l:caught = s:execute_try_catch(":lfile " . l:file)
  call assert_equal(0, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch(":lfile! " . l:file)
  call assert_equal(0, l:caught)
  call assert_equal(l:first, bufnr())

  call delete(l:file)
  call delete("first.unittest")
  call delete("second.unittest")
endfunc

" Fail :ll but :ll! is allowed
func Test_ll()
  CheckFeature quickfix
  call s:reset_all_buffers()
  let [l:first, l:middle, l:last] = s:make_simple_location_list()

  lopen
  lfirst!
  execute "normal \<C-w>j"
  normal j

  let l:caught = s:execute_try_catch(".ll")
  call assert_equal(1, l:caught)
  execute "normal \<C-w>k"
  call assert_equal(l:first, bufnr())
  execute "normal \<C-w>j"
  let l:caught = s:execute_try_catch(".ll!")
  call assert_equal(0, l:caught)
  execute "normal \<C-w>k"
  call assert_equal(l:middle, bufnr())
endfunc

" Fail :llast but :llast! is allowed
func Test_llast()
  CheckFeature quickfix
  call s:reset_all_buffers()

  let [l:first, _, l:last] = s:make_simple_location_list()
  lfirst!

  let l:caught = s:execute_try_catch("llast")
  call assert_equal(1, l:caught)
  call assert_equal(l:first, bufnr())

  let l:caught = s:execute_try_catch("llast!")
  call assert_equal(0, l:caught)
  call assert_equal(l:last, bufnr())
endfunc

" Fail :lnext but :lnext! is allowed
func Test_lnext()
  CheckFeature quickfix
  call s:reset_all_buffers()
  let [l:first, l:middle, l:last] = s:make_simple_location_list()

  ll!

  let l:caught = s:execute_try_catch("lnext")
  call assert_equal(1, l:caught)
  call assert_equal(l:first, bufnr())

  let l:caught = s:execute_try_catch("lnext!")
  call assert_equal(0, l:caught)
  call assert_equal(l:middle, bufnr())
endfunc

" Fail :lnfile but :lnfile! is allowed
func Test_lnfile()
  CheckFeature quickfix
  call s:reset_all_buffers()
  let [_, l:current, l:last] = s:make_simple_location_list()

  lnext!

  let l:caught = s:execute_try_catch("lnfile")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  lprevious!  " Reset for the next test call

  let l:caught = s:execute_try_catch("lnfile!")
  call assert_equal(0, l:caught)
  call assert_equal(l:last, bufnr())
endfunc

" Fail :lpfile but :lpfile! is allowed
func Test_lpfile()
  CheckFeature quickfix
  call s:reset_all_buffers()
  let [l:first, l:current, _] = s:make_simple_location_list()

  lnext!

  let l:caught = s:execute_try_catch("lpfile")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  lnext!  " Reset for the next test call

  let l:caught = s:execute_try_catch("lpfile!")
  call assert_equal(0, l:caught)
  call assert_equal(l:first, bufnr())
endfunc

" Fail :lprevious but :lprevious! is allowed
func Test_lprevious()
  CheckFeature quickfix
  call s:reset_all_buffers()
  let [l:first, l:middle, _] = s:make_simple_location_list()

  lnext!

  let l:caught = s:execute_try_catch("lprevious")
  call assert_equal(1, l:caught)
  call assert_equal(l:middle, bufnr())

  lnext!  " Reset for the next test call

  let l:caught = s:execute_try_catch("lprevious!")
  call assert_equal(0, l:caught)
  call assert_equal(l:first, bufnr())
endfunc

" Fail :lrewind but :lrewind! is allowed
func Test_lrewind()
  CheckFeature quickfix
  call s:reset_all_buffers()
  let [l:first, l:middle, _] = s:make_simple_location_list()

  lnext!

  let l:caught = s:execute_try_catch("lrewind")
  call assert_equal(1, l:caught)
  call assert_equal(l:middle, bufnr())

  let l:caught = s:execute_try_catch("lrewind!")
  call assert_equal(0, l:caught)
  call assert_equal(l:first, bufnr())
endfunc

" Fail :ltag but :ltag! is allowed
func Test_ltag()
  call s:reset_all_buffers()

  set tags=Xtags
  call writefile(["!_TAG_FILE_ENCODING\tutf-8\t//",
        \ "one\tXfile\t1",
        \ "three\tXfile\t3",
        \ "two\tXfile\t2"],
        \ "Xtags")
  call writefile(["one", "two", "three"], "Xfile")
  edit Xfile
  execute "normal \<C-]>"
  set stickybuf

  let l:current = bufnr()

  let l:caught = s:execute_try_catch("ltag one")
  call assert_equal(1, l:caught)

  let l:caught = s:execute_try_catch("ltag! one")
  call assert_equal(0, l:caught)

  set tags&
  call delete("Xtags")
  call delete("Xfile")
endfunc

" Allow :lvimgrep but 'stickybuf' windows will not jump to the first match
" unless [!] is used.
func Test_lvimgrep()
  CheckFeature quickfix
  call s:reset_all_buffers()

  edit first.unittest
  call append(0, ["some-search-term"])
  write

  edit sticky.unittest
  call append(0, ["some-search-term"])
  write
  let l:current = bufnr()

  set stickybuf

  edit! last.unittest
  call append(0, ["some-search-term"])
  write
  let l:last = bufnr()

  buffer! sticky.unittest

  " Don't error but don't swap to the first match because the current window
  " has 'stickybuf' enabled
  let l:caught = s:execute_try_catch("lvimgrep /some-search-term/ *.unittest")
  call assert_equal(0, l:caught)
  call assert_equal(l:current, bufnr())

  " Don't error and also do swap to the first match because ! was included
  let l:caught = s:execute_try_catch("lvimgrep! /some-search-term/ *.unittest")
  call assert_equal(0, l:caught)
  call assert_notequal(l:current, bufnr())

  call delete("first.unittest")
  call delete("sticky.unittest")
  call delete("last.unittest")
endfunc

" Allow :lvimgrepadd but 'stickybuf' windows will not jump to the first match
" unless [!] is used.
func Test_lvimgrepadd()
  CheckFeature quickfix
  call s:reset_all_buffers()

  edit first.unittest
  call append(0, ["some-search-term"])
  write

  edit sticky.unittest
  call append(0, ["some-search-term"])
  write
  let l:current = bufnr()

  set stickybuf

  edit! last.unittest
  call append(0, ["some-search-term"])
  write
  let l:last = bufnr()

  buffer! sticky.unittest

  " Don't error but don't swap to the first match because the current window
  " has 'stickybuf' enabled
  let l:caught = s:execute_try_catch("lvimgrepadd /some-search-term/ *.unittest")
  call assert_equal(0, l:caught)
  call assert_equal(l:current, bufnr())

  " Don't error and also do swap to the first match because ! was included
  let l:caught = s:execute_try_catch("lvimgrepadd! /some-search-term/ *.unittest")
  call assert_equal(0, l:caught)
  call assert_notequal(l:current, bufnr())

  call delete("first.unittest")
  call delete("sticky.unittest")
  call delete("last.unittest")
endfunc

" Don't allow global marks to change the current 'stickybuf' window
func Test_marks_mappings_fail()
  call s:reset_all_buffers()

  let l:other = s:make_buffer_pairs()
  let l:current = bufnr()
  execute "buffer! " . l:other
  normal mA
  execute "buffer! " . l:current
  normal mB

  let l:caught = s:execute_try_catch("normal `A")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("normal 'A")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  set nostickybuf

  let l:caught = s:execute_try_catch("normal `A")
  call assert_equal(0, l:caught)
  call assert_equal(l:other, bufnr())
endfunc

" Allow global marks in a 'stickybuf' window if the jump is the same buffer
func Test_marks_mappings_pass_intra_move()
  call s:reset_all_buffers()

  let l:current = bufnr()
  call append(0, ["some line", "another line"])
  normal mA
  normal j
  normal mB

  set stickybuf

  let l:caught = s:execute_try_catch("normal `A")
  call assert_equal(0, l:caught)
  call assert_equal(l:current, bufnr())
endfunc

" Fail :next but :next! is allowed
func Test_next()
  call s:reset_all_buffers()

  let [_, l:last] = s:make_args_list()
  next!

  let l:caught = s:execute_try_catch("next")
  call assert_equal(1, l:caught)
  call assert_notequal(l:last, bufnr())

  let l:caught = s:execute_try_catch("next!")
  call assert_equal(0, l:caught)
  call assert_equal(l:last, bufnr())
endfunc

" Fail to jump to a tag with g<C-]> if 'stickybuf' is enabled
func Test_normal_g_ctrl_square_bracket_right()
  call s:reset_all_buffers()

  set tags=Xtags
  call writefile(["!_TAG_FILE_ENCODING\tutf-8\t//",
        \ "one\tXfile\t1",
        \ "three\tXfile\t3",
        \ "two\tXfile\t2"],
        \ "Xtags")
  call writefile(["one", "two", "three"], "Xfile")
  edit Xfile
  set stickybuf

  let l:current = bufnr()

  let l:caught = s:execute_try_catch("normal g\<C-]>")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  set tags&
  call delete("Xtags")
  call delete("Xfile")
endfunc

" Fail to jump to a tag with g] if 'stickybuf' is enabled
func Test_normal_g_square_bracket_right()
  call s:reset_all_buffers()

  set tags=Xtags
  call writefile(["!_TAG_FILE_ENCODING\tutf-8\t//",
        \ "one\tXfile\t1",
        \ "three\tXfile\t3",
        \ "two\tXfile\t2"],
        \ "Xtags")
  call writefile(["one", "two", "three"], "Xfile")
  edit Xfile
  set stickybuf

  let l:current = bufnr()

  let l:caught = s:execute_try_catch("normal g]")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  set tags&
  call delete("Xtags")
  call delete("Xfile")
endfunc

" Fail to jump to a tag with <C-t> if 'stickybuf' is enabled
func Test_normal_ctrl_T()
  call s:reset_all_buffers()

  set tags=Xtags
  call writefile(["!_TAG_FILE_ENCODING\tutf-8\t//",
        \ "one\tXfile\t1",
        \ "three\tXfile\t3",
        \ "two\tXfile\t2"],
        \ "Xtags")
  call writefile(["one", "two", "three"], "Xfile")
  edit Xfile
  execute "normal \<C-]>"

  set stickybuf

  let l:current = bufnr()

  let l:caught = s:execute_try_catch("normal \<C-t>")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  set tags&
  call delete("Xtags")
  call delete("Xfile")
endfunc

" Disallow <C-^> in 'stickybuf' windows
func Test_normal_ctrl_hat()
  call s:reset_all_buffers()
  clearjumps

  enew
  file first
  let l:first = bufnr()

  enew
  file current
  let l:current = bufnr()
  set stickybuf

  let l:caught = s:execute_try_catch("normal \<C-^>")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())
endfunc

" Allow <C-i> in 'stickybuf' windows if the movement stays within the buffer
func Test_normal_ctrl_i_pass()
  call s:reset_all_buffers()
  clearjumps

  enew
  file first
  let l:first = bufnr()

  enew!
  file current
  let l:current = bufnr()
  " Add some lines so we can populate a jumplist"
  call append(0, ["some line", "another line"])
  " Add an entry to the jump list
  " Go up another line
  normal m`
  normal k
  execute "normal \<C-o>"
  set stickybuf

  let l:caught = s:execute_try_catch("normal \\<C-i>")
  call assert_equal(0, l:caught)
  call assert_equal(l:current, bufnr())
endfunc

" Disallow <C-o> in 'stickybuf' windows if it would cause the buffer to switch
func Test_normal_ctrl_o_fail()
  call s:reset_all_buffers()
  clearjumps

  enew
  file first
  let l:first = bufnr()

  enew
  file current
  let l:current = bufnr()
  set stickybuf

  let l:caught = s:execute_try_catch("normal \<C-o>")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())
endfunc

" Allow <C-o> in 'stickybuf' windows if the movement stays within the buffer
func Test_normal_ctrl_o_pass()
  call s:reset_all_buffers()
  clearjumps

  enew
  file first
  let l:first = bufnr()

  enew!
  file current
  let l:current = bufnr()
  " Add some lines so we can populate a jumplist
  call append(0, ["some line", "another line"])
  " Add an entry to the jump list
  " Go up another line
  normal m`
  normal k
  set stickybuf

  let l:caught = s:execute_try_catch("normal \<C-o>")
  call assert_equal(0, l:caught)
  call assert_equal(l:current, bufnr())
endfunc

" Fail to jump to a tag with <C-]> if 'stickybuf' is enabled
func Test_normal_ctrl_square_bracket_right()
  call s:reset_all_buffers()

  set tags=Xtags
  call writefile(["!_TAG_FILE_ENCODING\tutf-8\t//",
        \ "one\tXfile\t1",
        \ "three\tXfile\t3",
        \ "two\tXfile\t2"],
        \ "Xtags")
  call writefile(["one", "two", "three"], "Xfile")
  edit Xfile
  set stickybuf

  let l:current = bufnr()

  let l:caught = s:execute_try_catch("normal \<C-]>")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  set tags&
  call delete("Xtags")
  call delete("Xfile")
endfunc

" Allow <C-w><C-]> with 'stickybuf' enabled because it runs in a new, split window
func Test_normal_ctrl_w_ctrl_square_bracket_right()
  call s:reset_all_buffers()

  set tags=Xtags
  call writefile(["!_TAG_FILE_ENCODING\tutf-8\t//",
        \ "one\tXfile\t1",
        \ "three\tXfile\t3",
        \ "two\tXfile\t2"],
        \ "Xtags")
  call writefile(["one", "two", "three"], "Xfile")
  edit Xfile
  set stickybuf

  let l:caught = s:execute_try_catch("normal \<C-w><C-]>")
  call assert_equal(0, l:caught)

  set tags&
  call delete("Xtags")
  call delete("Xfile")
endfunc

" Fail to jump to a tag with <C-]> if 'stickybuf' is enabled
func Test_normal_gt()
  call s:reset_all_buffers()

  set tags=Xtags
  call writefile(["!_TAG_FILE_ENCODING\tutf-8\t//",
        \ "one\tXfile\t1",
        \ "three\tXfile\t3",
        \ "two\tXfile\t2"],
        \ "Xtags")
  call writefile(["one", "two", "three"], "Xfile")
  edit Xfile
  set stickybuf

  let l:current = bufnr()

  let l:caught = s:execute_try_catch("normal \<C-]>")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  set tags&
  call delete("Xtags")
  call delete("Xfile")
endfunc

" Prevent gF from switching a 'stickybuf' window's buffer
func Test_normal_gF()
  call s:reset_all_buffers()


  let l:file = tempname()
  call append(0, [l:file])
  call writefile([], l:file)
  " Place the cursor onto the line that has `l:file`
  normal gg
  " Prevent Vim from erroring with "No write since last change @ command
  " line" when we try to call gF, later.
  set hidden

  set stickybuf
  let l:buffer = bufnr()

  let l:caught = s:execute_try_catch("normal gF")
  call assert_equal(1, l:caught)
  call assert_equal(l:buffer, bufnr())

  set nostickybuf

  let l:caught = s:execute_try_catch("normal gF")
  call assert_equal(0, l:caught)
  call assert_notequal(l:buffer, bufnr())
  call delete(l:file)
endfunc

" Prevent gf from switching a 'stickybuf' window's buffer
func Test_normal_gf()
  call s:reset_all_buffers()


  let l:file = tempname()
  call append(0, [l:file])
  call writefile([], l:file)
  " Place the cursor onto the line that has `l:file`
  normal gg
  " Prevent Vim from erroring with "No write since last change @ command
  " line" when we try to call gf, later.
  set hidden

  set stickybuf
  let l:buffer = bufnr()

  let l:caught = s:execute_try_catch("normal gf")
  call assert_equal(1, l:caught)
  call assert_equal(l:buffer, bufnr())

  set nostickybuf

  let l:caught = s:execute_try_catch("normal gf")
  call assert_equal(0, l:caught)
  call assert_notequal(l:buffer, bufnr())
  call delete(l:file)
endfunc

" Fail "goto file under the cursor" (using [f, which is the same as `:normal gf`)
func Test_normal_square_bracket_left_f()
  call s:reset_all_buffers()


  let l:file = tempname()
  call append(0, [l:file])
  call writefile([], l:file)
  " Place the cursor onto the line that has `l:file`
  normal gg
  " Prevent Vim from erroring with "No write since last change @ command
  " line" when we try to call gf, later.
  set hidden

  set stickybuf
  let l:buffer = bufnr()

  let l:caught = s:execute_try_catch("normal [f")
  call assert_equal(1, l:caught)
  call assert_equal(l:buffer, bufnr())

  set nostickybuf

  let l:caught = s:execute_try_catch("normal [f")
  call assert_equal(0, l:caught)
  call assert_notequal(l:buffer, bufnr())
  call delete(l:file)
endfunc

" Fail to go to a C macro with ]<C-d> if 'stickybuf' is enabled
func Test_normal_square_bracket_right_ctrl_d()
  call s:reset_all_buffers()

  let l:include_file = tempname() . ".h"
  call writefile(["min(1, 12);",
        \ '#include "' . l:include_file . '"'
        \ ],
        \ "main.c")
  call writefile(["#define min(X, Y)  ((X) < (Y) ? (X) : (Y))"], l:include_file)
  edit main.c
  set stickybuf

  let l:current = bufnr()

  let l:caught = s:execute_try_catch("normal ]\<C-d>")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  set nostickybuf
  let l:caught = s:execute_try_catch("normal ]\<C-d>")
  call assert_equal(0, l:caught)
  call assert_notequal(l:current, bufnr())

  set tags&
  call delete("main.c")
  call delete(l:include_file)
endfunc

" Fail to go to a C macro with ]<C-i> if 'stickybuf' is enabled
func Test_normal_square_bracket_right_ctrl_i()
  call s:reset_all_buffers()

  let l:include_file = tempname() . ".h"
  call writefile(["min(1, 12);",
        \ '#include "' . l:include_file . '"'
        \ ],
        \ "main.c")
  call writefile(["#define min(X, Y)  ((X) < (Y) ? (X) : (Y))"], l:include_file)
  edit main.c
  set stickybuf

  let l:current = bufnr()

  let l:caught = s:execute_try_catch("normal ]\<C-i>")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  set nostickybuf
  let l:caught = s:execute_try_catch("normal ]\<C-i>")
  call assert_equal(0, l:caught)
  call assert_notequal(l:current, bufnr())

  set tags&
  call delete("main.c")
  call delete(l:include_file)
endfunc

" Fail "goto file under the cursor" (using ]f, which is the same as `:normal gf`)
func Test_normal_square_bracket_right_f()
  call s:reset_all_buffers()


  let l:file = tempname()
  call append(0, [l:file])
  call writefile([], l:file)
  " Place the cursor onto the line that has `l:file`
  normal gg
  " Prevent Vim from erroring with "No write since last change @ command
  " line" when we try to call gf, later.
  set hidden

  set stickybuf
  let l:buffer = bufnr()

  let l:caught = s:execute_try_catch("normal ]f")
  call assert_equal(1, l:caught)
  call assert_equal(l:buffer, bufnr())

  set nostickybuf

  let l:caught = s:execute_try_catch("normal ]f")
  call assert_equal(0, l:caught)
  call assert_notequal(l:buffer, bufnr())
  call delete(l:file)
endfunc

" Allow :pedit because, unlike :edit, it uses a separate window
func Test_pedit()
  call s:reset_all_buffers()

  let l:other = s:make_buffer_pairs()
  let l:caught = s:execute_try_catch("pedit other")

  call assert_equal(0, l:caught)
  execute "normal \<C-w>w"
  call assert_equal(l:other, bufnr())
endfunc

" Fail :pop but :pop! is allowed
func Test_pop()
  call s:reset_all_buffers()

  set tags=Xtags
  call writefile(["!_TAG_FILE_ENCODING\tutf-8\t//",
        \ "thesame\tXfile\t1;\"\td\tfile:",
        \ "thesame\tXfile\t2;\"\td\tfile:",
        \ "thesame\tXfile\t3;\"\td\tfile:",
        \ ],
        \ 'Xtags')
  new Xfile
  call setline(1, ['thesame one', 'thesame two', 'thesame three'])
  write

  tag thesame
  execute "normal \<C-^>"

  set stickybuf

  let l:current = bufnr()

  let l:caught = s:execute_try_catch("pop")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("pop!")
  call assert_equal(0, l:caught)
  call assert_notequal(l:current, bufnr())

  set tags&
  call delete('Xtags')
  call delete('Xfile')
endfunc

" Fail :previous but :previous! is allowed
func Test_previous()
  call s:reset_all_buffers()

  let [l:first, _] = s:make_args_list()
  next!

  let l:caught = s:execute_try_catch("previous")
  call assert_equal(1, l:caught)
  call assert_notequal(l:first, bufnr())

  let l:caught = s:execute_try_catch("previous!")
  call assert_equal(0, l:caught)
  call assert_equal(l:first, bufnr())
endfunc

" Ensure remapping to a disabled action still triggers failures
func Test_remap_key_fail()
  call s:reset_all_buffers()

  enew
  file first
  let l:first = bufnr()

  enew
  file current
  let l:current = bufnr()
  set stickybuf

  nnoremap g <C-^>

  let l:caught = s:execute_try_catch("normal g")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  nunmap g
endfunc

" Ensure remapping a disabled key to something valid does trigger any failures
func Test_remap_key_pass()
  call s:reset_all_buffers()

  enew
  file first
  let l:first = bufnr()

  enew
  file current
  let l:current = bufnr()
  set stickybuf

  let l:caught = s:execute_try_catch("normal \<C-^>")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  " Disallow <C-^> by default but allow it if the command does something else
  nnoremap <C-^> :echo "hello!"

  let l:caught = s:execute_try_catch("normal \<C-^>")
  call assert_equal(0, l:caught)
  call assert_equal(l:current, bufnr())

  nunmap <C-^>
endfunc

" Fail :rewind but :rewind! is allowed
func Test_rewind()
  call s:reset_all_buffers()

  let [l:first, _] = s:make_args_list()
  next!

  let l:caught = s:execute_try_catch("rewind")
  call assert_equal(1, l:caught)
  call assert_notequal(l:first, bufnr())

  let l:caught = s:execute_try_catch("rewind!")
  call assert_equal(0, l:caught)
  call assert_equal(l:first, bufnr())
endfunc

" Allow :sblast because it opens the buffer in a new, split window
func Test_sblast()
  call s:reset_all_buffers()

  let l:other = s:make_buffer_pairs(1)
  bfirst!
  let l:current = bufnr()

  let l:caught = s:execute_try_catch("sblast")
  call assert_equal(0, l:caught)
  call assert_equal(l:other, bufnr())
endfunc

" Fail :sbprevious but :sbprevious! is allowed
func Test_sbprevious()
  call s:reset_all_buffers()

  let l:other = s:make_buffer_pairs()
  let l:current = bufnr()

  let l:caught = s:execute_try_catch("sbprevious")
  call assert_equal(0, l:caught)
  call assert_equal(l:other, bufnr())
endfunc

" Ensure the first has 'stickybuf' and a new split window is 'nostickybuf'
func Test_split_window()
  call s:reset_all_buffers()

  split
  execute "normal \<C-w>j"
  set stickybuf
  let l:sticky_window_1 = win_getid()
  vsplit
  let l:sticky_window_2 = win_getid()

  call assert_equal(1, getwinvar(l:sticky_window_1, "&stickybuf"))
  call assert_equal(0, getwinvar(l:sticky_window_2, "&stickybuf"))
endfunc

" Fail :tNext but :tNext! is allowed
func Test_tNext()
  call s:reset_all_buffers()

  set tags=Xtags
  call writefile(["!_TAG_FILE_ENCODING\tutf-8\t//",
        \ "thesame\tXfile\t1;\"\td\tfile:",
        \ "thesame\tXfile\t2;\"\td\tfile:",
        \ "thesame\tXfile\t3;\"\td\tfile:",
        \ ],
        \ 'Xtags')
  new Xfile
  call setline(1, ['thesame one', 'thesame two', 'thesame three'])
  write

  tag thesame
  execute "normal \<C-^>"
  tnext!

  set stickybuf

  let l:current = bufnr()

  let l:caught = s:execute_try_catch("tNext")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("tNext!")
  call assert_equal(0, l:caught)

  set tags&
  call delete('Xtags')
  call delete('Xfile')
endfunc

" Call :tabdo and choose the next available 'nostickybuf' window.
func Test_tabdo_choose_available_window()
  call s:reset_all_buffers()
  let [l:first, _] = s:make_args_list()

  " Make a split window that is 'nostickybuf' but make it the second-to-last
  " window so that :tabdo will first try the 'stickybuf' window, pass over it,
  " and prefer the other 'nostickybuf' window, instead.
  "
  " +-------------------+
  " |   'nostickybuf'   |
  " +-------------------+
  " |    'stickybuf'    |  <-- Cursor is here
  " +-------------------+
  split
  let l:nostickybuf_window = win_getid()
  " Move to the 'stickybuf' window now
  execute "normal \<C-w>j"
  let l:stickybuf_window = win_getid()

  let l:caught = s:execute_try_catch("tabdo echo ''")
  call assert_equal(0, l:caught)
  call assert_equal(l:nostickybuf_window, win_getid())
  call assert_equal(l:first, bufnr())
endfunc

" Call :tabdo and create a new split window if all available windows are 'stickybuf'.
func Test_tabdo_make_new_window()
  call s:reset_all_buffers()
  let [l:first, _] = s:make_buffers_list()
  execute "buffer! " . l:first

  let l:current = win_getid()

  let l:caught = s:execute_try_catch("tabdo echo ''")
  call assert_equal(0, l:caught)
  call assert_notequal(l:current, win_getid())
  call assert_equal(l:first, bufnr())
  execute "normal \<C-w>j"
  call assert_equal(l:first, bufnr())
endfunc

" Fail :tag but :tag! is allowed
func Test_tag()
  call s:reset_all_buffers()

  set tags=Xtags
  call writefile(["!_TAG_FILE_ENCODING\tutf-8\t//",
        \ "one\tXfile\t1",
        \ "three\tXfile\t3",
        \ "two\tXfile\t2"],
        \ "Xtags")
  call writefile(["one", "two", "three"], "Xfile")
  edit Xfile
  set stickybuf

  let l:current = bufnr()

  let l:caught = s:execute_try_catch("tag one")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("tag! one")
  call assert_equal(0, l:caught)
  call assert_equal(l:current, bufnr())

  set tags&
  call delete("Xtags")
  call delete("Xfile")
endfunc


" Fail :tfirst but :tfirst! is allowed
func Test_tfirst()
  call s:reset_all_buffers()

  set tags=Xtags
  call writefile(["!_TAG_FILE_ENCODING\tutf-8\t//",
        \ "one\tXfile\t1",
        \ "three\tXfile\t3",
        \ "two\tXfile\t2"],
        \ "Xtags")
  call writefile(["one", "two", "three"], "Xfile")
  edit Xfile
  tjump one
  edit Xfile

  set stickybuf
  let l:current = bufnr()

  let l:caught = s:execute_try_catch("tfirst")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("tfirst!")
  call assert_equal(0, l:caught)
  call assert_equal(l:current, bufnr())

  set tags&
  call delete("Xtags")
  call delete("Xfile")
endfunc

" Fail :tjump but :tjump! is allowed
func Test_tjump()
  call s:reset_all_buffers()

  set tags=Xtags
  call writefile(["!_TAG_FILE_ENCODING\tutf-8\t//",
        \ "one\tXfile\t1",
        \ "three\tXfile\t3",
        \ "two\tXfile\t2"],
        \ "Xtags")
  call writefile(["one", "two", "three"], "Xfile")
  edit Xfile
  set stickybuf

  let l:current = bufnr()

  let l:caught = s:execute_try_catch("tjump one")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("tjump! one")
  call assert_equal(0, l:caught)
  call assert_equal(l:current, bufnr())

  set tags&
  call delete("Xtags")
  call delete("Xfile")
endfunc

" Fail :tlast but :tlast! is allowed
func Test_tlast()
  call s:reset_all_buffers()

  set tags=Xtags
  call writefile(["!_TAG_FILE_ENCODING\tutf-8\t//",
        \ "one\tXfile\t1",
        \ "three\tXfile\t3",
        \ "two\tXfile\t2"],
        \ "Xtags")
  call writefile(["one", "two", "three"], "Xfile")
  edit Xfile
  tjump one
  edit Xfile

  set stickybuf
  let l:current = bufnr()

  let l:caught = s:execute_try_catch("tlast")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("tlast!")
  call assert_equal(0, l:caught)
  call assert_equal(l:current, bufnr())

  set tags&
  call delete("Xtags")
  call delete("Xfile")
endfunc

" Fail :tnext but :tnext! is allowed
func Test_tnext()
  call s:reset_all_buffers()

  set tags=Xtags
  call writefile(["!_TAG_FILE_ENCODING\tutf-8\t//",
        \ "thesame\tXfile\t1;\"\td\tfile:",
        \ "thesame\tXfile\t2;\"\td\tfile:",
        \ "thesame\tXfile\t3;\"\td\tfile:",
        \ ],
        \ 'Xtags')
  new Xfile
  call setline(1, ['thesame one', 'thesame two', 'thesame three'])
  write

  tag thesame
  execute "normal \<C-^>"

  set stickybuf

  let l:current = bufnr()

  let l:caught = s:execute_try_catch("tnext")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("tnext!")
  call assert_equal(0, l:caught)
  call assert_notequal(l:current, bufnr())

  set tags&
  call delete('Xtags')
  call delete('Xfile')
endfunc

" Fail :tprevious but :tprevious! is allowed
func Test_tprevious()
  call s:reset_all_buffers()

  set tags=Xtags
  call writefile(["!_TAG_FILE_ENCODING\tutf-8\t//",
        \ "thesame\tXfile\t1;\"\td\tfile:",
        \ "thesame\tXfile\t2;\"\td\tfile:",
        \ "thesame\tXfile\t3;\"\td\tfile:",
        \ ],
        \ 'Xtags')
  new Xfile
  call setline(1, ['thesame one', 'thesame two', 'thesame three'])
  write

  tag thesame
  execute "normal \<C-^>"
  tnext!

  set stickybuf

  let l:current = bufnr()

  let l:caught = s:execute_try_catch("tprevious")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("tprevious!")
  call assert_equal(0, l:caught)

  set tags&
  call delete('Xtags')
  call delete('Xfile')
endfunc

" Fail :view but :view! is allowed
func Test_view()
  call s:reset_all_buffers()

  let l:other = s:make_buffer_pairs()

  let l:current = bufnr()

  let l:caught = s:execute_try_catch("view other")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("view! other")
  call assert_equal(0, l:caught)
  call assert_equal(l:other, bufnr())
endfunc

" Fail :visual but :visual! is allowed
func Test_visual()
  call s:reset_all_buffers()

  let l:other = s:make_buffer_pairs()
  let l:current = bufnr()

  let l:caught = s:execute_try_catch("visual other")
  call assert_equal(1, l:caught)
  call assert_equal(l:current, bufnr())

  let l:caught = s:execute_try_catch("visual! other")
  call assert_equal(0, l:caught)
  call assert_equal(l:other, bufnr())
endfunc

" Allow :vimgrep but 'stickybuf' windows will not jump to the first match
" unless [!] is used.
func Test_vimgrep()
  CheckFeature quickfix
  call s:reset_all_buffers()

  edit first.unittest
  call append(0, ["some-search-term"])
  write

  edit sticky.unittest
  call append(0, ["some-search-term"])
  write
  let l:current = bufnr()

  set stickybuf

  edit! last.unittest
  call append(0, ["some-search-term"])
  write
  let l:last = bufnr()

  buffer! sticky.unittest

  " Don't error but don't swap to the first match because the current window
  " has 'stickybuf' enabled
  let l:caught = s:execute_try_catch("vimgrep /some-search-term/ *.unittest")
  call assert_equal(0, l:caught)
  call assert_equal(l:current, bufnr())

  " Don't error and also do swap to the first match because ! was included
  let l:caught = s:execute_try_catch("vimgrep! /some-search-term/ *.unittest")
  call assert_equal(0, l:caught)
  call assert_notequal(l:current, bufnr())

  call delete("first.unittest")
  call delete("sticky.unittest")
  call delete("last.unittest")
endfunc

" Allow :vimgrepadd but 'stickybuf' windows will not jump to the first match
" unless [!] is used.
func Test_vimgrepadd()
  CheckFeature quickfix
  call s:reset_all_buffers()

  edit first.unittest
  call append(0, ["some-search-term"])
  write

  edit sticky.unittest
  call append(0, ["some-search-term"])
  write
  let l:current = bufnr()

  set stickybuf

  edit! last.unittest
  call append(0, ["some-search-term"])
  write
  let l:last = bufnr()

  buffer! sticky.unittest

  " Don't error but don't swap to the first match because the current window
  " has 'stickybuf' enabled
  let l:caught = s:execute_try_catch("vimgrepadd /some-search-term/ *.unittest")
  call assert_equal(0, l:caught)
  call assert_equal(l:current, bufnr())

  " Don't error and also do swap to the first match because ! was included
  let l:caught = s:execute_try_catch("vimgrepadd! /some-search-term/ *.unittest")
  call assert_equal(0, l:caught)
  call assert_notequal(l:current, bufnr())

  call delete("first.unittest")
  call delete("sticky.unittest")
  call delete("last.unittest")
endfunc

" Fail :wNext but :wNext! is allowed
func Test_wNext()
  call s:reset_all_buffers()

  let [l:first, _] = s:make_args_list()
  next!

  let l:caught = s:execute_try_catch("wNext")
  call assert_equal(1, l:caught)
  call assert_notequal(l:first, bufnr())

  let l:caught = s:execute_try_catch("wNext!")
  call assert_equal(0, l:caught)
  call assert_equal(l:first, bufnr())

  call delete("first")
  call delete("middle")
  call delete("last")
endfunc

" Allow :windo unless `:windo foo` would change a 'stickybuf' window's buffer
func Test_windo()
  call s:reset_all_buffers()

  let l:current_window = win_getid()
  let l:current_buffer = bufnr()
  split
  execute "normal \<C-w>j"
  set stickybuf

  let l:current = win_getid()

  let l:caught = s:execute_try_catch("windo echo ''")
  call assert_equal(0, l:caught)
  call assert_equal(l:current_window, win_getid())

  let l:caught = s:execute_try_catch("windo buffer " . l:current_buffer)
  call assert_equal(1, l:caught)
  call assert_equal(l:current_window, win_getid())

  let l:caught = s:execute_try_catch("windo buffer! " . l:current_buffer)
  call assert_equal(0, l:caught)
  call assert_equal(l:current_window, win_getid())
endfunc

" Fail :wnext but :wnext! is allowed
func Test_wnext()
  call s:reset_all_buffers()

  let [_, l:last] = s:make_args_list()
  next!

  let l:caught = s:execute_try_catch("wnext")
  call assert_equal(1, l:caught)
  call assert_notequal(l:last, bufnr())

  let l:caught = s:execute_try_catch("wnext!")
  call assert_equal(0, l:caught)
  call assert_equal(l:last, bufnr())

  call delete("first")
  call delete("middle")
  call delete("last")
endfunc

" Fail :wprevious but :wprevious! is allowed
func Test_wprevious()
  call s:reset_all_buffers()

  let [l:first, _] = s:make_args_list()
  next!

  let l:caught = s:execute_try_catch("wprevious")
  call assert_equal(1, l:caught)
  call assert_notequal(l:first, bufnr())

  let l:caught = s:execute_try_catch("wprevious!")
  call assert_equal(0, l:caught)
  call assert_equal(l:first, bufnr())

  call delete("first")
  call delete("middle")
  call delete("last")
endfunc
