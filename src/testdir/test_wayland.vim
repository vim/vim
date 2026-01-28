source util/window_manager.vim

CheckFeature wayland
CheckFeature wayland_clipboard
CheckUnix
CheckFeature job
CheckWaylandCompositor
CheckNotGui

if !executable('wl-paste') || !executable('wl-copy')
  throw "Skipped: wl-clipboard is not available"
endif

" Process will be killed when the test ends
let s:global_wayland_display = StartWaylandCompositor()
let s:old_wayland_display = $WAYLAND_DISPLAY

" For some reason if $WAYLAND_DISPLAY is set in the global namespace (not in a
" function), it won't actually be set if $WAYLAND_DISPLAY was not set before
" (such as in a CI environment) ? Solution is to just set it before the code of
" every test function
func s:PreTest()
  let $WAYLAND_DISPLAY=s:global_wayland_display
  " Always reconnect so we have a clean state each time and clear both
  " selections.
  call system('wl-copy -c')
  call system('wl-copy -p -c')
  exe 'wlrestore! ' .. $WAYLAND_DISPLAY

  set cpm=wayland
endfunc

func s:SetupFocusStealing()
  CheckFeature wayland_focus_steal
  if !executable('wayland-info')
    throw "Skipped: wayland-info program not available"
  endif

  " Starting a headless compositor won't expose a keyboard capability for its
  " seat, so we must use the user's existing Wayland session if they are in one.
  let $WAYLAND_DISPLAY = s:old_wayland_display

  exe 'wlrestore! ' .. $WAYLAND_DISPLAY

  " Check if we have keyboard capability for seat
  if system("wayland-info -i wl_seat | grep capabilities") !~? "keyboard"
    throw "Skipped: seat does not have keyboard"
  endif

  let $VIM_WAYLAND_FORCE_FS=1
  wlrestore!
endfunc

func s:UnsetupFocusStealing()
  unlet $VIM_WAYLAND_FORCE_FS
endfunc

func s:CheckClientserver()
  CheckFeature clientserver

  if has('socketserver') && !has('x11')
    if v:servername == ""
      call remote_startserver('VIMSOCKETSERVER')
    endif
  endif
endfunc

func s:EndRemoteVim(name, job)
  eval remote_send(a:name, "\<Esc>:qa!\<CR>")
  try
    call WaitForAssert({-> assert_equal("dead", job_status(a:job))})
  finally
    if job_status(a:job) != 'dead'
      call assert_report('Vim instance did not exit')
      call job_stop(a:job, 'kill')
    endif
  endtry
endfunc

func Test_wayland_startup()
  call s:PreTest()
  call s:CheckClientserver()

  let l:name = 'WLVIMTEST'
  let l:cmd = GetVimCommand() .. ' --servername ' .. l:name
  let l:job = job_start(cmd, {'stoponexit': 'kill', 'out_io': 'null'})

  call WaitForAssert({-> assert_equal("run", job_status(l:job))})
  call WaitForAssert({-> assert_match(name, serverlist())})

  call WaitForAssert({-> assert_equal($WAYLAND_DISPLAY,
        \ remote_expr(l:name, 'v:wayland_display'))})

  call s:EndRemoteVim(l:name, l:job)

  " When $WAYLAND_DISPLAY is invalid
  let l:job = job_start(cmd, { 'stoponexit': 'kill', 'out_io': 'null',
        \ 'env': {'WAYLAND_DISPLAY': 'UNKNOWN'}})

  call WaitForAssert({-> assert_equal("run", job_status(l:job))})
  call WaitForAssert({-> assert_match(name, serverlist())})

  call assert_equal('', remote_expr(l:name, 'v:wayland_display'))
  call s:EndRemoteVim(l:name, l:job)
endfunc

