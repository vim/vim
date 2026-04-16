" Test for "rvim" or "vim -Z"

"if has('win32') && has('gui')
"  " Win32 GUI shows a dialog instead of displaying the error in the last line.
"  finish
"endif

func Test_restricted_mode()
  let lines =<< trim END
    if has('lua')
      call assert_fails('lua print("Hello, Vim!")', 'E981:')
      call assert_fails('luado return "hello"', 'E981:')
      call assert_fails('luafile somefile', 'E981:')
      call assert_fails('call luaeval("expression")', 'E145:')
    endif

    if has('mzscheme')
      call assert_fails('mzscheme statement', 'E981:')
      call assert_fails('mzfile somefile', 'E981:')
      call assert_fails('call mzeval("expression")', 'E145:')
    endif

    if has('perl')
      " TODO: how to make Safe mode fail?
      " call assert_fails('perl system("ls")', 'E981:')
      " call assert_fails('perldo system("hello")', 'E981:')
      " call assert_fails('perlfile somefile', 'E981:')
      " call assert_fails('call perleval("system(\"ls\")")', 'E145:')
    endif

    if has('python')
      call assert_fails('python print "hello"', 'E981:')
      call assert_fails('pydo return "hello"', 'E981:')
      call assert_fails('pyfile somefile', 'E981:')
      call assert_fails('call pyeval("expression")', 'E145:')
    endif

    if has('python3')
      call assert_fails('py3 print "hello"', 'E981:')
      call assert_fails('py3do return "hello"', 'E981:')
      call assert_fails('py3file somefile', 'E981:')
      call assert_fails('call py3eval("expression")', 'E145:')
    endif

    if has('ruby')
      call assert_fails('ruby print "Hello"', 'E981:')
      call assert_fails('rubydo print "Hello"', 'E981:')
      call assert_fails('rubyfile somefile', 'E981:')
    endif

    if has('tcl')
      call assert_fails('tcl puts "Hello"', 'E981:')
      call assert_fails('tcldo puts "Hello"', 'E981:')
      call assert_fails('tclfile somefile', 'E981:')
    endif

    if has('clientserver')
      call assert_fails('let s=remote_peek(10)', 'E145:')
      call assert_fails('let s=remote_read(10)', 'E145:')
      call assert_fails('let s=remote_send("vim", "abc")', 'E145:')
      call assert_fails('let s=server2client(10, "abc")', 'E145:')
    endif

    if has('terminal')
      call assert_fails('terminal', 'E145:')
      call assert_fails('call term_start("vim")', 'E145:')
      call assert_fails('call term_dumpwrite(1, "Xfile")', 'E145:')
    endif

    if has('channel')
      call assert_fails("call ch_logfile('Xlog')", 'E145:')
      call assert_fails("call ch_open('localhost:8765')", 'E145:')
    endif

    if has('job')
      call assert_fails("call job_start('vim')", 'E145:')
    endif

    if has('unix') && has('libcall')
      call assert_fails("echo libcall('libc.so', 'getenv', 'HOME')", 'E145:')
    endif
    call assert_fails("call rename('a', 'b')", 'E145:')
    call assert_fails("call delete('Xfile')", 'E145:')
    call assert_fails("call mkdir('Xdir')", 'E145:')
    call assert_fails('!ls', 'E145:')
    call assert_fails('shell', 'E145:')
    call assert_fails('stop', 'E145:')
    call assert_fails('exe "normal \<C-Z>"', 'E145:')
    set insertmode
    call assert_fails('call feedkeys("\<C-Z>", "xt")', 'E145:')
    set insertmode&
    call assert_fails('suspend', 'E145:')
    call assert_fails('call system("ls")', 'E145:')
    call assert_fails('call systemlist("ls")', 'E145:')
    if has('unix')
      call assert_fails('cd `pwd`', 'E145:')
    endif

    call writefile(v:errors, 'Xresult')
    qa!
  END
  call writefile(lines, 'Xrestricted', 'D')
  if RunVim([], [], '-Z --clean -S Xrestricted')
    call assert_equal([], readfile('Xresult'))
  endif
  call delete('Xresult')
  if has('unix') && RunVimPiped([], [], '--clean -S Xrestricted', 'SHELL=/bin/false ')
    call assert_equal([], readfile('Xresult'))
  endif
  call delete('Xresult')
  if has('unix') && RunVimPiped([], [], '--clean -S Xrestricted', 'SHELL=/sbin/nologin')
    call assert_equal([], readfile('Xresult'))
  endif

  call delete('Xresult')
endfunc

