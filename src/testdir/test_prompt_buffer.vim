" Tests for setting 'buftype' to "prompt"

if !has('channel')
  finish
endif

source shared.vim
source screendump.vim

func Test_prompt_basic()
  " We need to use a terminal window to be able to feed keys without leaving
  " Insert mode.
  if !has('terminal')
    call assert_report('no terminal')
    return
  endif
  call writefile([
	\ 'func TextEntered(text)',
	\ '  if a:text == "exit"',
	\ '    stopinsert',
	\ '    close',
	\ '  else',
	\ '    " Add the output above the current prompt.',
	\ '    call append(line("$") - 1, "Command: \"" . a:text . "\"")',
	\ '    " Reset &modified to allow the buffer to be closed.',
	\ '    set nomodified',
	\ '    call timer_start(20, {id -> TimerFunc(a:text)})',
	\ '  endif',
	\ 'endfunc',
	\ '',
	\ 'func TimerFunc(text)',
	\ '  " Add the output above the current prompt.',
	\ '  call append(line("$") - 1, "Result: \"" . a:text . "\"")',
	\ 'endfunc',
	\ '',
	\ 'call setline(1, "other buffer")',
	\ 'new',
	\ 'set buftype=prompt',
	\ 'call prompt_setcallback(bufnr(""), function("TextEntered"))',
	\ 'startinsert',
	\ ], 'Xpromptscript')
  let buf = RunVimInTerminal('-S Xpromptscript', {})
  call WaitForAssert({-> assert_equal('%', term_getline(buf, 1))})

  call term_sendkeys(buf, "hello\<CR>")
  call WaitForAssert({-> assert_equal('% hello', term_getline(buf, 1))})
  call WaitForAssert({-> assert_equal('Command: "hello"', term_getline(buf, 2))})
  call WaitForAssert({-> assert_equal('Result: "hello"', term_getline(buf, 3))})

  call term_sendkeys(buf, "exit\<CR>")
  call WaitForAssert({-> assert_equal('other buffer', term_getline(buf, 1))})

  call StopVimInTerminal(buf)
  call delete('Xpromptscript')
endfunc