func Test_wayland_wlrestore()
  call s:PreTest()

  let l:wayland_display = StartWaylandCompositor()
  let l:env_cmd = 'WAYLAND_DISPLAY=' .. l:wayland_display .. ' '

  exe "wlrestore " .. l:wayland_display

  call assert_equal(l:wayland_display, v:wayland_display)

  " Check if calling wlrestore without arguments uses the existing Wayland
  " display.
  wlrestore!
  call assert_equal(l:wayland_display, v:wayland_display)

  " If called with invalid display
  wlrestore IDONTEXIST
  call assert_equal("", v:wayland_display)

  wlrestore!
  call assert_equal("", v:wayland_display)

  exe "wlrestore " .. l:wayland_display
  call assert_equal(l:wayland_display, v:wayland_display)

  " Actually check if connected display is different in case of regression with
  " v:wayland_display
  call system('wl-copy "1"')
  call system(l:env_cmd .. 'wl-copy "2"')

  call assert_equal('2', getreg('+'))

  " Check if wlrestore doesn't disconnect the display if not necessary by seeing
  " if Vim doesn't lose the selection
  call setreg('+', 'testing', 'c')

  wlrestore
  call assert_match('_VIM_TEXT', system(l:env_cmd .. 'wl-paste -l'))

  " Forcibly disconnect and reconnect the display
  wlrestore!
  call assert_notmatch('_VIM_TEXT', system(l:env_cmd .. 'wl-paste -l'))

  call EndWaylandCompositor(l:wayland_display)
endfunc

" Test behaviour when Wayland display connection is lost
func Test_wayland_connection_lost()
  call s:PreTest()

  let l:wayland_display = StartWaylandCompositor()
  let l:env_cmd = 'WAYLAND_DISPLAY=' .. l:wayland_display .. ' '

  exe "wlrestore " .. l:wayland_display

  call system(l:env_cmd .. 'wl-copy test')

  call assert_equal(l:wayland_display, v:wayland_display)
  call assert_equal('test', getreg('+'))

  call EndWaylandCompositor(l:wayland_display)

  call assert_equal('', getreg('+'))
endfunc

" Basic paste tests
func Test_wayland_paste()
  call s:PreTest()

  " Regular selection
  new

  call system('wl-copy "TESTING"')
  put +

  call assert_equal("TESTING", getline(2))

  call system('printf "LINE1\nLINE2\nLINE3" | wl-copy -n')
  put +

  call assert_equal(["LINE1", "LINE2", "LINE3"], getline(3, 5))
  bw!

  " Primary selection
  new

  call system('wl-copy -p "TESTING"')
  put *

  call assert_equal("TESTING", getline(2))

  call system('printf "LINE1\nLINE2\nLINE3" | wl-copy -p')
  put *

  call assert_equal(["LINE1", "LINE2", "LINE3"], getline(3, 5))

  bw!

  " Check behaviour when selection is cleared (empty)
  call system('wl-copy --clear')
  call assert_fails('put +', 'E353:')
endfunc

" Basic yank/copy tests
func Test_wayland_yank()
  call s:PreTest()

  new

  call setline(1, 'testing')
  yank +

  call assert_equal("testing\n", system('wl-paste -n'))

  call setline(2, 'testing2')
  call setline(3, 'testing3')
  exe '1,3yank +'

  call assert_equal("testing\ntesting2\ntesting3\n", system('wl-paste -n'))

  bw!

  " Primary selection
  new

  call setline(1, 'testing')
  yank *

  call assert_equal("testing\n", system('wl-paste -p -n'))

  call setline(2, 'testing2')
  call setline(3, 'testing3')
  exe '1,3yank *'

  call assert_equal("testing\ntesting2\ntesting3\n", system('wl-paste -p -n'))

  bw!
endfunc


" Check if correct mime types are advertised when we own the selection
func Test_wayland_mime_types_correct()
  call s:PreTest()

  let l:mimes = [
        \ '_VIMENC_TEXT',
        \ '_VIM_TEXT',
        \ 'text/plain;charset=utf-8',
        \ 'text/plain',
        \ 'UTF8_STRING',
        \ 'STRING',
        \ 'TEXT',
        \ 'application/x-vim-instance-' .. getpid()
        \ ]

  call setreg('+', 'text', 'c')

  for mime in split(system('wl-paste -l'), "\n")
    if index(l:mimes, mime) == -1
      call assert_report("'" .. mime .. "' is not a supported mime type")
    endif
  endfor

  call setreg('*', 'text', 'c')

  for mime in split(system('wl-paste -p -l'), "\n")
    if index(l:mimes, mime) == -1
      call assert_report("'" .. mime .. "' is not a supported mime type")
    endif
  endfor
endfunc

" Test if the _VIM_TEXT and _VIMENC_TEXT formats are correct:
" _VIM_TEXT: preserves motion type (line/char/block wise)
" _VIMENC_TEXT: same but also indicates the encoding type
func Test_wayland_paste_vim_format_correct()
  call s:PreTest()

  " Vim doesn't support null characters in strings, so we use the -v flag of the
  " cat program to show them in a printable way, if it is available.
  call system("cat -v")
  if v:shell_error != 0
    throw 'Skipped: cat program does not have -v command-line flag'
  endif

  set encoding=utf-8

  let l:GetSel = {type -> system('wl-paste -t ' .. type .. ' | cat -v')}
  let l:GetSelP = {type -> system('wl-paste -p -t ' .. type .. ' | cat -v')}

  " Regular selection
  call setreg('+', 'text', 'c')
  call assert_equal("^@text", l:GetSel('_VIM_TEXT'))
  call setreg('+', 'text', 'c')
  call assert_equal("^@utf-8^@text", l:GetSel('_VIMENC_TEXT'))

  call setreg('+', 'text', 'l')
  call assert_equal("^Atext\n", l:GetSel('_VIM_TEXT'))
  call setreg('+', 'text', 'l')
  call assert_equal("^Autf-8^@text\n",l:GetSel('_VIMENC_TEXT'))

  call setreg('+', 'text', 'b')
  call assert_equal("^Btext\n", l:GetSel('_VIM_TEXT'))
  call setreg('+', 'text', 'b')
  call assert_equal("^Butf-8^@text\n", l:GetSel('_VIMENC_TEXT'))

  " Primary selection
  call setreg('*', 'text', 'c')
  call assert_equal("^@text", l:GetSelP('_VIM_TEXT'))
  call setreg('*', 'text', 'c')
  call assert_equal("^@utf-8^@text", l:GetSelP('_VIMENC_TEXT'))

  call setreg('*', 'text', 'l')
  call assert_equal("^Atext\n", l:GetSelP('_VIM_TEXT'))
  call setreg('*', 'text', 'l')
  call assert_equal("^Autf-8^@text\n",l:GetSelP('_VIMENC_TEXT'))

  call setreg('*', 'text', 'b')
  call assert_equal("^Btext\n", l:GetSelP('_VIM_TEXT'))
  call setreg('*', 'text', 'b')
  call assert_equal("^Butf-8^@text\n", l:GetSelP('_VIMENC_TEXT'))

  set encoding&
endfunc

" Test checking if * and + registers are not the same
func Test_wayland_plus_star_not_same()
  call s:PreTest()
  new

  call system('wl-copy "regular"')
  call system('wl-copy -p "primary"')

  call assert_notequal(getreg('+'), getreg('*'))

  " Check if when we are the source client
  call setreg('+', 'REGULAR')
  call setreg('*', 'PRIMARY')

  call assert_notequal(system('wl-paste -p'), system('wl-paste'))

  bw!
endfunc

