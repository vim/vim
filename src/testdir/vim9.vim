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

def CheckScriptFailureList(lines: list<string>, errors: list<string>, lnum = -3)
  var cwd = getcwd()
  var fname = 'XScriptFailure' .. s:sequence
  s:sequence += 1
  writefile(lines, fname)
  try
    assert_fails('so ' .. fname, errors, lines, lnum)
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

" As CheckDefAndScriptFailure() but with two different exepected errors.
def CheckDefAndScriptFailure2(
  	lines: list<string>,
	errorDef: string,
	errorScript: string,
	lnum = -3)
  CheckDefFailure(lines, errorDef, lnum)
  CheckScriptFailure(['vim9script'] + lines, errorScript, lnum + 1)
enddef

" Check that a command fails with the same error  when executed in a :def
" function and when used in Vim9 script.
def CheckDefExecAndScriptFailure(lines: list<string>, error: string, lnum = -3)
  CheckDefExecFailure(lines, error, lnum)
  CheckScriptFailure(['vim9script'] + lines, error, lnum + 1)
enddef

" As CheckDefExecAndScriptFailure() but with two different expected errors.
def CheckDefExecAndScriptFailure2(
  	lines: list<string>,
	errorDef: string,
	errorScript: string,
	lnum = -3)
  CheckDefExecFailure(lines, errorDef, lnum)
  CheckScriptFailure(['vim9script'] + lines, errorScript, lnum + 1)
enddef


" Check that "lines" inside a legacy function has no error.
func CheckLegacySuccess(lines)
  let cwd = getcwd()
  let fname = 'XlegacySuccess' .. s:sequence
  let s:sequence += 1
  call writefile(['func Func()'] + a:lines + ['endfunc'], fname)
  try
    exe 'so ' .. fname
    call Func()
  finally
    delfunc! Func
    call chdir(cwd)
    call delete(fname)
  endtry
endfunc

" Check that "lines" inside a legacy function results in the expected error
func CheckLegacyFailure(lines, error)
  let cwd = getcwd()
  let fname = 'XlegacyFails' .. s:sequence
  let s:sequence += 1
  call writefile(['func Func()'] + a:lines + ['endfunc', 'call Func()'], fname)
  try
    call assert_fails('so ' .. fname, a:error)
  finally
    delfunc! Func
    call chdir(cwd)
    call delete(fname)
  endtry
endfunc

" Execute "lines" in a legacy function, :def function and Vim9 script.
" Use 'VAR' for a declaration.
" Use 'LET' for an assignment
" Use ' #"' for a comment
def CheckLegacyAndVim9Success(lines: list<string>)
  var legacylines = lines->mapnew((_, v) =>
  				v->substitute('\<VAR\>', 'let', 'g')
		           	 ->substitute('\<LET\>', 'let', 'g')
		           	 ->substitute('#"', ' "', 'g'))
  CheckLegacySuccess(legacylines)

  var vim9lines = lines->mapnew((_, v) =>
  				v->substitute('\<VAR\>', 'var', 'g')
		           	 ->substitute('\<LET ', '', 'g'))
  CheckDefSuccess(vim9lines)
  CheckScriptSuccess(['vim9script'] + vim9lines)
enddef

" Execute "lines" in a legacy function, :def function and Vim9 script.
" Use 'VAR' for a declaration.
" Use 'LET' for an assignment
" Use ' #"' for a comment
def CheckLegacyAndVim9Failure(lines: list<string>, error: any)
  var legacyError: string
  var defError: string
  var scriptError: string

  if type(error) == type('string')
    legacyError = error
    defError = error
    scriptError = error
  else
    legacyError = error[0]
    defError = error[1]
    scriptError = error[2]
  endif

  var legacylines = lines->mapnew((_, v) =>
  				v->substitute('\<VAR\>', 'let', 'g')
		           	 ->substitute('\<LET\>', 'let', 'g')
		           	 ->substitute('#"', ' "', 'g'))
  CheckLegacyFailure(legacylines, legacyError)

  var vim9lines = lines->mapnew((_, v) =>
  				v->substitute('\<VAR\>', 'var', 'g')
		           	 ->substitute('\<LET ', '', 'g'))
  CheckDefExecFailure(vim9lines, defError)
  CheckScriptFailure(['vim9script'] + vim9lines, scriptError)
enddef
