" Tests for the terminal window.
" This is split in two, because it can take a lot of time.
" See test_terminal2.vim for further tests.

source check.vim
CheckFeature terminal

source shared.vim
source screendump.vim
source mouse.vim
source term_util.vim

let s:python = PythonProg()
let $PROMPT_COMMAND=''

func Api_drop_common(options)
  call assert_equal(1, winnr('$'))

  " Use the title termcap entries to output the escape sequence.
  call writefile([
	\ 'set title',
	\ 'exe "set t_ts=\<Esc>]51; t_fs=\x07"',
	\ 'let &titlestring = ''["drop","Xtextfile"' . a:options . ']''',
	\ 'redraw',
	\ "set t_ts=",
	\ ], 'Xscript')
  let buf = RunVimInTerminal('-S Xscript', {})
  call WaitFor({-> bufnr('Xtextfile') > 0})
  call assert_equal('Xtextfile', expand('%:t'))
  call assert_true(winnr('$') >= 3)
  return buf
endfunc

func Test_terminal_api_drop_newwin()
  CheckRunVimInTerminal
  let buf = Api_drop_common('')
  call assert_equal(0, &bin)
  call assert_equal('', &fenc)

  call StopVimInTerminal(buf)
  call delete('Xscript')
  bwipe Xtextfile
endfunc

func Test_terminal_api_drop_newwin_bin()
  CheckRunVimInTerminal
  let buf = Api_drop_common(',{"bin":1}')
  call assert_equal(1, &bin)

  call StopVimInTerminal(buf)
  call delete('Xscript')
  bwipe Xtextfile
endfunc

func Test_terminal_api_drop_newwin_binary()
  CheckRunVimInTerminal
  let buf = Api_drop_common(',{"binary":1}')
  call assert_equal(1, &bin)

  call StopVimInTerminal(buf)
  call delete('Xscript')
  bwipe Xtextfile
endfunc

func Test_terminal_api_drop_newwin_nobin()
  CheckRunVimInTerminal
  set binary
  let buf = Api_drop_common(',{"nobin":1}')
  call assert_equal(0, &bin)

  call StopVimInTerminal(buf)
  call delete('Xscript')
  bwipe Xtextfile
  set nobinary
endfunc

func Test_terminal_api_drop_newwin_nobinary()
  CheckRunVimInTerminal
  set binary
  let buf = Api_drop_common(',{"nobinary":1}')
  call assert_equal(0, &bin)

  call StopVimInTerminal(buf)
  call delete('Xscript')
  bwipe Xtextfile
  set nobinary
endfunc

func Test_terminal_api_drop_newwin_ff()
  CheckRunVimInTerminal
  let buf = Api_drop_common(',{"ff":"dos"}')
  call assert_equal("dos", &ff)

  call StopVimInTerminal(buf)
  call delete('Xscript')
  bwipe Xtextfile
endfunc

func Test_terminal_api_drop_newwin_fileformat()
  CheckRunVimInTerminal
  let buf = Api_drop_common(',{"fileformat":"dos"}')
  call assert_equal("dos", &ff)

  call StopVimInTerminal(buf)
  call delete('Xscript')
  bwipe Xtextfile
endfunc

func Test_terminal_api_drop_newwin_enc()
  CheckRunVimInTerminal
  let buf = Api_drop_common(',{"enc":"utf-16"}')
  call assert_equal("utf-16", &fenc)

  call StopVimInTerminal(buf)
  call delete('Xscript')
  bwipe Xtextfile
endfunc

func Test_terminal_api_drop_newwin_encoding()
  CheckRunVimInTerminal
  let buf = Api_drop_common(',{"encoding":"utf-16"}')
  call assert_equal("utf-16", &fenc)

  call StopVimInTerminal(buf)
  call delete('Xscript')
  bwipe Xtextfile
endfunc

func Test_terminal_api_drop_oldwin()
  CheckRunVimInTerminal
  let firstwinid = win_getid()
  split Xtextfile
  let textfile_winid = win_getid()
  call assert_equal(2, winnr('$'))
  call win_gotoid(firstwinid)

  " Use the title termcap entries to output the escape sequence.
  call writefile([
	\ 'set title',
	\ 'exe "set t_ts=\<Esc>]51; t_fs=\x07"',
	\ 'let &titlestring = ''["drop","Xtextfile"]''',
	\ 'redraw',
	\ "set t_ts=",
	\ ], 'Xscript')
  let buf = RunVimInTerminal('-S Xscript', {'rows': 10})
  call WaitForAssert({-> assert_equal('Xtextfile', expand('%:t'))})
  call assert_equal(textfile_winid, win_getid())

  call StopVimInTerminal(buf)
  call delete('Xscript')
  bwipe Xtextfile
endfunc

func Tapi_TryThis(bufnum, arg)
  let g:called_bufnum = a:bufnum
  let g:called_arg = a:arg
endfunc

func WriteApiCall(funcname)
  " Use the title termcap entries to output the escape sequence.
  call writefile([
	\ 'set title',
	\ 'exe "set t_ts=\<Esc>]51; t_fs=\x07"',
	\ 'let &titlestring = ''["call","' . a:funcname . '",["hello",123]]''',
	\ 'redraw',
	\ "set t_ts=",
	\ ], 'Xscript')
endfunc

func Test_terminal_api_call()
  CheckRunVimInTerminal

  unlet! g:called_bufnum
  unlet! g:called_arg

  call WriteApiCall('Tapi_TryThis')

  " Default
  let buf = RunVimInTerminal('-S Xscript', {})
  call WaitFor({-> exists('g:called_bufnum')})
  call assert_equal(buf, g:called_bufnum)
  call assert_equal(['hello', 123], g:called_arg)
  call StopVimInTerminal(buf)

  unlet! g:called_bufnum
  unlet! g:called_arg

  " Enable explicitly
  let buf = RunVimInTerminal('-S Xscript', {'term_api': 'Tapi_Try'})
  call WaitFor({-> exists('g:called_bufnum')})
  call assert_equal(buf, g:called_bufnum)
  call assert_equal(['hello', 123], g:called_arg)
  call StopVimInTerminal(buf)

  unlet! g:called_bufnum
  unlet! g:called_arg

  func! ApiCall_TryThis(bufnum, arg)
    let g:called_bufnum2 = a:bufnum
    let g:called_arg2 = a:arg
  endfunc

  call WriteApiCall('ApiCall_TryThis')

  " Use prefix match
  let buf = RunVimInTerminal('-S Xscript', {'term_api': 'ApiCall_'})
  call WaitFor({-> exists('g:called_bufnum2')})
  call assert_equal(buf, g:called_bufnum2)
  call assert_equal(['hello', 123], g:called_arg2)
  call StopVimInTerminal(buf)

  call assert_fails("call term_start('ls', {'term_api' : []})", 'E475:')

  unlet! g:called_bufnum2
  unlet! g:called_arg2

  call delete('Xscript')
  delfunction! ApiCall_TryThis
  unlet! g:called_bufnum2
  unlet! g:called_arg2
endfunc

