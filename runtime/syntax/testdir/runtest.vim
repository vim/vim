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

" Erase the cursor line and do not advance the cursor.  (Call the function
" after each passing test report.)
def EraseLineAndReturnCarriage(line: string)
  const full_width: number = winwidth(0)
  const half_width: number = full_width - (full_width + 1) / 2
  if strlen(line) > half_width
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

if syntaxDir !~ '[/\\]runtime[/\\]syntax\>'
  call Fatal('Current directory must be "runtime/syntax"')
endif
if !isdirectory('testdir')
  call Fatal('"testdir" directory not found')
endif

" Use the script for source code screendump testing.  It sources other scripts,
" therefore we must "cd" there.
cd ../../src/testdir
source util/screendump.vim
source util/term_util.vim
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

if !isdirectory('failed')
  call mkdir('failed')
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

if !empty($VIM_SYNTAX_TEST_LOG) && filewritable($VIM_SYNTAX_TEST_LOG)
  " Trace liveness.
  def s:TraceLiveness(context: string, times: number, tail: string)
    writefile([printf('%s: %4d: %s', context, times, tail)],
	$VIM_SYNTAX_TEST_LOG,
	'a')
  enddef

  " Anticipate rendering idiosyncrasies (see #16559).
  def s:CanFindRenderedFFFDChars(
	  buf: number,
	  in_name_and_out_name: string,
	  times: number): bool
    if CannotUseRealEstate(in_name_and_out_name)
      return false
    endif
    # Expect a 20-line terminal buffer (see "term_util#RunVimInTerminal()"),
    # where the bottom, reserved line is of the default "&cmdheight".
    var lines: list<number> = []
    for lnum: number in range(1, 19)
      if stridx(term_getline(buf, lnum), "\xef\xbf\xbd") >= 0
	add(lines, lnum)
      endif
    endfor
    TraceLiveness('F', times, string(lines))
    return !empty(lines)
  enddef
else
  " Do not trace liveness.
  def s:TraceLiveness(_: string, _: number, _: string)
  enddef

  " Anticipate rendering idiosyncrasies (see #16559).
  def s:CanFindRenderedFFFDChars(
	  buf: number,
	  in_name_and_out_name: string,
	  _: number): bool
    if CannotUseRealEstate(in_name_and_out_name)
      return false
    endif
    # Expect a 20-line terminal buffer (see "term_util#RunVimInTerminal()"),
    # where the bottom, reserved line is of the default "&cmdheight".
    for lnum: number in range(1, 19)
      if stridx(term_getline(buf, lnum), "\xef\xbf\xbd") >= 0
	return true
      endif
    endfor
    return false
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

def s:CannotUseRealEstate(in_name_and_out_name: string): bool
  # Expect defaults from "term_util#RunVimInTerminal()".
  if winwidth(1) != 75 || winheight(1) != 20
    ch_log(printf('Aborting for %s: (75 x 20) != (%d x %d)',
	in_name_and_out_name,
	winwidth(1),
	winheight(1)))
    return true
  endif
  return false
enddef

