" Tests for the XDG feature

source check.vim
source shared.vim

func s:get_rcs()
  let rcs = {
        \ 'file1': { 'path': '~/.vimrc', 'dir': expand('~/.vim/') },
        \ 'file2': { 'path': '~/.vim/vimrc', 'dir': expand('~/.vim/') },
        \ 'xdg': { 'path': exists('$XDG_CONFIG_HOME') ? '$XDG_CONFIG_HOME' : "~/.config",
                  \ 'dir': exists('$XDG_CONFIG_HOME') ? expand("$XDG_CONFIG_HOME/vim") : '~/.config/vim/'},
        \}
  for v in values(rcs)
    let v.exists = filereadable(expand(v.path))
  endfor
  return rcs
endfunc

func Test_xdg_rc_detection()
  CheckUnix
  let rc = s:get_rcs()
  let before =<< trim CODE
    call writefile([expand('$MYVIMRC')], "XMY_VIMRC")
    call writefile([expand('$MYVIMRCDIR')], "XMY_VIMDIR")
    quit!
  CODE
  call RunVim(before, [], "")
  let my_rc = readfile("XMY_VIMRC")
  let my_rcdir = readfile("XMY_VIMDIR")
  if rc.file1.exists
    call assert_equal(rc.file1.path, my_rc)
    call assert_equal(rc.file1.dir, my_rcdir)
  elseif !rc.file1.exists && rc.file2.exists
    call assert_equal(rc.file2.path, my_rc)
    call assert_equal(rc.file2.dir, my_rcdir)
  elseif !rc.file1.exists && !rc.file2.exists && rc.xdg.exists
    call assert_equal(rc.xdg.path, my_rc)
    call assert_equal(rc.xdg.dir, my_rcdir)
  endif
  call delete("XMY_VIMRC")
  call delete("XMY_VIMDIR")
endfunc