func Test_terminal_api_call_fails()
  CheckRunVimInTerminal

  func! TryThis(bufnum, arg)
    let g:called_bufnum3 = a:bufnum
    let g:called_arg3 = a:arg
  endfunc

  call WriteApiCall('TryThis')

  unlet! g:called_bufnum3
  unlet! g:called_arg3

  " Not permitted
  call ch_logfile('Xlog', 'w')
  let buf = RunVimInTerminal('-S Xscript', {'term_api': ''})
  call WaitForAssert({-> assert_match('Unpermitted function: TryThis', string(readfile('Xlog')))})
  call assert_false(exists('g:called_bufnum3'))
  call assert_false(exists('g:called_arg3'))
  call StopVimInTerminal(buf)

  " No match
  call ch_logfile('Xlog', 'w')
  let buf = RunVimInTerminal('-S Xscript', {'term_api': 'TryThat'})
  call WaitFor({-> string(readfile('Xlog')) =~ 'Unpermitted function: TryThis'})
  call assert_false(exists('g:called_bufnum3'))
  call assert_false(exists('g:called_arg3'))
  call StopVimInTerminal(buf)

  call delete('Xscript')
  call ch_logfile('')
  call delete('Xlog')
  delfunction! TryThis
  unlet! g:called_bufnum3
  unlet! g:called_arg3
endfunc

let s:caught_e937 = 0

func Tapi_Delete(bufnum, arg)
  try
    execute 'bdelete!' a:bufnum
  catch /E937:/
    let s:caught_e937 = 1
  endtry
endfunc

func Test_terminal_api_call_fail_delete()
  CheckRunVimInTerminal

  call WriteApiCall('Tapi_Delete')
  let buf = RunVimInTerminal('-S Xscript', {})
  call WaitForAssert({-> assert_equal(1, s:caught_e937)})

  call StopVimInTerminal(buf)
  call delete('Xscript')
  call ch_logfile('', '')
endfunc

func Test_terminal_ansicolors_default()
  CheckFunction term_getansicolors

  let colors = [
	\ '#000000', '#e00000',
	\ '#00e000', '#e0e000',
	\ '#0000e0', '#e000e0',
	\ '#00e0e0', '#e0e0e0',
	\ '#808080', '#ff4040',
	\ '#40ff40', '#ffff40',
	\ '#4040ff', '#ff40ff',
	\ '#40ffff', '#ffffff',
	\]

  let buf = Run_shell_in_terminal({})
  call assert_equal(colors, term_getansicolors(buf))
  call StopShellInTerminal(buf)
  call TermWait(buf)
  call assert_equal([], term_getansicolors(buf))

  exe buf . 'bwipe'
endfunc

let s:test_colors = [
	\ '#616e64', '#0d0a79',
	\ '#6d610d', '#0a7373',
	\ '#690d0a', '#6d696e',
	\ '#0d0a6f', '#616e0d',
	\ '#0a6479', '#6d0d0a',
	\ '#617373', '#0d0a69',
	\ '#6d690d', '#0a6e6f',
	\ '#610d0a', '#6e6479',
	\]

func Test_terminal_ansicolors_global()
  CheckFeature termguicolors
  CheckFunction term_getansicolors

  let g:terminal_ansi_colors = reverse(copy(s:test_colors))
  let buf = Run_shell_in_terminal({})
  call assert_equal(g:terminal_ansi_colors, term_getansicolors(buf))
  call StopShellInTerminal(buf)
  call TermWait(buf)

  exe buf . 'bwipe'
  unlet g:terminal_ansi_colors
endfunc

func Test_terminal_ansicolors_func()
  CheckFeature termguicolors
  CheckFunction term_getansicolors

  let g:terminal_ansi_colors = reverse(copy(s:test_colors))
  let buf = Run_shell_in_terminal({'ansi_colors': s:test_colors})
  call assert_equal(s:test_colors, term_getansicolors(buf))

  call term_setansicolors(buf, g:terminal_ansi_colors)
  call assert_equal(g:terminal_ansi_colors, buf->term_getansicolors())

  let colors = [
	\ 'ivory', 'AliceBlue',
	\ 'grey67', 'dark goldenrod',
	\ 'SteelBlue3', 'PaleVioletRed4',
	\ 'MediumPurple2', 'yellow2',
	\ 'RosyBrown3', 'OrangeRed2',
	\ 'white smoke', 'navy blue',
	\ 'grey47', 'gray97',
	\ 'MistyRose2', 'DodgerBlue4',
	\]
  eval buf->term_setansicolors(colors)

  let colors[4] = 'Invalid'
  call assert_fails('call term_setansicolors(buf, colors)', 'E474:')
  call assert_fails('call term_setansicolors(buf, {})', 'E714:')

  call StopShellInTerminal(buf)
  call TermWait(buf)
  call assert_equal(0, term_setansicolors(buf, []))
  exe buf . 'bwipe'
endfunc

func Test_terminal_all_ansi_colors()
  CheckRunVimInTerminal

  " Use all the ANSI colors.
  call writefile([
	\ 'call setline(1, "AABBCCDDEEFFGGHHIIJJKKLLMMNNOOPP XXYYZZ")',
	\ 'hi Tblack ctermfg=0 ctermbg=8',
	\ 'hi Tdarkred ctermfg=1 ctermbg=9',
	\ 'hi Tdarkgreen ctermfg=2 ctermbg=10',
	\ 'hi Tbrown ctermfg=3 ctermbg=11',
	\ 'hi Tdarkblue ctermfg=4 ctermbg=12',
	\ 'hi Tdarkmagenta ctermfg=5 ctermbg=13',
	\ 'hi Tdarkcyan ctermfg=6 ctermbg=14',
	\ 'hi Tlightgrey ctermfg=7 ctermbg=15',
	\ 'hi Tdarkgrey ctermfg=8 ctermbg=0',
	\ 'hi Tred ctermfg=9 ctermbg=1',
	\ 'hi Tgreen ctermfg=10 ctermbg=2',
	\ 'hi Tyellow ctermfg=11 ctermbg=3',
	\ 'hi Tblue ctermfg=12 ctermbg=4',
	\ 'hi Tmagenta ctermfg=13 ctermbg=5',
	\ 'hi Tcyan ctermfg=14 ctermbg=6',
	\ 'hi Twhite ctermfg=15 ctermbg=7',
	\ 'hi TdarkredBold ctermfg=1 cterm=bold',
	\ 'hi TgreenBold ctermfg=10 cterm=bold',
	\ 'hi TmagentaBold ctermfg=13 cterm=bold ctermbg=5',
	\ '',
	\ 'call  matchadd("Tblack", "A")',
	\ 'call  matchadd("Tdarkred", "B")',
	\ 'call  matchadd("Tdarkgreen", "C")',
	\ 'call  matchadd("Tbrown", "D")',
	\ 'call  matchadd("Tdarkblue", "E")',
	\ 'call  matchadd("Tdarkmagenta", "F")',
	\ 'call  matchadd("Tdarkcyan", "G")',
	\ 'call  matchadd("Tlightgrey", "H")',
	\ 'call  matchadd("Tdarkgrey", "I")',
	\ 'call  matchadd("Tred", "J")',
	\ 'call  matchadd("Tgreen", "K")',
	\ 'call  matchadd("Tyellow", "L")',
	\ 'call  matchadd("Tblue", "M")',
	\ 'call  matchadd("Tmagenta", "N")',
	\ 'call  matchadd("Tcyan", "O")',
	\ 'call  matchadd("Twhite", "P")',
	\ 'call  matchadd("TdarkredBold", "X")',
	\ 'call  matchadd("TgreenBold", "Y")',
	\ 'call  matchadd("TmagentaBold", "Z")',
	\ 'redraw',
	\ ], 'Xcolorscript')
  let buf = RunVimInTerminal('-S Xcolorscript', {'rows': 10})
  call VerifyScreenDump(buf, 'Test_terminal_all_ansi_colors', {})

  call term_sendkeys(buf, ":q\<CR>")
  call StopVimInTerminal(buf)
  call delete('Xcolorscript')
