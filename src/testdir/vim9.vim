" Utility functions for testing vim9 script

" Use a different file name for each run.
let s:sequence = 1

" Check that "lines" inside a ":def" function has no error.
func CheckDefSuccess(lines)
  let cwd = getcwd()
  let fname = 'XdefSuccess' .. s:sequence
  let s:sequence += 1
  call writefile(['def Func()'] + a:lines + ['enddef', 'defcompile'], fname)
  try
    exe 'so ' .. fname
    call Func()
    delfunc! Func
  finally
    call chdir(cwd)
    call delete(fname)
  endtry
endfunc

" Check that "lines" inside ":def" results in an "error" message.
" If "lnum" is given check that the error is reported for this line.
" Add a line before and after to make it less likely that the line number is
" accidentally correct.
func CheckDefFailure(lines, error, lnum = -3)
  let cwd = getcwd()
  let fname = 'XdefFailure' .. s:sequence
  let s:sequence += 1
  call writefile(['def Func()', '# comment'] + a:lines + ['#comment', 'enddef', 'defcompile'], fname)
  try
    call assert_fails('so ' .. fname, a:error, a:lines, a:lnum + 1)
  finally
    call chdir(cwd)
    call delete(fname)
    delfunc! Func
  endtry
endfunc

" Check that "lines" inside ":def" results in an "error" message when executed.
" If "lnum" is given check that the error is reported for this line.
" Add a line before and after to make it less likely that the line number is
" accidentally correct.
func CheckDefExecFailure(lines, error, lnum = -3)
  let cwd = getcwd()
  let fname = 'XdefExecFailure' .. s:sequence
  let s:sequence += 1
  call writefile(['def Func()', '# comment'] + a:lines + ['#comment', 'enddef'], fname)
  try
    exe 'so ' .. fname
    call assert_fails('call Func()', a:error, a:lines, a:lnum + 1)
  finally
    call chdir(cwd)
    call delete(fname)
    delfunc! Func
  endtry
endfunc

def CheckScriptFailure(lines: list<string>, error: string, lnum = -3)
  var cwd = getcwd()
  var fname = 'XScriptFailure' .. s:sequence
  s:sequence += 1
  writefile(lines, fname)
  try
    assert_fails('so ' .. fname, error, lines, lnum)
  finally
    chdir(cwd)
    delete(fname)
  endtry
enddef

def CheckScriptSuccess(lines: list<string>)
  var cwd = getcwd()
  var fname = 'XScriptSuccess' .. s:sequence
  s:sequence += 1
  writefile(lines, fname)
  try
    exe 'so ' .. fname
  finally
    chdir(cwd)
    delete(fname)
  endtry
enddef

def CheckDefAndScriptSuccess(lines: list<string>)
  CheckDefSuccess(lines)
  CheckScriptSuccess(['vim9script'] + lines)
enddef

" Check that a command fails with the same error when used in a :def function
" and when used in Vim9 script.
def CheckDefAndScriptFailure(lines: list<string>, error: string, lnum = -3)
  CheckDefFailure(lines, error, lnum)
  CheckScriptFailure(['vim9script'] + lines, error, lnum + 1)
enddef

" Check that a command fails with the same error  when executed in a :def
" function and when used in Vim9 script.
def CheckDefExecAndScriptFailure(lines: list<string>, error: string, lnum = -3)
  CheckDefExecFailure(lines, error, lnum)
  CheckScriptFailure(['vim9script'] + lines, error, lnum + 1)
enddef
