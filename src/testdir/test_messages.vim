" Tests for :messages, :echomsg, :echoerr

source shared.vim

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
  call test_ignore_error('<lambda>')
  call assert_match("function('<lambda>\\d*')", execute(':echoerr {-> 1234}'))
  call test_ignore_error('RESET')
endfunc

func Test_mode_message_at_leaving_insert_by_ctrl_c()
  if !has('terminal') || has('gui_running')
    return
  endif

  " Set custom statusline built by user-defined function.
  let testfile = 'Xtest.vim'
  call writefile([
        \ 'func StatusLine() abort',
        \ '  return ""',
        \ 'endfunc',
        \ 'set statusline=%!StatusLine()',
        \ 'set laststatus=2',
        \ ], testfile)

  let rows = 10
  let buf = term_start([GetVimProg(), '--clean', '-S', testfile], {'term_rows': rows})
  call term_wait(buf, 200)
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
  call writefile([
        \ 'set laststatus=2',
        \ 'inoremap <Esc> <Esc>00',
        \ ], testfile)

  let rows = 10
  let buf = term_start([GetVimProg(), '--clean', '-S', testfile], {'term_rows': rows})
  call term_wait(buf, 200)
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
