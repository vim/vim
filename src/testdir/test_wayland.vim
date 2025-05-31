source check.vim
source shared.vim
source window_manager.vim

CheckFeature wayland
CheckFeature wayland_clipboard
CheckFeature unix
CheckFeature job
CheckWaylandCompositor

if !executable('wl-paste') || !executable('wl-copy')
  throw "Skipped: wl-clipboard is not available"
endif

if !executable('wayland-info')
  throw "Skipped: wayland-info is not available"
endif

" Check if primary selection is supported either zwlr or ext data control
" protcol exists.
if system("wayland-info -i zwp_primary_selection_device_manager_v1") ==? "" ||
      \ (system("wayland-info -i zwlr_data_control_manager_v1") ==? "" &&
      \ system("wayland-info -i ext_data_control_manager_v1") ==? "")
  let s:lines =<< trim END
  Skipped: zwp_primary_selection_device_manager_v1,
  zwlr_data_control_manager_v1, or ext_data_control_manager_v1
  interfaces do not exist
  END

  throw join(s:lines, " ")
  unlet s:lines
endif

" Convert blob to a string and represent null bytes as '^@'
func s:ConvertBlob(blob)
  let l:str = ''

  for byte in a:blob
    if byte == 0x0
      let l:str ..= '^@'
    else
      let l:str ..= nr2char(byte)
    endif
  endfor

  return l:str
endfunc

" Get output of wl-paste -n .. flags and return a string
func s:GetPaste(flags)
  call system('wl-paste -n ' .. a:flags .. ' > paste.bin')

  let l:paste = readblob('paste.bin')
  call delete('paste.bin')

  return s:ConvertBlob(l:paste)
endfunc

" Check wayland behaviour on startup
func Test_wayland_startup()
  set cpm=wayland

  let l:name = 'WLVIMTEST'
  let l:cmd = GetVimCommand() .. ' --servername ' .. l:name
  let l:job = job_start(cmd, {'stoponexit': 'kill', 'out_io': 'null'})

  call WaitForAssert({-> assert_equal("run", job_status(l:job))})
  call WaitForAssert({-> assert_match(name, serverlist())})

  call assert_equal($WAYLAND_DISPLAY, remote_expr(l:name, 'v:wayland_display'))

  eval remote_send(l:name, "\<Esc>:qa!\<CR>")
  try
    call WaitForAssert({-> assert_equal("dead", job_status(l:job))})
  finally
    if job_status(l:job) != 'dead'
      call assert_report('Vim instance did not exit')
      call job_stop(l:job, 'kill')
    endif
  endtry

  " When $WAYLAND_DISPLAY is invalid
  let l:job = job_start(cmd, { 'stoponexit': 'kill', 'out_io': 'null',
        \ 'env': {'WAYLAND_DISPLAY': 'UNKNOWN'} })

  call WaitForAssert({-> assert_equal("run", job_status(l:job))})
  call WaitForAssert({-> assert_match(name, serverlist())})

  call assert_equal('', remote_expr(l:name, 'v:wayland_display'))
endfunc

" Test if wlrestore works properly
func Test_wayland_wlrestore()
  set cpm=wayland

  let l:prev_display = v:wayland_display
  let [l:display, l:wljob] = Start_wayland_compositor()
  let l:env_cmd = 'WAYLAND_DISPLAY=' .. l:display .. ' '

  exe "wlrestore " .. l:display

  call assert_equal(l:display, v:wayland_display)

  " Check if calling wlrestore without arguments uses the existing wayland
  " display.
  wlrestore!
  call assert_equal(l:display, v:wayland_display)

  " If called with invalid display
  wlrestore IDONTEXIST
  call assert_equal("", v:wayland_display)

  wlrestore!
  call assert_equal("", v:wayland_display)

  exe "wlrestore " .. l:display
  call assert_equal(l:display, v:wayland_display)

  " Actually check if connected display is different in case of regression with
  " v:wayland_display
  call system('wl-copy "1"')
  call system(l:env_cmd .. 'wl-copy "2"')

  call assert_equal('2', getreg('+'))

  " Check if wlrestore doesn't disconnect the display if not nessecary by seeing
  " if Vim doesn't lose the selection
  call setreg('+', 'test', 'c')
  wlrestore
  call assert_match('_VIM_TEXT', system(l:env_cmd .. 'wl-paste -l'))
  " Forcibly disconnect and reconnect the display
  wlrestore!
  call assert_notmatch('_VIM_TEXT', system(l:env_cmd .. 'wl-paste -l'))

  wlrestore unknown

  call assert_equal('', v:wayland_display)

  exe "wlrestore " .. l:prev_display
  call End_wayland_compositor(l:wljob)
