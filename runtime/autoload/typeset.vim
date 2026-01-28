vim9script

# Language:           Generic TeX typesetting engine
# Maintainer:         Nicola Vitacolonna <nvitacolonna@gmail.com>
# Latest Revision:    2026 Jan 10

# Constants and helpers {{{
const SLASH = !exists("+shellslash") || &shellslash ? '/' : '\'

def Echo(msg: string, mode: string, label: string)
  redraw
  echo "\r"
  execute 'echohl' mode
  echomsg $'[{label}] {msg}'
  echohl None
enddef

def EchoMsg(msg: string, label = 'Notice')
  Echo(msg, 'ModeMsg', label)
enddef

def EchoWarn(msg: string, label = 'Warning')
  Echo(msg, 'WarningMsg', label)
enddef

def EchoErr(msg: string, label = 'Error')
  Echo(msg, 'ErrorMsg', label)
enddef
# }}}

# Track jobs {{{
var running_jobs: dict<list<job>> = {}

def AddJob(label: string, j: job)
  if !has_key(running_jobs, label)
    running_jobs[label] = []
  endif

  add(running_jobs[label], j)
enddef

def RemoveJob(label: string, j: job)
  if has_key(running_jobs, label) && index(running_jobs[label], j) != -1
    remove(running_jobs[label], index(running_jobs[label], j))
  endif
enddef

def GetRunningJobs(label: string): list<job>
  return has_key(running_jobs, label) ? running_jobs[label] : []
enddef
# }}}

# Callbacks {{{
def ProcessOutput(qfid: number, wd: string, efm: string, ch: channel, msg: string)
  # Make sure the quickfix list still exists
  if getqflist({'id': qfid}).id != qfid
    EchoErr("Quickfix list not found, stopping the job")
    job_stop(ch_getjob(ch))
    return
  endif

  # Make sure the working directory is correct
  silent execute "lcd" wd
  setqflist([], 'a', {'id': qfid, 'lines': [msg], 'efm': efm})
  silent lcd -
enddef

def CloseCb(ch: channel)
  job_status(ch_getjob(ch)) # Trigger exit_cb's callback
enddef

def ExitCb(label: string, jobid: job, exitStatus: number)
  RemoveJob(label, jobid)

  if exitStatus == 0
    botright cwindow
    EchoMsg('Success!', label)
  elseif exitStatus < 0
    EchoWarn('Job terminated', label)
  else
    botright copen
    wincmd p
    EchoWarn('There are errors.', label)
  endif
enddef
# }}}

# Create a new empty quickfix list at the end of the stack and return its id {{{
def NewQuickfixList(path: string): number
  if setqflist([], ' ', {'nr': '$', 'title': path}) == -1
    return -1
  endif

  return getqflist({'nr': '$', 'id': 0}).id
enddef
# }}}

# Public interface {{{
# When a TeX document is split into several source files, each source file
# may contain a "magic line" specifying the "root" file, e.g.:
#
#   % !TEX root = main.tex
#
# Using this line, Vim can know which file to typeset even if the current
# buffer is different from main.tex.
#
# This function searches for the magic line in the first ten lines of the
# given buffer, and returns the full path of the root document.
export def FindRootDocument(bufname: string = bufname("%")): string
  var docpath = fnamemodify(bufname, ":p")
  var bufnr = bufnr(bufname)
  var header: list<string>
  var rootpath = docpath

  if bufexists(bufnr)
    header = getbufline(bufnr, 1, 10)
  elseif filereadable(bufname)
    header = readfile(bufname, "", 10)
  else
    return simplify(rootpath)
  endif

  # Search for magic line `% !TEX root = ...` in the first ten lines
  var idx = match(header, '^\s*%\s\+!TEX\s\+root\s*=\s*\S')

  if idx > -1
    rootpath = matchstr(header[idx], '!TEX\s\+root\s*=\s*\zs.*$')

    if !isabsolutepath(rootpath) # Path is relative to the buffer's path
      rootpath = fnamemodify(docpath, ":h") .. SLASH .. rootpath
    endif
  endif

  return simplify(rootpath)
enddef

