source check.vim
source screendump.vim
source term_util.vim

func Test_stdin_no_newline()
  CheckScreendump
  CheckUnix
  CheckExecutable bash

  let $PS1 = 'TEST_PROMPT> '
  let buf = RunVimInTerminal('', #{rows: 20, cmd: 'bash --noprofile --norc'})
  call TermWait(buf, 1000)

  " Write input to temp file
  call term_sendkeys(buf, "echo hello > temp.txt\<CR>")
  call TermWait(buf, 500)

  call term_sendkeys(buf, "bash -c '../vim --not-a-term -u NONE -c \":q!\" -' < temp.txt\<CR>")
  call TermWait(buf, 3000)

  " Capture terminal output
  let lines = []
  for i in range(1, term_getsize(buf)[0])
    call add(lines, term_getline(buf, i))
  endfor

  " Find the command line in output
  let cmd_line = -1
  for i in range(len(lines))
    if lines[i] =~ '.*vim.*--not-a-term.*'
      let cmd_line = i
      break
    endif
  endfor

  if cmd_line == -1
    call assert_report('Command line not found in terminal output')
  else
    let next_line = -1
    for i in range(cmd_line + 1, len(lines))
      if lines[i] =~ '\S'
        let next_line = i
        break
      endif
    endfor

    if next_line == -1
      call assert_report('No prompt found after command execution')
    else
      call assert_equal(cmd_line + 1, next_line, 'Prompt should be on the immediate next line')
      call assert_match('.*TEST_PROMPT>.*', lines[next_line], 'Line should contain the prompt PS1')
    endif
  endif

  " Clean up temp file and exit shell
  call term_sendkeys(buf, "rm -f temp.txt\<CR>")
  call term_sendkeys(buf, "exit\<CR>")
  call TermWait(buf, 1000)

  if job_status(term_getjob(buf)) ==# 'run'
    call StopVimInTerminal(buf)
  endif

  unlet $PS1
endfunc

" vim: shiftwidth=2 sts=2 expandtab