" Test if autoselect option in 'clipboard' works properly for Wayland
func Test_wayland_autoselect_works()
  call s:PreTest()
  call s:CheckClientserver()

  let l:lines =<< trim END
  set cpm=wayland
  set clipboard=autoselect

  new
  call setline(1, 'LINE 1')
  call setline(2, 'LINE 2')
  call setline(3, 'LINE 3')

  call cursor(1, 1)
  END

  call writefile(l:lines, 'Wltester', 'D')

  let l:name = 'WLVIMTEST'
  let l:cmd = GetVimCommand() .. ' -S Wltester --servername ' .. l:name
  let l:job = job_start(cmd, {'stoponexit': 'kill', 'out_io': 'null'})

  call WaitForAssert({-> assert_equal("run", job_status(l:job))})
  call WaitForAssert({-> assert_match(name, serverlist())})

  call remote_send(l:name, "ve")
  call WaitForAssert({-> assert_equal('LINE', system('wl-paste -p -n'))})

  call remote_send(l:name, "w")
  call WaitForAssert({-> assert_equal('LINE 1', system('wl-paste -p -n'))})

  call remote_send(l:name, "V")
  call WaitForAssert({-> assert_equal("LINE 1\n", system('wl-paste -p -n'))})

  " Reset cursor
  call remote_send(l:name, "\<Esc>:call cursor(1, 1)\<CR>")
  call WaitForAssert({-> assert_equal("LINE 1\n", system('wl-paste -p -n'))})

  " Test visual block mode
  call remote_send(l:name, "\<C-q>jjj") " \<C-v> doesn't seem to work but \<C-q>
                                        " does...

  call WaitForAssert({-> assert_equal("L\nL\nL\n", system('wl-paste -p -n'))})

  eval remote_send(l:name, "\<Esc>:qa!\<CR>")

  call s:EndRemoteVim(l:name, l:job)
endfunc

" Check if the -Y flag works properly
func Test_no_wayland_connect_cmd_flag()
  call s:PreTest()
  call s:CheckClientserver()

  let l:name = 'WLFLAGVIMTEST'
  let l:cmd = GetVimCommand() .. ' -Y --servername ' .. l:name
  let l:job = job_start(cmd, {'stoponexit': 'kill', 'out_io': 'null'})

  call WaitForAssert({-> assert_equal("run", job_status(l:job))})
  call WaitForAssert({-> assert_match(name, serverlist())})

  call WaitForAssert({->assert_equal('',
        \ remote_expr(l:name, 'v:wayland_display'))})

  call remote_send(l:name, ":wlrestore\<CR>")
  call WaitForAssert({-> assert_equal('',
        \ remote_expr(l:name, 'v:wayland_display'))})

  call remote_send(l:name, ":wlrestore " .. $WAYLAND_DISPLAY .. "\<CR>")
  call WaitForAssert({-> assert_equal('',
        \ remote_expr(l:name, 'v:wayland_display'))})

  call remote_send(l:name, ":wlrestore IDONTEXIST\<CR>")
  call WaitForAssert({-> assert_equal('',
        \ remote_expr(l:name, 'v:wayland_display'))})

  call s:EndRemoteVim(l:name, l:job)
endfunc

" Test behaviour when we do something like suspend Vim
func Test_wayland_become_inactive()
  call s:PreTest()
  call s:CheckClientserver()

  let l:name = 'WLLOSEVIMTEST'
  let l:cmd = GetVimCommand() .. ' --servername ' .. l:name
  let l:job = job_start(cmd, {
        \ 'stoponexit': 'kill',
        \ 'out_io': 'null',
        \ })

  call WaitForAssert({-> assert_equal("run", job_status(l:job))})
  call WaitForAssert({-> assert_match(name, serverlist())})

  call remote_send(l:name, "iSOME TEXT\<Esc>\"+yy")

  call WaitForAssert({-> assert_equal("SOME TEXT\n",
        \ system('wl-paste -n'))})

  call remote_send(l:name, "\<C-z>")

  call WaitForAssert({-> assert_equal("Nothing is copied\n",
        \ system('wl-paste -n'))})

  call s:EndRemoteVim(l:name, l:job)
endfunc

" Test wlseat option
func Test_wayland_seat()
  call s:PreTest()

  " Don't know a way to create a virtual seat so just test using the existing
  " one only
  set wlseat=

  call system('wl-copy "TESTING"')
  call assert_equal('TESTING', getreg('+'))

  set wlseat=UNKNOWN

  call assert_equal('', getreg('+'))

  set wlseat=idontexist

  call assert_equal('', getreg('+'))

  set wlseat=

  call assert_equal('TESTING', getreg('+'))

  set wlseat&
