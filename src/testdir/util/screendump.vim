" Functions shared by tests making screen dumps.

" Only load this script once.
if exists('*VerifyScreenDump')
  finish
endif

" Skip the rest if there is no terminal feature at all.
if !has('terminal')
  finish
endif

" Read a dump file "fname" and if "filter" exists apply it to the text.
def ReadAndFilter(fname: string, filter: string): list<string>
  var contents = readfile(fname)

  if filereadable(filter)
    # do this in the bottom window so that the terminal window is unaffected
    wincmd j
    enew
    setline(1, contents)
    exe "source " .. filter
    contents = getline(1, '$')
    enew!
    wincmd k
    redraw
  endif

  return contents
enddef


" Verify that Vim running in terminal buffer "buf" matches the screen dump.
" "options" is passed to term_dumpwrite().
" Additionally, the "wait" entry can specify the maximum time to wait for the
" screen dump to match in msec (default 1000 msec).
" The file name used is "dumps/{filename}.dump".
"
" To ignore part of the dump, provide a "dumps/{filename}.vim" file with
" Vim commands to be applied to both the reference and the current dump, so
" that parts that are irrelevant are not used for the comparison.  The result
" is NOT written, thus "term_dumpdiff()" shows the difference anyway.
"
" Optionally an extra argument can be passed which is prepended to the error
" message.  Use this when using the same dump file with different options.
" Returns non-zero when verification fails.
func VerifyScreenDump(buf, filename, options, ...)
  if has('gui_running') && exists("g:check_screendump_called") && g:check_screendump_called == v:false
    echoerr "VerifyScreenDump() called from a test that lacks a CheckScreendump guard."
    return 1
  endif
  let reference = 'dumps/' . a:filename . '.dump'
  let filter = 'dumps/' . a:filename . '.vim'
  let testfile = 'failed/' . a:filename . '.dump'

  let max_loops = get(a:options, 'wait', 1000) / 1

  " Starting a terminal to make a screendump is always considered flaky.
  let g:test_is_flaky = 1
  let g:giveup_same_error = 0

  " wait for the pending updates to be handled.
  call TermWait(a:buf, 0)

  " Redraw to execute the code that updates the screen.  Otherwise we get the
  " text and attributes only from the internal buffer.
  redraw

  let did_mkdir = 0
  if !isdirectory('failed')
    let did_mkdir = 1
    call mkdir('failed')
  endif

  if !filereadable(reference)
    " Leave a bit of time for updating the original window while we spin wait.
    sleep 10m
    call delete(testfile)
    call term_dumpwrite(a:buf, testfile, a:options)
    call assert_report('See new dump file: call term_dumpload("testdir/' .. testfile .. '")')
    " No point in retrying.
    let g:run_nr = 10
    return 1
  endif

  let refdump = ReadAndFilter(reference, filter)
  let i = 0
  while 1
    " Leave a bit of time for updating the original window while we spin wait.
    sleep 1m
    call delete(testfile)
    call term_dumpwrite(a:buf, testfile, a:options)
    let testdump = ReadAndFilter(testfile, filter)
    if refdump == testdump
      call delete(testfile)
      if did_mkdir
	call delete('failed', 'd')
      endif
      if i > 0
	call remove(v:errors, -1)
      endif
      break
    endif

    " Leave the failed dump around for inspection.
    let msg = 'See dump file difference: call term_dumpdiff("testdir/' .. testfile .. '", "testdir/' .. reference .. '")'
    if a:0 == 1
      let msg = a:1 . ': ' . msg
    endif
    if len(testdump) != len(refdump)
      let msg = msg . '; line count is ' . len(testdump) . ' instead of ' . len(refdump)
    endif
    for j in range(len(refdump))
      if j >= len(testdump)
	break
      endif
      if testdump[j] != refdump[j]
	let msg = msg . '; difference in line ' . (j + 1) . ': "' . testdump[j] . '"'
      endif
    endfor

    " Always add the last error so that it is displayed on timeout.
    " See TestTimeout() in runtest.vim.
    if i > 0
      call remove(v:errors, -1)
    endif
    call assert_report(msg)

    let i += 1
    if i >= max_loops
      return 1
    endif
  endwhile
  return 0
endfunc

" vim:sw=2:ts=8:noet:
