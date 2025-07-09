" Test for the HelpTOC package

source util/screendump.vim

func Test_helptoc_markdown_with_comments()
  CheckScreendump
  let lines =<< trim END
    packadd helptoc
    e Xmarkdown.md
    call append(0, [
          \ '# My Document',
          \ '', 'Some text.',
          \ '',
          \ '```vim',
          \ '# This is a Vim9Script comment',
          \ 'def MyFunc()',
          \ '  # Another comment',
          \ 'enddef',
          \ '```'
          \])
  END

  let input_file = "Xhelptoc.vim"
  call writefile(lines, input_file, "D")

  let buf = RunVimInTerminal($"-S {input_file}", {})
  call term_sendkeys(buf, ":HelpToc\<cr>")
  call WaitFor({-> term_getline(buf, 1) =~ 'press ? for help'})
  call VerifyScreenDump(buf, 'Test_helptoc_markdown_01', {})
  call term_sendkeys(buf, ":qa!\<cr>")
  call StopVimInTerminal(buf)
endfunc
