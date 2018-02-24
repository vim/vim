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
" Options is a dictionary (not used yet).
func RunVimInTerminal(arguments, options)
  " Make a horizontal and vertical split, so that we can get exactly the right
  " size terminal window.  Works only when we currently have one window.
  call assert_equal(1, winnr('$'))
  split
  vsplit

  " Always do this with 256 colors and a light background.
  set t_Co=256 background=light
  hi Normal ctermfg=NONE ctermbg=NONE

  let cmd = GetVimCommandClean()
  " Add -v to have gvim run in the terminal (if possible)
  let cmd .= ' -v ' . a:arguments
  let buf = term_start(cmd, {'curwin': 1, 'term_rows': 20, 'term_cols': 75})
  call assert_equal([20, 75], term_getsize(buf))

  return buf
endfunc

" Stop a Vim running in terminal buffer "buf".
func StopVimInTerminal(buf)
  call assert_equal("running", term_getstatus(a:buf))
  call term_sendkeys(a:buf, "\<Esc>\<Esc>:qa!\<cr>")
  call WaitFor('term_getstatus(' . a:buf . ') == "finished"')
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