endfunc

func Test_terminal_termwinsize_option_fixed()
  CheckRunVimInTerminal
  set termwinsize=6x40
  let text = []
  for n in range(10)
    call add(text, repeat(n, 50))
  endfor
  call writefile(text, 'Xwinsize')
  let buf = RunVimInTerminal('Xwinsize', {})
  let win = bufwinid(buf)
  call assert_equal([6, 40], term_getsize(buf))
  call assert_equal(6, winheight(win))
  call assert_equal(40, winwidth(win))

  " resizing the window doesn't resize the terminal.
  resize 10
  vertical resize 60
  call assert_equal([6, 40], term_getsize(buf))
  call assert_equal(10, winheight(win))
  call assert_equal(60, winwidth(win))

  call StopVimInTerminal(buf)
  call delete('Xwinsize')

  call assert_fails('set termwinsize=40', 'E474')
  call assert_fails('set termwinsize=10+40', 'E474')
  call assert_fails('set termwinsize=abc', 'E474')

  set termwinsize=
endfunc

func Test_terminal_termwinsize_option_zero()
  set termwinsize=0x0
  let buf = Run_shell_in_terminal({})
  let win = bufwinid(buf)
  call assert_equal([winheight(win), winwidth(win)], term_getsize(buf))
  call StopShellInTerminal(buf)
  call TermWait(buf)
  exe buf . 'bwipe'

  set termwinsize=7x0
  let buf = Run_shell_in_terminal({})
  let win = bufwinid(buf)
  call assert_equal([7, winwidth(win)], term_getsize(buf))
  call StopShellInTerminal(buf)
  call TermWait(buf)
  exe buf . 'bwipe'

  set termwinsize=0x33
  let buf = Run_shell_in_terminal({})
  let win = bufwinid(buf)
  call assert_equal([winheight(win), 33], term_getsize(buf))
  call StopShellInTerminal(buf)
  call TermWait(buf)
  exe buf . 'bwipe'

  set termwinsize=
endfunc

func Test_terminal_termwinsize_minimum()
  set termwinsize=10*50
  vsplit
  let buf = Run_shell_in_terminal({})
  let win = bufwinid(buf)
  call assert_inrange(10, 1000, winheight(win))
  call assert_inrange(50, 1000, winwidth(win))
  call assert_equal([winheight(win), winwidth(win)], term_getsize(buf))

  resize 15
  vertical resize 60
  redraw
  call assert_equal([15, 60], term_getsize(buf))
  call assert_equal(15, winheight(win))
  call assert_equal(60, winwidth(win))

  resize 7
  vertical resize 30
  redraw
  call assert_equal([10, 50], term_getsize(buf))
  call assert_equal(7, winheight(win))
  call assert_equal(30, winwidth(win))

  call StopShellInTerminal(buf)
  call TermWait(buf)
  exe buf . 'bwipe'

  set termwinsize=0*0
  let buf = Run_shell_in_terminal({})
  let win = bufwinid(buf)
  call assert_equal([winheight(win), winwidth(win)], term_getsize(buf))
  call StopShellInTerminal(buf)
  call TermWait(buf)
  exe buf . 'bwipe'

  set termwinsize=
endfunc

func Test_terminal_termwinkey()
  " make three tabpages, terminal in the middle
  0tabnew
  tabnext
  tabnew
  tabprev
  call assert_equal(1, winnr('$'))
  call assert_equal(2, tabpagenr())
  let thiswin = win_getid()

  let buf = Run_shell_in_terminal({})
  let termwin = bufwinid(buf)
  set termwinkey=<C-L>
  call feedkeys("\<C-L>w", 'tx')
  call assert_equal(thiswin, win_getid())
  call feedkeys("\<C-W>w", 'tx')
  call assert_equal(termwin, win_getid())

  if has('langmap')
    set langmap=xjyk
    call feedkeys("\<C-L>x", 'tx')
    call assert_equal(thiswin, win_getid())
    call feedkeys("\<C-W>y", 'tx')
    call assert_equal(termwin, win_getid())
    set langmap=
  endif

  call feedkeys("\<C-L>gt", "xt")
  call assert_equal(3, tabpagenr())
  tabprev
  call assert_equal(2, tabpagenr())
  call assert_equal(termwin, win_getid())

  call feedkeys("\<C-L>gT", "xt")
  call assert_equal(1, tabpagenr())
  tabnext
  call assert_equal(2, tabpagenr())
  call assert_equal(termwin, win_getid())

  let job = term_getjob(buf)
  call feedkeys("\<C-L>\<C-C>", 'tx')
  call WaitForAssert({-> assert_equal("dead", job_status(job))})

  set termwinkey&
  tabnext
  tabclose
  tabprev
  tabclose
endfunc

func Test_terminal_out_err()
  CheckUnix

  call writefile([
	\ '#!/bin/sh',
	\ 'echo "this is standard error" >&2',
	\ 'echo "this is standard out" >&1',
	\ ], 'Xechoerrout.sh')
  call setfperm('Xechoerrout.sh', 'rwxrwx---')

  let outfile = 'Xtermstdout'
  let buf = term_start(['./Xechoerrout.sh'], {'out_io': 'file', 'out_name': outfile})

  call WaitFor({-> !empty(readfile(outfile)) && !empty(term_getline(buf, 1))})
  call assert_equal(['this is standard out'], readfile(outfile))
  call assert_equal('this is standard error', term_getline(buf, 1))

  call WaitForAssert({-> assert_equal('dead', job_status(term_getjob(buf)))})
  exe buf . 'bwipe'
  call delete('Xechoerrout.sh')
  call delete(outfile)
endfunc

func Test_termwinscroll()
  CheckUnix

  " Let the terminal output more than 'termwinscroll' lines, some at the start
  " will be dropped.
  exe 'set termwinscroll=' . &lines
  let buf = term_start('/bin/sh')
  for i in range(1, &lines)
    call feedkeys("echo " . i . "\<CR>", 'xt')
    call WaitForAssert({-> assert_match(string(i), term_getline(buf, term_getcursor(buf)[0] - 1))})
  endfor
  " Go to Terminal-Normal mode to update the buffer.
  call feedkeys("\<C-W>N", 'xt')
  call assert_inrange(&lines, &lines * 110 / 100 + winheight(0), line('$'))

  " Every "echo nr" must only appear once
  let lines = getline(1, line('$'))
  for i in range(&lines - len(lines) / 2 + 2, &lines)
    let filtered = filter(copy(lines), {idx, val -> val =~ 'echo ' . i . '\>'})
    call assert_equal(1, len(filtered), 'for "echo ' . i . '"')
  endfor

  exe buf . 'bwipe!'
endfunc

" Resizing the terminal window caused an ml_get error.
" TODO: This does not reproduce the original problem.
func Test_terminal_resize()
  set statusline=x
  terminal
  call assert_equal(2, winnr('$'))

  " Fill the terminal with text.
  if has('win32')
    call feedkeys("dir\<CR>", 'xt')
  else
    call feedkeys("ls\<CR>", 'xt')
  endif
  " Go to Terminal-Normal mode for a moment.
  call feedkeys("\<C-W>N", 'xt')
  " Open a new window
  call feedkeys("i\<C-W>n", 'xt')
  call assert_equal(3, winnr('$'))
  redraw

  close
  call assert_equal(2, winnr('$'))
  call feedkeys("exit\<CR>", 'xt')
  set statusline&
