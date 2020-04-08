" Tests for :messages, :echomsg, :echoerr

source shared.vim
source term_util.vim

function Test_messages()
  let oldmore = &more
  try
    set nomore
    " Avoid the "message maintainer" line.
    let $LANG = ''

    let arr = map(range(10), '"hello" . v:val')
    for s in arr
      echomsg s | redraw
    endfor
    let result = ''

    " get last two messages
    redir => result
    2messages | redraw
    redir END
    let msg_list = split(result, "\n")
    call assert_equal(["hello8", "hello9"], msg_list)

    " clear messages without last one
    1messages clear
    redir => result
    redraw | messages
    redir END
    let msg_list = split(result, "\n")
    call assert_equal(['hello9'], msg_list)

    " clear all messages
    messages clear
    redir => result
    redraw | messages
    redir END
    call assert_equal('', result)
  finally
    let &more = oldmore
  endtry
endfunction

" Patch 7.4.1696 defined the "clearmode()" function for clearing the mode
" indicator (e.g., "-- INSERT --") when ":stopinsert" is invoked.  Message
" output could then be disturbed when 'cmdheight' was greater than one.
" This test ensures that the bugfix for this issue remains in place.
func Test_stopinsert_does_not_break_message_output()
  set cmdheight=2
  redraw!

  stopinsert | echo 'test echo'
  call assert_equal(116, screenchar(&lines - 1, 1))
  call assert_equal(32, screenchar(&lines, 1))
  redraw!

  stopinsert | echomsg 'test echomsg'
  call assert_equal(116, screenchar(&lines - 1, 1))
  call assert_equal(32, screenchar(&lines, 1))
  redraw!

  set cmdheight&
endfunc

func Test_message_completion()
  call feedkeys(":message \<C-A>\<C-B>\"\<CR>", 'tx')
  call assert_equal('"message clear', @:)
endfunc

func Test_echomsg()
  call assert_equal("\nhello", execute(':echomsg "hello"'))
  call assert_equal("\n", execute(':echomsg ""'))
  call assert_equal("\n12345", execute(':echomsg 12345'))
  call assert_equal("\n[]", execute(':echomsg []'))
  call assert_equal("\n[1, 2, 3]", execute(':echomsg [1, 2, 3]'))
  call assert_equal("\n{}", execute(':echomsg {}'))
  call assert_equal("\n{'a': 1, 'b': 2}", execute(':echomsg {"a": 1, "b": 2}'))
  if has('float')
    call assert_equal("\n1.23", execute(':echomsg 1.23'))
  endif
  call assert_match("function('<lambda>\\d*')", execute(':echomsg {-> 1234}'))
endfunc

func Test_echoerr()
  call test_ignore_error('IgNoRe')
  call assert_equal("\nIgNoRe hello", execute(':echoerr "IgNoRe hello"'))
  call assert_equal("\n12345 IgNoRe", execute(':echoerr 12345 "IgNoRe"'))
  call assert_equal("\n[1, 2, 'IgNoRe']", execute(':echoerr [1, 2, "IgNoRe"]'))
  call assert_equal("\n{'IgNoRe': 2, 'a': 1}", execute(':echoerr {"a": 1, "IgNoRe": 2}'))
  if has('float')
    call assert_equal("\n1.23 IgNoRe", execute(':echoerr 1.23 "IgNoRe"'))
  endif
  eval '<lambda>'->test_ignore_error()
  call assert_match("function('<lambda>\\d*')", execute(':echoerr {-> 1234}'))
  call test_ignore_error('RESET')
endfunc

func Test_mode_message_at_leaving_insert_by_ctrl_c()
  if !has('terminal') || has('gui_running')
    return
  endif

  " Set custom statusline built by user-defined function.
  let testfile = 'Xtest.vim'
  let lines =<< trim END
        func StatusLine() abort
          return ""
        endfunc
        set statusline=%!StatusLine()
        set laststatus=2
  END
  call writefile(lines, testfile)

  let rows = 10
  let buf = term_start([GetVimProg(), '--clean', '-S', testfile], {'term_rows': rows})
  call TermWait(buf, 100)
  call assert_equal('run', job_status(term_getjob(buf)))

  call term_sendkeys(buf, "i")
  call WaitForAssert({-> assert_match('^-- INSERT --\s*$', term_getline(buf, rows))})
  call term_sendkeys(buf, "\<C-C>")
  call WaitForAssert({-> assert_match('^\s*$', term_getline(buf, rows))})

  call term_sendkeys(buf, ":qall!\<CR>")
  call WaitForAssert({-> assert_equal('dead', job_status(term_getjob(buf)))})
  exe buf . 'bwipe!'
  call delete(testfile)