endfunc

" Test behaviour when wayland display connection is lost
func Test_wayland_connection_lost()
  set cpm=wayland

  let l:prev_display = v:wayland_display
  let [l:display, l:wljob] = Start_wayland_compositor()
  let l:env_cmd = 'WAYLAND_DISPLAY=' .. l:display .. ' '

  call system(l:env_cmd .. ' wl-copy "test"')
  exe "wlrestore " .. l:display

  call assert_equal(l:display, v:wayland_display)
  call assert_equal('test', getreg('+'))

  call End_wayland_compositor(l:wljob)

  call assert_equal('', getreg('+'))
  call assert_fails('put +', 'E353:')
  call assert_fails('yank +', 'E1548:')

  " v:wayland_display shouldn't change until we call :wlrestore again
  call assert_equal(l:display, v:wayland_display)

  wlrestore

  call assert_equal("", v:wayland_display)

  exe "wlrestore " .. l:prev_display
endfunc

" Basic paste tests
func Test_wayland_paste()
  " Regular selection
  new
  set cpm=wayland

  " Prevent 'Register changed while using it' error, guessing this works because
  " it makes Vim lose the selection?
  wlrestore!

  call system('wl-copy "TESTING"')
  put +

  call assert_equal("TESTING", getline(2))

  call system('printf "LINE1\nLINE2\nLINE3" | wl-copy -n')
  put +

  call assert_equal(["LINE1", "LINE2", "LINE3"], getline(3, 5))
  bw!

  new
  " Primary selection
  call system('wl-copy -p "TESTING"')
  put *

  call assert_equal("TESTING", getline(2))

  call system('printf "LINE1\nLINE2\nLINE3" | wl-copy -p')
  put *

  call assert_equal(["LINE1", "LINE2", "LINE3"], getline(3, 5))

  bw!

  " Check behaviour when selecton is cleared (empty)

  " Run a separate compositor to avoid clipboard managers which prevent the
  " clipboard from being cleared
  let l:prev_display = v:wayland_display
  let [l:display, l:wljob] = Start_wayland_compositor()
  let l:env_cmd = 'WAYLAND_DISPLAY=' .. l:display .. ' '

  exe "wlrestore " .. l:display

  call assert_fails('put +', 'E353:')

  call assert_equal('', getline(1))

  call system(l:env_cmd .. 'wl-copy "test"')

  call assert_equal('test', getreg('+'))

  call system(l:env_cmd .. 'wl-copy --clear')

  call assert_fails('put +', 'E353:')

  exe "wlrestore " .. l:prev_display
  call End_wayland_compositor(l:wljob)
endfunc

