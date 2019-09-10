" Tests for setting 'buftype' to "prompt"

source check.vim
CheckFeature channel

source shared.vim
source screendump.vim

func CanTestPromptBuffer()
  " We need to use a terminal window to be able to feed keys without leaving
  " Insert mode.
  if !has('terminal')
    return 0
  endif
  if has('win32')
    " TODO: make the tests work on MS-Windows
    return 0
  endif
  return 1
endfunc

func WriteScript(name)
  call writefile([
	\ 'func TextEntered(text)',
	\ '  if a:text == "exit"',
	\ '    " Reset &modified to allow the buffer to be closed.',
	\ '    set nomodified',
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
	\ '  " Reset &modified to allow the buffer to be closed.',
	\ '  set nomodified',
	\ 'endfunc',
	\ '',
	\ 'call setline(1, "other buffer")',
	\ 'set nomodified',
	\ 'new',
	\ 'set buftype=prompt',
	\ 'call prompt_setcallback(bufnr(""), function("TextEntered"))',
	\ 'eval bufnr("")->prompt_setprompt("cmd: ")',
	\ 'startinsert',
	\ ], a:name)
endfunc

func Test_prompt_basic()
  if !CanTestPromptBuffer()
    return
  endif
  let scriptName = 'XpromptscriptBasic'
  call WriteScript(scriptName)

  let buf = RunVimInTerminal('-S ' . scriptName, {})
  call WaitForAssert({-> assert_equal('cmd:', term_getline(buf, 1))})

  call term_sendkeys(buf, "hello\<CR>")
  call WaitForAssert({-> assert_equal('cmd: hello', term_getline(buf, 1))})
  call WaitForAssert({-> assert_equal('Command: "hello"', term_getline(buf, 2))})
  call WaitForAssert({-> assert_equal('Result: "hello"', term_getline(buf, 3))})

  call term_sendkeys(buf, "exit\<CR>")
  call WaitForAssert({-> assert_equal('other buffer', term_getline(buf, 1))})

  call StopVimInTerminal(buf)
  call delete(scriptName)
endfunc

func Test_prompt_editing()
  if !CanTestPromptBuffer()
    return
  endif
  let scriptName = 'XpromptscriptEditing'
  call WriteScript(scriptName)

  let buf = RunVimInTerminal('-S ' . scriptName, {})
  call WaitForAssert({-> assert_equal('cmd:', term_getline(buf, 1))})

  let bs = "\<BS>"
  call term_sendkeys(buf, "hello" . bs . bs)
  call WaitForAssert({-> assert_equal('cmd: hel', term_getline(buf, 1))})

  let left = "\<Left>"
  call term_sendkeys(buf, left . left . left . bs . '-')
  call WaitForAssert({-> assert_equal('cmd: -hel', term_getline(buf, 1))})

  let end = "\<End>"
  call term_sendkeys(buf, end . "x")
  call WaitForAssert({-> assert_equal('cmd: -helx', term_getline(buf, 1))})

  call term_sendkeys(buf, "\<C-U>exit\<CR>")
  call WaitForAssert({-> assert_equal('other buffer', term_getline(buf, 1))})

  call StopVimInTerminal(buf)
  call delete(scriptName)
endfunc

func Test_prompt_garbage_collect()
  func MyPromptCallback(x, text)
    " NOP
  endfunc
  func MyPromptInterrupt(x)
    " NOP
  endfunc

  new
  set buftype=prompt
  eval bufnr('')->prompt_setcallback(function('MyPromptCallback', [{}]))
  eval bufnr('')->prompt_setinterrupt(function('MyPromptInterrupt', [{}]))
  call test_garbagecollect_now()
  " Must not crash
  call feedkeys("\<CR>\<C-C>", 'xt')
  call assert_true(v:true)

  delfunc MyPromptCallback
  bwipe!
endfunc