endfunc

" must be nearly the last, we can't go back from GUI to terminal
func Test_zz1_terminal_in_gui()
  CheckCanRunGui

  " Ignore the "failed to create input context" error.
  call test_ignore_error('E285:')

  gui -f

  call assert_equal(1, winnr('$'))
  let buf = Run_shell_in_terminal({'term_finish': 'close'})
  call StopShellInTerminal(buf)
  call TermWait(buf)

  " closing window wipes out the terminal buffer a with finished job
  call WaitForAssert({-> assert_equal(1, winnr('$'))})
  call assert_equal("", bufname(buf))

  unlet g:job
endfunc

func Test_zz2_terminal_guioptions_bang()
  CheckGui
  set guioptions+=!

  let filename = 'Xtestscript'
  if has('win32')
    let filename .= '.bat'
    let prefix = ''
    let contents = ['@echo off', 'exit %1']
  else
    let filename .= '.sh'
    let prefix = './'
    let contents = ['#!/bin/sh', 'exit $1']
  endif
  call writefile(contents, filename)
  call setfperm(filename, 'rwxrwx---')

  " Check if v:shell_error is equal to the exit status.
  let exitval = 0
  execute printf(':!%s%s %d', prefix, filename, exitval)
  call assert_equal(exitval, v:shell_error)

  let exitval = 9
  execute printf(':!%s%s %d', prefix, filename, exitval)
  call assert_equal(exitval, v:shell_error)

  set guioptions&
  call delete(filename)
endfunc

func Test_terminal_hidden()
  CheckUnix

  term ++hidden cat
  let bnr = bufnr('$')
  call assert_equal('terminal', getbufvar(bnr, '&buftype'))
  exe 'sbuf ' . bnr
  call assert_equal('terminal', &buftype)
  call term_sendkeys(bnr, "asdf\<CR>")
  call WaitForAssert({-> assert_match('asdf', term_getline(bnr, 2))})
  call term_sendkeys(bnr, "\<C-D>")
  call WaitForAssert({-> assert_equal('finished', bnr->term_getstatus())})
  bwipe!
endfunc

func Test_terminal_switch_mode()
  term
  let bnr = bufnr('$')
  call WaitForAssert({-> assert_equal('running', term_getstatus(bnr))})
  call feedkeys("\<C-W>N", 'xt')
  call WaitForAssert({-> assert_equal('running,normal', term_getstatus(bnr))})
  call feedkeys("A", 'xt')
  call WaitForAssert({-> assert_equal('running', term_getstatus(bnr))})
  call feedkeys("\<C-\>\<C-N>", 'xt')
  call WaitForAssert({-> assert_equal('running,normal', term_getstatus(bnr))})
  call feedkeys("I", 'xt')
  call WaitForAssert({-> assert_equal('running', term_getstatus(bnr))})
  call feedkeys("\<C-W>Nv", 'xt')
  call WaitForAssert({-> assert_equal('running,normal', term_getstatus(bnr))})
  call feedkeys("I", 'xt')
  call WaitForAssert({-> assert_equal('running', term_getstatus(bnr))})
  call feedkeys("\<C-W>Nv", 'xt')
  call WaitForAssert({-> assert_equal('running,normal', term_getstatus(bnr))})
  call feedkeys("A", 'xt')
  call WaitForAssert({-> assert_equal('running', term_getstatus(bnr))})
  bwipe!
endfunc

func Test_terminal_normal_mode()
  CheckRunVimInTerminal

  " Run Vim in a terminal and open a terminal window to run Vim in.
  let lines =<< trim END
    call setline(1, range(11111, 11122))
    3
  END
  call writefile(lines, 'XtermNormal')
  let buf = RunVimInTerminal('-S XtermNormal', {'rows': 8})
  call TermWait(buf)

  call term_sendkeys(buf, "\<C-W>N")
  call term_sendkeys(buf, ":set number cursorline culopt=both\r")
  call VerifyScreenDump(buf, 'Test_terminal_normal_1', {})

  call term_sendkeys(buf, ":set culopt=number\r")
  call VerifyScreenDump(buf, 'Test_terminal_normal_2', {})

  call term_sendkeys(buf, ":set culopt=line\r")
  call VerifyScreenDump(buf, 'Test_terminal_normal_3', {})

  call assert_fails('call term_sendkeys(buf, [])', 'E730:')
  call term_sendkeys(buf, "a:q!\<CR>:q\<CR>:q\<CR>")
  call StopVimInTerminal(buf)
  call delete('XtermNormal')
endfunc

func Test_terminal_hidden_and_close()
  CheckUnix

  call assert_equal(1, winnr('$'))
  term ++hidden ++close ls
  let bnr = bufnr('$')
  call assert_equal('terminal', getbufvar(bnr, '&buftype'))
  call WaitForAssert({-> assert_false(bufexists(bnr))})
  call assert_equal(1, winnr('$'))
endfunc

func Test_terminal_does_not_truncate_last_newlines()
  " This test does not pass through ConPTY.
  if has('conpty')
    return
  endif
  let contents = [
  \   [ 'One', '', 'X' ],
  \   [ 'Two', '', '' ],
  \   [ 'Three' ] + repeat([''], 30)
  \ ]

  for c in contents
    call writefile(c, 'Xfile')
    if has('win32')
      term cmd /c type Xfile
    else
      term cat Xfile
    endif
    let bnr = bufnr('$')
    call assert_equal('terminal', getbufvar(bnr, '&buftype'))
    call WaitForAssert({-> assert_equal('finished', term_getstatus(bnr))})
    sleep 100m
    call assert_equal(c, getline(1, line('$')))
    quit
  endfor

  call delete('Xfile')
endfunc

func Test_terminal_no_job()
  if has('win32')
    let cmd = 'cmd /c ""'
  else
    CheckExecutable false
    let cmd = 'false'
  endif
  let term = term_start(cmd, {'term_finish': 'close'})
  call WaitForAssert({-> assert_equal(v:null, term_getjob(term)) })
endfunc

