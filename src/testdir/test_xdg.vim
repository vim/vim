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
  if !has('unix')
    return v:false
  endif
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

" vim: shiftwidth=2 sts=2 expandtab