# ConTeXt documents may specify an output directory in a comment using the
# following syntax:
#
# runpath=texruns:<output directory>
#
# This function looks for such a comment in the first ten lines of the given
# buffer, and returns the full path of the output directory. If the comment is
# not found then the output directory coincides with the directory of the
# buffer.
export def GetOutputDirectory(bufname: string = bufname("%")): string
  var basedir = fnamemodify(bufname, ':p:h')
  var bufnr = bufnr(bufname)
  var header: list<string>
  var outdir = basedir

  if bufexists(bufnr)
    header = getbufline(bufnr, 1, 10)
  elseif filereadable(bufname)
    header = readfile(bufname, "", 10)
  else
    return simplify(outdir)
  endif

  # Search for output path in the first ten lines
  var idx = match(header, '^\s*%.*\<runpath\s*=\s*texruns\s*:\s*\S')

  if idx > -1
    outdir = matchstr(header[idx], '\<runpath\s*=\s*texruns\s*:\s*\zs.*$')

    if !isabsolutepath(outdir) # Path is relative to the buffer's directory
      outdir = basedir .. SLASH .. outdir
    endif
  endif

  return simplify(outdir)
enddef

export def LogPath(bufname: string): string
  var rootdoc = FindRootDocument(bufname)
  var docname = fnamemodify(rootdoc, ":t:r")
  var outdir = GetOutputDirectory(rootdoc)

  if empty(docname) # Set an arbitrary name to avoid returning a dotfile (.log)
    docname = '[NotFound]'
  endif

  return $'{outdir}{SLASH}{docname}.log'
enddef

# Typeset the specified path
#
# Parameters:
#   label: a descriptive string used in messages to identify the kind of job
#   Cmd:   a function that takes the path of a document and returns the typesetting command
#   path:  the path of the document to be typeset. To avoid ambiguities, pass a *full* path.
#   efm:   the error format string to parse the output of the command.
#   env:   environment variables for the process (passed to job_start())
#
# Returns:
#   true if the job is started successfully;
#   false otherwise.
export def Typeset(
    label: string,
    Cmd:   func(string): list<string>,
    path:  string,
    efm:   string,
    env:   dict<string> = {}
    ): bool
  var fp   = fnamemodify(path, ':p')
  var wd   = fnamemodify(fp, ':h')
  var qfid = NewQuickfixList(fp)

  if qfid == -1
    EchoErr('Could not create quickfix list', label)
    return false
  endif

  if !filereadable(fp)
    EchoErr($'File not readable: {fp}', label)
    return false
  endif

  var jobid = job_start(Cmd(path), {
    env: env,
    cwd: wd,
    in_io: "null",
    callback: (c, m) => ProcessOutput(qfid, wd, efm, c, m),
    close_cb: CloseCb,
    exit_cb: (j, e) => ExitCb(label, j, e),
  })

  if job_status(jobid) ==# "fail"
    EchoErr("Failed to start job", label)
    return false
  endif

  AddJob(label, jobid)

  EchoMsg('Typesetting...', label)

  return true
enddef

export def JobStatus(label: string)
  EchoMsg($'Jobs still running: {len(GetRunningJobs(label))}', label)
enddef

export def StopJobs(label: string)
  for job in GetRunningJobs(label)
    job_stop(job)
  endfor

  EchoMsg('Done.', label)
enddef

# Typeset the specified buffer
#
# Parameters:
#   name:    a buffer's name. this may be empty to indicate the current buffer.
#   cmd:     a function that takes the path of a document and returns the typesetting command
#   label:   a descriptive string used in messages to identify the kind of job
#   env:     environment variables for the process (passed to job_start())
#
# Returns:
#   true if the job is started successfully;
#   false otherwise.
export def TypesetBuffer(
    name: string,
    Cmd: func(string): list<string>,
    env = {},
    label = 'Typeset'
    ): bool
  var bufname = bufname(name)

  if empty(bufname)
    EchoErr('Please save the buffer first.', label)
    return false
  endif

  var efm = getbufvar(bufnr(bufname), "&efm")
  var rootpath = FindRootDocument(bufname)

  return Typeset('ConTeXt', Cmd, rootpath, efm, env)
enddef
# }}}

# vim: sw=2 fdm=marker