func Test_term_getcursor()
  CheckUnix

  let buf = Run_shell_in_terminal({})

  " Wait for the shell to display a prompt.
  call WaitForAssert({-> assert_notequal('', term_getline(buf, 1))})

  " Hide the cursor.
  call term_sendkeys(buf, "echo -e '\\033[?25l'\r")
  call WaitForAssert({-> assert_equal(0, term_getcursor(buf)[2].visible)})

  " Show the cursor.
  call term_sendkeys(buf, "echo -e '\\033[?25h'\r")
  call WaitForAssert({-> assert_equal(1, buf->term_getcursor()[2].visible)})

  " Change color of cursor.
  call WaitForAssert({-> assert_equal('', term_getcursor(buf)[2].color)})
  call term_sendkeys(buf, "echo -e '\\033]12;blue\\007'\r")
  call WaitForAssert({-> assert_equal('blue', term_getcursor(buf)[2].color)})
  call term_sendkeys(buf, "echo -e '\\033]12;green\\007'\r")
  call WaitForAssert({-> assert_equal('green', term_getcursor(buf)[2].color)})

  " Make cursor a blinking block.
  call term_sendkeys(buf, "echo -e '\\033[1 q'\r")
  call WaitForAssert({-> assert_equal([1, 1],
  \ [term_getcursor(buf)[2].blink, term_getcursor(buf)[2].shape])})

  " Make cursor a steady block.
  call term_sendkeys(buf, "echo -e '\\033[2 q'\r")
  call WaitForAssert({-> assert_equal([0, 1],
  \ [term_getcursor(buf)[2].blink, term_getcursor(buf)[2].shape])})

  " Make cursor a blinking underline.
  call term_sendkeys(buf, "echo -e '\\033[3 q'\r")
  call WaitForAssert({-> assert_equal([1, 2],
  \ [term_getcursor(buf)[2].blink, term_getcursor(buf)[2].shape])})

  " Make cursor a steady underline.
  call term_sendkeys(buf, "echo -e '\\033[4 q'\r")
  call WaitForAssert({-> assert_equal([0, 2],
  \ [term_getcursor(buf)[2].blink, term_getcursor(buf)[2].shape])})

  " Make cursor a blinking vertical bar.
  call term_sendkeys(buf, "echo -e '\\033[5 q'\r")
  call WaitForAssert({-> assert_equal([1, 3],
  \ [term_getcursor(buf)[2].blink, term_getcursor(buf)[2].shape])})

  " Make cursor a steady vertical bar.
  call term_sendkeys(buf, "echo -e '\\033[6 q'\r")
  call WaitForAssert({-> assert_equal([0, 3],
  \ [term_getcursor(buf)[2].blink, term_getcursor(buf)[2].shape])})

  call StopShellInTerminal(buf)
endfunc

" Test for term_gettitle()
func Test_term_gettitle()
  " term_gettitle() returns an empty string for a non-terminal buffer
  " and for a non-existing buffer.
  call assert_equal('', bufnr('%')->term_gettitle())
  call assert_equal('', term_gettitle(bufnr('$') + 1))

  if !has('title') || empty(&t_ts)
    throw "Skipped: can't get/set title"
  endif

  let term = term_start([GetVimProg(), '--clean', '-c', 'set noswapfile', '-c', 'set title'])
  if has('autoservername')
    call WaitForAssert({-> assert_match('^\[No Name\] - VIM\d\+$', term_gettitle(term)) })
    call term_sendkeys(term, ":e Xfoo\r")
    call WaitForAssert({-> assert_match('^Xfoo (.*[/\\]testdir) - VIM\d\+$', term_gettitle(term)) })
  else
    call WaitForAssert({-> assert_equal('[No Name] - VIM', term_gettitle(term)) })
    call term_sendkeys(term, ":e Xfoo\r")
    call WaitForAssert({-> assert_match('^Xfoo (.*[/\\]testdir) - VIM$', term_gettitle(term)) })
  endif

  call term_sendkeys(term, ":set titlestring=foo\r")
  call WaitForAssert({-> assert_equal('foo', term_gettitle(term)) })

  exe term . 'bwipe!'
endfunc

func Test_term_gettty()
  let buf = Run_shell_in_terminal({})
  let gettty = term_gettty(buf)

  if has('unix') && executable('tty')
    " Find tty using the tty shell command.
    call WaitForAssert({-> assert_notequal('', term_getline(buf, 1))})
    call term_sendkeys(buf, "tty\r")
    call WaitForAssert({-> assert_notequal('', term_getline(buf, 3))})
    let tty = term_getline(buf, 2)
    call assert_equal(tty, gettty)
  endif

  let gettty0 = term_gettty(buf, 0)
  let gettty1 = term_gettty(buf, 1)

  call assert_equal(gettty, gettty0)
  call assert_equal(job_info(g:job).tty_out, gettty0)
  call assert_equal(job_info(g:job).tty_in,  gettty1)

  if has('unix')
    " For unix, term_gettty(..., 0) and term_gettty(..., 1)
    " are identical according to :help term_gettty()
    call assert_equal(gettty0, gettty1)
    call assert_match('^/dev/', gettty)
  else
    " ConPTY works on anonymous pipe.
    if !has('conpty')
      call assert_match('^\\\\.\\pipe\\', gettty0)
      call assert_match('^\\\\.\\pipe\\', gettty1)
    endif
  endif

  call assert_fails('call term_gettty(buf, 2)', 'E475:')
  call assert_fails('call term_gettty(buf, -1)', 'E475:')

  call assert_equal('', term_gettty(buf + 1))

  call StopShellInTerminal(buf)
  call TermWait(buf)
  exe buf . 'bwipe'
endfunc

func Test_terminal_getwinpos()
  CheckRunVimInTerminal

  " split, go to the bottom-right window
  split
  wincmd j
  set splitright

  call writefile([
	\ 'echo getwinpos()',
	\ ], 'XTest_getwinpos')
  let buf = RunVimInTerminal('-S XTest_getwinpos', {'cols': 60})
  call TermWait(buf)

  " Find the output of getwinpos() in the bottom line.
  let rows = term_getsize(buf)[0]
  call WaitForAssert({-> assert_match('\[\d\+, \d\+\]', term_getline(buf, rows))})
  let line = term_getline(buf, rows)
  let xpos = str2nr(substitute(line, '\[\(\d\+\), \d\+\]', '\1', ''))
  let ypos = str2nr(substitute(line, '\[\d\+, \(\d\+\)\]', '\1', ''))

  " Position must be bigger than the getwinpos() result of Vim itself.
  " The calculation in the console assumes a 10 x 7 character cell.
  " In the GUI it can be more, let's assume a 20 x 14 cell.
  " And then add 100 / 200 tolerance.
  let [xroot, yroot] = getwinpos()
  let winpos = 50->getwinpos()
  call assert_equal(xroot, winpos[0])
  call assert_equal(yroot, winpos[1])
  let [winrow, wincol] = win_screenpos('.')
  let xoff = wincol * (has('gui_running') ? 14 : 7) + 100
  let yoff = winrow * (has('gui_running') ? 20 : 10) + 200
  call assert_inrange(xroot + 2, xroot + xoff, xpos)
  call assert_inrange(yroot + 2, yroot + yoff, ypos)

  call TermWait(buf)
  call term_sendkeys(buf, ":q\<CR>")
  call StopVimInTerminal(buf)
  call delete('XTest_getwinpos')
  exe buf . 'bwipe!'
  set splitright&
  only!
endfunc

func Test_terminal_altscreen()
  " somehow doesn't work on MS-Windows
  CheckUnix
  let cmd = "cat Xtext\<CR>"

  let buf = term_start(&shell, {})
  call writefile(["\<Esc>[?1047h"], 'Xtext')
  call term_sendkeys(buf, cmd)
  call WaitForAssert({-> assert_equal(1, term_getaltscreen(buf))})

  call writefile(["\<Esc>[?1047l"], 'Xtext')
  call term_sendkeys(buf, cmd)
  call WaitForAssert({-> assert_equal(0, term_getaltscreen(buf))})

  call term_sendkeys(buf, "exit\r")
  exe buf . "bwipe!"
  call delete('Xtext')
endfunc

func Test_terminal_shell_option()
  if has('unix')
    " exec is a shell builtin command, should fail without a shell.
    term exec ls runtest.vim
    call WaitForAssert({-> assert_match('job failed', term_getline(bufnr(), 1))})
    bwipe!

    term ++shell exec ls runtest.vim
    call WaitForAssert({-> assert_match('runtest.vim', term_getline(bufnr(), 1))})
    bwipe!
  elseif has('win32')
    " dir is a shell builtin command, should fail without a shell.
    try
      term dir /b runtest.vim
      call WaitForAssert({-> assert_match('job failed\|cannot access .*: No such file or directory', term_getline(bufnr(), 1))})
    catch /CreateProcess/
      " ignore
    endtry
    bwipe!

    term ++shell dir /b runtest.vim
    call WaitForAssert({-> assert_match('runtest.vim', term_getline(bufnr(), 1))})
    bwipe!
  endif
