" Functions shared by tests making screen dumps.

" Only load this script once.
if exists('*VerifyScreenDump')
  finish
endif

source shared.vim
source term_util.vim

" Skip the rest if there is no terminal feature at all.
if !has('terminal')
  finish
endif

" Verify that Vim running in terminal buffer "buf" matches the screen dump.
" "options" is passed to term_dumpwrite().
" Additionally, the "wait" entry can specify the maximum time to wait for the
" screen dump to match in msec (default 1000 msec).
" The file name used is "dumps/{filename}.dump".
" Optionally an extra argument can be passed which is prepended to the error
" message.  Use this when using the same dump file with different options.
" Returns non-zero when verification fails.
func VerifyScreenDump(buf, filename, options, ...)
  let reference = 'dumps/' . a:filename . '.dump'
  let testfile = 'failed/' . a:filename . '.dump'

  let max_loops = get(a:options, 'wait', 1000) / 10

  " Starting a terminal to make a screendump is always considered flaky.
  let g:test_is_flaky = 1

  " wait for the pending updates to be handled.
  call TermWait(a:buf)

  " Redraw to execute the code that updates the screen.  Otherwise we get the
  " text and attributes only from the internal buffer.
  redraw

  let did_mkdir = 0
  if !isdirectory('failed')
    let did_mkdir = 1
    call mkdir('failed')
  endif

  let i = 0
  while 1
    " leave some time for updating the original window
    sleep 10m
    call delete(testfile)
    call term_dumpwrite(a:buf, testfile, a:options)
    let testdump = readfile(testfile)
    if filereadable(reference)
      let refdump = readfile(reference)
    else
      " Must be a new screendump, always fail
      let refdump = []
    endif
    if refdump == testdump
      call delete(testfile)
      if did_mkdir
	call delete('failed', 'd')
      endif
      break
    endif
    if i == max_loops
      " Leave the failed dump around for inspection.
      if filereadable(reference)
	let msg = 'See dump file difference: call term_dumpdiff("testdir/' .. testfile .. '", "testdir/' .. reference .. '")'
	if a:0 == 1
	  let msg = a:1 . ': ' . msg
	endif
	if len(testdump) != len(refdump)
	  let msg = msg . '; line count is ' . len(testdump) . ' instead of ' . len(refdump)
	endif
      else
	let msg = 'See new dump file: call term_dumpload("testdir/' .. testfile .. '")'
      endif
      for i in range(len(refdump))
	if i >= len(testdump)
	  break
	endif
	if testdump[i] != refdump[i]
	  let msg = msg . '; difference in line ' . (i + 1) . ': "' . testdump[i] . '"'
	endif
      endfor
      call assert_report(msg)
      return 1
    endif
    let i += 1
  endwhile
  return 0
endfunc
