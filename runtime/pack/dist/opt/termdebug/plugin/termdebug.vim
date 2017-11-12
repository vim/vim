" Debugger plugin using gdb.
"
" WORK IN PROGRESS - much doesn't work yet
"
" Open two visible terminal windows:
" 1. run a pty, as with ":term NONE"
" 2. run gdb, passing the pty
" The current window is used to view source code and follows gdb.
"
" A third terminal window is hidden, it is used for communication with gdb.
"
" The communication with gdb uses GDB/MI.  See:
" https://sourceware.org/gdb/current/onlinedocs/gdb/GDB_002fMI.html
"
" Author: Bram Moolenaar
" Copyright: Vim license applies, see ":help license"

" In case this gets loaded twice.
if exists(':Termdebug')
  finish
endif

" Uncomment this line to write logging in "debuglog".
" call ch_logfile('debuglog', 'w')

" The command that starts debugging, e.g. ":Termdebug vim".
" To end type "quit" in the gdb window.
command -nargs=* -complete=file Termdebug call s:StartDebug(<q-args>)

" Name of the gdb command, defaults to "gdb".
if !exists('termdebugger')
  let termdebugger = 'gdb'
endif

let s:pc_id = 12
let s:break_id = 13
let s:stopped = 1

if &background == 'light'
  hi default debugPC term=reverse ctermbg=lightblue guibg=lightblue
else
  hi default debugPC term=reverse ctermbg=darkblue guibg=darkblue
endif
hi default debugBreakpoint term=reverse ctermbg=red guibg=red

func s:StartDebug(cmd)
  let s:startwin = win_getid(winnr())
  let s:startsigncolumn = &signcolumn

  let s:save_columns = 0
  if exists('g:termdebug_wide')
    if &columns < g:termdebug_wide
      let s:save_columns = &columns
      let &columns = g:termdebug_wide
    endif
    let vertical = 1
  else
    let vertical = 0
  endif

  " Open a terminal window without a job, to run the debugged program
  let s:ptybuf = term_start('NONE', {
	\ 'term_name': 'gdb program',
	\ 'vertical': vertical,
	\ })
  if s:ptybuf == 0
    echoerr 'Failed to open the program terminal window'
    return
  endif
  let pty = job_info(term_getjob(s:ptybuf))['tty_out']
  let s:ptywin = win_getid(winnr())

  " Create a hidden terminal window to communicate with gdb
  let s:commbuf = term_start('NONE', {
	\ 'term_name': 'gdb communication',
	\ 'out_cb': function('s:CommOutput'),
	\ 'hidden': 1,
	\ })
  if s:commbuf == 0
    echoerr 'Failed to open the communication terminal window'
    exe 'bwipe! ' . s:ptybuf
    return
  endif
  let commpty = job_info(term_getjob(s:commbuf))['tty_out']

  " Open a terminal window to run the debugger.
  " Add -quiet to avoid the intro message causing a hit-enter prompt.
  let cmd = [g:termdebugger, '-quiet', '-tty', pty, a:cmd]
  echomsg 'executing "' . join(cmd) . '"'
  let s:gdbbuf = term_start(cmd, {
	\ 'exit_cb': function('s:EndDebug'),
	\ 'term_finish': 'close',
	\ })
  if s:gdbbuf == 0
    echoerr 'Failed to open the gdb terminal window'
    exe 'bwipe! ' . s:ptybuf
    exe 'bwipe! ' . s:commbuf
    return
  endif
  let s:gdbwin = win_getid(winnr())

  " Connect gdb to the communication pty, using the GDB/MI interface
  " If you get an error "undefined command" your GDB is too old.
  call term_sendkeys(s:gdbbuf, 'new-ui mi ' . commpty . "\r")

  " Interpret commands while the target is running.  This should usualy only be
  " exec-interrupt, since many commands don't work properly while the target is
  " running.
  call s:SendCommand('-gdb-set mi-async on')

  " Sign used to highlight the line where the program has stopped.
  " There can be only one.
  sign define debugPC linehl=debugPC

  " Sign used to indicate a breakpoint.
  " Can be used multiple times.
  sign define debugBreakpoint text=>> texthl=debugBreakpoint

  " Install debugger commands in the text window.
  call win_gotoid(s:startwin)
  call s:InstallCommands()
  call win_gotoid(s:gdbwin)

  let s:breakpoints = {}

  augroup TermDebug
    au BufRead * call s:BufRead()
    au BufUnload * call s:BufUnloaded()
  augroup END
endfunc

func s:EndDebug(job, status)
  exe 'bwipe! ' . s:ptybuf
  exe 'bwipe! ' . s:commbuf

  let curwinid = win_getid(winnr())

  call win_gotoid(s:startwin)
  let &signcolumn = s:startsigncolumn
  call s:DeleteCommands()

  call win_gotoid(curwinid)
  if s:save_columns > 0
    let &columns = s:save_columns
  endif

  au! TermDebug
