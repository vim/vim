vim9script

# Vim runtime support library
#
# Maintainer:   The Vim Project <https://github.com/vim/vim>
# Last Change:  2025 Aug 15

export def IsSafeExecutable(filetype: string, executable: string): bool
  if empty(exepath(executable))
    return v:false
  endif
  var cwd = getcwd()
  return get(g:, filetype .. '_exec', get(g:, 'plugin_exec', 0))
    && (fnamemodify(exepath(executable), ':p:h') !=# cwd
      || (split($PATH, has('win32') ? ';' : ':')->index(cwd) != -1
        && cwd != '.'))
enddef

def Redir(): string
  if get(g:, 'netrw_suppress_gx_mesg', true)
    if &srr =~# "%s"
      return printf(&srr, has("win32") ? "nul" : "/dev/null")
    elseif &srr =~# '>&\?$'
      return &srr .. (has("win32") ? "nul" : "/dev/null")
    else
      return &srr .. (has("win32") ? "> nul" : "> /dev/null")
    endif
  endif
  return ''
enddef

if has('unix')
  if has('win32unix')
    # Cygwin provides cygstart
    if executable('cygstart')
      export def Launch(args: string)
        execute $'silent ! cygstart --hide {args} {Redir()}' | redraw!
      enddef
    elseif !empty($MSYSTEM) && executable('start')
      # MSYS2/Git Bash comes by default without cygstart; see
      # https://www.msys2.org/wiki/How-does-MSYS2-differ-from-Cygwin
      # Instead it provides /usr/bin/start script running `cmd.exe //c start`
      # Adding "" //b` sets void title, hides cmd window and blocks path conversion
      # of /b to \b\ " by MSYS2; see https://www.msys2.org/docs/filesystem-paths/
      export def Launch(args: string)
        execute $'silent !start "" //b {args} {Redir()}' | redraw!
      enddef
    else
      # imitate /usr/bin/start script for other environments and hope for the best
      export def Launch(args: string)
        execute $'silent !cmd /c start "" /b {args} {Redir()}' | redraw!
      enddef
    endif
  elseif exists('$WSL_DISTRO_NAME') # use cmd.exe to start GUI apps in WSL
    export def Launch(args: string)
      const command = (args =~? '\v<\f+\.(exe|com|bat|cmd)>')
        ? $'cmd.exe /c start /b {args} {Redir()}'
        : $'nohup {args} {Redir()} &'
      execute $'silent ! {command}' | redraw!
    enddef
  else
    export def Launch(args: string)
      const fork = has('gui_running') ? '' : '&'
      execute $':silent ! nohup {args} {Redir()} {fork}' | redraw!
    enddef
  endif
elseif has('win32')
  export def Launch(args: string)
    const shell = (&shell =~? '\<cmd\.exe\>') ? '' : 'cmd.exe /c'
    const quotes = empty(shell) ? '' : '""'
    execute $'silent ! {shell} start {quotes} /b {args} {Redir()}' | redraw!
  enddef
else
  export def Launch(dummy: string)
    echom 'No common launcher found'
  enddef
endif

var os_viewer = null_string
# Git Bash
if has('win32unix')
  # (cyg)start suffices
  os_viewer = ''
# Windows / WSL
elseif executable('explorer.exe')
  os_viewer = 'explorer.exe'
# Linux / BSD
elseif executable('xdg-open')
  os_viewer = 'xdg-open'
# MacOS
elseif executable('open')
  os_viewer = 'open'
endif

def Viewer(): string
  # g:Openprg could be a string of program + its arguments, test if first
  # argument is executable
  var user_viewer = get(g:, "Openprg", get(g:, "netrw_browsex_viewer", ""))

  # Take care of an off-by-one check for "for" too
  if executable(trim(user_viewer))
    return user_viewer
  endif

  var args = split(user_viewer, '\s\+\zs')
  var viewer = get(args, 0, '')

  for arg in args[1 :]
    if executable(trim(viewer))
      return user_viewer
    endif

    viewer ..= arg
  endfor

  if os_viewer == null
    echoerr "No program to open this path found. See :help Open for more information."
  endif

  return os_viewer
enddef

export def Open(file: string)
  # disable shellslash for shellescape, required on Windows #17995
  if exists('+shellslash') && &shellslash
    &shellslash = false
    defer setbufvar('%', '&shellslash', true)
  endif
  Launch($"{Viewer()} {shellescape(file, 1)}")
enddef

# Uncomment this line to check for compilation errors early
# defcompile

# vim: ts=8 sts=2 sw=2 et
