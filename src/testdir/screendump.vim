" Functions shared by tests making screen dumps.

" Only load this script once.
if exists('*CanRunVimInTerminal')
  finish
endif

" Need to be able to run terminal Vim with 256 colors.  On MS-Windows the
" console only has 16 colors and the GUI can't run in a terminal.
if !has('terminal') || has('win32')
  func CanRunVimInTerminal()
    return 0
  endfunc
  finish
endif

func CanRunVimInTerminal()
  return 1
endfunc

source shared.vim

" Run Vim with "arguments" in a new terminal window.
" By default uses a size of 20 lines and 75 columns.
" Returns the buffer number of the terminal.
"
" Options is a dictionary, these items are recognized:
" "rows" - height of the terminal window (max. 20)
" "cols" - width of the terminal window (max. 78)
func RunVimInTerminal(arguments, options)
  " If Vim doesn't exit a swap file remains, causing other tests to fail.
  " Remove it here.
  call delete(".swp")

  if exists('$COLORFGBG')
    " Clear $COLORFGBG to avoid 'background' being set to "dark", which will
    " only be corrected if the response to t_RB is received, which may be too
    " late.
    let $COLORFGBG = ''
  endif

  " Make a horizontal and vertical split, so that we can get exactly the right
  " size terminal window.  Works only when the current window is full width.
  call assert_equal(&columns, winwidth(0))
  split
  vsplit

  " Always do this with 256 colors and a light background.
  set t_Co=256 background=light
  hi Normal ctermfg=NONE ctermbg=NONE

  " Make the window 20 lines high and 75 columns, unless told otherwise.
  let rows = get(a:options, 'rows', 20)
  let cols = get(a:options, 'cols', 75)

  let cmd = GetVimCommandClean()
  " Add -v to have gvim run in the terminal (if possible)
  let cmd .= ' -v ' . a:arguments
  let buf = term_start(cmd, {'curwin': 1, 'term_rows': rows, 'term_cols': cols})
  call assert_equal([rows, cols], term_getsize(buf))

  " Wait for "All" of the ruler in the status line to be shown.
  " This can be quite slow (e.g. when using valgrind), wait up to 4 seconds.
  call WaitFor({-> len(term_getline(buf, rows)) >= cols - 1}, 4000)

  return buf
endfunc

" Stop a Vim running in terminal buffer "buf".
func StopVimInTerminal(buf)
  call assert_equal("running", term_getstatus(a:buf))
  call term_sendkeys(a:buf, "\<Esc>\<Esc>:qa!\<cr>")
  call WaitFor('term_getstatus(' . a:buf . ') == "finished"', 3000)
  only!
endfunc

" Verify that Vim running in terminal buffer "buf" matches the screen dump.
" "options" is passed to term_dumpwrite().
" The file name used is "dumps/{filename}.dump".
" Will wait for up to a second for the screen dump to match.
func VerifyScreenDump(buf, filename, options)
  let reference = 'dumps/' . a:filename . '.dump'
  let testfile = a:filename . '.dump.failed'

  let i = 0
  while 1
    call delete(testfile)
    call term_dumpwrite(a:buf, testfile, a:options)
    if readfile(reference) == readfile(testfile)
      call delete(testfile)
      break
    endif
    if i == 100
      " Leave the test file around for inspection.
      call assert_report('See dump file difference: call term_dumpdiff("' . testfile . '", "' . reference . '")')
      break
    endif
    sleep 10m
    let i += 1
  endwhile
endfunc
