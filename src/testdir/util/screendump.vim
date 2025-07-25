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

" Accommodate rendering idiosyncrasies (see #16559).  For details, refer to
" "VerifyScreenDump()" and the "options" dictionary passed to it: this is
" an implementation of its "FileComparisonPreAction" entry.  (This function
" runs in couples with "g:ScreenDumpLookForFFFDChars()".)
def g:ScreenDumpDiscardFFFDChars(
	state: dict<number>,
	testdump: list<string>,
	refdump: list<string>)
  if empty(state) || len(testdump) != len(refdump)
    return
  endif
  for lstr: string in keys(state)
    const lnum: number = str2nr(lstr)
    const fst_fffd_idx: number = stridx(testdump[lnum], "\xef\xbf\xbd")
    # Retroactively discard non-equal line suffixes.  It is assumed that no
    # runs of U+EFU+BFU+BD and no U+FFFDs are present in "refdump".
    if fst_fffd_idx >= 0
      # Mask the "||" character cells and the cursor cell ">.".
      const masked_part: string = substitute(
	  substitute(
	      strpart(testdump[lnum], 0, (fst_fffd_idx - 1)),
	      '[>|]|', '|.', 'g'),
	  '|\@<!>', '|', 'g')
      const prev_cell_idx: number = strridx(masked_part, '|')
      # A series of repeated characters will be found recorded in shorthand;
      # e.g. "|α@3" stands for a cell of four "α"s.  Replacing any repeated
      # multibyte character of a series with a U+FFFD character will split the
      # series and its shorthand record will reflect this fact: "|α@2|�".
      # Therefore, a common prefix to share for two corresponding lines can
      # extend to either an ASCII character(s) cell before the leftmost U+FFFD
      # character cell; or, a last-but-one arbitrary cell before the leftmost
      # U+FFFD character cell; or, an empty string.
      const prefix: number = (prev_cell_idx >= 0)
	  ? (char2nr(strpart(masked_part, (prev_cell_idx + 1), 1), true) < 128)
	      ? fst_fffd_idx - 1
	      : (strridx(masked_part, '|', (prev_cell_idx - 1)) >= 0)
		  ? prev_cell_idx
		  : 0
	  : 0
      refdump[lnum] = strpart(refdump[lnum], 0, prefix)
      testdump[lnum] = strpart(testdump[lnum], 0, prefix)
    endif
  endfor
enddef

" Accommodate rendering idiosyncrasies (see #16559).  For details, refer to
" "VerifyScreenDump()" and the "options" dictionary passed to it: this is
" an implementation of its "NonEqualLineComparisonPostAction" entry.  (This
" function runs in couples with "g:ScreenDumpDiscardFFFDChars()".)
def g:ScreenDumpLookForFFFDChars(
	state: dict<number>,
	testdump: list<string>,
	lnum: number)
  if stridx(testdump[lnum], "\xef\xbf\xbd") >= 0
    state[string(lnum)] = 1
  endif
enddef

" Verify that Vim running in terminal buffer "buf" matches the screen dump.
"
" A copy of "options" is passed to "term_dumpwrite()".  For convenience, this
" dictionary supports other optional entries:
"   "wait", (default to 1000 msec at least)
"	the maximum time to wait for the screen dump to match in msec.
"   "FileComparisonPreAction", (default to a no-op action)
"	some Funcref to call, passing the following three arguments, each time
"	before the file contents of two screen dumps are compared:
"	    some dictionary with some state entries;
"	    the file contents of the newly generated screen dump;
"	    the file contents of the reference screen dump.
"   "NonEqualLineComparisonPostAction", (default to a no-op action)
"	some Funcref to call, passing the following three arguments, each time
"	after a corresponding pair of lines is found not equal:
"	    some dictionary with some state entries;
"	    the file contents of the newly generated screen dump;
"	    the zero-based number of the line whose copies are not equal.
" (See an example in runtime/syntax/testdir/runtest.vim.)
"
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

  let options_copy = copy(a:options)
  if has_key(options_copy, 'wait')
    let max_loops = max([0, remove(options_copy, 'wait')])
  else
    let max_loops = 1000
  endif
  if has_key(options_copy, 'FileComparisonPreAction')
    let FileComparisonPreAction = remove(options_copy, 'FileComparisonPreAction')
    let CopyStringList = {_refdump -> copy(_refdump)}
  else
    let FileComparisonPreAction = {_state, _testdump, _refdump -> 0}
    let CopyStringList = {_refdump -> _refdump}
  endif
  if has_key(options_copy, 'NonEqualLineComparisonPostAction')
    let NonEqualLineComparisonPostAction = remove(options_copy, 'NonEqualLineComparisonPostAction')
  else
    let NonEqualLineComparisonPostAction = {_state, _testdump, _lnum -> 0}
  endif

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
    call term_dumpwrite(a:buf, testfile, options_copy)
    call assert_report('See new dump file: call term_dumpload("testdir/' .. testfile .. '")')
    " No point in retrying.
    let g:run_nr = 10
    return 1
  endif

  let refdump_orig = ReadAndFilter(reference, filter)
  let state = {}
  let i = 0
  while 1
    " Leave a bit of time for updating the original window while we spin wait.
    sleep 1m
    call delete(testfile)
    call term_dumpwrite(a:buf, testfile, options_copy)
    " Filtering done with "FileComparisonPreAction()" may change "refdump*".
    let refdump = CopyStringList(refdump_orig)
    let testdump = ReadAndFilter(testfile, filter)
    call FileComparisonPreAction(state, testdump, refdump)
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
	call NonEqualLineComparisonPostAction(state, testdump, j)
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