" Check if correct mime types are advertised when we own the selection
func Test_wayland_mime_types_correct()
  set cpm=wayland

  let l:mimes = [
        \ '_VIMENC_TEXT',
        \ '_VIM_TEXT',
        \ 'text/plain;charset=utf-8',
        \ 'text/plain',
        \ 'UTF8_STRING',
        \ 'STRING',
        \ 'TEXT'
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

" Test if the _VIM_TEXT and _VIMENC_TEXT formats are correct
" _VIM_TEXT: preserves motion type (line/char/block wise)
" _VIMENC_TEXT: same but also indicates the encoding type
func Test_wayland_paste_vim_format_correct()
  set cpm=wayland

  set encoding=utf-8

  " Regular selection
  call setreg('+', 'text', 'c')
  call assert_equal("^@text", s:GetPaste('-t _VIM_TEXT'))
  call setreg('+', 'text', 'c')
  call assert_equal("^@utf-8^@text", s:GetPaste('-t _VIMENC_TEXT'))

  call setreg('+', 'text', 'l')
  call assert_equal("\x01text\n", s:GetPaste('-t _VIM_TEXT'))
  call setreg('+', 'text', 'l')
  call assert_equal("\x01utf-8^@text\n", s:GetPaste('-t _VIMENC_TEXT'))

  call setreg('+', 'text', 'b')
  call assert_equal("\x02text\n", s:GetPaste('-t _VIM_TEXT'))
  call setreg('+', 'text', 'b')
  call assert_equal("\x02utf-8^@text\n", s:GetPaste('-t _VIMENC_TEXT'))

  " Primary selection
  call setreg('*', 'text', 'c')
  call assert_equal("^@text", s:GetPaste('-p -t _VIM_TEXT'))
  call setreg('*', 'text', 'c')
  call assert_equal("^@utf-8^@text", s:GetPaste('-p -t _VIMENC_TEXT'))

  call setreg('*', 'text', 'l')
  call assert_equal("\x01text\n", s:GetPaste('-p -t _VIM_TEXT'))
  call setreg('*', 'text', 'l')
  call assert_equal("\x01utf-8^@text\n", s:GetPaste('-p -t _VIMENC_TEXT'))

  call setreg('*', 'text', 'b')
  call assert_equal("\x02text\n", s:GetPaste('-t _VIM_TEXT'))
  call setreg('*', 'text', 'b')
  call assert_equal("\x02utf-8^@text\n", s:GetPaste('-p -t _VIMENC_TEXT'))
endfunc

" Test checking if * and + registers are not the same
func Test_wayland_plus_star_not_same()
  new
  set cpm=wayland

  call system('wl-copy "regular"')
  call system('wl-copy -p "primary"')

  call assert_notequal(getreg('+'), getreg('*'))

  " Check if when we are the source client
  call setline(1, 'REGULAR')
  call setline(2, 'PRIMARY')

  execute '1yank +'
  execute '2yank *'

  call assert_notequal(system('wl-paste -p'), system('wl-paste'))
  bw!
endfunc

" Test if autoselect option in 'clipboard' works properly for wayland
func Test_wayland_autoselect_works()
  set cpm=wayland

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
  1
  call remote_send(l:name, "ve")
  sleep 100m
  call assert_equal('LINE', system('wl-paste -p -n'))

  call remote_send(l:name, "w")
  sleep 100m
  call assert_equal('LINE 1', system('wl-paste -p -n'))

  call remote_send(l:name, "V")
  sleep 100m
  call assert_equal("LINE 1\n", system('wl-paste -p -n'))

  " Reset cursor
  call remote_send(l:name, "\<Esc>:call cursor(1, 1)\<CR>")
  call assert_equal("LINE 1\n", system('wl-paste -p -n'))

  " Test visual block mode
  call remote_send(l:name, "\<C-q>jjj") " \<C-v> doesn't seem to work but \<C-q>
                                        " does...
  sleep 100m
  call assert_equal("L\nL\nL\n", system('wl-paste -p -n'))

  eval remote_send(l:name, "\<Esc>:qa!\<CR>")
  try
    call WaitForAssert({-> assert_equal("dead", job_status(l:job))})
  finally
    if job_status(l:job) != 'dead'
      call assert_report('Vim instance did not exit')
      call job_stop(l:job, 'kill')
    endif
  endtry
endfunc

" Check if the -Y flag works properly
func Test_no_wayland_connect_cmd_flag()
  set cpm=wayland

  let l:name = 'WLFLAGVIMTEST'
  let l:cmd = GetVimCommand() .. ' -Y --servername ' .. l:name
  let l:job = job_start(cmd, {'stoponexit': 'kill', 'out_io': 'null'})

  call WaitForAssert({-> assert_equal("run", job_status(l:job))})
  call WaitForAssert({-> assert_match(name, serverlist())})

  call assert_equal('', remote_expr(l:name, 'v:wayland_display'))
 
  call remote_send(l:name, ":wlrestore\<CR>")
  call assert_equal('', remote_expr(l:name, 'v:wayland_display'))

  call remote_send(l:name, ":wlrestore " .. $WAYLAND_DISPLAY .. "\<CR>")
  call assert_equal('', remote_expr(l:name, 'v:wayland_display'))

  call remote_send(l:name, ":wlrestore IDONTEXIST\<CR>")
  call assert_equal('', remote_expr(l:name, 'v:wayland_display'))

  eval remote_send(l:name, "\<Esc>:qa!\<CR>")
  try
    call WaitForAssert({-> assert_equal("dead", job_status(l:job))})
  finally
    if job_status(l:job) != 'dead'
      call assert_report('Vim instance did not exit')
      call job_stop(l:job, 'kill')
    endif
  endtry
endfunc

" Test if selection is disowned when we do something like suspend Vim
func Test_wayland_lose_selection()
  set cpm=wayland

  " Use a separate compositor instance to avoid clipboard managers
  let [l:display, l:wljob] = Start_wayland_compositor()
  let l:env_cmd = 'WAYLAND_DISPLAY=' .. l:display .. ' '

  let l:name = 'WLLOSEVIMTEST'
  let l:cmd = GetVimCommand() .. ' --servername ' .. l:name
  let l:job = job_start(cmd, {
        \ 'stoponexit': 'kill',
        \ 'out_io': 'null',
        \ 'env': {'WAYLAND_DISPLAY': l:display}
        \ })

  call WaitForAssert({-> assert_equal("run", job_status(l:job))})
  call WaitForAssert({-> assert_match(name, serverlist())})

  call remote_send(l:name, "iSOME TEXT\<Esc>\"+yy:let g:done = 1\<CR>")

  sleep 100m
  call assert_equal("SOME TEXT\n", system(l:env_cmd .. 'wl-paste -n'))

  call remote_send(l:name, "\<C-z>")

  sleep 100m
  call assert_equal("Nothing is copied\n", system(l:env_cmd .. 'wl-paste -n'))

  eval remote_send(l:name, "\<Esc>:qa!\<CR>")
  try
    call WaitForAssert({-> assert_equal("dead", job_status(l:job))})
  finally
    if job_status(l:job) != 'dead'
      call assert_report('Vim instance did not exit')
      call job_stop(l:job, 'kill')
    endif
  endtry

  call End_wayland_compositor(l:wljob)
endfunc

" Test wlseat option
func Test_wayland_seat()
  set cpm=wayland

  " Don't know a way to create a virtual seat so just test using the existing
  " one only
  set wlseat=seat0

  call system('wl-copy "TESTING"')
  call assert_equal('TESTING', getreg('+'))

  set wlseat=UNKNOWN

  call assert_equal('', getreg('+'))

  set wlseat=idontexist

  call assert_equal('', getreg('+'))

  set wlseat=seat0

  call assert_equal('TESTING', getreg('+'))

  set wlseat&
endfunc

" Test focus stealing
func Test_wayland_focus_steal()
  set cpm=wayland
  set wlstealf

  call system('wl-copy regular')

  call assert_equal('regular', getreg('+'))

  call system('wl-copy -p primary')

  call assert_equal('primary', getreg('*'))

  call setreg('+', 'REGULAR')

  call assert_equal('REGULAR', system('wl-paste -n'))

  call setreg('*', 'PRIMARY')

  call assert_equal('PRIMARY', system('wl-paste -p -n'))


  set nowlstealf
endfunc

" vim: shiftwidth=2 sts=2 expandtab