endfunc

" Test focus stealing
func Test_wayland_focus_steal()
  CheckFeature wayland_focus_steal
  call s:PreTest()
  call s:SetupFocusStealing()

  call system('wl-copy regular')

  call assert_equal('regular', getreg('+'))

  call system('wl-copy -p primary')

  call assert_equal('primary', getreg('*'))

  call setreg('+', 'REGULAR')

  call assert_equal('REGULAR', system('wl-paste -n'))

  call setreg('*', 'PRIMARY')

  call assert_equal('PRIMARY', system('wl-paste -p -n'))

  call s:UnsetupFocusStealing()
endfunc

" Test when environment is not suitable for Wayland
func Test_wayland_bad_environment()
  call s:PreTest()
  call s:CheckClientserver()

  unlet $WAYLAND_DISPLAY

  let l:old = $XDG_RUNTIME_DIR
  unlet $XDG_RUNTIME_DIR

  let l:name = 'WLVIMTEST'
  let l:cmd = GetVimCommand() .. ' --servername ' .. l:name
  let l:job = job_start(cmd, {
        \ 'stoponexit': 'kill',
        \ 'out_io': 'null',
        \ })

  call WaitForAssert({-> assert_equal("run", job_status(l:job))})
  call WaitForAssert({-> assert_match(name, serverlist())})

  call WaitForAssert({-> assert_equal('',
        \ remote_expr(l:name, 'v:wayland_display'))})

  call s:EndRemoteVim(l:name, l:job)

  let $XDG_RUNTIME_DIR = l:old
endfunc

" Test if Vim still works properly after losing the selection
func Test_wayland_lost_selection()
  call s:PreTest()

  call setreg('+', 'regular')
  call setreg('*', 'primary')

  call assert_equal('regular', getreg('+'))
  call assert_equal('primary', getreg('*'))

  call system('wl-copy overwrite')
  call system('wl-copy -p overwrite')

  call assert_equal('overwrite', getreg('+'))
  call assert_equal('overwrite', getreg('*'))

  call setreg('+', 'regular')
  call setreg('*', 'primary')

  call assert_equal('regular', getreg('+'))
  call assert_equal('primary', getreg('*'))

endfunc

" Same as above but for the focus stealing method
func Test_wayland_lost_selection_focus_steal()
  call s:PreTest()
  call s:SetupFocusStealing()

  call setreg('+', 'regular')
  call setreg('*', 'primary')

  call assert_equal('regular', getreg('+'))
  call assert_equal('primary', getreg('*'))

  call system('wl-copy overwrite')
  call system('wl-copy -p overwrite')

  call assert_equal('overwrite', getreg('+'))
  call assert_equal('overwrite-primary', getreg('*'))

  call setreg('+', 'regular')
  call setreg('*', 'primary')

  call assert_equal('regular', getreg('+'))
  call assert_equal('primary', getreg('*'))

  call s:UnsetupFocusStealing()
endfunc

" Test when there are no supported mime types for the selection
func Test_wayland_no_mime_types_supported()
  call s:PreTest()

  call system('wl-copy tester')
  call assert_equal('tester', getreg('+'))

  call system('wl-copy -t image/png testing')
  call assert_equal('', getreg('+'))
  call assert_fails('put +', 'E353:')
endfunc

" Test behaviour with large selections in terms of data size
func Test_wayland_handle_large_data()
  call s:PreTest()

  let l:file = tempname()
  let l:contents = repeat('c', 1000000) " ~ 1 MB

  call writefile([l:contents], l:file, 'b')
  call system('cat ' .. l:file .. ' | wl-copy -t TEXT')

  call assert_equal(l:contents, getreg('+'))

  call setreg('+', l:contents, 'c')

  call assert_equal(l:contents, system('wl-paste -n -t TEXT'))
endfunc

" vim: shiftwidth=2 sts=2 expandtab
