source shared.vim
source term_util.vim

command -nargs=1 MissingFeature throw 'Skipped: ' .. <args> .. ' feature missing'

" Command to check for the presence of a feature.
command -nargs=1 CheckFeature call CheckFeature(<f-args>)
func CheckFeature(name)
  if !has(a:name, 1)
    throw 'Checking for non-existent feature ' .. a:name
  endif
  if !has(a:name)
    MissingFeature a:name
  endif
endfunc

" Command to check for the presence of a working option.
command -nargs=1 CheckOption call CheckOption(<f-args>)
func CheckOption(name)
  if !exists('&' .. a:name)
    throw 'Checking for non-existent option ' .. a:name
  endif
  if !exists('+' .. a:name)
    throw 'Skipped: ' .. a:name .. ' option not supported'
  endif
endfunc

" Command to check for the presence of a built-in function.
command -nargs=1 CheckFunction call CheckFunction(<f-args>)
func CheckFunction(name)
  if !exists('?' .. a:name)
    throw 'Checking for non-existent function ' .. a:name
  endif
  if !exists('*' .. a:name)
    throw 'Skipped: ' .. a:name .. ' function missing'
  endif
endfunc

" Command to check for the presence of an Ex command
command -nargs=1 CheckCommand call CheckCommand(<f-args>)
func CheckCommand(name)
  if !exists(':' .. a:name)
    throw 'Skipped: ' .. a:name .. ' command not supported'
  endif
endfunc

" Command to check for the presence of a shell command
command -nargs=1 CheckExecutable call CheckExecutable(<f-args>)
func CheckExecutable(name)
  if !executable(a:name)
    throw 'Skipped: ' .. a:name .. ' program not executable'
  endif
endfunc

" Command to check for the presence of python.  Argument should have been
" obtained with PythonProg()
func CheckPython(name)
  if a:name == ''
    throw 'Skipped: python command not available'
  endif
endfunc

" Command to check for running on MS-Windows
command CheckMSWindows call CheckMSWindows()
func CheckMSWindows()
  if !has('win32')
    throw 'Skipped: only works on MS-Windows'
  endif
endfunc

" Command to check for NOT running on MS-Windows
command CheckNotMSWindows call CheckNotMSWindows()
func CheckNotMSWindows()
  if has('win32')
    throw 'Skipped: does not work on MS-Windows'
  endif
endfunc

" Command to check for running on Unix
command CheckUnix call CheckUnix()
func CheckUnix()
  if !has('unix')
    throw 'Skipped: only works on Unix'
  endif
endfunc

" Command to check for not running on a BSD system.
" TODO: using this checks should not be needed
command CheckNotBSD call CheckNotBSD()
func CheckNotBSD()
  if has('bsd')
    throw 'Skipped: does not work on BSD'
  endif
endfunc

" Command to check that making screendumps is supported.
" Caller must source screendump.vim
command CheckScreendump call CheckScreendump()
func CheckScreendump()
  if !CanRunVimInTerminal()
    throw 'Skipped: cannot make screendumps'
  endif
endfunc

" Command to check that we can Run Vim in a terminal window
command CheckRunVimInTerminal call CheckRunVimInTerminal()
func CheckRunVimInTerminal()
  if !CanRunVimInTerminal()
    throw 'Skipped: cannot run Vim in a terminal window'
  endif
endfunc

" Command to check that we can run the GUI
command CheckCanRunGui call CheckCanRunGui()
func CheckCanRunGui()
  if !has('gui') || ($DISPLAY == "" && !has('gui_running'))
    throw 'Skipped: cannot start the GUI'
  endif
endfunc

" Command to check that we are using the GUI
command CheckGui call CheckGui()
func CheckGui()
  if !has('gui_running')
    throw 'Skipped: only works in the GUI'
  endif
endfunc

" Command to check that not currently using the GUI
command CheckNotGui call CheckNotGui()
func CheckNotGui()
  if has('gui_running')
    throw 'Skipped: only works in the terminal'
  endif
endfunc

" Command to check that test is not running as root
command CheckNotRoot call CheckNotRoot()
func CheckNotRoot()
  if IsRoot()
    throw 'Skipped: cannot run test as root'
  endif
endfunc

" Command to check that the current language is English
command CheckEnglish call CheckEnglish()
func CheckEnglish()
  if v:lang != "C" && v:lang !~ '^[Ee]n'
      throw 'Skipped: only works in English language environment'
  endif
endfunc

" Command to check that loopback device has IPv6 address
command CheckIPv6 call CheckIPv6()
func CheckIPv6()
  if !has('ipv6')
    throw 'Skipped: cannot use IPv6 networking'
  endif
  if !exists('s:ipv6_loopback')
    let s:ipv6_loopback = s:CheckIPv6Loopback()
  endif
  if !s:ipv6_loopback
    throw 'Skipped: no IPv6 address for loopback device'
  endif
endfunc

func s:CheckIPv6Loopback()
  if has('win32')
    return system('netsh interface ipv6 show interface') =~? '\<Loopback\>'
  elseif filereadable('/proc/net/if_inet6')
    return (match(readfile('/proc/net/if_inet6'), '\slo$') >= 0)
  elseif executable('ifconfig')
    for dev in ['lo0', 'lo', 'loop']
      " NOTE: On SunOS, need specify address family 'inet6' to get IPv6 info.
      if system('ifconfig ' .. dev .. ' inet6 2>/dev/null') =~? '\<inet6\>'
            \ || system('ifconfig ' .. dev .. ' 2>/dev/null') =~? '\<inet6\>'
        return v:true
      endif
    endfor
  else
    " TODO: How to check it in other platforms?
  endif
  return v:false
endfunc

" vim: shiftwidth=2 sts=2 expandtab
