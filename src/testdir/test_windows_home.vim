" Test for $HOME on Windows.

if !has('win32')
  finish
endif

func s:home()
  let env = filter(split(system('set'), "\n"), 'v:val=~"^HOME="')
  return len(env) == 0 ? "" : substitute(env[0], '[^=]\+=', '', '')
endfunction

func Test_windows_home()
  let oldhome = s:home()
  if oldhome != ''
    return
  endif

  try
    " should not have HOME if not set yet
    let env = ''
    let job = job_start('cmd /c set', {'out_cb': {ch,x->[env,execute('let env=x')]}})
    sleep 1
    let env = filter(split(env, "\n"), 'v:val=="HOME"')
    let home = len(env) == 0 ? "" : env[0]
    call assert_equal('', home)

    let $HOME = 'c:/WindowS/sysTem32'
    call assert_equal('c:\WindowS\sysTem32\foo', expand('~/foo'))
    call assert_equal('c:\WindowS\sysTem32\foo', expand('$HOME/foo'))
    call assert_equal($HOME, s:home())
    if !has('channel')
      return
    endif
  finally
    let $HOME = oldhome
  endtry
endfunc

call Test_windows_home()
