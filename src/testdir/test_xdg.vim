" Tests for the XDG feature

source check.vim
CheckFeature terminal

source shared.vim
source screendump.vim
source mouse.vim
source term_util.vim

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
  " init files are not source
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

  let rows = 20
  let buf = RunVimInTerminal('', #{rows: rows, no_clean: 1})
  call TermWait(buf)
  call term_sendkeys(buf, ":echo \$MYVIMRC\<cr>")
  call WaitForAssert({-> assert_match('XfakeHOME/\.vimrc', term_getline(buf, rows))})
  call term_sendkeys(buf, ":call filter(g:, {idx, _ -> idx =~ '^rc'})\<cr>")
  call TermWait(buf)
  call term_sendkeys(buf, ":redraw!\<cr>")
  call TermWait(buf)
  call term_sendkeys(buf, ":let g:\<cr>")
  call VerifyScreenDump(buf, 'Test_xdg_1', {})
  call StopVimInTerminal(buf)
  call delete(rc1)
  bw

  let buf = RunVimInTerminal('', #{rows: rows, no_clean: 1})
  call TermWait(buf)
  call term_sendkeys(buf, ":echo \$MYVIMRC\<cr>")
  call WaitForAssert({-> assert_match('XfakeHOME/\.vim/vimrc', term_getline(buf, rows))})
  call term_sendkeys(buf, ":call filter(g:, {idx, _ -> idx =~ '^rc'})\<cr>")
  call TermWait(buf)
  call term_sendkeys(buf, ":redraw!\<cr>")
  call TermWait(buf)
  call term_sendkeys(buf, ":let g:\<cr>")
  call VerifyScreenDump(buf, 'Test_xdg_2', {})
  call StopVimInTerminal(buf)
  call delete(rc2)
  bw

  let buf = RunVimInTerminal('', #{rows: rows, no_clean: 1})
  call TermWait(buf)
  call term_sendkeys(buf, ":echo \$MYVIMRC\<cr>")
  call WaitForAssert({-> assert_match('XfakeHOME/\.config/vim/vimrc', term_getline(buf, rows))})
  call term_sendkeys(buf, ":call filter(g:, {idx, _ -> idx =~ '^rc'})\<cr>")
  call TermWait(buf)
  call term_sendkeys(buf, ":redraw!\<cr>")
  call TermWait(buf)
  call term_sendkeys(buf, ":let g:\<cr>")
  call VerifyScreenDump(buf, 'Test_xdg_3', {})
  call StopVimInTerminal(buf)
  call delete(rc3)
  bw

  let $XDG_CONFIG_HOME=expand('~/xdg/')
  let buf = RunVimInTerminal('', #{rows: rows, no_clean: 1})
  call TermWait(buf)
  call term_sendkeys(buf, ":redraw!\<cr>")
  call TermWait(buf)
  call term_sendkeys(buf, ":echo \$MYVIMRC\<cr>")
  call WaitForAssert({-> assert_match('xdg/vim/vimrc', term_getline(buf, rows))})
  call term_sendkeys(buf, ":call filter(g:, {idx, _ -> idx =~ '^rc'})\<cr>")
  call TermWait(buf)
  call term_sendkeys(buf, ":let g:\<cr>")
  call VerifyScreenDump(buf, 'Test_xdg_4', {})
  call StopVimInTerminal(buf)
  call delete(rc4)
  bw
  unlet $XDG_CONFIG_HOME
endfunc

" vim: shiftwidth=2 sts=2 expandtab