" Throw an "FFFD" string if U+FFFD characters are found in the terminal buffer
" during a non-last test round; otherwise, generate a screendump and proceed
" with its verification.
def s:VerifyScreenDumpOrThrowFFFD(
	buf: number,
	which_page: string,
	in_name_and_out_name: string,
	aborted_count: number,
	max_aborted_count: number,
	basename: string,
	opts: dict<any>,
	page_quota: dict<number>,
	seen_pages: list<number>,
	page_nr: number): number
  if !has_key(page_quota, page_nr)
    # Constrain management of unseen pages to the product of "wait" times
    # "max_aborted_count" (see "opts" below).  When _test repetition_ and
    # _line rewriting_ FAIL page verification, the page gets to keep its
    # unseen mark; when _test repetition_ is FAILING for a later page, all
    # earlier unseen pages get another chance at _test repetition_ etc. before
    # further progress can be made for the later page.
    page_quota[page_nr] = max_aborted_count
  endif
  const with_fffd: bool = CanFindRenderedFFFDChars(
      buf,
      in_name_and_out_name,
      (max_aborted_count - aborted_count + 1))
  if with_fffd && aborted_count > 1
    throw 'FFFD'
  endif
  ch_log(which_page .. ' screendump for ' .. in_name_and_out_name)
  # Generate a screendump of every 19 lines of "buf", reusing the bottom line
  # (or the bottom six or so lines for "*_01.dump") from the previous dump as
  # the top line(s) in the next dump for continuity.  Constrain generation of
  # unseen pages for the last test round (via "wait").
  const status: number = g:VerifyScreenDump(
      buf,
      basename,
      (aborted_count != max_aborted_count)
	  ? extend({wait: max_aborted_count}, opts, 'keep')
	  : opts)
  if !with_fffd || (!status || !page_quota[page_nr])
    add(seen_pages, page_nr)
  else
    TraceLiveness('Q', (max_aborted_count - aborted_count + 1), string(page_quota))
  endif
  page_quota[page_nr] -= 1
  return status
enddef

" Poll for updates of the cursor position in the terminal buffer occupying the
" first window.  (ALWAYS call the function or its equivalent before calling
" "VerifyScreenDump()" *and* after calling any number of "term_sendkeys()".)
def s:TermPollRuler(
	CannotDumpPage: func,	# (TYPE FOR LEGACY CONTEXT CALL SITES.)
	buf: number,
	in_name_and_out_name: string): list<string>
  if CannotUseRealEstate(in_name_and_out_name)
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
  TraceLiveness('P', (2048 - times), in_name_and_out_name)
  return ruler
enddef

