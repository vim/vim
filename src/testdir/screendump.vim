" Functions shared by tests making screen dumps.

" Only load this script once.
if exists('*RunVimInTerminal')
  finish
endif

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

  " Always doo this with 256 colors and a light background.
  set t_Co=256
  hi Normal ctermfg=0 ctermbg=15

  let cmd = GetVimCommandClean()
  let cmd .= ' ' . a:arguments
  let buf = term_start(cmd, {'curwin': 1, 'term_rows': 20, 'term_cols': 75})
  call assert_equal([20, 75], term_getsize(buf))

  return buf
endfunc

" Stop a Vim running in terminal buffer "buf".
func StopVimInTerminal(buf)
  call assert_equal("running", term_getstatus(a:buf))
  call term_sendkeys(a:buf, ":qa!\<cr>")
  call WaitFor('term_getstatus(' . a:buf . ') == "finished"')
  only!
endfunc

" Verify that Vim running in terminal buffer "buf" matches the screen dump.
" The file name used is "dumps/{filename}.dump".
" Will wait for up to a second for the screen dump to match.
func VerifyScreenDump(buf, filename)
  let reference = 'dumps/' . a:filename . '.dump'
  let testfile = a:filename . '.dump.failed'

  let i = 0
  while 1
    call delete(testfile)
    call term_dumpwrite(a:buf, testfile)
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
