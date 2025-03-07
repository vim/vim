" Runs all the syntax tests for which there is no "done/name" file.
"
" Current directory must be runtime/syntax.

" needed because of line-continuation lines
set cpo&vim

" Only do this with the +eval feature
if 1

" Remember the directory where we started.  Will change to "testdir" below.
let syntaxDir = getcwd()

let s:messagesFname = fnameescape(syntaxDir .. '/testdir/messages')

let s:messages = []

" Erase the cursor line and do not advance the cursor.
def EraseLineAndReturnCarriage(rname: string)
  const full_width: number = winwidth(0)
  const half_width: number = full_width - (full_width + 1) / 2
  if (strlen(rname) + strlen('Test' .. "\x20\x20" .. 'FAILED')) > half_width
    echon "\r" .. repeat("\x20", full_width) .. "\r"
  else
    echon repeat("\x20", half_width) .. "\r"
  endif
enddef

" Add one message to the list of messages
func Message(msg)
  echomsg a:msg
  call add(s:messages, a:msg)
endfunc

" Report a fatal message and exit
func Fatal(msg)
  echoerr a:msg
  call AppendMessages(a:msg)
  qall!
endfunc

" Append s:messages to the messages file and make it empty.
func AppendMessages(header)
  silent exe 'split ' .. s:messagesFname
  call append(line('$'), '')
  call append(line('$'), a:header)
  call append(line('$'), s:messages)
  let s:messages = []
  silent wq
endfunc

" Relevant messages are written to the "messages" file.
" If the file already exists it is appended to.
silent exe 'split ' .. s:messagesFname
call append(line('$'), repeat('=-', 70))
call append(line('$'), '')
let s:test_run_message = 'Test run on ' .. strftime("%Y %b %d %H:%M:%S")
call append(line('$'), s:test_run_message)
silent wq
echo "\n"

if syntaxDir !~ '[/\\]runtime[/\\]syntax\>'
  call Fatal('Current directory must be "runtime/syntax"')
endif
if !isdirectory('testdir')
  call Fatal('"testdir" directory not found')
endif

" Use the script for source code screendump testing.  It sources other scripts,
" therefore we must "cd" there.
cd ../../src/testdir
source screendump.vim
exe 'cd ' .. fnameescape(syntaxDir)

" For these tests we need to be able to run terminal Vim with 256 colors.  On
" MS-Windows the console only has 16 colors and the GUI can't run in a
" terminal.
if !CanRunVimInTerminal()
  call Fatal('Cannot make screendumps, aborting')
endif

cd testdir
if !isdirectory('done')
  call mkdir('done')
endif

set nocp
set nowrapscan
set report=9999
set modeline
set debug=throw
set nomore

au! SwapExists * call HandleSwapExists()
func HandleSwapExists()
  " Ignore finding a swap file for the test input, the user might be editing
  " it and that's OK.
  if expand('<afile>') =~ 'input[/\\].*\..*'
    let v:swapchoice = 'e'
  endif
endfunc

" Trace ruler liveness on demand.
if !empty($VIM_SYNTAX_TEST_LOG) && filewritable($VIM_SYNTAX_TEST_LOG)
  def s:TraceRulerLiveness(context: string, times: number, tail: string)
    writefile([printf('%s: %4d: %s', context, times, tail)],
	$VIM_SYNTAX_TEST_LOG,
	'a')
  enddef
else
  def s:TraceRulerLiveness(_: string, _: number, _: string)
  enddef
endif