endfunc

func Test_terminal_setapi_and_call()
  CheckRunVimInTerminal

  call WriteApiCall('Tapi_TryThis')
  call ch_logfile('Xlog', 'w')

  unlet! g:called_bufnum
  unlet! g:called_arg

  let buf = RunVimInTerminal('-S Xscript', {'term_api': ''})
  call WaitForAssert({-> assert_match('Unpermitted function: Tapi_TryThis', string(readfile('Xlog')))})
  call assert_false(exists('g:called_bufnum'))
  call assert_false(exists('g:called_arg'))

  eval buf->term_setapi('Tapi_')
  call term_sendkeys(buf, ":set notitle\<CR>")
  call term_sendkeys(buf, ":source Xscript\<CR>")
  call WaitFor({-> exists('g:called_bufnum')})
  call assert_equal(buf, g:called_bufnum)
  call assert_equal(['hello', 123], g:called_arg)

  call StopVimInTerminal(buf)

  call delete('Xscript')
  call ch_logfile('')
  call delete('Xlog')
  unlet! g:called_bufnum
  unlet! g:called_arg
endfunc

func Test_terminal_api_arg()
  CheckRunVimInTerminal

  call WriteApiCall('Tapi_TryThis')
  call ch_logfile('Xlog', 'w')

  unlet! g:called_bufnum
  unlet! g:called_arg

  execute 'term ++api= ' .. GetVimCommandCleanTerm() .. '-S Xscript'
  let buf = bufnr('%')
  call WaitForAssert({-> assert_match('Unpermitted function: Tapi_TryThis', string(readfile('Xlog')))})
  call assert_false(exists('g:called_bufnum'))
  call assert_false(exists('g:called_arg'))

  call StopVimInTerminal(buf)

  call ch_logfile('Xlog', 'w')

  execute 'term ++api=Tapi_ ' .. GetVimCommandCleanTerm() .. '-S Xscript'
  let buf = bufnr('%')
  call WaitFor({-> exists('g:called_bufnum')})
  call assert_equal(buf, g:called_bufnum)
  call assert_equal(['hello', 123], g:called_arg)

  call StopVimInTerminal(buf)

  call delete('Xscript')
  call ch_logfile('')
  call delete('Xlog')
  unlet! g:called_bufnum
  unlet! g:called_arg
endfunc

func Test_terminal_invalid_arg()
  call assert_fails('terminal ++xyz', 'E181:')
endfunc

