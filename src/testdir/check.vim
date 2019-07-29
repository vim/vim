" Command to check for the presence of a feature.
command -nargs=1 CheckFeature call CheckFeature(<f-args>)
func CheckFeature(name)
  if !has(a:name)
    throw 'Skipped: ' .. a:name .. ' feature missing'
  endif
endfunc

" Command to check for the presence of a working option.
command -nargs=1 CheckOption call CheckOption(<f-args>)
func CheckOption(name)
  if !exists('+' .. a:name)
    throw 'Skipped: ' .. a:name .. ' option not supported'
  endif
endfunc

" Command to check for the presence of a function.
command -nargs=1 CheckFunction call CheckFunction(<f-args>)
func CheckFunction(name)
  if !exists('*' .. a:name)
    throw 'Skipped: ' .. a:name .. ' function missing'
  endif
endfunc

" Command to check for running on MS-Windows
command CheckMSWindows call CheckMSWindows()
func CheckMSWindows()
  if !has('win32')
    throw 'Skipped: only works on MS-Windows'
  endif
endfunc

" Command to check for running on Unix
command CheckUnix call CheckUnix()
func CheckUnix()
  if !has('unix')
    throw 'Skipped: only works on Unix'
  endif
endfunc