endfunc

" Handle a message received from gdb on the GDB/MI interface.
func s:CommOutput(chan, msg)
  let msgs = split(a:msg, "\r")

  for msg in msgs
    " remove prefixed NL
    if msg[0] == "\n"
      let msg = msg[1:]
    endif
    if msg != ''
      if msg =~ '^\(\*stopped\|\*running\|=thread-selected\)'
	call s:HandleCursor(msg)
      elseif msg =~ '^\^done,bkpt=' || msg =~ '^=breakpoint-created,'
	call s:HandleNewBreakpoint(msg)
      elseif msg =~ '^=breakpoint-deleted,'
	call s:HandleBreakpointDelete(msg)
      elseif msg =~ '^\^done,value='
	call s:HandleEvaluate(msg)
      elseif msg =~ '^\^error,msg='
	call s:HandleError(msg)
      endif
    endif
  endfor
endfunc

" Install commands in the current window to control the debugger.
func s:InstallCommands()
  command Break call s:SetBreakpoint()
  command Delete call s:DeleteBreakpoint()
  command Step call s:SendCommand('-exec-step')
  command Over call s:SendCommand('-exec-next')
  command Finish call s:SendCommand('-exec-finish')
  command -nargs=* Run call s:Run(<q-args>)
  command -nargs=* Arguments call s:SendCommand('-exec-arguments ' . <q-args>)
  command Stop call s:SendCommand('-exec-interrupt')
  command Continue call s:SendCommand('-exec-continue')
  command -range -nargs=* Evaluate call s:Evaluate(<range>, <q-args>)
  command Gdb call win_gotoid(s:gdbwin)
  command Program call win_gotoid(s:ptywin)

  " TODO: can the K mapping be restored?
  nnoremap K :Evaluate<CR>

  if has('menu')
    nnoremenu WinBar.Step :Step<CR>
    nnoremenu WinBar.Next :Over<CR>
    nnoremenu WinBar.Finish :Finish<CR>
    nnoremenu WinBar.Cont :Continue<CR>
    nnoremenu WinBar.Stop :Stop<CR>
    nnoremenu WinBar.Eval :Evaluate<CR>
  endif
endfunc

" Delete installed debugger commands in the current window.
func s:DeleteCommands()
  delcommand Break
  delcommand Delete
  delcommand Step
  delcommand Over
  delcommand Finish
  delcommand Run
  delcommand Arguments
  delcommand Stop
  delcommand Continue
  delcommand Evaluate
  delcommand Gdb
  delcommand Program

  nunmap K

  if has('menu')
    aunmenu WinBar.Step
    aunmenu WinBar.Next
    aunmenu WinBar.Finish
    aunmenu WinBar.Cont
    aunmenu WinBar.Stop
    aunmenu WinBar.Eval
  endif

  exe 'sign unplace ' . s:pc_id
  for key in keys(s:breakpoints)
    exe 'sign unplace ' . (s:break_id + key)
  endfor
  sign undefine debugPC
  sign undefine debugBreakpoint
  unlet s:breakpoints
endfunc

" :Break - Set a breakpoint at the cursor position.
func s:SetBreakpoint()
  " Setting a breakpoint may not work while the program is running.
  " Interrupt to make it work.
  let do_continue = 0
  if !s:stopped
    let do_continue = 1
    call s:SendCommand('-exec-interrupt')
    sleep 10m
  endif
  call s:SendCommand('-break-insert --source '
	\ . fnameescape(expand('%:p')) . ' --line ' . line('.'))
  if do_continue
    call s:SendCommand('-exec-continue')
  endif
endfunc

" :Delete - Delete a breakpoint at the cursor position.
func s:DeleteBreakpoint()
  let fname = fnameescape(expand('%:p'))
  let lnum = line('.')
  for [key, val] in items(s:breakpoints)
    if val['fname'] == fname && val['lnum'] == lnum
      call term_sendkeys(s:commbuf, '-break-delete ' . key . "\r")
      " Assume this always wors, the reply is simply "^done".
      exe 'sign unplace ' . (s:break_id + key)
      unlet s:breakpoints[key]
      break
    endif
  endfor
endfunc

" :Next, :Continue, etc - send a command to gdb
func s:SendCommand(cmd)
  call term_sendkeys(s:commbuf, a:cmd . "\r")
endfunc

func s:Run(args)
  if a:args != ''
    call s:SendCommand('-exec-arguments ' . a:args)
  endif
  call s:SendCommand('-exec-run')
endfunc

" :Evaluate - evaluate what is under the cursor
func s:Evaluate(range, arg)
  if a:arg != ''
    let expr = a:arg
  elseif a:range == 2
    let pos = getcurpos()
    let reg = getreg('v', 1, 1)
    let regt = getregtype('v')
    normal! gv"vy
    let expr = @v
    call setpos('.', pos)
    call setreg('v', reg, regt)
  else
    let expr = expand('<cexpr>')
  endif
  call s:SendCommand('-data-evaluate-expression "' . expr . '"')
  let s:evalexpr = expr
endfunc

" Handle the result of data-evaluate-expression
func s:HandleEvaluate(msg)
  let value = substitute(a:msg, '.*value="\(.*\)"', '\1', '')
  let value = substitute(value, '\\"', '"', 'g')
  echomsg '"' . s:evalexpr . '": ' . value

  if s:evalexpr[0] != '*' && value =~ '^0x' && value != '0x0' && value !~ '"$'
    " Looks like a pointer, also display what it points to.
    let s:evalexpr = '*' . s:evalexpr
    call term_sendkeys(s:commbuf, '-data-evaluate-expression "' . s:evalexpr . "\"\r")
  endif
endfunc

" Handle an error.
func s:HandleError(msg)
  echoerr substitute(a:msg, '.*msg="\(.*\)"', '\1', '')
endfunc

" Handle stopping and running message from gdb.
" Will update the sign that shows the current position.
func s:HandleCursor(msg)
  let wid = win_getid(winnr())

  if a:msg =~ '^\*stopped'
    let s:stopped = 1
  elseif a:msg =~ '^\*running'
    let s:stopped = 0
  endif

  if win_gotoid(s:startwin)
    let fname = substitute(a:msg, '.*fullname="\([^"]*\)".*', '\1', '')
    if a:msg =~ '^\(\*stopped\|=thread-selected\)' && filereadable(fname)
      let lnum = substitute(a:msg, '.*line="\([^"]*\)".*', '\1', '')
      if lnum =~ '^[0-9]*$'
	if expand('%:p') != fnamemodify(fname, ':p')
	  if &modified
	    " TODO: find existing window
	    exe 'split ' . fnameescape(fname)
	    let s:startwin = win_getid(winnr())
	  else
	    exe 'edit ' . fnameescape(fname)
	  endif
	endif
	exe lnum
	exe 'sign unplace ' . s:pc_id
	exe 'sign place ' . s:pc_id . ' line=' . lnum . ' name=debugPC file=' . fname
	setlocal signcolumn=yes
      endif
    else
      exe 'sign unplace ' . s:pc_id
    endif

    call win_gotoid(wid)
  endif
endfunc

" Handle setting a breakpoint
" Will update the sign that shows the breakpoint
func s:HandleNewBreakpoint(msg)
  let nr = substitute(a:msg, '.*number="\([0-9]\)*\".*', '\1', '') + 0
  if nr == 0
    return
  endif

  if has_key(s:breakpoints, nr)
    let entry = s:breakpoints[nr]
  else
    let entry = {}
    let s:breakpoints[nr] = entry
  endif

  let fname = substitute(a:msg, '.*fullname="\([^"]*\)".*', '\1', '')
  let lnum = substitute(a:msg, '.*line="\([^"]*\)".*', '\1', '')
  let entry['fname'] = fname
  let entry['lnum'] = lnum

  if bufloaded(fname)
    call s:PlaceSign(nr, entry)
  endif
endfunc

func s:PlaceSign(nr, entry)
  exe 'sign place ' . (s:break_id + a:nr) . ' line=' . a:entry['lnum'] . ' name=debugBreakpoint file=' . a:entry['fname']
  let a:entry['placed'] = 1
endfunc

" Handle deleting a breakpoint
" Will remove the sign that shows the breakpoint
func s:HandleBreakpointDelete(msg)
  let nr = substitute(a:msg, '.*id="\([0-9]*\)\".*', '\1', '') + 0
  if nr == 0
    return
  endif
  if has_key(s:breakpoints, nr)
    let entry = s:breakpoints[nr]
    if has_key(entry, 'placed')
      exe 'sign unplace ' . (s:break_id + nr)
      unlet entry['placed']
    endif
    unlet s:breakpoints[nr]
  endif
endfunc

" Handle a BufRead autocommand event: place any signs.
func s:BufRead()
  let fname = expand('<afile>:p')
  for [nr, entry] in items(s:breakpoints)
    if entry['fname'] == fname
      call s:PlaceSign(nr, entry)
    endif
  endfor
endfunc

" Handle a BufUnloaded autocommand event: unplace any signs.
func s:BufUnloaded()
  let fname = expand('<afile>:p')
  for [nr, entry] in items(s:breakpoints)
    if entry['fname'] == fname
      let entry['placed'] = 0
    endif
  endfor
endfunc

