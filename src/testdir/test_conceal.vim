" Tests for 'conceal'.
" Also see test88.in (should be converted to a test function here).

if !has('conceal')
  finish
endif

source screendump.vim
if !CanRunVimInTerminal()
  finish
endif

func Test_conceal_two_windows()
  let code =<< trim [CODE]
    let lines = ["one one one one one", "two |hidden| here", "three |hidden| three"]
    call setline(1, lines)
    syntax match test /|hidden|/ conceal
    set conceallevel=2
    set concealcursor=
    exe "normal /here\r"
    new
    call setline(1, lines)
    call setline(4, "Second window")
    syntax match test /|hidden|/ conceal
    set conceallevel=2
    set concealcursor=nc
    exe "normal /here\r"
  [CODE]

  call writefile(code, 'XTest_conceal')
  " Check that cursor line is concealed
  let buf = RunVimInTerminal('-S XTest_conceal', {})
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_01', {})

  " Check that with concealed text vertical cursor movement is correct.
  call term_sendkeys(buf, "k")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_02', {})

  " Check that with cursor line is not concealed
  call term_sendkeys(buf, "j")
  call term_sendkeys(buf, ":set concealcursor=\r")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_03', {})

  " Check that with cursor line is not concealed when moving cursor down
  call term_sendkeys(buf, "j")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_04', {})

  " Check that with cursor line is not concealed when switching windows
  call term_sendkeys(buf, "\<C-W>\<C-W>")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_05', {})

  " Check that with cursor line is only concealed in Normal mode
  call term_sendkeys(buf, ":set concealcursor=n\r")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_06n', {})
  call term_sendkeys(buf, "a")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_06i', {})
  call term_sendkeys(buf, "\<Esc>/e")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_06c', {})
  call term_sendkeys(buf, "\<Esc>v")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_06v', {})
  call term_sendkeys(buf, "\<Esc>")

  " Check that with cursor line is only concealed in Insert mode
  call term_sendkeys(buf, ":set concealcursor=i\r")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_07n', {})
  call term_sendkeys(buf, "a")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_07i', {})
  call term_sendkeys(buf, "\<Esc>/e")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_07c', {})
  call term_sendkeys(buf, "\<Esc>v")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_07v', {})
  call term_sendkeys(buf, "\<Esc>")

  " Check that with cursor line is only concealed in Command mode
  call term_sendkeys(buf, ":set concealcursor=c\r")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_08n', {})
  call term_sendkeys(buf, "a")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_08i', {})
  call term_sendkeys(buf, "\<Esc>/e")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_08c', {})
  call term_sendkeys(buf, "\<Esc>v")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_08v', {})
  call term_sendkeys(buf, "\<Esc>")

  " Check that with cursor line is only concealed in Visual mode
  call term_sendkeys(buf, ":set concealcursor=v\r")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_09n', {})
  call term_sendkeys(buf, "a")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_09i', {})
  call term_sendkeys(buf, "\<Esc>/e")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_09c', {})
  call term_sendkeys(buf, "\<Esc>v")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_09v', {})
  call term_sendkeys(buf, "\<Esc>")

  " Check moving the cursor while in insert mode.
  call term_sendkeys(buf, ":set concealcursor=\r")
  call term_sendkeys(buf, "a")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_10', {})
  call term_sendkeys(buf, "\<Down>")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_11', {})
  call term_sendkeys(buf, "\<Esc>")

  " Check the "o" command
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_12', {})
  call term_sendkeys(buf, "o")
  call VerifyScreenDump(buf, 'Test_conceal_two_windows_13', {})
  call term_sendkeys(buf, "\<Esc>")

  " clean up
  call StopVimInTerminal(buf)
  call delete('XTest_conceal')
endfunc

func Test_conceal_with_cursorline()
  " Opens a help window, where 'conceal' is set, switches to the other window
  " where 'cursorline' needs to be updated when the cursor moves.
  let code =<< trim [CODE]
    set cursorline
    normal othis is a test
    new
    call setline(1, ["one", "two", "three", "four", "five"])
    set ft=help
    normal M
  [CODE]

  call writefile(code, 'XTest_conceal_cul')
  let buf = RunVimInTerminal('-S XTest_conceal_cul', {})
  call VerifyScreenDump(buf, 'Test_conceal_cul_01', {})

  call term_sendkeys(buf, ":wincmd w\r")
  call VerifyScreenDump(buf, 'Test_conceal_cul_02', {})

  call term_sendkeys(buf, "k")
  call VerifyScreenDump(buf, 'Test_conceal_cul_03', {})

  " clean up
  call StopVimInTerminal(buf)
  call delete('XTest_conceal_cul')
endfunc