func Test_xdg_runtime_files()
  " This tests, that the initialization file from
  " ~/.vimrc, ~/.vim/vimrc and ~/.config/vim/vimrc (or
  " $XDG_CONFIG_HOME/vim/vimrc) are sourced in that order
  CheckUnix
  call mkdir(expand('~/.vim/'), 'pD')
  call mkdir(expand('~/.config/vim/'), 'pD')
  call mkdir(expand('~/xdg/vim/'), 'pD')

  let rc1=expand('~/.vimrc')
  let rc2=expand('~/.vim/vimrc')
  let rc3=expand('~/.config/vim/vimrc')
  let rc4=expand('~/xdg/vim/vimrc')

  " g:rc_one|two|three|four is to verify, that the other
  " init files are not sourced
  " g:rc is to verify which rc file has been loaded.
  let file1 =<< trim CODE
    let g:rc_one = 'one'
    let g:rc = '.vimrc'
  CODE
  let file2 =<< trim CODE
    let g:rc_two = 'two'
    let g:rc = '.vim/vimrc'
  CODE
  let file3 =<< trim CODE
    let g:rc_three = 'three'
    let g:rc = '.config/vim/vimrc'
  CODE
  let file4 =<< trim CODE
    let g:rc_four = 'four'
    let g:rc = 'xdg/vim/vimrc'
  CODE
  call writefile(file1, rc1)
  call writefile(file2, rc2)
  call writefile(file3, rc3)
  call writefile(file4, rc4)

  " Get the Vim command to run without the '-u NONE' argument
  let vimcmd = substitute(GetVimCommand(), '-u NONE', '', '')

  " Test for ~/.vimrc
  let lines =<< trim END
    call assert_match('XfakeHOME/\.vimrc', $MYVIMRC)
    call assert_match('XfakeHOME/.vim/', $MYVIMDIR)
    call filter(g:, {idx, _ -> idx =~ '^rc'})
    call assert_equal(#{rc_one: 'one', rc: '.vimrc'}, g:)
    call assert_match('XfakeHOME/\.vim/view', &viewdir)
    call writefile(v:errors, 'Xresult')
    quit
  END
  call writefile(lines, 'Xscript', 'D')
  call system($'{vimcmd} -S Xscript')
  call assert_equal([], readfile('Xresult'))

  call delete(rc1)

  " Test for ~/.vim/vimrc
  let lines =<< trim END
    call assert_match('XfakeHOME/\.vim/vimrc', $MYVIMRC)
    call assert_match('XfakeHOME/\.vim/', $MYVIMDIR)
    call filter(g:, {idx, _ -> idx =~ '^rc'})
    call assert_equal(#{rc_two: 'two', rc: '.vim/vimrc'}, g:)
    call assert_match('XfakeHOME/\.vim/view', &viewdir)
    call writefile(v:errors, 'Xresult')
    quit
  END
  call writefile(lines, 'Xscript', 'D')
  call system($'{vimcmd} -S Xscript')
  call assert_equal([], readfile('Xresult'))

  call delete(rc2)

  " XDG_CONFIG_HOME is set in Github CI runners
  unlet $XDG_CONFIG_HOME

  " Test for ~/.config/vim/vimrc
  let lines =<< trim END
    let msg = $'HOME="{$HOME}", ~="{expand("~")}"'
    call assert_match('XfakeHOME/\.config/vim/vimrc', $MYVIMRC, msg)
    call assert_match('XfakeHOME/\.config/vim/', $MYVIMDIR, msg)
    call filter(g:, {idx, _ -> idx =~ '^rc'})
    call assert_equal(#{rc_three: 'three', rc: '.config/vim/vimrc'}, g:)
    call assert_match('XfakeHOME/\.config/vim/view', &viewdir)
    call writefile(v:errors, 'Xresult')
    quit
  END
  call writefile(lines, 'Xscript', 'D')
  call system($'{vimcmd} -S Xscript')
  call assert_equal([], readfile('Xresult'))

  call delete(rc3)

  " Test for ~/xdg/vim/vimrc
  let $XDG_CONFIG_HOME=expand('~/xdg/')
  let lines =<< trim END
    let msg = $'HOME="{$HOME}", XDG_CONFIG_HOME="{$XDG_CONFIG_HOME}"'
    call assert_match('XfakeHOME/xdg/vim/vimrc', $MYVIMRC, msg)
    call assert_match('XfakeHOME/xdg/vim/', $MYVIMDIR, msg)
    call filter(g:, {idx, _ -> idx =~ '^rc'})
    call assert_equal(#{rc_four: 'four', rc: 'xdg/vim/vimrc'}, g:)
    call assert_match('XfakeHOME/xdg/vim/view, &viewdir)
    call writefile(v:errors, 'Xresult')
    quit
  END
  call writefile(lines, 'Xscript', 'D')
  call system($'{vimcmd} -S Xscript')
  call assert_equal([], readfile('Xresult'))

  call delete(rc4)
  unlet $XDG_CONFIG_HOME
endfunc

func Test_xdg_version()
  CheckUnix
  let $HOME = getcwd() .. '/XfakeHOME'
  unlet $XDG_CONFIG_HOME
  let a = execute(':version')->split('\n')
  let a = filter(a, { _, val -> val =~ '\.config\|XDG_CONFIG_HOME' })
  " There should be 1 entry for gvimrc and 1 entry for vimrc,
  " but only if Vim was compiled with gui support
  call assert_equal(1 + has("gui"), len(a))
  call assert_match('\~/\.config/vim/vimrc', a[0])
  if has("gui")
    call assert_match('\~/\.config/vim/gvimrc', a[1])
  endif

  let $XDG_CONFIG_HOME = expand('~/.xdg')
  let a = execute(':version')->split('\n')
  let a = filter(a, { _, val -> val =~ '\.config\|XDG_CONFIG_HOME' })
  call assert_equal(1 + has("gui"), len(a))
  call assert_match('XDG_CONFIG_HOME/vim/vimrc', a[0])
  if has("gui")
    call assert_match('XDG_CONFIG_HOME/vim/gvimrc', a[1])
  endif
  unlet $XDG_CONFIG_HOME
endfunc

" Test for gvimrc, must be last, since it starts the GUI
" and sources a few extra test files
func Test_zzz_xdg_runtime_files()
  CheckCanRunGui
  CheckUnix

  " Is setup in Github Runner
  unlet $XDG_CONFIG_HOME
  source setup_gui.vim
  call GUISetUpCommon()

  " This tests, that the GUI initialization file from
  " ~/.gvimrc, ~/.vim/gvimrc, ~/.config/vim/gvimrc
  " and ~/XDG_CONFIG_HOME/vim/gvimrc is sourced
  " when starting GUI mode
  call mkdir(expand('~/.vim/'), 'pD')
  call mkdir(expand('~/.config/vim/'), 'pD')
  call mkdir(expand('~/xdg/vim/'), 'pD')

  let rc1=expand('~/.gvimrc')
  let rc2=expand('~/.vim/gvimrc')
  let rc3=expand('~/.config/vim/gvimrc')
  let rc4=expand('~/xdg/vim/gvimrc')

  " g:rc_one|two|three|four is to verify, that the other
  " init files are not sourced
  " g:rc is to verify which rc file has been loaded.
  let file1 =<< trim CODE
    let g:rc_one = 'one'
    let g:rc = '.gvimrc'
  CODE
  let file2 =<< trim CODE
    let g:rc_two = 'two'
    let g:rc = '.vim/gvimrc'
  CODE
  let file3 =<< trim CODE
    let g:rc_three = 'three'
    let g:rc = '.config/vim/gvimrc'
  CODE
  let file4 =<< trim CODE
    let g:rc_four = 'four'
    let g:rc = 'xdg/vim/gvimrc'
  CODE
  call writefile(file1, rc1)
  call writefile(file2, rc2)
  call writefile(file3, rc3)
  call writefile(file4, rc4)

  " Get the Vim command to run without the '-u NONE' argument
  let vimcmd = substitute(GetVimCommand(), '-u NONE', '', '')

  " Test for ~/.gvimrc
  let lines =<< trim END
    " Ignore the "failed to create input context" error.
    call test_ignore_error('E285')
    gui -f
    call assert_match('Xhome/\.gvimrc', $MYGVIMRC)
    call assert_match('Xhome/\.vim/', $MYVIMDIR)
    call filter(g:, {idx, _ -> idx =~ '^rc'})
    call assert_equal(#{rc_one: 'one', rc: '.gvimrc'}, g:)
    call writefile(v:errors, 'Xresult')
    quit
  END
  call writefile(lines, 'Xscript', 'D')
  call system($'{vimcmd} -S Xscript')
  call assert_equal([], readfile('Xresult'))

  call delete(rc1)

  " Test for ~/.vim/gvimrc
  let lines =<< trim END
    " Ignore the "failed to create input context" error.
    call test_ignore_error('E285')
    gui -f
    call assert_match('Xhome/\.vim/gvimrc', $MYGVIMRC)
    call assert_match('Xhome/\.vim/', $MYVIMDIR)
    call filter(g:, {idx, _ -> idx =~ '^rc'})
    call assert_equal(#{rc_two: 'two', rc: '.vim/gvimrc'}, g:)
    call writefile(v:errors, 'Xresult')
    quit
  END
  call writefile(lines, 'Xscript', 'D')
  call system($'{vimcmd} -S Xscript')
  call assert_equal([], readfile('Xresult'))

  call delete(rc2)

  " Test for ~/.config/vim/gvimrc
  let lines =<< trim END
    " Ignore the "failed to create input context" error.
    call test_ignore_error('E285')
    gui -f
    let msg = $'HOME="{$HOME}", ~="{expand("~")}"'
    call assert_match('Xhome/\.config/vim/gvimrc', $MYGVIMRC, msg)
    call assert_match('Xhome/\.config/vim/', $MYVIMDIR, msg)
    call filter(g:, {idx, _ -> idx =~ '^rc'})
    call assert_equal(#{rc_three: 'three', rc: '.config/vim/gvimrc'}, g:)
    call writefile(v:errors, 'Xresult')
    quit
  END
  call writefile(lines, 'Xscript', 'D')
  call system($'{vimcmd} -S Xscript')
  call assert_equal([], readfile('Xresult'))

  call delete(rc3)

  " Test for ~/xdg/vim/gvimrc
  let $XDG_CONFIG_HOME=expand('~/xdg/')
  let lines =<< trim END
    " Ignore the "failed to create input context" error.
    call test_ignore_error('E285')
    gui -f
    let msg = $'HOME="{$HOME}", XDG_CONFIG_HOME="{$XDG_CONFIG_HOME}"'
    call assert_match('Xhome/xdg/vim/gvimrc', $MYGVIMRC, msg)
    call assert_match('Xhome/xdg/vim/', $MYVIMDIR, msg)
    call filter(g:, {idx, _ -> idx =~ '^rc'})
    call assert_equal(#{rc_four: 'four', rc: 'xdg/vim/gvimrc'}, g:)
    call writefile(v:errors, 'Xresult')
    quit
  END
  call writefile(lines, 'Xscript', 'D')
  call system($'{vimcmd} -S Xscript')
  call assert_equal([], readfile('Xresult'))

  call delete(rc4)

  " Clean up
  unlet $XDG_CONFIG_HOME
  call GUITearDownCommon()
  call delete('Xhome', 'rf')
endfunc

" vim: shiftwidth=2 sts=2 expandtab