endfunc

func Test_mode_message_at_leaving_insert_with_esc_mapped()
  if !has('terminal') || has('gui_running')
    return
  endif

  " Set custom statusline built by user-defined function.
  let testfile = 'Xtest.vim'
  let lines =<< trim END
        set laststatus=2
        inoremap <Esc> <Esc>00
  END
  call writefile(lines, testfile)

  let rows = 10
  let buf = term_start([GetVimProg(), '--clean', '-S', testfile], {'term_rows': rows})
  call WaitForAssert({-> assert_match('0,0-1\s*All$', term_getline(buf, rows - 1))})
  call assert_equal('run', job_status(term_getjob(buf)))

  call term_sendkeys(buf, "i")
  call WaitForAssert({-> assert_match('^-- INSERT --\s*$', term_getline(buf, rows))})
  call term_sendkeys(buf, "\<Esc>")
  call WaitForAssert({-> assert_match('^\s*$', term_getline(buf, rows))})

  call term_sendkeys(buf, ":qall!\<CR>")
  call WaitForAssert({-> assert_equal('dead', job_status(term_getjob(buf)))})
  exe buf . 'bwipe!'
  call delete(testfile)
endfunc

func Test_echospace()
  set noruler noshowcmd laststatus=1
  call assert_equal(&columns - 1, v:echospace)
  split
  call assert_equal(&columns - 1, v:echospace)
  set ruler
  call assert_equal(&columns - 1, v:echospace)
  close
  call assert_equal(&columns - 19, v:echospace)
  set showcmd noruler
  call assert_equal(&columns - 12, v:echospace)
  set showcmd ruler
  call assert_equal(&columns - 29, v:echospace)

  set ruler& showcmd&
endfunc

" Test more-prompt (see :help more-prompt).
func Test_message_more()
  if !CanRunVimInTerminal()
    throw 'Skipped: cannot run vim in terminal'
  endif
  let buf = RunVimInTerminal('', {'rows': 6})
  call term_sendkeys(buf, ":call setline(1, range(1, 100))\n")

  call term_sendkeys(buf, ":%p#\n")
  call WaitForAssert({-> assert_equal('  5 5', term_getline(buf, 5))})
  call WaitForAssert({-> assert_equal('-- More --', term_getline(buf, 6))})

  call term_sendkeys(buf, '?')
  call WaitForAssert({-> assert_equal('  5 5', term_getline(buf, 5))})
  call WaitForAssert({-> assert_equal('-- More -- SPACE/d/j: screen/page/line down, b/u/k: up, q: quit ', term_getline(buf, 6))})

  " Down a line with j, <CR>, <NL> or <Down>.
  call term_sendkeys(buf, "j")
  call WaitForAssert({-> assert_equal('  6 6', term_getline(buf, 5))})
  call WaitForAssert({-> assert_equal('-- More --', term_getline(buf, 6))})
  call term_sendkeys(buf, "\<NL>")
  call WaitForAssert({-> assert_equal('  7 7', term_getline(buf, 5))})
  call term_sendkeys(buf, "\<CR>")
  call WaitForAssert({-> assert_equal('  8 8', term_getline(buf, 5))})
  call term_sendkeys(buf, "\<Down>")
  call WaitForAssert({-> assert_equal('  9 9', term_getline(buf, 5))})

  " Down a screen with <Space>, f, or <PageDown>.
  call term_sendkeys(buf, 'f')
  call WaitForAssert({-> assert_equal(' 14 14', term_getline(buf, 5))})
  call WaitForAssert({-> assert_equal('-- More --', term_getline(buf, 6))})
  call term_sendkeys(buf, ' ')
  call WaitForAssert({-> assert_equal(' 19 19', term_getline(buf, 5))})
  call term_sendkeys(buf, "\<PageDown>")
  call WaitForAssert({-> assert_equal(' 24 24', term_getline(buf, 5))})

  " Down a page (half a screen) with d.
  call term_sendkeys(buf, 'd')
  call WaitForAssert({-> assert_equal(' 27 27', term_getline(buf, 5))})

  " Down all the way with 'G'.
  call term_sendkeys(buf, 'G')
  call WaitForAssert({-> assert_equal('100 100', term_getline(buf, 5))})
  call WaitForAssert({-> assert_equal('Press ENTER or type command to continue', term_getline(buf, 6))})

  " Up a line k, <BS> or <Up>.
  call term_sendkeys(buf, 'k')
  call WaitForAssert({-> assert_equal(' 99 99', term_getline(buf, 5))})
  call term_sendkeys(buf, "\<BS>")
  call WaitForAssert({-> assert_equal(' 98 98', term_getline(buf, 5))})
  call term_sendkeys(buf, "\<Up>")
  call WaitForAssert({-> assert_equal(' 97 97', term_getline(buf, 5))})

  " Up a screen with b or <PageUp>.
  call term_sendkeys(buf, 'b')
  call WaitForAssert({-> assert_equal(' 92 92', term_getline(buf, 5))})
  call term_sendkeys(buf, "\<PageUp>")
  call WaitForAssert({-> assert_equal(' 87 87', term_getline(buf, 5))})

  " Up a page (half a screen) with u.
  call term_sendkeys(buf, 'u')
  call WaitForAssert({-> assert_equal(' 84 84', term_getline(buf, 5))})

  " Up all the way with 'g'.
  call term_sendkeys(buf, 'g')
  call WaitForAssert({-> assert_equal('  5 5', term_getline(buf, 5))})
  call WaitForAssert({-> assert_equal('-- More --', term_getline(buf, 6))})

  " All the way down. Pressing f should do nothing but pressing
  " space should end the more prompt.
  call term_sendkeys(buf, 'G')
  call WaitForAssert({-> assert_equal('100 100', term_getline(buf, 5))})
  call WaitForAssert({-> assert_equal('Press ENTER or type command to continue', term_getline(buf, 6))})
  call term_sendkeys(buf, 'f')
  call WaitForAssert({-> assert_equal('100 100', term_getline(buf, 5))})
  call term_sendkeys(buf, ' ')
  call WaitForAssert({-> assert_equal('100', term_getline(buf, 5))})

  " Pressing g< shows the previous command output.
  call term_sendkeys(buf, 'g<')
  call WaitForAssert({-> assert_equal('100 100', term_getline(buf, 5))})
  call WaitForAssert({-> assert_equal('Press ENTER or type command to continue', term_getline(buf, 6))})

  call term_sendkeys(buf, ":%p#\n")
  call WaitForAssert({-> assert_equal('  5 5', term_getline(buf, 5))})
  call WaitForAssert({-> assert_equal('-- More --', term_getline(buf, 6))})

  " Stop command output with q, <Esc> or CTRL-C.
  call term_sendkeys(buf, 'q')
  call WaitForAssert({-> assert_equal('100', term_getline(buf, 5))})

  call StopVimInTerminal(buf)
