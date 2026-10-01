" Test for the HelpTOC package

source util/screendump.vim

func Test_helptoc_markdown_with_comments()
  CheckScreendump
  let lines =<< trim END
    packadd helptoc
    e Xmarkdown.md
    call append(0, [
          \ '# Heading 1',
          \ '',
          \ 'Some text.',
          \ '',
          \ '```vim',
          \ '# This is a Vim9Script comment',
          \ 'def MyFunc()',
          \ '  # Another comment',
          \ 'enddef',
          \ '```',
          \ '# Another Heading 1',
          \ '',
          \ '```',
          \ '# This is a comment in a codeblock',
          \ '```',
          \ '# Last Heading 1',
          \ 'and more text'
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

func Test_helptoc_fuzzy_multibyte()
  packadd helptoc
  new Xhelptoc_md.md
  call setline(1, ['# 中文标题'])
  setfiletype markdown

  HelpToc
  redraw
  " Drive the fuzzy search through the popup filter: "/" opens the input()
  " prompt, the query chars are typed into it, <Esc> leaves it.
  call feedkeys("/标题\<Esc>", 'xt')
  redraw

  let winid = popup_list()[0]
  let buf = winbufnr(winid)
  let props = []
  for lnum in range(1, line('$', winid))
    for p in prop_list(lnum, {'bufnr': buf})
      if p.type == 'help-fuzzy-toc'
        call add(props, [p.col, p.length])
      endif
    endfor
  endfor
  " 中文标题: 中/文 are 3 bytes each, so 标 starts at byte 6 (col 7) and 题 at
  " byte 9 (col 10), each 3 bytes long.
  call assert_equal([[7, 3], [10, 3]], props)

  call popup_close(winid)
  bwipe!
endfunc
