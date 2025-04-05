source check.vim
source screendump.vim
source term_util.vim

func Test_stdin_no_newline()
  CheckScreendump
  CheckUnix
  CheckExecutable echo

  let $PS1 = 'TEST_PROMPT> '
  let buf = RunVimInTerminal('', #{rows: 10, cmd: 'sh'})
  call TermWait(buf, 500)  " Wait for shell to start


  call term_sendkeys(buf, "echo hello | ../vim --not-a-term -u NONE -c ':q!' -\<CR>")

  call TermWait(buf, 2000)

  " colleting all lines after cmd execution in vim terminal
  let lines = []
  for i in range(1, term_getsize(buf)[0])
    let line = term_getline(buf, i)
    call add(lines, line)
  endfor

  " Find the command line first as it will appear
  let cmd_line = -1
  for i in range(len(lines))
    if lines[i] =~ '.*echo hello.*vim.*'
      let cmd_line = i
      break
    endif
  endfor

  " Check that the next non-empty line is the TEST_PROMPT>
  if cmd_line == -1
    call assert_report('Command line not found in terminal output')
  else
    let next_line = -1
    for i in range(cmd_line + 1, len(lines))
      if i < len(lines) && lines[i] != ''
        let next_line = i
        break
      endif
    endfor

    if next_line == -1
      call assert_report('No prompt found after command execution')
    else
      call assert_equal(cmd_line + 1, next_line, 'Prompt should be on the immediate next line')
      call assert_match($PS1, lines[next_line], 'Line should contain the prompt PS1')
    endif
  endif


  call term_sendkeys(buf, "exit\<CR>")
  call TermWait(buf, 1000)

  " Only try to stop if job is still running
  if job_status(term_getjob(buf)) ==# 'run'
    call StopVimInTerminal(buf)
  endif

  unlet $PS1
endfunc

" vim: shiftwidth=2 sts=2 expandtab