endfunc

func Test_ask_yesno()
  if !CanRunVimInTerminal()
    throw 'Skipped: cannot run vim in terminal'
  endif
  let buf = RunVimInTerminal('', {'rows': 6})
  call term_sendkeys(buf, ":call setline(1, range(1, 2))\n")

  call term_sendkeys(buf, ":2,1s/^/n/\n")
  call WaitForAssert({-> assert_equal('Backwards range given, OK to swap (y/n)?', term_getline(buf, 6))})
  call term_sendkeys(buf, "n")
  call WaitForAssert({-> assert_match('^Backwards range given, OK to swap (y/n)?n *1,1 *All$', term_getline(buf, 6))})
  call WaitForAssert({-> assert_equal('1', term_getline(buf, 1))})

  call term_sendkeys(buf, ":2,1s/^/Esc/\n")
  call WaitForAssert({-> assert_equal('Backwards range given, OK to swap (y/n)?', term_getline(buf, 6))})
  call term_sendkeys(buf, "\<Esc>")
  call WaitForAssert({-> assert_match('^Backwards range given, OK to swap (y/n)?n *1,1 *All$', term_getline(buf, 6))})
  call WaitForAssert({-> assert_equal('1', term_getline(buf, 1))})

  call term_sendkeys(buf, ":2,1s/^/y/\n")
  call WaitForAssert({-> assert_equal('Backwards range given, OK to swap (y/n)?', term_getline(buf, 6))})
  call term_sendkeys(buf, "y")
  call WaitForAssert({-> assert_match('^Backwards range given, OK to swap (y/n)?y *2,1 *All$', term_getline(buf, 6))})
  call WaitForAssert({-> assert_equal('y1', term_getline(buf, 1))})
  call WaitForAssert({-> assert_equal('y2', term_getline(buf, 2))})

  call StopVimInTerminal(buf)
endfunc

func Test_null()
  echom test_null_list()
  echom test_null_dict()
  echom test_null_blob()
  echom test_null_string()
  echom test_null_partial()
  if has('job')
    echom test_null_job()
    echom test_null_channel()
  endif
endfunc
