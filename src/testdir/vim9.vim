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
func CheckDefFailure(lines, error, lnum = -1)
  call writefile(['def Func()'] + a:lines + ['enddef', 'defcompile'], 'Xdef')
  call assert_fails('so Xdef', a:error, a:lines, a:lnum)
  call delete('Xdef')
endfunc

" Check that "lines" inside ":def" results in an "error" message when executed.
" If "lnum" is given check that the error is reported for this line.
func CheckDefExecFailure(lines, error, lnum = -1)
  call writefile(['def Func()'] + a:lines + ['enddef'], 'Xdef')
  so Xdef
  call assert_fails('call Func()', a:error, a:lines, a:lnum)
  call delete('Xdef')
endfunc

def CheckScriptFailure(lines: list<string>, error: string)
  writefile(lines, 'Xdef')
  assert_fails('so Xdef', error, lines)
  delete('Xdef')
enddef

def CheckScriptSuccess(lines: list<string>)
  writefile(lines, 'Xdef')
  so Xdef
  delete('Xdef')
enddef
