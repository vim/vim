" Utility functions for testing vim9 script

" Check that "lines" inside ":def" has no error.
func CheckDefSuccess(lines)
  call writefile(['def Func()'] + a:lines + ['enddef', 'defcompile'], 'Xdef')
  so Xdef
  call Func()
  call delete('Xdef')
endfunc

" Check that "lines" inside ":def" results in an "error" message.
" If "lnum" is given check that the error is reported for this line.
" Add a line before and after to make it less likely that the line number is
" accidentally correct.
func CheckDefFailure(lines, error, lnum = -3)
  call writefile(['def Func()', '# comment'] + a:lines + ['#comment', 'enddef', 'defcompile'], 'Xdef')
  call assert_fails('so Xdef', a:error, a:lines, a:lnum + 1)
  call delete('Xdef')
endfunc

" Check that "lines" inside ":def" results in an "error" message when executed.
" If "lnum" is given check that the error is reported for this line.
" Add a line before and after to make it less likely that the line number is
" accidentally correct.
func CheckDefExecFailure(lines, error, lnum = -3)
  call writefile(['def Func()', '# comment'] + a:lines + ['#comment', 'enddef'], 'Xdef')
  so Xdef
  call assert_fails('call Func()', a:error, a:lines, a:lnum + 1)
  call delete('Xdef')
endfunc

def CheckScriptFailure(lines: list<string>, error: string, lnum = -3)
  writefile(lines, 'Xdef')
  assert_fails('so Xdef', error, lines, lnum)
  delete('Xdef')
enddef

def CheckScriptSuccess(lines: list<string>)
  writefile(lines, 'Xdef')
  so Xdef
  delete('Xdef')
enddef

def CheckDefAndScriptSuccess(lines: list<string>)
  CheckDefSuccess(lines)
  CheckScriptSuccess(['vim9script'] + lines)
enddef

" Check that a command fails both when used in a :def function and when used
" in Vim9 script.
def CheckScriptAndDefFailure(lines: list<string>, error: string, lnum = -3)
  CheckDefFailure(lines, error, lnum)
  CheckScriptFailure(['vim9script'] + lines, error, lnum + 1)
enddef