" Test that external diff is blocked in restricted mode.
" Using :diffupdate with 'diffopt' excluding "internal" would call an external
" diff program via call_shell(), which must be blocked.
func Test_restricted_diff()
  let lines =<< trim END
    set diffopt=filler
    call writefile(['line1', 'line2'], 'Xrfile1', 'D')
    call writefile(['line1', 'line3'], 'Xrfile2', 'D')
    edit Xrfile1
    diffthis
    split Xrfile2
    diffthis
    call assert_fails('diffupdate', 'E145:')
    call writefile(v:errors, 'Xresult')
    qa!
  END
  call writefile(lines, 'Xrestricteddiff', 'D')
  if RunVim([], [], '-Z --clean -S Xrestricteddiff')
    call assert_equal([], readfile('Xresult'))
  endif
  call delete('Xresult')
endfunc

func Test_restricted_env()
  let lines =<< trim END
      vim9script
      def SetEnv()
          $ENV = '123'
      enddef
      var result = 'okay'
      try
        SetEnv()
      catch /^Vim\%((\S\+)\)\=:E145:/
        result = 'not-allowed'
      endtry
      writefile([result], 'XResult_env')
      qa!
  END
  call writefile(lines, 'Xrestrictedvim9', 'D')
  if RunVim([], [], '-Z --clean -S Xrestrictedvim9')
    call assert_equal(['not-allowed'], readfile('XResult_env'))
  endif
  call delete('XResult_env')

  let lines =<< trim END
      try
        let $ENV_TEST = 'val'
        let result = 'okay'
      catch /^Vim\%((\S\+)\)\=:E145:/
        let result = 'not-allowed'
      endtry
      call writefile([result], 'XResult_env')
      qa!
  END
  call writefile(lines, 'Xrestricted_legacy', 'D')
  if RunVim([], [], '-Z --clean -S Xrestricted_legacy')
    call assert_equal(['not-allowed'], readfile('XResult_env'))
  endif
  call delete('XResult_env')
endfunc

func Test_restricted_grep()
  CheckScreendump

  let lines =<< trim END
    let result = 'okay'
    try
      " Try to use grep to execute an external command
      grep 'Vim' ./*.vim
    catch /^Vim\%((\S\+)\)\=:E145:/
      let result = 'grep-blocked'
    endtry
    call writefile([result], 'XResult_grep')
    qa!
  END

  call writefile(lines, 'Xrestricted_grep', 'D')
  if RunVim([], [], '-Z --clean -S Xrestricted_grep')
    call assert_equal(['grep-blocked'], readfile('XResult_grep'))
  endif
  call delete('XResult_grep')
endfunc

func Test_restricted_cscope()
  CheckFeature cscope

  " File does not exist, but shouldn't matter, it must be disallowed
  let lines =<< trim END
    let result = 'okay'
    try
      cscope add Xfoobar.out
    catch /^Vim\%((\S\+)\)\=:E145:/
      let result = 'blocked'
    endtry
    call writefile([result], 'XResult_cscope')
    qa!
  END

  call writefile(lines, 'Xrestricted_cscope', 'D')
  if RunVim([], [], '-Z --clean -S Xrestricted_cscope')
    call assert_equal(['blocked'], readfile('XResult_cscope'))
  endif
  call delete('XResult_cscope')
endfunc

func Test_vim9_storeenv_sandbox()
  let lines =<< trim END
    vim9script

    function g:LegacySetEnv()
      let $VIM_SANDBOX_TEST = 'legacy'
    endfunc

    def Vim9SetEnv()
      $VIM_SANDBOX_TEST = 'vim9_bypass'
    enddef

    # Legacy path should be blocked by check_secure()
    var legacy_blocked = false
    try
      legacy sandbox call LegacySetEnv()
    catch /E48/
      legacy_blocked = true
    endtry
    assert_true(legacy_blocked, 'legacy $ENV assignment should be blocked in sandbox')
    assert_false(exists('$VIM_SANDBOX_TEST'))

    # Vim9 path should also be blocked by check_secure()
    var vim9_blocked = false
    try
      sandbox Vim9SetEnv()
    catch /E48/
      vim9_blocked = true
    endtry
    assert_true(vim9_blocked, 'Vim9 ISN_STOREENV should be blocked in sandbox')
    assert_false(exists('$VIM_SANDBOX_TEST'))
    writefile([
    legacy_blocked,
    vim9_blocked,
    string(v:errors)], 'XResult_storeenv')
    qa
  END
  call writefile(lines, 'Xtest_storeenv_sandbox.vim', 'D')
  let expected = ['true', 'true', '[]']
  if RunVim([], [], '-u NONE -N -i NONE --not-a-term -S Xtest_storeenv_sandbox.vim')
    call assert_equal(expected, readfile('XResult_storeenv'))
  endif
  call delete('XResult_storeenv')
endfunc

" vim: shiftwidth=2 sts=2 expandtab