func Test_terminal_in_popup()
  CheckRunVimInTerminal

  let text =<< trim END
    some text
    to edit
    in a popup window
  END
  call writefile(text, 'Xtext')
  let cmd = GetVimCommandCleanTerm()
  let lines = [
	\ 'call setline(1, range(20))',
	\ 'hi PopTerm ctermbg=grey',
	\ 'func OpenTerm(setColor)',
	\ "  set noruler",
	\ "  let s:buf = term_start('" .. cmd .. " Xtext', #{hidden: 1, term_finish: 'close'})",
	\ '  let g:winid = popup_create(s:buf, #{minwidth: 45, minheight: 7, border: [], drag: 1, resize: 1})',
	\ '  if a:setColor',
	\ '    call win_execute(g:winid, "set wincolor=PopTerm")',
	\ '  endif',
	\ 'endfunc',
	\ 'func HidePopup()',
	\ '  call popup_hide(g:winid)',
	\ 'endfunc',
	\ 'func ClosePopup()',
	\ '  call popup_close(g:winid)',
	\ 'endfunc',
	\ 'func ReopenPopup()',
	\ '  call popup_create(s:buf, #{minwidth: 40, minheight: 6, border: []})',
	\ 'endfunc',
	\ ]
  call writefile(lines, 'XtermPopup')
  let buf = RunVimInTerminal('-S XtermPopup', #{rows: 15})
  call TermWait(buf, 100)
  call term_sendkeys(buf, ":call OpenTerm(0)\<CR>")
  call TermWait(buf, 100)
  call term_sendkeys(buf, ":\<CR>")
  call TermWait(buf, 100)
  call term_sendkeys(buf, "\<C-W>:echo getwinvar(g:winid, \"&buftype\") win_gettype(g:winid)\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_1', {})

  call term_sendkeys(buf, ":q\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_2', {})
 
  call term_sendkeys(buf, ":call OpenTerm(1)\<CR>")
  call TermWait(buf, 150)
  call term_sendkeys(buf, ":set hlsearch\<CR>")
  call TermWait(buf, 100)
  call term_sendkeys(buf, "/edit\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_3', {})
 
  call term_sendkeys(buf, "\<C-W>:call HidePopup()\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_4', {})
  call term_sendkeys(buf, "\<CR>")
  call TermWait(buf, 50)

  call term_sendkeys(buf, "\<C-W>:call ClosePopup()\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_5', {})

  call term_sendkeys(buf, "\<C-W>:call ReopenPopup()\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_6', {})

  " Go to terminal-Normal mode and visually select text.
  call term_sendkeys(buf, "\<C-W>Ngg/in\<CR>vww")
  call VerifyScreenDump(buf, 'Test_terminal_popup_7', {})

  " Back to job mode, redraws
  call term_sendkeys(buf, "A")
  call VerifyScreenDump(buf, 'Test_terminal_popup_8', {})

  call TermWait(buf, 50)
  call term_sendkeys(buf, ":q\<CR>")
  call TermWait(buf, 150)  " wait for terminal to vanish

  call StopVimInTerminal(buf)
  call delete('Xtext')
  call delete('XtermPopup')
endfunc

" Check a terminal in popup window uses the default mininum size.
func Test_terminal_in_popup_min_size()
  CheckRunVimInTerminal

  let text =<< trim END
    another text
    to show
    in a popup window
  END
  call writefile(text, 'Xtext')
  let lines = [
	\ 'call setline(1, range(20))',
	\ 'func OpenTerm()',
	\ "  let s:buf = term_start('cat Xtext', #{hidden: 1})",
	\ '  let g:winid = popup_create(s:buf, #{ border: []})',
	\ 'endfunc',
	\ ]
  call writefile(lines, 'XtermPopup')
  let buf = RunVimInTerminal('-S XtermPopup', #{rows: 15})
  call TermWait(buf, 100)
  call term_sendkeys(buf, ":set noruler\<CR>")
  call term_sendkeys(buf, ":call OpenTerm()\<CR>")
  call TermWait(buf, 50)
  call term_sendkeys(buf, ":\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_m1', {})

  call TermWait(buf, 50)
  call term_sendkeys(buf, ":q\<CR>")
  call TermWait(buf, 50)  " wait for terminal to vanish
  call StopVimInTerminal(buf)
  call delete('Xtext')
  call delete('XtermPopup')
endfunc

" Check a terminal in popup window with different colors
func Terminal_in_popup_colored(group_name, highlight_cmd, highlight_opt)
  CheckRunVimInTerminal
  CheckUnix

  let lines = [
	\ 'call setline(1, range(20))',
	\ 'func OpenTerm()',
	\ "  let s:buf = term_start('cat', #{hidden: 1, "
	\ .. a:highlight_opt .. "})",
	\ '  let g:winid = popup_create(s:buf, #{ border: []})',
	\ 'endfunc',
	\ a:highlight_cmd,
	\ ]
  call writefile(lines, 'XtermPopup')
  let buf = RunVimInTerminal('-S XtermPopup', #{rows: 15})
  call TermWait(buf, 100)
  call term_sendkeys(buf, ":set noruler\<CR>")
  call term_sendkeys(buf, ":call OpenTerm()\<CR>")
  call TermWait(buf, 50)
  call term_sendkeys(buf, "hello\<CR>")
  call VerifyScreenDump(buf, 'Test_terminal_popup_' .. a:group_name, {})

  call term_sendkeys(buf, "\<C-D>")
  call TermWait(buf, 50)
  call term_sendkeys(buf, ":q\<CR>")
  call TermWait(buf, 50)  " wait for terminal to vanish
  call StopVimInTerminal(buf)
  call delete('XtermPopup')
endfunc

func Test_terminal_in_popup_colored_Terminal()
  call Terminal_in_popup_colored("Terminal", "highlight Terminal ctermfg=blue ctermbg=yellow", "")
endfunc

func Test_terminal_in_popup_colored_group()
  call Terminal_in_popup_colored("MyTermCol", "highlight MyTermCol ctermfg=darkgreen ctermbg=lightblue", "term_highlight: 'MyTermCol',")
endfunc

func Test_double_popup_terminal()
  let buf1 = term_start(&shell, #{hidden: 1})
  let win1 = popup_create(buf1, {})
  let buf2 = term_start(&shell, #{hidden: 1})
  call assert_fails('call popup_create(buf2, {})', 'E861:')
  call popup_close(win1)
  exe buf1 .. 'bwipe!'
  exe buf2 .. 'bwipe!'
endfunc

func Test_issue_5607()
  let wincount = winnr('$')
  exe 'terminal' &shell &shellcmdflag 'exit'
  let job = term_getjob(bufnr())
  call WaitForAssert({-> assert_equal("dead", job_status(job))})

  let old_wincolor = &wincolor
  try
    set wincolor=
  finally
    let &wincolor = old_wincolor
    bw!
  endtry
endfunc

func Test_hidden_terminal()
  let buf = term_start(&shell, #{hidden: 1})
  call assert_equal('', bufname('^$'))
  call StopShellInTerminal(buf)
endfunc

func Test_term_nasty_callback()
  CheckExecutable sh

  set hidden
  let g:buf0 = term_start('sh', #{hidden: 1, term_finish: 'close'})
  call popup_create(g:buf0, {})
  call assert_fails("call term_start(['sh', '-c'], #{curwin: 1})", 'E863:')

  call popup_clear(1)
  set hidden&
endfunc

func Test_term_and_startinsert()
  CheckRunVimInTerminal
  CheckUnix

  let lines =<< trim EOL
     put='some text'
     term
     startinsert
  EOL
  call writefile(lines, 'XTest_startinsert')
  let buf = RunVimInTerminal('-S XTest_startinsert', {})

  call term_sendkeys(buf, "exit\r")
  call WaitForAssert({-> assert_equal("some text", term_getline(buf, 1))})
  call term_sendkeys(buf, "0l")
  call term_sendkeys(buf, "A<\<Esc>")
  call WaitForAssert({-> assert_equal("some text<", term_getline(buf, 1))})

  call StopVimInTerminal(buf)
  call delete('XTest_startinsert')
endfunc

" Test for passing invalid arguments to terminal functions
func Test_term_func_invalid_arg()
  call assert_fails('let b = term_getaltscreen([])', 'E745:')
  call assert_fails('let a = term_getattr(1, [])', 'E730:')
  call assert_fails('let c = term_getcursor([])', 'E745:')
  call assert_fails('let l = term_getline([], 1)', 'E745:')
  call assert_fails('let l = term_getscrolled([])', 'E745:')
  call assert_fails('let s = term_getsize([])', 'E745:')
  call assert_fails('let s = term_getstatus([])', 'E745:')
  call assert_fails('let s = term_scrape([], 1)', 'E745:')
  call assert_fails('call term_sendkeys([], "a")', 'E745:')
  call assert_fails('call term_setapi([], "")', 'E745:')
  call assert_fails('call term_setrestore([], "")', 'E745:')
  call assert_fails('call term_setkill([], "")', 'E745:')
  if has('gui') || has('termguicolors')
    call assert_fails('let p = term_getansicolors([])', 'E745:')
    call assert_fails('call term_setansicolors([], [])', 'E745:')
  endif
endfunc

" Test for sending various special keycodes to a terminal
func Test_term_keycode_translation()
  CheckRunVimInTerminal

  let buf = RunVimInTerminal('', {})
  call term_sendkeys(buf, ":set nocompatible\<CR>")

  let keys = ["\<F1>", "\<F2>", "\<F3>", "\<F4>", "\<F5>", "\<F6>", "\<F7>",
        \ "\<F8>", "\<F9>", "\<F10>", "\<F11>", "\<F12>", "\<Home>",
        \ "\<S-Home>", "\<C-Home>", "\<End>", "\<S-End>", "\<C-End>",
	\ "\<Ins>", "\<Del>", "\<Left>", "\<S-Left>", "\<C-Left>", "\<Right>",
        \ "\<S-Right>", "\<C-Right>", "\<Up>", "\<S-Up>", "\<Down>",
        \ "\<S-Down>"]
  let output = ['<F1>', '<F2>', '<F3>', '<F4>', '<F5>', '<F6>', '<F7>',
        \ '<F8>', '<F9>', '<F10>', '<F11>', '<F12>', '<Home>', '<S-Home>',
        \ '<C-Home>', '<End>', '<S-End>', '<C-End>', '<Insert>', '<Del>',
        \ '<Left>', '<S-Left>', '<C-Left>', '<Right>', '<S-Right>',
        \ '<C-Right>', '<Up>', '<S-Up>', '<Down>', '<S-Down>']

  call term_sendkeys(buf, "i")
  for i in range(len(keys))
    call term_sendkeys(buf, "\<C-U>\<C-K>" .. keys[i])
    call WaitForAssert({-> assert_equal(output[i], term_getline(buf, 1))})
  endfor

  let keypad_keys = ["\<k0>", "\<k1>", "\<k2>", "\<k3>", "\<k4>", "\<k5>",
        \ "\<k6>", "\<k7>", "\<k8>", "\<k9>", "\<kPoint>", "\<kPlus>",
        \ "\<kMinus>", "\<kMultiply>", "\<kDivide>"]
  let keypad_output = ['0', '1', '2', '3', '4', '5',
        \ '6', '7', '8', '9', '.', '+',
        \ '-', '*', '/']
  for i in range(len(keypad_keys))
    " TODO: Mysteriously keypad 3 and 9 do not work on some systems.
    if keypad_output[i] == '3' || keypad_output[i] == '9'
      continue
    endif
    call term_sendkeys(buf, "\<C-U>" .. keypad_keys[i])
    call WaitForAssert({-> assert_equal(keypad_output[i], term_getline(buf, 1))})
  endfor

  call feedkeys("\<C-U>\<kEnter>\<BS>one\<C-W>.two", 'xt')
  call WaitForAssert({-> assert_equal('two', term_getline(buf, 1))})

  call StopVimInTerminal(buf)
endfunc

" Test for using the mouse in a terminal
func Test_term_mouse()
  CheckNotGui
  CheckRunVimInTerminal

  let save_mouse = &mouse
  let save_term = &term
  let save_ttymouse = &ttymouse
  let save_clipboard = &clipboard
  set mouse=a term=xterm ttymouse=sgr mousetime=200 clipboard=

  let lines =<< trim END
    one two three four five
    red green yellow red blue
    vim emacs sublime nano
  END
  call writefile(lines, 'Xtest_mouse')

  " Create a terminal window running Vim for the test with mouse enabled
  let prev_win = win_getid()
  let buf = RunVimInTerminal('Xtest_mouse -n', {})
  call term_sendkeys(buf, ":set nocompatible\<CR>")
  call term_sendkeys(buf, ":set mouse=a term=xterm ttymouse=sgr\<CR>")
  call term_sendkeys(buf, ":set clipboard=\<CR>")
  call term_sendkeys(buf, ":set mousemodel=extend\<CR>")
  call term_wait(buf)
  redraw!

  " Use the mouse to enter the terminal window
  call win_gotoid(prev_win)
  call feedkeys(MouseLeftClickCode(1, 1), 'x')
  call feedkeys(MouseLeftReleaseCode(1, 1), 'x')
  call assert_equal(1, getwininfo(win_getid())[0].terminal)

  " Test for <LeftMouse> click/release
  call test_setmouse(2, 5)
  call feedkeys("\<LeftMouse>\<LeftRelease>", 'xt')
  call test_setmouse(3, 8)
  call term_sendkeys(buf, "\<LeftMouse>\<LeftRelease>")
  call term_wait(buf, 50)
  call term_sendkeys(buf, ":call writefile([json_encode(getpos('.'))], 'Xbuf')\<CR>")
  call term_wait(buf, 50)
  let pos = json_decode(readfile('Xbuf')[0])
  call assert_equal([3, 8], pos[1:2])

  " Test for selecting text using mouse
  call delete('Xbuf')
  call test_setmouse(2, 11)
  call term_sendkeys(buf, "\<LeftMouse>")
  call test_setmouse(2, 16)
  call term_sendkeys(buf, "\<LeftRelease>y")
  call term_wait(buf, 50)
  call term_sendkeys(buf, ":call writefile([@\"], 'Xbuf')\<CR>")
  call term_wait(buf, 50)
  call assert_equal('yellow', readfile('Xbuf')[0])

  " Test for selecting text using doubleclick
  call delete('Xbuf')
  call test_setmouse(1, 11)
  call term_sendkeys(buf, "\<LeftMouse>\<LeftRelease>\<LeftMouse>")
  call test_setmouse(1, 17)
  call term_sendkeys(buf, "\<LeftRelease>y")
  call term_wait(buf, 50)
  call term_sendkeys(buf, ":call writefile([@\"], 'Xbuf')\<CR>")
  call term_wait(buf, 50)
  call assert_equal('three four', readfile('Xbuf')[0])

  " Test for selecting a line using triple click
  call delete('Xbuf')
  call test_setmouse(3, 2)
  call term_sendkeys(buf, "\<LeftMouse>\<LeftRelease>\<LeftMouse>\<LeftRelease>\<LeftMouse>\<LeftRelease>y")
  call term_wait(buf, 50)
  call term_sendkeys(buf, ":call writefile([@\"], 'Xbuf')\<CR>")
  call term_wait(buf, 50)
  call assert_equal("vim emacs sublime nano\n", readfile('Xbuf')[0])

  " Test for selecting a block using qudraple click
  call delete('Xbuf')
  call test_setmouse(1, 11)
  call term_sendkeys(buf, "\<LeftMouse>\<LeftRelease>\<LeftMouse>\<LeftRelease>\<LeftMouse>\<LeftRelease>\<LeftMouse>")
  call test_setmouse(3, 13)
  call term_sendkeys(buf, "\<LeftRelease>y")
  call term_wait(buf, 50)
  call term_sendkeys(buf, ":call writefile([@\"], 'Xbuf')\<CR>")
  call term_wait(buf, 50)
  call assert_equal("ree\nyel\nsub", readfile('Xbuf')[0])

  " Test for extending a selection using right click
  call delete('Xbuf')
  call test_setmouse(2, 9)
  call term_sendkeys(buf, "\<LeftMouse>\<LeftRelease>")
  call test_setmouse(2, 16)
  call term_sendkeys(buf, "\<RightMouse>\<RightRelease>y")
  call term_wait(buf, 50)
  call term_sendkeys(buf, ":call writefile([@\"], 'Xbuf')\<CR>")
  call term_wait(buf, 50)
  call assert_equal("n yellow", readfile('Xbuf')[0])

  " Test for pasting text using middle click
  call delete('Xbuf')
  call term_sendkeys(buf, ":let @r='bright '\<CR>")
  call test_setmouse(2, 22)
  call term_sendkeys(buf, "\"r\<MiddleMouse>\<MiddleRelease>")
  call term_wait(buf, 50)
  call term_sendkeys(buf, ":call writefile([getline(2)], 'Xbuf')\<CR>")
  call term_wait(buf, 50)
  call assert_equal("red bright blue", readfile('Xbuf')[0][-15:])

  " cleanup
  call term_wait(buf)
  call StopVimInTerminal(buf)
  let &mouse = save_mouse
  let &term = save_term
  let &ttymouse = save_ttymouse
  let &clipboard = save_clipboard
  set mousetime&
  call delete('Xtest_mouse')
  call delete('Xbuf')
endfunc

" Test for modeless selection in a terminal
func Test_term_modeless_selection()
  CheckUnix
  CheckNotGui
  CheckRunVimInTerminal
  CheckFeature clipboard_working

  let save_mouse = &mouse
  let save_term = &term
  let save_ttymouse = &ttymouse
  set mouse=a term=xterm ttymouse=sgr mousetime=200
  set clipboard=autoselectml

  let lines =<< trim END
    one two three four five
    red green yellow red blue
    vim emacs sublime nano
  END
  call writefile(lines, 'Xtest_modeless')

  " Create a terminal window running Vim for the test with mouse disabled
  let prev_win = win_getid()
  let buf = RunVimInTerminal('Xtest_modeless -n', {})
  call term_sendkeys(buf, ":set nocompatible\<CR>")
  call term_sendkeys(buf, ":set mouse=\<CR>")
  call term_wait(buf)
  redraw!

  " Use the mouse to enter the terminal window
  call win_gotoid(prev_win)
  call feedkeys(MouseLeftClickCode(1, 1), 'x')
  call feedkeys(MouseLeftReleaseCode(1, 1), 'x')
  call term_wait(buf)
  call assert_equal(1, getwininfo(win_getid())[0].terminal)

  " Test for copying a modeless selection to clipboard
  let @* = 'clean'
  " communicating with X server may take a little time
  sleep 100m
  call feedkeys(MouseLeftClickCode(2, 3), 'x')
  call feedkeys(MouseLeftDragCode(2, 11), 'x')
  call feedkeys(MouseLeftReleaseCode(2, 11), 'x')
  call assert_equal("d green y", @*)

  " cleanup
  call term_wait(buf)
  call StopVimInTerminal(buf)
  let &mouse = save_mouse
  let &term = save_term
  let &ttymouse = save_ttymouse
  set mousetime& clipboard&
  call delete('Xtest_modeless')
  new | only!
endfunc


" vim: shiftwidth=2 sts=2 expandtab
