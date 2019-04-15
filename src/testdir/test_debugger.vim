" Tests for the Vim debugger

source shared.vim
source screendump.vim

" Run a Vim debugger command
" If the expected output argument is supplied, then check for it.
func RunDbgCmd(buf, cmd, ...)
  call term_sendkeys(a:buf, a:cmd . "\r")
  call term_wait(a:buf)

  if a:0 != 0
    " Verify the expected output
    let lnum = 20 - len(a:1)
    for l in a:1
      call WaitForAssert({-> assert_equal(l, term_getline(a:buf, lnum))})
      let lnum += 1
    endfor
  endif
endfunc

" Debugger tests
func Test_Debugger()
  if !CanRunVimInTerminal()
    return
  endif

  " Create a Vim script with some functions
  call writefile([
	      \ 'func Foo()',
	      \ '  let var1 = 1',
	      \ '  let var2 = Bar(var1) + 9',
	      \ '  return var2',
	      \ 'endfunc',
	      \ 'func Bar(var)',
	      \ '  let var1 = 2 + a:var',
	      \ '  let var2 = Bazz(var1) + 4',
	      \ '  return var2',
	      \ 'endfunc',
	      \ 'func Bazz(var)',
	      \ '  let var1 = 3 + a:var',
	      \ '  let var3 = "another var"',
	      \ '  return var1',
	      \ 'endfunc'], 'Xtest.vim')

  " Start Vim in a terminal
  let buf = RunVimInTerminal('-S Xtest.vim', {})

  " Start the Vim debugger
  call RunDbgCmd(buf, ':debug echo Foo()')

  " Create a few stack frames by stepping through functions
  call RunDbgCmd(buf, 'step')
  call RunDbgCmd(buf, 'step')
  call RunDbgCmd(buf, 'step')
  call RunDbgCmd(buf, 'step')
  call RunDbgCmd(buf, 'step')
  call RunDbgCmd(buf, 'step')

  " check backtrace
  call RunDbgCmd(buf, 'backtrace', [
	      \ '  2 function Foo[2]',
	      \ '  1 Bar[2]',
	      \ '->0 Bazz',
	      \ 'line 2: let var3 = "another var"'])

  " Check variables in different stack frames
  call RunDbgCmd(buf, 'echo var1', ['6'])

  call RunDbgCmd(buf, 'up')
  call RunDbgCmd(buf, 'back', [
	      \ '  2 function Foo[2]',
	      \ '->1 Bar[2]',
	      \ '  0 Bazz',
	      \ 'line 2: let var3 = "another var"'])
  call RunDbgCmd(buf, 'echo var1', ['3'])

  call RunDbgCmd(buf, 'u')
  call RunDbgCmd(buf, 'bt', [
	      \ '->2 function Foo[2]',
	      \ '  1 Bar[2]',
	      \ '  0 Bazz',
	      \ 'line 2: let var3 = "another var"'])
  call RunDbgCmd(buf, 'echo var1', ['1'])

  " Undefined variables
  call RunDbgCmd(buf, 'step')
  call RunDbgCmd(buf, 'frame 2')
  call RunDbgCmd(buf, 'echo var3', [
	\ 'Error detected while processing function Foo[2]..Bar[2]..Bazz:',
	\ 'line    3:',
	\ 'E121: Undefined variable: var3'])

  " var3 is defined in this level with some other value
  call RunDbgCmd(buf, 'fr 0')
  call RunDbgCmd(buf, 'echo var3', ['another var'])

  call RunDbgCmd(buf, 'step')
  call RunDbgCmd(buf, 'step')
  call RunDbgCmd(buf, 'step')
  call RunDbgCmd(buf, 'up')

  " Undefined var2
  call RunDbgCmd(buf, 'echo var2', [
	      \ 'Error detected while processing function Foo[2]..Bar:',
	      \ 'line    3:',
	      \ 'E121: Undefined variable: var2'])

  " Var2 is defined with 10
  call RunDbgCmd(buf, 'down')
  call RunDbgCmd(buf, 'echo var2', ['10'])

  " Backtrace movements
  call RunDbgCmd(buf, 'b', [
	      \ '  1 function Foo[2]',
	      \ '->0 Bar',
	      \ 'line 3: End of function'])

  " next command cannot go down, we are on bottom
  call RunDbgCmd(buf, 'down', ['frame is zero'])
  call RunDbgCmd(buf, 'up')

  " next command cannot go up, we are on top
  call RunDbgCmd(buf, 'up', ['frame at highest level: 1'])
  call RunDbgCmd(buf, 'b', [
	      \ '->1 function Foo[2]',
	      \ '  0 Bar',
	      \ 'line 3: End of function'])

  " fil is not frame or finish, it is file
  call RunDbgCmd(buf, 'fil', ['"[No Name]" --No lines in buffer--'])

  " relative backtrace movement
  call RunDbgCmd(buf, 'fr -1')
  call RunDbgCmd(buf, 'frame', [
	      \ '  1 function Foo[2]',
	      \ '->0 Bar',
	      \ 'line 3: End of function'])

  call RunDbgCmd(buf, 'fr +1')
  call RunDbgCmd(buf, 'fram', [
	      \ '->1 function Foo[2]',
	      \ '  0 Bar',
	      \ 'line 3: End of function'])

  " go beyond limits does not crash
  call RunDbgCmd(buf, 'fr 100', ['frame at highest level: 1'])
  call RunDbgCmd(buf, 'fra', [
	      \ '->1 function Foo[2]',
	      \ '  0 Bar',
	      \ 'line 3: End of function'])

  call RunDbgCmd(buf, 'frame -40', ['frame is zero'])
  call RunDbgCmd(buf, 'fram', [
	      \ '  1 function Foo[2]',
	      \ '->0 Bar',
	      \ 'line 3: End of function'])

  " final result 19
  call RunDbgCmd(buf, 'cont', ['19'])

  call StopVimInTerminal(buf)

  call delete('Xtest.vim')
endfunc
