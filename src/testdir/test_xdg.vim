" Tests for the XDG feature

source check.vim

source shared.vim
source mouse.vim

func s:get_rcs()
  let rcs = {
        \ 'file1': { 'path': '~/.vimrc' },
        \ 'file2': { 'path': '~/.vim/vimrc' },
        \ 'xdg': { 'path': exists('$XDG_CONFIG_HOME') ? '$XDG_CONFIG_HOME' : "~/.config" },
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
    quit!
  CODE
  call RunVim(before, [], "")
  let my_rc = readfile("XMY_VIMRC")
  if rc.file1.exists
    call assert_equal(rc.file1.path, my_rc)
  elseif !rc.file1.exists && rc.file2.exists
    call assert_equal(rc.file2.path, my_rc)
  elseif !rc.file1.exists && !rc.file2.exists && rc.xdg.exists
    call assert_equal(rc.xdg.path, my_rc)
  endif
  call delete("XMY_VIMRC")
endfunc

func Test_xdg_runtime_files()
  " This tests, that the initialization file from
  " ~/.vimrc, ~/.vim/vimrc and ~/.config/vim/vimrc (or
  " $XDG_HOMECONFIG/vim/vimrc) are sourced in that order
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
    call filter(g:, {idx, _ -> idx =~ '^rc'})
    call assert_equal(#{rc_one: 'one', rc: '.vimrc'}, g:)
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
    call filter(g:, {idx, _ -> idx =~ '^rc'})
    call assert_equal(#{rc_two: 'two', rc: '.vim/vimrc'}, g:)
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
    call filter(g:, {idx, _ -> idx =~ '^rc'})
    call assert_equal(#{rc_three: 'three', rc: '.config/vim/vimrc'}, g:)
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
    call filter(g:, {idx, _ -> idx =~ '^rc'})
    call assert_equal(#{rc_four: 'four', rc: 'xdg/vim/vimrc'}, g:)
    call writefile(v:errors, 'Xresult')
    quit
  END
  call writefile(lines, 'Xscript', 'D')
  call system($'{vimcmd} -S Xscript')
  call assert_equal([], readfile('Xresult'))

  call delete(rc4)
  unlet $XDG_CONFIG_HOME
endfunc

" vim: shiftwidth=2 sts=2 expandtab
