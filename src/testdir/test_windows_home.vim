" Test for $HOME on Windows.

if !has('win32')
  return
endif

func! s:home()
  let env = filter(split(system('set'), "\n"), 'v:val=="HOME"')
  return len(env) == 0 ? "" : env[0]
endfunction

func! Test_windows_home()
  let oldhome = s:home()
  try
    let $HOME = 'c:/WindowS/sysTem32'
    call assert_equal('c:\WindowS\sysTem32\foo', expand('~/foo'))
    call assert_equal('c:\WindowS\sysTem32\foo', expand('$HOME/foo'))
    call assert_equal('', s:home())
    if !has('channel')
      return
    endif
    let env = ''
    let job = job_start('cmd /c echo %HOME%', {'out_cb': {ch,x->[env,execute('let s:env=x')]}})
    sleep 1
    call assert_equal('', env)
  finally
    let $HOME = oldhome
  endtry
endfunc

call Test_windows_home()