" Prevent "s:TermPollRuler()" from prematurely reading the cursor position,
" which is available at ":edit", after outracing the loading of syntax etc. in
" the terminal buffer.  (Call the function before calling "VerifyScreenDump()"
" for the first time.)
def s:TermWaitAndPollRuler(buf: number, in_name_and_out_name: string): list<string>
  if CannotUseRealEstate(in_name_and_out_name)
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
  TraceLiveness('W', (32768 - times), in_name_and_out_name)
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
  let XTESTSCRIPT =<< trim END
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

    def CollectFFFDChars()
      const fffd: string = "\xef\xbf\xbd"
      const flags: string = 'eW'
      const pos: list<number> = getpos('.')
      var fffds: list<list<number>> = []
      try
	cursor(1, 1)
	var prev: list<number> = [0, 0]
	var next: list<number> = [0, 0]
	next = searchpos(fffd, 'c' .. flags)
	while next[0] > 0 && prev != next
	  add(fffds, next)
	  prev = next
	  next = searchpos(fffd, flags)
	endwhile
      finally
	setpos('.', pos)
      endtry
      if !empty(fffds)
	# Use "actions/upload-artifact@v4" of ci.yml for delivery.
	writefile(
	  [printf('%s: %s', bufname('%'), string(fffds))],
	  'failed/10-FFFDS',
	  'a')
      endif
      redraw!
    enddef

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
  let MAX_ABORTED_COUNT = 5
  let MAX_FAILED_COUNT = 5
  let DUMP_OPTS = extend(
	  \ exists("$VIM_SYNTAX_TEST_WAIT_TIME") &&
	  \ !empty($VIM_SYNTAX_TEST_WAIT_TIME)
	      \ ? {'wait': max([1, str2nr($VIM_SYNTAX_TEST_WAIT_TIME)])}
	      \ : {},
      \ {'FileComparisonPreAction':
	  \ function('g:ScreenDumpDiscardFFFDChars'),
      \ 'NonEqualLineComparisonPostAction':
	  \ function('g:ScreenDumpLookForFFFDChars')})
  lockvar DUMP_OPTS MAX_FAILED_COUNT MAX_ABORTED_COUNT XTESTSCRIPT
  let ok_count = 0
  let disused_pages = []
  let failed_tests = []
  let skipped_count = 0
  let last_test_status = 'invalid'
  let filter = ''
  " Create a map of setup configuration filenames with their basenames as keys.
  let setup = glob('input/setup/*.vim', 1, 1)
    \ ->reduce({d, f -> extend(d, {fnamemodify(f, ':t:r'): f})}, {})
  " Turn a subset of filenames etc. requested for testing into a pattern.
  if filereadable('../testdir/Xfilter')
    let filter = readfile('../testdir/Xfilter')
	\ ->map({_, v -> '^' .. escape(substitute(v, '_$', '', ''), '.')})
	\ ->join('\|')
    call delete('../testdir/Xfilter')
  endif

  " Treat "^self-testing" as a string NOT as a regexp.
  if filter ==# '^self-testing'
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

    " Execute the test if the "done" file does not exist or when the input file
    " is newer.
    let in_time = getftime(fname)
    let out_time = getftime('done/' .. root)
    if out_time < 0 || in_time > out_time
      call ch_log('running tests for: ' .. fname)
      let filetype = substitute(root, '\([^_.]*\)[_.].*', '\1', '')
      let failed_root = 'failed/' .. root

      for pagename in glob(failed_root .. '_\d*\.dump', 1, 1)
	call delete(pagename)
      endfor
      call delete('done/' .. root)
      call writefile(XTESTSCRIPT, 'Xtestscript')

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
      let fail = 0

      try
	let aborted_count = MAX_ABORTED_COUNT
	let collected_count = 0
	let seen_pages = []
	let page_quota = {}

	" See #16559.  For each processed page, repeat pre-verification steps
	" from scratch (subject to page cacheing) whenever U+FFFD characters
	" are found in the terminal buffer with "term_getline()", i.e. treat
	" these pages as if they were distinct test files.  U+FFFD characters
	" found at the last attempt (see "MAX_ABORTED_COUNT") will be ignored
	" and "VerifyScreenDump()" will take over with own filtering.
	while aborted_count > 0
	  let buf = RunVimInTerminal(args, {})
	  try
	    " edit the file only after catching the SwapExists event
	    call term_sendkeys(buf, ":edit " .. fname .. "\<CR>")
	    " set up the testing environment
	    call term_sendkeys(buf, ":call SetUpVim()\<CR>")
	    " load filetype specific settings
	    call term_sendkeys(buf, ":call LoadFiletype('" .. filetype .. "')\<CR>")

	    " Collect all *non-spurious* U+FFFD characters for scrutiny.
	    if aborted_count == 1 && collected_count != 1
	      let collected_count = 1
	      call term_sendkeys(buf, ":call CollectFFFDChars()\<CR>")
	    endif

	    " Make a synchronisation point between a terminal buffer and
	    " another buffer by requesting to echo a known token in the former
	    " and asserting its availability with "s:TermWaitAndPollRuler()"
	    " from the latter.
	    if filetype == 'sh'
	      call term_sendkeys(buf, ":call ShellInfo()\<CR>")
	    else
	      call term_sendkeys(buf, ":echo 'is_nonce'\<CR>")
	    endif

	    let page_nr = 0
	    let root_00 = root .. '_00'
	    let in_name_and_out_name = fname .. ': failed/' .. root_00 .. '.dump'
	    " Queue up all "term_sendkeys()"es and let them finish before
	    " returning from "s:TermWaitAndPollRuler()".
	    let ruler = s:TermWaitAndPollRuler(buf, in_name_and_out_name)
	    if index(seen_pages, page_nr) < 0
	      let fail += s:VerifyScreenDumpOrThrowFFFD(
		  \ buf,
		  \ 'First',
		  \ in_name_and_out_name,
		  \ aborted_count,
		  \ MAX_ABORTED_COUNT,
		  \ root_00,
		  \ DUMP_OPTS,
		  \ page_quota,
		  \ seen_pages,
		  \ page_nr)
	      " Reset "aborted_count" for another page.
	      let aborted_count = MAX_ABORTED_COUNT
	    endif
	    let keys_a = ":call ScrollToSecondPage((18 * 75 + 1), 19, 5) | redraw!\<CR>"
	    let keys_b = ":call ScrollToNextPage((18 * 75 + 1), 19, 5) | redraw!\<CR>"

	    while s:CannotSeeLastLine(ruler)
	      call term_sendkeys(buf, keys_a)
	      let keys_a = keys_b
	      let page_nr += 1
	      let root_next = printf('%s_%02d', root, page_nr)
	      let in_name_and_out_name = fname .. ': failed/' .. root_next .. '.dump'
	      let ruler = s:TermPollRuler(
		  \ function('s:CannotDumpNextPage', [buf, ruler]),
		  \ buf,
		  \ in_name_and_out_name)
	      if index(seen_pages, page_nr) < 0
		let fail += s:VerifyScreenDumpOrThrowFFFD(
		    \ buf,
		    \ 'Next',
		    \ in_name_and_out_name,
		    \ aborted_count,
		    \ MAX_ABORTED_COUNT,
		    \ root_next,
		    \ DUMP_OPTS,
		    \ page_quota,
		    \ seen_pages,
		    \ page_nr)
		" Reset "aborted_count" for another page.
		let aborted_count = MAX_ABORTED_COUNT
	      endif
	    endwhile
	    call StopVimInTerminal(buf)
	    break
	  catch /^FFFD$/
	    " Clear out.
	    call StopVimInTerminal(buf)
	    while winnr('$') > 1
	      close
	    endwhile
	    let aborted_count -= 1
	  endtry
	endwhile
      finally
	call delete('Xtestscript')
      endtry

      let page_nr += 1
      let pagename = printf('dumps/%s_%02d.dump', root, page_nr)

      while filereadable(pagename)
	call add(disused_pages, pagename)
	let page_nr += 1
	let pagename = printf('dumps/%s_%02d.dump', root, page_nr)
      endwhile

      " redraw here to avoid the following messages to get mixed up with screen
      " output.
      redraw

      " Add any assert errors to s:messages.
      if len(v:errors) > 0
	call extend(s:messages, v:errors)
	if last_test_status == 'passed'
	  call EraseLineAndReturnCarriage('Test ' .. root .. ' OK')
	else
	  echon "\n"
	endif
	" Echo the errors here, in case the script aborts or the "messages" file
	" is not displayed later.
	echomsg v:errors
	let v:errors = []
	let fail += 1
      endif

      if fail == 0
	if last_test_status == 'skipped'
	  echon "\n"
	endif
	let last_test_status = 'passed'
	let msg = "Test " .. root .. " OK"
	call Message(msg)
	call EraseLineAndReturnCarriage(msg)

	call writefile(['OK'], 'done/' .. root)

	let ok_count += 1
      else
	let last_test_status = 'failed'
	call Message("Test " .. root .. " FAILED")
	echon "\n"

	call delete('done/' .. root)

	eval failed_tests->add(root)
	if len(failed_tests) > MAX_FAILED_COUNT
	  call Message('')
	  call Message('Too many errors, aborting')
	endif
      endif
    else
      if last_test_status == 'passed'
	call EraseLineAndReturnCarriage('Test ' .. root .. ' OK')
      endif
      let last_test_status = 'skipped'
      call Message("Test " .. root .. " skipped")
      let skipped_count += 1
    endif

    " Append messages to the file "testdir/messages"
    call AppendMessages('Input file ' .. fname .. ':')

    if len(failed_tests) > MAX_FAILED_COUNT
      break
    endif
  endfor

  if last_test_status == 'passed' && exists('root')
    call EraseLineAndReturnCarriage('Test ' .. root .. ' OK')
  endif

  call Message(s:test_run_message)
  call Message('OK: ' .. ok_count)
  call Message('FAILED: ' .. len(failed_tests) .. ': ' .. string(failed_tests))
  call Message('skipped: ' .. skipped_count)

  for pagename in disused_pages
    call Message(printf('No input page found for "%s"', pagename))
  endfor

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