" See ":help 'ruler'".
def s:CannotSeeLastLine(ruler: list<string>): bool
  return !(get(ruler, -1, '') ==# 'All' || get(ruler, -1, '') ==# 'Bot')
enddef

def s:CannotDumpNextPage(buf: number, prev_ruler: list<string>, ruler: list<string>): bool
  return !(ruler !=# prev_ruler &&
      len(ruler) == 2 &&
      ruler[1] =~# '\%(\d%\|\<Bot\)$' &&
      get(term_getcursor(buf), 0) != 20)
enddef

def s:CannotDumpFirstPage(buf: number, _: list<string>, ruler: list<string>): bool
  return !(len(ruler) == 2 &&
      ruler[1] =~# '\%(\<All\|\<Top\)$' &&
      get(term_getcursor(buf), 0) != 20)
enddef

def s:CannotDumpShellFirstPage(buf: number, _: list<string>, ruler: list<string>): bool
  return !(len(ruler) > 3 &&
      get(ruler, -1, '') =~# '\%(\<All\|\<Top\)$' &&
      get(term_getcursor(buf), 0) != 20)
enddef

" Poll for updates of the cursor position in the terminal buffer occupying the
" first window.  (ALWAYS call the function or its equivalent before calling
" "VerifyScreenDump()" *and* after calling any number of "term_sendkeys()".)
def s:TermPollRuler(
	CannotDumpPage: func,	# (TYPE FOR LEGACY CONTEXT CALL SITES.)
	buf: number,
	in_name_and_out_name: string): list<string>
  # Expect defaults from "term_util#RunVimInTerminal()".
  if winwidth(1) != 75 || winheight(1) != 20
    ch_log(printf('Aborting for %s: (75 x 20) != (%d x %d)',
      in_name_and_out_name,
      winwidth(1),
      winheight(1)))
    return ['0,0-1', 'All']
  endif
  # A two-fold role for redrawing:
  # (*) in case the terminal buffer cannot redraw itself just yet;
  # (*) to avoid extra "real estate" checks.
  redraw
  # The contents of "ruler".
  var ruler: list<string> = []
  # Attempts at most, targeting ASan-instrumented Vim builds.
  var times: number = 2048
  # Check "real estate" of the terminal buffer.  Read and compare its ruler
  # line and let "Xtestscript#s:AssertCursorForwardProgress()" do the rest.
  # Note that the cursor ought to be advanced after each successive call of
  # this function yet its relative position need not be changed (e.g. "0%").
  while CannotDumpPage(ruler) && times > 0
    ruler = split(term_getline(buf, 20))
    sleep 1m
    times -= 1
    if times % 8 == 0
      redraw
    endif
  endwhile
  TraceRulerLiveness('P', (2048 - times), in_name_and_out_name)
  return ruler
enddef

" Prevent "s:TermPollRuler()" from prematurely reading the cursor position,
" which is available at ":edit", after outracing the loading of syntax etc. in
" the terminal buffer.  (Call the function before calling "VerifyScreenDump()"
" for the first time.)
def s:TermWaitAndPollRuler(buf: number, in_name_and_out_name: string): list<string>
  # Expect defaults from "term_util#RunVimInTerminal()".
  if winwidth(1) != 75 || winheight(1) != 20
    ch_log(printf('Aborting for %s: (75 x 20) != (%d x %d)',
      in_name_and_out_name,
      winwidth(1),
      winheight(1)))
    return ['0,0-1', 'All']
  endif
  # The contents of "ruler".
  var ruler: string = ''
  # Attempts at most, targeting ASan-instrumented Vim builds.
  var times: number = 32768
  # Check "real estate" of the terminal buffer.  Expect a known token to be
  # rendered in the terminal buffer; its prefix must be "is_" so that buffer
  # variables from "sh.vim" can be matched (see "Xtestscript#ShellInfo()").
  # Verify that the whole line is available!
  while ruler !~# '^is_.\+\s\%(All\|Top\)$' && times > 0
    ruler = term_getline(buf, 20)
    sleep 1m
    times -= 1
    if times % 16 == 0
      redraw
    endif
  endwhile
  TraceRulerLiveness('W', (32768 - times), in_name_and_out_name)
  if strpart(ruler, 0, 8) !=# 'is_nonce'
    # Retain any of "b:is_(bash|dash|kornshell|posix|sh)" entries and let
    # "CannotDumpShellFirstPage()" win the cursor race.
    return TermPollRuler(
	function(CannotDumpShellFirstPage, [buf, []]),
	buf,
	in_name_and_out_name)
  else
    # Clear the "is_nonce" token and let "CannotDumpFirstPage()" win any
    # race.
    term_sendkeys(buf, ":redraw!\<CR>")
  endif
  return TermPollRuler(
      function(CannotDumpFirstPage, [buf, []]),
      buf,
      in_name_and_out_name)
enddef

func RunTest()
  let ok_count = 0
  let failed_tests = []
  let skipped_count = 0
  let MAX_FAILED_COUNT = 5
  " Create a map of setup configuration filenames with their basenames as keys.
  let setup = glob('input/setup/*.vim', 1, 1)
    \ ->reduce({d, f -> extend(d, {fnamemodify(f, ':t:r'): f})}, {})
  " Turn a subset of filenames etc. requested for testing into a pattern.
  let filter = filereadable('../testdir/Xfilter')
    \ ? readfile('../testdir/Xfilter')
	\ ->map({_, v -> '^' .. substitute(v, '_$', '', '')})
	\ ->join('\|')
    \ : ''

  " Treat "\.self-testing$" as a string NOT as a regexp.
  if filter ==# '\.self-testing$'
    let dirpath = 'input/selftestdir/'
    let fnames = readdir(dirpath, {fname -> fname !~ '^README\.txt$'})
  else
    let dirpath = 'input/'
    let filter ..= exists("$VIM_SYNTAX_TEST_FILTER") &&
		\ !empty($VIM_SYNTAX_TEST_FILTER)
      \ ? (empty(filter) ? '' : '\|') .. $VIM_SYNTAX_TEST_FILTER
      \ : ''
    let fnames = readdir(dirpath,
	\ {subset -> {fname -> fname !~ '\~$' && fname =~# subset}}(
		\ empty(filter) ? '^.\+\..\+$' : filter))
  endif

  for fname in fnames
    let root = fnamemodify(fname, ':r')
    let fname = dirpath .. fname
    let filetype = substitute(root, '\([^_.]*\)[_.].*', '\1', '')
    let failed_root = 'failed/' .. root

    " Execute the test if the "done" file does not exist or when the input file
    " is newer.
    let in_time = getftime(fname)
    let out_time = getftime('done/' .. root)
    if out_time < 0 || in_time > out_time
      call ch_log('running tests for: ' .. fname)

      for dumpname in glob(failed_root .. '_\d*\.dump', 1, 1)
	call delete(dumpname)
      endfor
      call delete('done/' .. root)

      let lines =<< trim END
	" Track the cursor progress through a syntax test file so that any
	" degenerate input can be reported.  Each file will have its own cursor.
	let s:cursor = 1

	" extra info for shell variables
	func ShellInfo()
	  let msg = ''
	  for [key, val] in items(b:)
	    if key =~ '^is_'
	      let msg ..= key .. ': ' .. val .. ', '
	    endif
	  endfor
	  if msg != ''
	    echomsg msg
	  endif
	endfunc

	au! SwapExists * call HandleSwapExists()
	func HandleSwapExists()
	  " Ignore finding a swap file for the test input, the user might be
	  " editing it and that's OK.
	  if expand('<afile>') =~ 'input[/\\].*\..*'
	    let v:swapchoice = 'e'
	  endif
	endfunc

	func LoadFiletype(type)
	  for file in glob("ftplugin/" .. a:type .. "*.vim", 1, 1)
	    exe "source " .. file
	  endfor
	  redraw!
	endfunc

	func SetUpVim()
	  call cursor(1, 1)
	  " Defend against rogue VIM_TEST_SETUP commands.
	  for _ in range(20)
	    let lnum = search('\C\<VIM_TEST_SETUP\>', 'eW', 20)
	    if lnum < 1
	      break
	    endif
	    exe substitute(getline(lnum), '\C.*\<VIM_TEST_SETUP\>', '', '')
	  endfor
	  call cursor(1, 1)
	  " BEGIN [runtime/defaults.vim]
	  " Also, disable italic highlighting to avoid issues on some terminals.
	  set display=lastline ruler scrolloff=5 t_ZH= t_ZR=
	  syntax on
	  " END [runtime/defaults.vim]
	  redraw!
	endfunc

	def s:AssertCursorForwardProgress(): bool
	  const curnum: number = line('.')
	  if curnum <= cursor
	    # Use "actions/upload-artifact@v4" of ci.yml for delivery.
	    writefile([printf('No cursor progress: %d <= %d (%s).  Please file an issue.',
		  curnum,
		  cursor,
		  bufname('%'))],
	      'failed/00-FIXME',
	      'a')
	    bwipeout!
	  endif
	  cursor = curnum
	  return true
	enddef

	def ScrollToSecondPage(estate: number, op_wh: number, op_so: number): bool
	  if line('.') != 1 || line('w$') >= line('$')
	    return AssertCursorForwardProgress()
	  endif
	  try
	    set scrolloff=0
	    # Advance mark "c"[ursor] along with the cursor.
	    norm! Lmc
	    if foldclosed('.') < 0 &&
		(strdisplaywidth(getline('.')) + &l:fdc * winheight(1)) >= estate
	      # Make for an exit for a screenful long line.
	      norm! j^
	      return AssertCursorForwardProgress()
	    else
	      # Place the cursor on the actually last visible line.
	      while winline() < op_wh
		const lastnum: number = winline()
		norm! gjmc
		if lastnum > winline()
		  break
		endif
	      endwhile
	      norm! zt
	    endif
	  finally
	    # COMPATIBILITY: Scroll up around "scrolloff" lines.
	    &scrolloff = max([1, op_so])
	  endtry
	  norm! ^
	  return AssertCursorForwardProgress()
	enddef

	def ScrollToNextPage(estate: number, op_wh: number, op_so: number): bool
	  if line('.') == 1 || line('w$') >= line('$')
	    return AssertCursorForwardProgress()
	  endif
	  try
	    set scrolloff=0
	    # Advance mark "c"[ursor] along with the cursor.
	    norm! Lmc
	    if foldclosed('.') < 0 &&
		(strdisplaywidth(getline('.')) + &l:fdc * winheight(1)) >= estate
	      # Make for an exit for a screenful long line.
	      norm! j^
	      return AssertCursorForwardProgress()
	    else
	      # Place the cursor on the actually last visible line.
	      while winline() < op_wh
		const lastnum: number = winline()
		norm! gjmc
		if lastnum > winline()
		  break
		endif
	      endwhile
	    endif
	  finally
	    # COMPATIBILITY: Scroll up/down around "scrolloff" lines.
	    &scrolloff = max([1, op_so])
	  endtry
	  norm! zt
	  const marknum: number = line("'c")
	  # Eschew &smoothscroll since line("`c") is not supported.
	  # Remember that "w0" can point to the first line of a _closed_ fold
	  # whereas the last line of a _closed_ fold can be marked.
	  if line('w0') > marknum
	    while line('w0') > marknum
	      exe "norm! \<C-y>"
	    endwhile
	    if line('w0') != marknum
	      exe "norm! \<C-e>H"
	    endif
	  # Handle non-wrapped lines.
	  elseif line('w0') < marknum
	    while line('w0') < marknum
	      exe "norm! \<C-e>"
	    endwhile
	    if line('w0') != marknum
	      exe "norm! \<C-y>H"
	    endif
	  endif
	  norm! ^
	  return AssertCursorForwardProgress()
	enddef
      END
      call writefile(lines, 'Xtestscript')

      " close all but the last window
      while winnr('$') > 1
	close
      endwhile

      " Redraw to make sure that messages are cleared and there is enough space
      " for the terminal window.
      redraw

      " Let "Xtestscript#SetUpVim()" turn the syntax on.
      let prefix = '-Nu NONE -S Xtestscript'
      let path = get(setup, root, '')
      " Source the found setup configuration file.
      let args = !empty(path)
	\ ? prefix .. ' -S ' .. path
	\ : prefix
      let buf = RunVimInTerminal(args, {})
      " edit the file only after catching the SwapExists event
      call term_sendkeys(buf, ":edit " .. fname .. "\<CR>")
      " set up the testing environment
      call term_sendkeys(buf, ":call SetUpVim()\<CR>")
      " load filetype specific settings
      call term_sendkeys(buf, ":call LoadFiletype('" .. filetype .. "')\<CR>")

      " Make a synchronisation point between buffers by requesting to echo
      " a known token in the terminal buffer and asserting its availability
      " with "s:TermWaitAndPollRuler()".
      if filetype == 'sh'
	call term_sendkeys(buf, ":call ShellInfo()\<CR>")
      else
	call term_sendkeys(buf, ":echo 'is_nonce'\<CR>")
      endif

      let root_00 = root .. '_00'
      let in_name_and_out_name = fname .. ': failed/' .. root_00 .. '.dump'
      " Queue up all "term_sendkeys()"es and let them finish before returning
      " from "s:TermWaitAndPollRuler()".
      let ruler = s:TermWaitAndPollRuler(buf, in_name_and_out_name)
      call ch_log('First screendump for ' .. in_name_and_out_name)
      " Make a screendump at the start of the file: failed/root_00.dump
      let fail = VerifyScreenDump(buf, root_00, {})

      " Accommodate the next code block to "buf"'s contingency for self
      " wipe-out.
      try
	let nr = 0
	let keys_a = ":call ScrollToSecondPage((18 * 75 + 1), 19, 5) | redraw!\<CR>"
	let keys_b = ":call ScrollToNextPage((18 * 75 + 1), 19, 5) | redraw!\<CR>"
	while s:CannotSeeLastLine(ruler)
	  call term_sendkeys(buf, keys_a)
	  let keys_a = keys_b
	  let nr += 1
	  let root_next = printf('%s_%02d', root, nr)
	  let in_name_and_out_name = fname .. ': failed/' .. root_next .. '.dump'
	  let ruler = s:TermPollRuler(
	      \ function('s:CannotDumpNextPage', [buf, ruler]),
	      \ buf,
	      \ in_name_and_out_name)
	  call ch_log('Next screendump for ' .. in_name_and_out_name)
	  " Make a screendump of every 18 lines of the file: failed/root_NN.dump
	  let fail += VerifyScreenDump(buf, root_next, {})
	endwhile
	call StopVimInTerminal(buf)
      finally
	call delete('Xtestscript')
      endtry

      " redraw here to avoid the following messages to get mixed up with screen
      " output.
      redraw

      " Add any assert errors to s:messages.
      if len(v:errors) > 0
	call extend(s:messages, v:errors)
	" Echo the errors here, in case the script aborts or the "messages" file
	" is not displayed later.
	echomsg v:errors
	let v:errors = []
	let fail += 1
      endif

      if fail == 0
	call Message("Test " .. root .. " OK")

	call writefile(['OK'], 'done/' .. root)

	let ok_count += 1
      else
	call Message("Test " .. root .. " FAILED")

	call delete('done/' .. root)

	eval failed_tests->add(root)
	if len(failed_tests) > MAX_FAILED_COUNT
	  call Message('')
	  call Message('Too many errors, aborting')
	endif
      endif
    else
      call Message("Test " .. root .. " skipped")
      let skipped_count += 1
    endif

    call EraseLineAndReturnCarriage(root)

    " Append messages to the file "testdir/messages"
    call AppendMessages('Input file ' .. fname .. ':')

    if len(failed_tests) > MAX_FAILED_COUNT
      break
    endif
  endfor

  call EraseLineAndReturnCarriage('')
  call Message(s:test_run_message)
  call Message('OK: ' .. ok_count)
  call Message('FAILED: ' .. len(failed_tests) .. ': ' .. string(failed_tests))
  call Message('skipped: ' .. skipped_count)

  if !empty(failed_tests)
    call Message('')
    call Message('View generated screendumps with "../../src/vim --clean -S testdir/viewdumps.vim"')
  endif

  call AppendMessages('== SUMMARY SYNTAX TESTS ==')

  if len(failed_tests) > 0
    " have make report an error
    cquit
  endif
endfunc

call RunTest()

" Matching "if 1" at the start.
endif

qall!

" vim:sw=2:ts=8:noet:
