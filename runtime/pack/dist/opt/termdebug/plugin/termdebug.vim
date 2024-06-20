vim9script

# Debugger plugin using gdb.

# Author: Bram Moolenaar
# Copyright: Vim license applies, see ":help license"
# Last Change: 2024 Jun 16
# Converted to Vim9: Ubaldo Tiberi <ubaldo.tiberi@gmail.com>

# WORK IN PROGRESS - The basics works stable, more to come
# Note: In general you need at least GDB 7.12 because this provides the
# frame= response in MI thread-selected events we need to sync stack to file.
# The one included with "old" MingW is too old (7.6.1), you may upgrade it or
# use a newer version from http://www.equation.com/servlet/equation.cmd?fa=gdb

# There are two ways to run gdb:
# - In a terminal window; used if possible, does not work on MS-Windows
#   Not used when g:termdebug_use_prompt is set to 1.
# - Using a "prompt" buffer; may use a terminal window for the program

# For both the current window is used to view source code and shows the
# current statement from gdb.

# USING A TERMINAL WINDOW

# Opens two visible terminal windows:
# 1. runs a pty for the debugged program, as with ":term NONE"
# 2. runs gdb, passing the pty of the debugged program
# A third terminal window is hidden, it is used for communication with gdb.

# USING A PROMPT BUFFER

# Opens a window with a prompt buffer to communicate with gdb.
# Gdb is run as a job with callbacks for I/O.
# On Unix another terminal window is opened to run the debugged program
# On MS-Windows a separate console is opened to run the debugged program

# The communication with gdb uses GDB/MI.  See:
# https://sourceware.org/gdb/current/onlinedocs/gdb/GDB_002fMI.html

def Echoerr(msg: string)
  echohl ErrorMsg | echom $'[termdebug] {msg}' | echohl None
enddef


# Variables to keep their status among multiple instances of Termdebug
# Avoid to source the script twice.
# if exists('g:termdebug_loaded')
#     Echoerr('Termdebug already loaded.')
#     finish
# endif
# g:termdebug_loaded = true
g:termdebug_is_running = false


# The command that starts debugging, e.g. ":Termdebug vim".
# To end type "quit" in the gdb window.
command -nargs=* -complete=file -bang Termdebug StartDebug(<bang>0, <f-args>)
command -nargs=+ -complete=file -bang TermdebugCommand StartDebugCommand(<bang>0, <f-args>)

# Script variables declaration. These variables are re-initialized at every
# Termdebug instance
var way: string
var err: string

var pc_id: number
var asm_id: number
var break_id: number
var stopped: bool
var running: bool

var parsing_disasm_msg: number
var asm_lines: list<string>
var asm_addr: string

# These shall be constants but cannot be initialized here
# They indicate the buffer numbers of the main buffers used
var gdbbufnr: number
var gdbbufname: string
var varbufnr: number
var varbufname: string
var asmbufnr: number
var asmbufname: string
var promptbufnr: number
# This is for the "debugged-program" thing
var ptybufnr: number
var ptybufname: string
var commbufnr: number
var commbufname: string

var gdbjob: job
var gdb_channel: channel
# These changes because they relate to windows
var pid: number
var gdbwin: number
var varwin: number
var asmwin: number
var ptywin: number
var sourcewin: number

# Contains breakpoints that have been placed, key is a string with the GDB
# breakpoint number.
# Each entry is a dict, containing the sub-breakpoints.  Key is the subid.
# For a breakpoint that is just a number the subid is zero.
# For a breakpoint "123.4" the id is "123" and subid is "4".
# Example, when breakpoint "44", "123", "123.1" and "123.2" exist:
# {'44': {'0': entry}, '123': {'0': entry, '1': entry, '2': entry}}
var breakpoints: dict<any>

# Contains breakpoints by file/lnum.  The key is "fname:lnum".
# Each entry is a list of breakpoint IDs at that position.
var breakpoint_locations: dict<any>
var BreakpointSigns: list<string>

var evalFromBalloonExpr: bool
var evalFromBalloonExprResult: string
var ignoreEvalError: bool
var evalexpr: string
# Remember the old value of 'signcolumn' for each buffer that it's set in, so
# that we can restore the value for all buffers.
var signcolumn_buflist: list<number>
var saved_columns: number

var allleft: bool
# This was s:vertical but I cannot use vertical as variable name
var vvertical: bool

var winbar_winids: list<number>

var saved_mousemodel: string

var saved_K_map: dict<any>
var saved_plus_map: dict<any>
var saved_minus_map: dict<any>


def InitScriptVariables()
  if exists('g:termdebug_config') && has_key(g:termdebug_config, 'use_prompt')
    way = g:termdebug_config['use_prompt'] ? 'prompt' : 'terminal'
  elseif exists('g:termdebug_use_prompt')
    way = g:termdebug_use_prompt
  elseif has('terminal') && !has('win32')
    way = 'terminal'
  else
    way = 'prompt'
  endif
  err = ''

  pc_id = 12
  asm_id = 13
  break_id = 14  # breakpoint number is added to this
  stopped = true
  running = false

  parsing_disasm_msg = 0
  asm_lines = []
  asm_addr = ''

  # They indicate the buffer numbers of the main buffers used
  gdbbufnr = 0
  gdbbufname = 'gdb'
  varbufnr = 0
  varbufname = 'Termdebug-variables-listing'
  asmbufnr = 0
  asmbufname = 'Termdebug-asm-listing'
  promptbufnr = 0
  # This is for the "debugged-program" thing
  ptybufname = "debugged-program"
  ptybufnr = 0
  commbufname = "gdb-communication"
  commbufnr = 0

  gdbjob = null_job
  gdb_channel = null_channel
  # These changes because they relate to windows
  pid = 0
  gdbwin = 0
  varwin = 0
  asmwin = 0
  ptywin = 0
  sourcewin = 0

  # Contains breakpoints that have been placed, key is a string with the GDB
  # breakpoint number.
  # Each entry is a dict, containing the sub-breakpoints.  Key is the subid.
  # For a breakpoint that is just a number the subid is zero.
  # For a breakpoint "123.4" the id is "123" and subid is "4".
  # Example, when breakpoint "44", "123", "123.1" and "123.2" exist:
  # {'44': {'0': entry}, '123': {'0': entry, '1': entry, '2': entry}}
  breakpoints = {}

  # Contains breakpoints by file/lnum.  The key is "fname:lnum".
  # Each entry is a list of breakpoint IDs at that position.
  breakpoint_locations = {}
  BreakpointSigns = []

  evalFromBalloonExpr = false
  evalFromBalloonExprResult = ''
  ignoreEvalError = false
  evalexpr = ''
  # Remember the old value of 'signcolumn' for each buffer that it's set in, so
  # that we can restore the value for all buffers.
  signcolumn_buflist = [bufnr()]
  saved_columns = &columns

  winbar_winids = []

  saved_K_map = maparg('K', 'n', false, true)
  saved_plus_map = maparg('+', 'n', false, true)
  saved_minus_map = maparg('-', 'n', false, true)

  if has('menu')
    saved_mousemodel = &mousemodel
  endif
enddef

def SanityCheck(): bool
  # CHECKME: This is checked after InitScriptVariables(). Perhaps we need a
  # check also before initialization?
  var gdb_cmd = GetCommand()[0]
  var is_check_ok = true
  # Need either the +terminal feature or +channel and the prompt buffer.
  # The terminal feature does not work with gdb on win32.
  if (way ==# 'prompt') && !has('channel')
    err = 'Cannot debug, +channel feature is not supported'
  elseif way ==# 'prompt' && !exists('*prompt_setprompt')
    err = 'Cannot debug, missing prompt buffer support'
  elseif way ==# 'prompt' && !empty(glob(gdb_cmd))
    err = $"You have a file/folder named '{gdb_cmd}' in the current directory Termdebug may not work properly. Please exit and rename such a file/folder."
  elseif !empty(glob(asmbufname))
    err = $"You have a file/folder named '{asmbufname}' in the current directory Termdebug may not work properly. Please exit and rename such a file/folder."
  elseif !empty(glob(varbufname))
    err = $"You have a file/folder named '{varbufname}' in the current directory Termdebug may not work properly. Please exit and rename such a file/folder."
  elseif !executable(gdb_cmd)
    err = $"Cannot execute debugger program '{gdb_cmd}'"
  endif

  if !empty(err)
    Echoerr(err)
    is_check_ok = false
  endif
  return is_check_ok
enddef


# Take a breakpoint number as used by GDB and turn it into an integer.
# The breakpoint may contain a dot: 123.4 -> 123004
# The main breakpoint has a zero subid.
def Breakpoint2SignNumber(id: number, subid: number): number
  return break_id + id * 1000 + subid
enddef

# Define or adjust the default highlighting, using background "new".
# When the 'background' option is set then "old" has the old value.
def Highlight(init: bool, old: string, new: string)
  var default = init ? 'default ' : ''
  if new ==# 'light' && old !=# 'light'
    exe $"hi {default}debugPC term=reverse ctermbg=lightblue guibg=lightblue"
  elseif new ==# 'dark' && old !=# 'dark'
    exe $"hi {default}debugPC term=reverse ctermbg=darkblue guibg=darkblue"
  endif
enddef

# Define the default highlighting, using the current 'background' value.
def InitHighlight()
  Highlight(1, '', &background)
  hi default debugBreakpoint term=reverse ctermbg=red guibg=red
  hi default debugBreakpointDisabled term=reverse ctermbg=gray guibg=gray
enddef

# Setup an autocommand to redefine the default highlight when the colorscheme
# is changed.
def InitAutocmd()
  augroup TermDebug
    autocmd!
    autocmd ColorScheme * InitHighlight()
  augroup END
enddef

# Get the command to execute the debugger as a list, defaults to ["gdb"].
def GetCommand(): list<string>
  var cmd = 'gdb'
  if exists('g:termdebug_config')
    cmd = get(g:termdebug_config, 'command', 'gdb')
  elseif exists('g:termdebugger')
    cmd = g:termdebugger
  endif

  return type(cmd) == v:t_list ? copy(cmd) : [cmd]
enddef

def StartDebug(bang: bool, ...gdb_args: list<string>)
  if g:termdebug_is_running == true
    Echoerr('Terminal debugger already running, cannot run two')
    return
  endif

  InitScriptVariables()
  if !SanityCheck()
    return
  endif
  # First argument is the command to debug, second core file or process ID.
  StartDebug_internal({gdb_args: gdb_args, bang: bang})
enddef

def StartDebugCommand(bang: bool, ...args: list<string>)
  if g:termdebug_is_running == true
    Echoerr('Terminal debugger already running, cannot run two')
    return
  endif

  InitScriptVariables()
  if !SanityCheck()
    return
  endif
  # First argument is the command to debug, rest are run arguments.
  StartDebug_internal({gdb_args: [args[0]], proc_args: args[1 : ], bang: bang})
enddef

def StartDebug_internal(dict: dict<any>)

  if exists('#User#TermdebugStartPre')
    doauto <nomodeline> User TermdebugStartPre
  endif

  # Uncomment this line to write logging in "debuglog".
  # call ch_logfile('debuglog', 'w')

  # Assume current window is the source code window
  sourcewin = win_getid()
  var wide = 0

  if exists('g:termdebug_config')
    wide = get(g:termdebug_config, 'wide', 0)
  elseif exists('g:termdebug_wide')
    wide = g:termdebug_wide
  endif
  if wide > 0
    if &columns < wide
      &columns = wide
      # If we make the Vim window wider, use the whole left half for the debug
      # windows.
      allleft = true
    endif
    vvertical = true
  else
    vvertical = false
  endif

  if way == 'prompt'
    StartDebug_prompt(dict)
  else
    StartDebug_term(dict)
  endif

  if GetDisasmWindow()
    var curwinid = win_getid()
    GotoAsmwinOrCreateIt()
    win_gotoid(curwinid)
  endif

  if GetVariablesWindow()
    var curwinid = win_getid()
    GotoVariableswinOrCreateIt()
    win_gotoid(curwinid)
  endif

  if exists('#User#TermdebugStartPost')
    doauto <nomodeline> User TermdebugStartPost
  endif
  g:termdebug_is_running = true
enddef

# Use when debugger didn't start or ended.
def CloseBuffers()
  var buf_numbers = [promptbufnr, ptybufnr, commbufnr, asmbufnr, varbufnr]
  for buf_nr in buf_numbers
    if buf_nr > 0 && bufexists(buf_nr)
      exe $'bwipe! {buf_nr}'
    endif
  endfor

  running = false
  gdbwin = 0
enddef

def IsGdbStarted(): bool
  var gdbproc_status = job_status(term_getjob(gdbbufnr))
  if gdbproc_status !=# 'run'
    return false
  endif
  return true
enddef

def CreateProgramPty(): string
  ptybufnr = term_start('NONE', {
    term_name: ptybufname,
    vertical: vvertical})
  if ptybufnr == 0
    return null_string
  endif
  ptywin = win_getid()

  if vvertical
    # Assuming the source code window will get a signcolumn, use two more
    # columns for that, thus one less for the terminal window.
    exe $":{(&columns / 2 - 1)}wincmd |"
    if allleft
      # use the whole left column
      wincmd H
    endif
  endif

  return job_info(term_getjob(ptybufnr))['tty_out']
enddef

def CreateCommunicationPty(): string
  # Create a hidden terminal window to communicate with gdb
  commbufnr = term_start('NONE', {
    term_name: commbufname,
    out_cb: function('CommOutput'),
    hidden: 1
  })
  if commbufnr == 0
    return null_string
  endif
  return job_info(term_getjob(commbufnr))['tty_out']
enddef

def CreateGdbConsole(dict: dict<any>, pty: string, commpty: string): string
  # Start the gdb buffer
  var gdb_args = get(dict, 'gdb_args', [])
  var proc_args = get(dict, 'proc_args', [])

  var gdb_cmd = GetCommand()

  gdbbufname = gdb_cmd[0]

  if exists('g:termdebug_config') && has_key(g:termdebug_config, 'command_add_args')
    gdb_cmd = g:termdebug_config.command_add_args(gdb_cmd, pty)
  else
    # Add -quiet to avoid the intro message causing a hit-enter prompt.
    gdb_cmd += ['-quiet']
    # Disable pagination, it causes everything to stop at the gdb
    gdb_cmd += ['-iex', 'set pagination off']
    # Interpret commands while the target is running.  This should usually only
    # be exec-interrupt, since many commands don't work properly while the
    # target is running (so execute during startup).
    gdb_cmd += ['-iex', 'set mi-async on']
    # Open a terminal window to run the debugger.
    gdb_cmd += ['-tty', pty]
    # Command executed _after_ startup is done, provides us with the necessary
    # feedback
    gdb_cmd += ['-ex', 'echo startupdone\n']
  endif

  if exists('g:termdebug_config') && has_key(g:termdebug_config, 'command_filter')
    gdb_cmd = g:termdebug_config.command_filter(gdb_cmd)
  endif

  # Adding arguments requested by the user
  gdb_cmd += gdb_args

  ch_log($'executing "{join(gdb_cmd)}"')
  gdbbufnr = term_start(gdb_cmd, {
        term_name: gdbbufname,
        term_finish: 'close',
        })
  if gdbbufnr == 0
    return 'Failed to open the gdb terminal window'
  endif
  gdbwin = win_getid()

  # Wait for the "startupdone" message before sending any commands.
  var counter = 0
  var counter_max = 300
  if exists('g:termdebug_config') && has_key(g:termdebug_config, 'timeout')
    counter_max = g:termdebug_config['timeout']
  endif

  var success = false
  while !success && counter < counter_max
    if !IsGdbStarted()
      return $'{gdbbufname} exited unexpectedly'
    endif

    for lnum in range(1, 200)
      if term_getline(gdbbufnr, lnum) =~ 'startupdone'
        success = true
      endif
    endfor

    # Each count is 10ms
    counter += 1
    sleep 10m
  endwhile

  if !success
    return 'Failed to startup the gdb program.'
  endif

  # ---- gdb started. Next, let's set the MI interface. ---
  # Set arguments to be run.
  if len(proc_args)
    term_sendkeys(gdbbufnr, $"server set args {join(proc_args)}\r")
  endif

  # Connect gdb to the communication pty, using the GDB/MI interface.
  # Prefix "server" to avoid adding this to the history.
  term_sendkeys(gdbbufnr, $"server new-ui mi {commpty}\r")

  # Wait for the response to show up, users may not notice the error and wonder
  # why the debugger doesn't work.
   counter = 0
   counter_max = 300
   success = false
  while !success && counter < counter_max
    if !IsGdbStarted()
      return $'{gdbbufname} exited unexpectedly'
    endif

    var response = ''
    for lnum in range(1, 200)
      var line1 = term_getline(gdbbufnr, lnum)
      var line2 = term_getline(gdbbufnr, lnum + 1)
      if line1 =~ 'new-ui mi '
        # response can be in the same line or the next line
        response = $"{line1}{line2}"
        if response =~ 'Undefined command'
          # CHECKME: possibly send a "server show version" here
          return 'Sorry, your gdb is too old, gdb 7.12 is required'
        endif
        if response =~ 'New UI allocated'
          # Success!
          success = true
        endif
      elseif line1 =~ 'Reading symbols from' && line2 !~ 'new-ui mi '
        # Reading symbols might take a while, try more times
        counter -= 1
      endif
    endfor
    if response =~ 'New UI allocated'
      break
    endif
    counter += 1
    sleep 10m
  endwhile

  if !success
    return 'Cannot check if your gdb works, continuing anyway'
  endif
  return ''
enddef


# Open a terminal window without a job, to run the debugged program in.
def StartDebug_term(dict: dict<any>)

  var programpty = CreateProgramPty()
  if programpty is null_string
    Echoerr('Failed to open the program terminal window')
    CloseBuffers()
    return
  endif

  var commpty = CreateCommunicationPty()
  if commpty is null_string
    Echoerr('Failed to open the communication terminal window')
    CloseBuffers()
    return
  endif

  var err_message = CreateGdbConsole(dict, programpty, commpty)
  if !empty(err_message)
    Echoerr(err_message)
    CloseBuffers()
    return
  endif

  job_setoptions(term_getjob(gdbbufnr), {exit_cb: function('EndDebug')})

  # Set the filetype, this can be used to add mappings.
  set filetype=termdebug

  StartDebugCommon(dict)
enddef

# Open a window with a prompt buffer to run gdb in.
def StartDebug_prompt(dict: dict<any>)
  var gdb_cmd = GetCommand()
  gdbbufname = gdb_cmd[0]

  if vvertical == true
    vertical new
  else
    new
  endif
  gdbwin = win_getid()
  promptbufnr = bufnr('')
  prompt_setprompt(promptbufnr, 'gdb> ')
  set buftype=prompt
  exe $"file {gdbbufname}"

  prompt_setcallback(promptbufnr, function('PromptCallback'))
  prompt_setinterrupt(promptbufnr, function('PromptInterrupt'))

  if vvertical
    # Assuming the source code window will get a signcolumn, use two more
    # columns for that, thus one less for the terminal window.
    exe $":{(&columns / 2 - 1)}wincmd |"
  endif

  var gdb_args = get(dict, 'gdb_args', [])
  var proc_args = get(dict, 'proc_args', [])

  # Add -quiet to avoid the intro message causing a hit-enter prompt.
  gdb_cmd += ['-quiet']
  # Disable pagination, it causes everything to stop at the gdb, needs to be run early
  gdb_cmd += ['-iex', 'set pagination off']
  # Interpret commands while the target is running.  This should usually only
  # be exec-interrupt, since many commands don't work properly while the
  # target is running (so execute during startup).
  gdb_cmd += ['-iex', 'set mi-async on']
  # directly communicate via mi2
  gdb_cmd += ['--interpreter=mi2']

  # Adding arguments requested by the user
  gdb_cmd += gdb_args

  ch_log($'executing "{join(gdb_cmd)}"')
  gdbjob = job_start(gdb_cmd, {
    # exit_cb: function('EndPromptDebug'),
    exit_cb: function('EndDebug'),
    out_cb: function('GdbOutCallback'),
  })
  if job_status(gdbjob) != "run"
    Echoerr('Failed to start gdb')
    exe $'bwipe! {promptbufnr}'
    return
  endif
  exe $'au BufUnload <buffer={promptbufnr}> ++once ' ..
       'call job_stop(gdbjob, ''kill'')'
  # Mark the buffer modified so that it's not easy to close.
  set modified
  gdb_channel = job_getchannel(gdbjob)

  ptybufnr = 0
  if has('win32')
    # MS-Windows: run in a new console window for maximum compatibility
    SendCommand('set new-console on')
  elseif has('terminal')
    # Unix: Run the debugged program in a terminal window.  Open it below the
    # gdb window.
    belowright ptybufnr = term_start('NONE', {
      term_name: 'debugged program',
      vertical: vvertical
    })
    if ptybufnr == 0
      Echoerr('Failed to open the program terminal window')
      job_stop(gdbjob)
      return
    endif
    ptywin = win_getid()
    var pty = job_info(term_getjob(ptybufnr))['tty_out']
    SendCommand($'tty {pty}')

    # Since GDB runs in a prompt window, the environment has not been set to
    # match a terminal window, need to do that now.
    SendCommand('set env TERM = xterm-color')
    SendCommand($'set env ROWS = {winheight(ptywin)}')
    SendCommand($'set env LINES = {winheight(ptywin)}')
    SendCommand($'set env COLUMNS = {winwidth(ptywin)}')
    SendCommand($'set env COLORS = {&t_Co}')
    SendCommand($'set env VIM_TERMINAL = {v:version}')
  else
    # TODO: open a new terminal, get the tty name, pass on to gdb
    SendCommand('show inferior-tty')
  endif
  SendCommand('set print pretty on')
  SendCommand('set breakpoint pending on')

  # Set arguments to be run
  if len(proc_args)
    SendCommand($'set args {join(proc_args)}')
  endif

  StartDebugCommon(dict)
  startinsert
enddef

def StartDebugCommon(dict: dict<any>)
  # Sign used to highlight the line where the program has stopped.
  # There can be only one.
  sign_define('debugPC', {linehl: 'debugPC'})

  # Install debugger commands in the text window.
  win_gotoid(sourcewin)
  InstallCommands()
  win_gotoid(gdbwin)

  # Enable showing a balloon with eval info
  if has("balloon_eval") || has("balloon_eval_term")
    set balloonexpr=TermDebugBalloonExpr()
    if has("balloon_eval")
      set ballooneval
    endif
    if has("balloon_eval_term")
      set balloonevalterm
    endif
  endif

  augroup TermDebug
    au BufRead * BufRead()
    au BufUnload * BufUnloaded()
    au OptionSet background Highlight(0, v:option_old, v:option_new)
  augroup END

  # Run the command if the bang attribute was given and got to the debug
  # window.
  if get(dict, 'bang', 0)
    SendResumingCommand('-exec-run')
    win_gotoid(ptywin)
  endif
enddef

# Send a command to gdb.  "cmd" is the string without line terminator.
def SendCommand(cmd: string)
  ch_log($'sending to gdb: {cmd}')
  if way == 'prompt'
    ch_sendraw(gdb_channel, $"{cmd}\n")
  else
    term_sendkeys(commbufnr, $"{cmd}\r")
  endif
enddef

# Interrupt or stop the program
def StopCommand()
  if way == 'prompt'
    PromptInterrupt()
  else
    SendCommand('-exec-interrupt')
  endif
enddef

# Continue the program
def ContinueCommand()
  if way == 'prompt'
    SendCommand('continue')
  else
    # using -exec-continue results in CTRL-C in the gdb window not working,
    # communicating via commbuf (= use of SendCommand) has the same result
    SendCommand('-exec-continue')
    # command Continue  term_sendkeys(gdbbuf, "continue\r")
  endif
enddef

# This is global so that a user can create their mappings with this.
def g:TermDebugSendCommand(cmd: string)
  if way == 'prompt'
    ch_sendraw(gdb_channel, $"{cmd}\n")
  else
    var do_continue = false
    if !stopped
      do_continue = true
      StopCommand()
      sleep 10m
    endif
    # TODO: should we prepend CTRL-U to clear the command?
    term_sendkeys(gdbbufnr, $"{cmd}\r")
    if do_continue
      ContinueCommand()
    endif
  endif
enddef

# Send a command that resumes the program.  If the program isn't stopped the
# command is not sent (to avoid a repeated command to cause trouble).
# If the command is sent then reset stopped.
def SendResumingCommand(cmd: string)
  if stopped
    # reset stopped here, it may take a bit of time before we get a response
    stopped = false
    ch_log('assume that program is running after this command')
    SendCommand(cmd)
  else
    ch_log($'dropping command, program is running: {cmd}')
  endif
enddef

# Function called when entering a line in the prompt buffer.
def PromptCallback(text: string)
  SendCommand(text)
enddef

# Function called when pressing CTRL-C in the prompt buffer and when placing a
# breakpoint.
def PromptInterrupt()
  ch_log('Interrupting gdb')
  if has('win32')
    # Using job_stop() does not work on MS-Windows, need to send SIGTRAP to
    # the debugger program so that gdb responds again.
    if pid == 0
      Echoerr('Cannot interrupt gdb, did not find a process ID')
    else
      debugbreak(pid)
    endif
  else
    job_stop(gdbjob, 'int')
  endif
enddef

# Function called when gdb outputs text.
def GdbOutCallback(channel: channel, text: string)
  ch_log($'received from gdb: {text}')

  # Disassembly messages need to be forwarded as-is.
  if parsing_disasm_msg > 0
    CommOutput(channel, text)
    return
  endif

  # Drop the gdb prompt, we have our own.
  # Drop status and echo'd commands.
  if text == '(gdb) ' || text == '^done' ||
        (text[0] == '&' && text !~ '^&"disassemble')
    return
  endif

  var decoded_text = ''
  if text =~ '^\^error,msg='
    decoded_text = DecodeMessage(text[11 : ], false)
    if !empty(evalexpr) && decoded_text =~ 'A syntax error in expression, near\|No symbol .* in current context'
      # Silently drop evaluation errors.
      evalexpr = ''
      return
    endif
  elseif text[0] == '~'
    decoded_text = DecodeMessage(text[1 : ], false)
  else
    CommOutput(channel, text)
    return
  endif

  var curwinid = win_getid()
  win_gotoid(gdbwin)

  # Add the output above the current prompt.
  append(line('$') - 1, decoded_text)
  set modified

  win_gotoid(curwinid)
enddef

# Decode a message from gdb.  "quotedText" starts with a ", return the text up
# to the next unescaped ", unescaping characters:
# - remove line breaks (unless "literal" is true)
# - change \" to "
# - change \\t to \t (unless "literal" is true)
# - change \0xhh to \xhh (disabled for now)
# - change \ooo to octal
# - change \\ to \
def DecodeMessage(quotedText: string, literal: bool): string
  if quotedText[0] != '"'
    Echoerr($'DecodeMessage(): missing quote in {quotedText}')
    return ''
  endif
  var msg = quotedText
        ->substitute('^"\|[^\\]\zs".*', '', 'g')
        ->substitute('\\"', '"', 'g')
        #\ multi-byte characters arrive in octal form
        #\ NULL-values must be kept encoded as those break the string otherwise
        ->substitute('\\000', NullRepl, 'g')
        ->substitute('\\\(\o\o\o\)', (m) => nr2char(str2nr(m[1], 8)), 'g')
        # You could also  use ->substitute('\\\\\(\o\o\o\)', '\=nr2char(str2nr(submatch(1), 8))', "g")
        #\ Note: GDB docs also mention hex encodings - the translations below work
        #\       but we keep them out for performance-reasons until we actually see
        #\       those in mi-returns
        #\ \ ->substitute('\\0x\(\x\x\)', {-> eval('"\x' .. submatch(1) .. '"')}, 'g')
        #\ \ ->substitute('\\0x00', NullRepl, 'g')
        ->substitute('\\\\', '\', 'g')
        ->substitute(NullRepl, '\\000', 'g')
  if !literal
    return msg
      ->substitute('\\t', "\t", 'g')
      ->substitute('\\n', '', 'g')
  else
    return msg
  endif
enddef
const NullRepl = 'XXXNULLXXX'

# Extract the "name" value from a gdb message with fullname="name".
def GetFullname(msg: string): string
  if msg !~ 'fullname'
    return ''
  endif

  var name = DecodeMessage(substitute(msg, '.*fullname=', '', ''), true)
  if has('win32') && name =~ ':\\\\'
    # sometimes the name arrives double-escaped
    name = substitute(name, '\\\\', '\\', 'g')
  endif

  return name
enddef

# Extract the "addr" value from a gdb message with addr="0x0001234".
def GetAsmAddr(msg: string): string
  if msg !~ 'addr='
    return ''
  endif

  var addr = DecodeMessage(substitute(msg, '.*addr=', '', ''), false)
  return addr
enddef

def EndDebug(job: any, status: any)
  if exists('#User#TermdebugStopPre')
    doauto <nomodeline> User TermdebugStopPre
  endif

  if way == 'prompt'
    ch_log("Returning from EndDebug()")
  endif

  var curwinid = win_getid()
  CloseBuffers()
  running = false

  # Restore 'signcolumn' in all buffers for which it was set.
  win_gotoid(sourcewin)
  var was_buf = bufnr()
  for bufnr in signcolumn_buflist
    if bufexists(bufnr)
      exe $":{bufnr}buf"
      if exists('b:save_signcolumn')
        &signcolumn = b:save_signcolumn
        unlet b:save_signcolumn
      endif
    endif
  endfor
  if bufexists(was_buf)
    exe $":{was_buf}buf"
  endif

  DeleteCommands()

  win_gotoid(curwinid)

  &columns = saved_columns

  if has("balloon_eval") || has("balloon_eval_term")
    set balloonexpr=
    if has("balloon_eval")
      set noballooneval
    endif
    if has("balloon_eval_term")
      set noballoonevalterm
    endif
  endif

  if exists('#User#TermdebugStopPost')
    doauto <nomodeline> User TermdebugStopPost
  endif

  au! TermDebug
  g:termdebug_is_running = false
enddef

# Disassembly window - added by Michael Sartain
#
# - CommOutput: &"disassemble $pc\n"
# - CommOutput: ~"Dump of assembler code for function main(int, char**):\n"
# - CommOutput: ~"   0x0000555556466f69 <+0>:\tpush   rbp\n"
# ...
# - CommOutput: ~"   0x0000555556467cd0:\tpop    rbp\n"
# - CommOutput: ~"   0x0000555556467cd1:\tret    \n"
# - CommOutput: ~"End of assembler dump.\n"
# - CommOutput: ^done

# - CommOutput: &"disassemble $pc\n"
# - CommOutput: &"No function contains specified address.\n"
# - CommOutput: ^error,msg="No function contains specified address."
def HandleDisasmMsg(msg: string)
  if msg =~ '^\^done'
    var curwinid = win_getid()
    if win_gotoid(asmwin)
      silent! :%delete _
      setline(1, asm_lines)
      set nomodified
      set filetype=asm

      var lnum = search($'^{asm_addr}')
      if lnum != 0
        sign_unplace('TermDebug', {id: asm_id})
        sign_place(asm_id, 'TermDebug', 'debugPC', '%', {lnum: lnum})
      endif

      win_gotoid(curwinid)
    endif

    parsing_disasm_msg = 0
    asm_lines = []

  elseif msg =~ '^\^error,msg='
    if parsing_disasm_msg == 1
      # Disassemble call ran into an error. This can happen when gdb can't
      # find the function frame address, so let's try to disassemble starting
      # at current PC
      SendCommand('disassemble $pc,+100')
    endif
    parsing_disasm_msg = 0
  elseif msg =~ '^&"disassemble \$pc'
    if msg =~ '+100'
      # This is our second disasm attempt
      parsing_disasm_msg = 2
    endif
  elseif msg !~ '^&"disassemble'
    var value = substitute(msg, '^\~\"[ ]*', '', '')
     ->substitute('^=>[ ]*', '', '')
     ->substitute('\\n\"\r$', '', '')
     ->substitute('\\n\"$', '', '')
     ->substitute('\r', '', '')
     ->substitute('\\t', ' ', 'g')

    if value != '' || !empty(asm_lines)
      add(asm_lines, value)
    endif
  endif
enddef


def ParseVarinfo(varinfo: string): dict<any>
  var dict = {}
  var nameIdx = matchstrpos(varinfo, '{name="\([^"]*\)"')
  dict['name'] = varinfo[nameIdx[1] + 7 : nameIdx[2] - 2]
  var typeIdx = matchstrpos(varinfo, ',type="\([^"]*\)"')
  # 'type' maybe is a url-like string,
  # try to shorten it and show only the /tail
  dict['type'] = (varinfo[typeIdx[1] + 7 : typeIdx[2] - 2])->fnamemodify(':t')
  var valueIdx = matchstrpos(varinfo, ',value="\(.*\)"}')
  if valueIdx[1] == -1
    dict['value'] = 'Complex value'
  else
    dict['value'] = varinfo[valueIdx[1] + 8 : valueIdx[2] - 3]
  endif
  return dict
enddef

def HandleVariablesMsg(msg: string)
  var curwinid = win_getid()
  if win_gotoid(varwin)
    silent! :%delete _
    var spaceBuffer = 20
    var spaces = repeat(' ', 16)
    setline(1, $'Type{spaces}Name{spaces}Value')
    var cnt = 1
    var capture = '{name=".\{-}",\%(arg=".\{-}",\)\{0,1\}type=".\{-}"\%(,value=".\{-}"\)\{0,1\}}'
    var varinfo = matchstr(msg, capture, 0, cnt)

    while varinfo != ''
      var vardict = ParseVarinfo(varinfo)
      setline(cnt + 1, vardict['type'] ..
        repeat(' ', max([20 - len(vardict['type']), 1])) ..
        vardict['name'] ..
        repeat(' ', max([20 - len(vardict['name']), 1])) ..
        vardict['value'])
      cnt += 1
      varinfo = matchstr(msg, capture, 0, cnt)
    endwhile
  endif
  win_gotoid(curwinid)
enddef


# Handle a message received from gdb on the GDB/MI interface.
def CommOutput(chan: channel, message: string)
  # We may use the standard MI message formats? See #10300 on github that mentions
  # the following links:
  # https://sourceware.org/gdb/current/onlinedocs/gdb.html/GDB_002fMI-Input-Syntax.html#GDB_002fMI-Input-Syntax
  # https://sourceware.org/gdb/current/onlinedocs/gdb.html/GDB_002fMI-Output-Syntax.html#GDB_002fMI-Output-Syntax

  var msgs = split(message, "\r")

  var msg = ''
  for received_msg in msgs
    # remove prefixed NL
    if received_msg[0] == "\n"
      msg = received_msg[1 : ]
    else
      msg = received_msg
    endif

    if parsing_disasm_msg > 0
      HandleDisasmMsg(msg)
    elseif msg != ''
      if msg =~ '^\(\*stopped\|\*running\|=thread-selected\)'
        HandleCursor(msg)
      elseif msg =~ '^\^done,bkpt=' || msg =~ '^=breakpoint-created,'
        HandleNewBreakpoint(msg, 0)
      elseif msg =~ '^=breakpoint-modified,'
        HandleNewBreakpoint(msg, 1)
      elseif msg =~ '^=breakpoint-deleted,'
        HandleBreakpointDelete(msg)
      elseif msg =~ '^=thread-group-started'
        HandleProgramRun(msg)
      elseif msg =~ '^\^done,value='
        HandleEvaluate(msg)
      elseif msg =~ '^\^error,msg='
        HandleError(msg)
      elseif msg =~ '^&"disassemble'
        parsing_disasm_msg = 1
        asm_lines = []
        HandleDisasmMsg(msg)
      elseif msg =~ '^\^done,variables='
        HandleVariablesMsg(msg)
      endif
    endif
  endfor
enddef

def GotoProgram()
  if has('win32')
    if executable('powershell')
      system(printf('powershell -Command "add-type -AssemblyName microsoft.VisualBasic;[Microsoft.VisualBasic.Interaction]::AppActivate(%d);"', pid))
    endif
  else
    win_gotoid(ptywin)
  endif
enddef

# Install commands in the current window to control the debugger.
def InstallCommands()

  command -nargs=? Break  SetBreakpoint(<q-args>)
  command -nargs=? Tbreak  SetBreakpoint(<q-args>, true)
  command Clear  ClearBreakpoint()
  command Step  SendResumingCommand('-exec-step')
  command Over  SendResumingCommand('-exec-next')
  command -nargs=? Until  Until(<q-args>)
  command Finish  SendResumingCommand('-exec-finish')
  command -nargs=* Run  Run(<q-args>)
  command -nargs=* Arguments  SendResumingCommand('-exec-arguments ' .. <q-args>)
  command Stop StopCommand()
  command Continue ContinueCommand()

  command -nargs=* Frame  Frame(<q-args>)
  command -count=1 Up  Up(<count>)
  command -count=1 Down  Down(<count>)

  command -range -nargs=* Evaluate  Evaluate(<range>, <q-args>)
  command Gdb  win_gotoid(gdbwin)
  command Program  GotoProgram()
  command Source  GotoSourcewinOrCreateIt()
  command Asm  GotoAsmwinOrCreateIt()
  command Var  GotoVariableswinOrCreateIt()
  command Winbar  InstallWinbar(true)

  var map = true
  if exists('g:termdebug_config')
    map = get(g:termdebug_config, 'map_K', true)
  elseif exists('g:termdebug_map_K')
    map = g:termdebug_map_K
  endif

  if map
    if !empty(saved_K_map) && !saved_K_map.buffer || empty(saved_K_map)
      nnoremap K :Evaluate<CR>
    endif
  endif

  map = true
  if exists('g:termdebug_config')
    map = get(g:termdebug_config, 'map_plus', true)
  endif
  if map
    if !empty(saved_plus_map) && !saved_plus_map.buffer || empty(saved_plus_map)
      nnoremap <expr> + $'<Cmd>{v:count1}Up<CR>'
    endif
  endif

  map = true
  if exists('g:termdebug_config')
    map = get(g:termdebug_config, 'map_minus', true)
  endif
  if map
    if !empty(saved_minus_map) && !saved_minus_map.buffer || empty(saved_minus_map)
      nnoremap <expr> - $'<Cmd>{v:count1}Down<CR>'
    endif
  endif


  if has('menu') && &mouse != ''
    InstallWinbar(false)

    var pup = true
    if exists('g:termdebug_config')
      pup = get(g:termdebug_config, 'popup', true)
    elseif exists('g:termdebug_popup')
      pup = g:termdebug_popup
    endif

    if pup
      &mousemodel = 'popup_setpos'
      an 1.200 PopUp.-SEP3-	<Nop>
      an 1.210 PopUp.Set\ breakpoint	:Break<CR>
      an 1.220 PopUp.Clear\ breakpoint	:Clear<CR>
      an 1.230 PopUp.Run\ until		:Until<CR>
      an 1.240 PopUp.Evaluate		:Evaluate<CR>
    endif
  endif

enddef

# Install the window toolbar in the current window.
def InstallWinbar(force: bool)
  # install the window toolbar by default, can be disabled in the config
  var winbar = true
  if exists('g:termdebug_config')
    winbar = get(g:termdebug_config, 'winbar', true)
  endif

  if has('menu') && &mouse != '' && (winbar || force)
    nnoremenu WinBar.Step   :Step<CR>
    nnoremenu WinBar.Next   :Over<CR>
    nnoremenu WinBar.Finish :Finish<CR>
    nnoremenu WinBar.Cont   :Continue<CR>
    nnoremenu WinBar.Stop   :Stop<CR>
    nnoremenu WinBar.Eval   :Evaluate<CR>
    add(winbar_winids, win_getid())
  endif
enddef

# Delete installed debugger commands in the current window.
def DeleteCommands()
  delcommand Break
  delcommand Tbreak
  delcommand Clear
  delcommand Step
  delcommand Over
  delcommand Until
  delcommand Finish
  delcommand Run
  delcommand Arguments
  delcommand Stop
  delcommand Continue
  delcommand Frame
  delcommand Up
  delcommand Down
  delcommand Evaluate
  delcommand Gdb
  delcommand Program
  delcommand Source
  delcommand Asm
  delcommand Var
  delcommand Winbar


  if !empty(saved_K_map) && !saved_K_map.buffer
    mapset(saved_K_map)
  elseif empty(saved_K_map)
    silent! nunmap K
  endif

  if !empty(saved_plus_map) && !saved_plus_map.buffer
    mapset(saved_plus_map)
  elseif empty(saved_plus_map)
    silent! nunmap +
  endif

  if !empty(saved_minus_map) && !saved_minus_map.buffer
    mapset(saved_minus_map)
  elseif empty(saved_minus_map)
    silent! nunmap -
  endif


  if has('menu')
    # Remove the WinBar entries from all windows where it was added.
    var curwinid = win_getid()
    for winid in winbar_winids
      if win_gotoid(winid)
        aunmenu WinBar.Step
        aunmenu WinBar.Next
        aunmenu WinBar.Finish
        aunmenu WinBar.Cont
        aunmenu WinBar.Stop
        aunmenu WinBar.Eval
      endif
    endfor
    win_gotoid(curwinid)
    # winbar_winids = []

    &mousemodel = saved_mousemodel
    try
      aunmenu PopUp.-SEP3-
      aunmenu PopUp.Set\ breakpoint
      aunmenu PopUp.Clear\ breakpoint
      aunmenu PopUp.Run\ until
      aunmenu PopUp.Evaluate
    catch
      # ignore any errors in removing the PopUp menu
    endtry
  endif

  sign_unplace('TermDebug')
  breakpoints = {}
  breakpoint_locations = {}

  sign_undefine('debugPC')
  sign_undefine(BreakpointSigns->map("'debugBreakpoint' .. v:val"))
  BreakpointSigns = []
enddef


# :Until - Execute until past a specified position or current line
def Until(at: string)

  if stopped
    # reset stopped here, it may take a bit of time before we get a response
    stopped = false
    ch_log('assume that program is running after this command')

    # Use the fname:lnum format
    var AT = empty(at) ? $"{fnameescape(expand('%:p'))}:{line('.')}" : at
    SendCommand($'-exec-until {AT}')
  else
    ch_log('dropping command, program is running: exec-until')
  endif
enddef

# :Break - Set a breakpoint at the cursor position.
def SetBreakpoint(at: string, tbreak=false)
  # Setting a breakpoint may not work while the program is running.
  # Interrupt to make it work.
  var do_continue = false
  if !stopped
    do_continue = true
    StopCommand()
    sleep 10m
  endif

  # Use the fname:lnum format, older gdb can't handle --source.
  var AT = empty(at) ? $"{fnameescape(expand('%:p'))}:{line('.')}" : at
  var cmd = ''
  if tbreak
    cmd = $'-break-insert -t {AT}'
  else
    cmd = $'-break-insert {AT}'
  endif
  SendCommand(cmd)
  if do_continue
    ContinueCommand()
  endif
enddef

def ClearBreakpoint()
  var fname = fnameescape(expand('%:p'))
  var lnum = line('.')
  var bploc = printf('%s:%d', fname, lnum)
  var nr = 0
  if has_key(breakpoint_locations, bploc)
    var idx = 0
    for id in breakpoint_locations[bploc]
      if has_key(breakpoints, id)
        # Assume this always works, the reply is simply "^done".
        SendCommand($'-break-delete {id}')
        for subid in keys(breakpoints[id])
          sign_unplace('TermDebug',
            {id: Breakpoint2SignNumber(id, str2nr(subid))})
        endfor
        remove(breakpoints, id)
        remove(breakpoint_locations[bploc], idx)
        nr = id
        break
      else
        idx += 1
      endif
    endfor

    if nr != 0
      if empty(breakpoint_locations[bploc])
        remove(breakpoint_locations, bploc)
      endif
      echomsg $'Breakpoint {nr} cleared from line {lnum}.'
    else
      Echoerr($'Internal error trying to remove breakpoint at line {lnum}!')
    endif
  else
    echomsg $'No breakpoint to remove at line {lnum}.'
  endif
enddef

def Run(args: string)
  if args != ''
    SendResumingCommand($'-exec-arguments {args}')
  endif
  SendResumingCommand('-exec-run')
enddef

# :Frame - go to a specific frame in the stack
def Frame(arg: string)
  # Note: we explicit do not use mi's command
  # call SendCommand('-stack-select-frame "' . arg .'"')
  # as we only get a "done" mi response and would have to open the file
  # 'manually' - using cli command "frame" provides us with the mi response
  # already parsed and allows for more formats
  if arg =~ '^\d\+$' || arg == ''
    # specify frame by number
    SendCommand($'-interpreter-exec mi "frame {arg}"')
  elseif arg =~ '^0x[0-9a-fA-F]\+$'
    # specify frame by stack address
    SendCommand($'-interpreter-exec mi "frame address {arg}"')
  else
    # specify frame by function name
    SendCommand($'-interpreter-exec mi "frame function {arg}"')
  endif
enddef

# :Up - go count frames in the stack "higher"
def Up(count: number)
  # the 'correct' one would be -stack-select-frame N, but we don't know N
  SendCommand($'-interpreter-exec console "up {count}"')
enddef

# :Down - go count frames in the stack "below"
def Down(count: number)
  # the 'correct' one would be -stack-select-frame N, but we don't know N
  SendCommand($'-interpreter-exec console "down {count}"')
enddef

def SendEval(expr: string)
  # check for "likely" boolean expressions, in which case we take it as lhs
  var exprLHS = substitute(expr, ' *=.*', '', '')
  if expr =~ "[=!<>]="
    exprLHS = expr
  endif

  # encoding expression to prevent bad errors
  var expr_escaped = expr
    ->substitute('\\', '\\\\', 'g')
    ->substitute('"', '\\"', 'g')
  SendCommand($'-data-evaluate-expression "{expr_escaped}"')
  evalexpr = exprLHS
enddef

# :Evaluate - evaluate what is specified / under the cursor
def Evaluate(range: number, arg: string)
  var expr = GetEvaluationExpression(range, arg)
  echom $"expr: {expr}"
  ignoreEvalError = false
  SendEval(expr)
enddef


# get what is specified / under the cursor
def GetEvaluationExpression(range: number, arg: string): string
  var expr = ''
  if arg != ''
    # user supplied evaluation
    expr = CleanupExpr(arg)
    # DSW: replace "likely copy + paste" assignment
    expr = substitute(expr, '"\([^"]*\)": *', '\1=', 'g')
  elseif range == 2
    # no evaluation but provided but range set
    var pos = getcurpos()
    var regst = getreg('v', 1, 1)
    var regt = getregtype('v')
    normal! gv"vy
    expr = CleanupExpr(@v)
    setpos('.', pos)
    setreg('v', regst, regt)
  else
    # no evaluation provided: get from C-expression under cursor
    # TODO: allow filetype specific lookup #9057
    expr = expand('<cexpr>')
  endif
  return expr
enddef

# clean up expression that may get in because of range
# (newlines and surrounding whitespace)
# As it can also be specified via ex-command for assignments this function
# may not change the "content" parts (like replacing contained spaces)
def CleanupExpr(passed_expr: string): string
  # replace all embedded newlines/tabs/...
  var expr = substitute(passed_expr, '\_s', ' ', 'g')

  if &filetype ==# 'cobol'
    # extra cleanup for COBOL:
    # - a semicolon nmay be used instead of a space
    # - a trailing comma or period is ignored as it commonly separates/ends
    #   multiple expr
    expr = substitute(expr, ';', ' ', 'g')
    expr = substitute(expr, '[,.]\+ *$', '', '')
  endif

  # get rid of leading and trailing spaces
  expr = substitute(expr, '^ *', '', '')
  expr = substitute(expr, ' *$', '', '')
  return expr
enddef

def HandleEvaluate(msg: string)
  var value = msg
        ->substitute('.*value="\(.*\)"', '\1', '')
        ->substitute('\\"', '"', 'g')
        ->substitute('\\\\', '\\', 'g')
        #\ multi-byte characters arrive in octal form, replace everything but NULL values
        ->substitute('\\000', NullRepl, 'g')
        ->substitute('\\\(\o\o\o\)', (m) => nr2char(str2nr(m[1], 8)), 'g')
        #\ Note: GDB docs also mention hex encodings - the translations below work
        #\       but we keep them out for performance-reasons until we actually see
        #\       those in mi-returns
        #\ ->substitute('\\0x00', NullRep, 'g')
        #\ ->substitute('\\0x\(\x\x\)', {-> eval('"\x' .. submatch(1) .. '"')}, 'g')
        ->substitute(NullRepl, '\\000', 'g')
  if evalFromBalloonExpr
    if empty(evalFromBalloonExprResult)
      evalFromBalloonExprResult = $'{evalexpr}: {value}'
    else
      evalFromBalloonExprResult ..= $' = {value}'
    endif
    balloon_show(evalFromBalloonExprResult)
  else
    echomsg $'"{evalexpr}": {value}'
  endif

  if evalexpr[0] != '*' && value =~ '^0x' && value != '0x0' && value !~ '"$'
    # Looks like a pointer, also display what it points to.
    ignoreEvalError = true
    SendEval($'*{evalexpr}')
  else
    evalFromBalloonExpr = false
  endif
enddef


# Show a balloon with information of the variable under the mouse pointer,
# if there is any.
def TermDebugBalloonExpr(): string
  if v:beval_winid != sourcewin
    return ''
  endif
  if !stopped
    # Only evaluate when stopped, otherwise setting a breakpoint using the
    # mouse triggers a balloon.
    return ''
  endif
  evalFromBalloonExpr = true
  evalFromBalloonExprResult = ''
  ignoreEvalError = true
  var expr = CleanupExpr(v:beval_text)
  SendEval(expr)
  return ''
enddef

# Handle an error.
def HandleError(msg: string)
  if ignoreEvalError
    # Result of SendEval() failed, ignore.
    ignoreEvalError = false
    evalFromBalloonExpr = true
    return
  endif
  var msgVal = substitute(msg, '.*msg="\(.*\)"', '\1', '')
  Echoerr(substitute(msgVal, '\\"', '"', 'g'))
enddef

def GotoSourcewinOrCreateIt()
  if !win_gotoid(sourcewin)
    new
    sourcewin = win_getid()
    InstallWinbar(false)
  endif
enddef


def GetDisasmWindow(): number
  if exists('g:termdebug_config')
    return get(g:termdebug_config, 'disasm_window', 0)
  endif
  if exists('g:termdebug_disasm_window')
    return g:termdebug_disasm_window
  endif
  return 0
enddef

def GetDisasmWindowHeight(): number
  if exists('g:termdebug_config')
    return get(g:termdebug_config, 'disasm_window_height', 0)
  endif
  if exists('g:termdebug_disasm_window') && g:termdebug_disasm_window > 1
    return g:termdebug_disasm_window
  endif
  return 0
enddef

def GotoAsmwinOrCreateIt()
  var mdf = ''
  if !win_gotoid(asmwin)
    if win_gotoid(sourcewin)
      # 60 is approx spaceBuffer * 3
      if winwidth(0) > (78 + 60)
        mdf = 'vert'
        exe $'{mdf} :60new'
      else
        exe 'rightbelow new'
      endif
    else
      exe 'new'
    endif

    asmwin = win_getid()

    setlocal nowrap
    setlocal number
    setlocal noswapfile
    setlocal buftype=nofile
    setlocal bufhidden=wipe
    setlocal signcolumn=no
    setlocal modifiable

    if asmbufnr > 0 && bufexists(asmbufnr)
      exe $'buffer {asmbufnr}'
    else
      exe $"silent file {asmbufname}"
      asmbufnr = bufnr(asmbufname)
    endif

    if mdf != 'vert' && GetDisasmWindowHeight() > 0
      exe $'resize {GetDisasmWindowHeight()}'
    endif
  endif

  if asm_addr != ''
    var lnum = search($'^{asm_addr}')
    if lnum == 0
      if stopped
        SendCommand('disassemble $pc')
      endif
    else
      sign_unplace('TermDebug', {id: asm_id})
      sign_place(asm_id, 'TermDebug', 'debugPC', '%', {lnum: lnum})
    endif
  endif
enddef

def GetVariablesWindow(): number
  if exists('g:termdebug_config')
    return get(g:termdebug_config, 'variables_window', 0)
  endif
  if exists('g:termdebug_disasm_window')
    return g:termdebug_variables_window
  endif
  return 0
enddef

def GetVariablesWindowHeight(): number
  if exists('g:termdebug_config')
    return get(g:termdebug_config, 'variables_window_height', 0)
  endif
  if exists('g:termdebug_variables_window') && g:termdebug_variables_window > 1
    return g:termdebug_variables_window
  endif
  return 0
enddef


def GotoVariableswinOrCreateIt()
  var mdf = ''
  if !win_gotoid(varwin)
    if win_gotoid(sourcewin)
      # 60 is approx spaceBuffer * 3
      if winwidth(0) > (78 + 60)
        mdf = 'vert'
        exe $'{mdf} :60new'
      else
        exe 'rightbelow new'
      endif
    else
      exe 'new'
    endif

    varwin = win_getid()

    setlocal nowrap
    setlocal noswapfile
    setlocal buftype=nofile
    setlocal bufhidden=wipe
    setlocal signcolumn=no
    setlocal modifiable

    # If exists, then open, otherwise create
    if varbufnr > 0 && bufexists(varbufnr)
      exe $'buffer {varbufnr}'
    else
      exe $"silent file {varbufname}"
      varbufnr = bufnr(varbufname)
    endif

    if mdf != 'vert' && GetVariablesWindowHeight() > 0
      exe $'resize {GetVariablesWindowHeight()}'
    endif
  endif

  if running
    SendCommand('-stack-list-variables 2')
  endif
enddef

# Handle stopping and running message from gdb.
# Will update the sign that shows the current position.
def HandleCursor(msg: string)
  var wid = win_getid()

  if msg =~ '^\*stopped'
    ch_log('program stopped')
    stopped = true
    if msg =~ '^\*stopped,reason="exited-normally"'
      running = false
    endif
  elseif msg =~ '^\*running'
    ch_log('program running')
    stopped = false
    running = true
  endif

  var fname = ''
  if msg =~ 'fullname='
    fname = GetFullname(msg)
  endif

  if msg =~ 'addr='
    var asm_addr_local = GetAsmAddr(msg)
    if asm_addr_local != ''
      asm_addr = asm_addr_local

      var curwinid = win_getid()
      var lnum = 0
      if win_gotoid(asmwin)
        lnum = search($'^{asm_addr}')
        if lnum == 0
          SendCommand('disassemble $pc')
        else
          sign_unplace('TermDebug', {id: asm_id})
          sign_place(asm_id, 'TermDebug', 'debugPC', '%', {lnum: lnum})
        endif

        win_gotoid(curwinid)
      endif
    endif
  endif

  if running && stopped && bufwinnr(varbufname) != -1
    SendCommand('-stack-list-variables 2')
  endif

  if msg =~ '^\(\*stopped\|=thread-selected\)' && filereadable(fname)
    var lnum = substitute(msg, '.*line="\([^"]*\)".*', '\1', '')
    if lnum =~ '^[0-9]*$'
      GotoSourcewinOrCreateIt()
      if expand('%:p') != fnamemodify(fname, ':p')
        echomsg $"different fname: '{expand('%:p')}' vs '{fnamemodify(fname, ':p')}'"
        augroup Termdebug
          # Always open a file read-only instead of showing the ATTENTION
          # prompt, since it is unlikely we want to edit the file.
          # The file may be changed but not saved, warn for that.
          au SwapExists * echohl WarningMsg
            | echo 'Warning: file is being edited elsewhere'
            | echohl None
            | v:swapchoice = 'o'
        augroup END
        if &modified
          # TODO: find existing window
          exe $'split {fnameescape(fname)}'
          sourcewin = win_getid()
          InstallWinbar(false)
        else
          exe $'edit {fnameescape(fname)}'
        endif
        augroup Termdebug
          au! SwapExists
        augroup END
      endif
      exe $":{lnum}"
      normal! zv
      sign_unplace('TermDebug', {id: pc_id})
      sign_place(pc_id, 'TermDebug', 'debugPC', fname,
            {lnum: str2nr(lnum), priority: 110})
      if !exists('b:save_signcolumn')
        b:save_signcolumn = &signcolumn
        add(signcolumn_buflist, bufnr())
      endif
      setlocal signcolumn=yes
    endif
  elseif !stopped || fname != ''
    sign_unplace('TermDebug', {id: pc_id})
  endif

  win_gotoid(wid)
enddef

# Create breakpoint sign
def CreateBreakpoint(id: number, subid: number, enabled: string)
  var nr = printf('%d.%d', id, subid)
  if index(BreakpointSigns, nr) == -1
    add(BreakpointSigns, nr)
    var hiName = ''
    if enabled == "n"
      hiName = "debugBreakpointDisabled"
    else
      hiName = "debugBreakpoint"
    endif
    var label = ''
    if exists('g:termdebug_config') && has_key(g:termdebug_config, 'sign')
      label = g:termdebug_config['sign']
    else
      label = printf('%02X', id)
      if id > 255
        label = 'F+'
      endif
    endif
    sign_define($'debugBreakpoint{nr}',
      {text: slice(label, 0, 2),
        texthl: hiName})
  endif
enddef

def SplitMsg(str: string): list<string>
  return split(str, '{.\{-}}\zs')
enddef


# Handle setting a breakpoint
# Will update the sign that shows the breakpoint
def HandleNewBreakpoint(msg: string, modifiedFlag: any)
  var nr = ''

  if msg !~ 'fullname='
    # a watch or a pending breakpoint does not have a file name
    if msg =~ 'pending='
      nr = substitute(msg, '.*number=\"\([0-9.]*\)\".*', '\1', '')
      var target = substitute(msg, '.*pending=\"\([^"]*\)\".*', '\1', '')
      echomsg $'Breakpoint {nr} ({target}) pending.'
    endif
    return
  endif

  for mm in SplitMsg(msg)
    var fname = GetFullname(mm)
    if empty(fname)
      continue
    endif
    nr = substitute(mm, '.*number="\([0-9.]*\)\".*', '\1', '')
    if empty(nr)
      return
    endif

    # If "nr" is 123 it becomes "123.0" and subid is "0".
    # If "nr" is 123.4 it becomes "123.4.0" and subid is "4"; "0" is discarded.
    var [id, subid; _] = map(split(nr .. '.0', '\.'), 'str2nr(v:val) + 0')
    # var [id, subid; _] = map(split(nr .. '.0', '\.'), 'v:val + 0')
    var enabled = substitute(mm, '.*enabled="\([yn]\)".*', '\1', '')
    CreateBreakpoint(id, subid, enabled)

    var entries = {}
    var entry = {}
    if has_key(breakpoints, id)
      entries = breakpoints[id]
    else
      breakpoints[id] = entries
    endif
    if has_key(entries, subid)
      entry = entries[subid]
    else
      entries[subid] = entry
    endif

    var lnum = str2nr(substitute(mm, '.*line="\([^"]*\)".*', '\1', ''))
    entry['fname'] = fname
    entry['lnum'] = lnum

    var bploc = printf('%s:%d', fname, lnum)
    if !has_key(breakpoint_locations, bploc)
      breakpoint_locations[bploc] = []
    endif
    breakpoint_locations[bploc] += [id]

    var posMsg = ''
    if bufloaded(fname)
      PlaceSign(id, subid, entry)
      posMsg = $' at line {lnum}.'
    else
      posMsg = $' in {fname} at line {lnum}.'
    endif
    var actionTaken = ''
    if !modifiedFlag
      actionTaken = 'created'
    elseif enabled == 'n'
      actionTaken = 'disabled'
    else
      actionTaken = 'enabled'
    endif
    echom $'Breakpoint {nr} {actionTaken}{posMsg}'
  endfor
enddef


def PlaceSign(id: number, subid: number, entry: dict<any>)
  var nr = printf('%d.%d', id, subid)
  sign_place(Breakpoint2SignNumber(id, subid), 'TermDebug',
    $'debugBreakpoint{nr}', entry['fname'],
    {lnum: entry['lnum'], priority: 110})
  entry['placed'] = 1
enddef

# Handle deleting a breakpoint
# Will remove the sign that shows the breakpoint
def HandleBreakpointDelete(msg: string)
  var id = substitute(msg, '.*id="\([0-9]*\)\".*', '\1', '')
  if empty(id)
    return
  endif
  if has_key(breakpoints, id)
    for [subid, entry] in items(breakpoints[id])
      if has_key(entry, 'placed')
        sign_unplace('TermDebug',
          {id: Breakpoint2SignNumber(str2nr(id), str2nr(subid))})
        remove(entry, 'placed')
      endif
    endfor
    remove(breakpoints, id)
    echomsg $'Breakpoint {id} cleared.'
  endif
enddef

# Handle the debugged program starting to run.
# Will store the process ID in pid
def HandleProgramRun(msg: string)
  var nr = str2nr(substitute(msg, '.*pid="\([0-9]*\)\".*', '\1', ''))
  if nr == 0
    return
  endif
  pid = nr
  ch_log($'Detected process ID: {pid}')
enddef

# Handle a BufRead autocommand event: place any signs.
def BufRead()
  var fname = expand('<afile>:p')
  for [id, entries] in items(breakpoints)
    for [subid, entry] in items(entries)
      if entry['fname'] == fname
        PlaceSign(str2nr(id), str2nr(subid), entry)
      endif
    endfor
  endfor
enddef

# Handle a BufUnloaded autocommand event: unplace any signs.
def BufUnloaded()
  var fname = expand('<afile>:p')
  for [id, entries] in items(breakpoints)
    for [subid, entry] in items(entries)
      if entry['fname'] == fname
        entry['placed'] = 0
      endif
    endfor
  endfor
enddef

InitHighlight()
InitAutocmd()

# vim: sw=2 sts=2 et
