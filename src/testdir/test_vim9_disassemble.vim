" Test the :disassemble command, and compilation as a side effect

func NotCompiled()
  echo "not"
endfunc

let s:scriptvar = 4
let g:globalvar = 'g'

def s:ScriptFuncLoad(arg: string)
  let local = 1
  buffers
  echo arg
  echo local
  echo v:version
  echo s:scriptvar
  echo g:globalvar
  echo &tabstop
  echo $ENVVAR
  echo @z
enddef

def Test_disassembleLoad()
  assert_fails('disass NoFunc', 'E1061:')
  assert_fails('disass NotCompiled', 'E1062:')

  let res = execute('disass s:ScriptFuncLoad')
  assert_match('<SNR>\d*_ScriptFuncLoad.*'
        \ .. 'buffers.*'
        \ .. ' EXEC \+buffers.*'
        \ .. ' LOAD arg\[-1\].*'
        \ .. ' LOAD $0.*'
        \ .. ' LOADV v:version.*'
        \ .. ' LOADS s:scriptvar from .*test_vim9_disassemble.vim.*'
        \ .. ' LOADG g:globalvar.*'
        \ .. ' LOADENV $ENVVAR.*'
        \ .. ' LOADREG @z.*'
        \, res)
enddef

def s:ScriptFuncPush()
  let localbool = true
  let localspec = v:none
  let localblob = 0z1234
  if has('float')
    let localfloat = 1.234
  endif
enddef

def Test_disassemblePush()
  let res = execute('disass s:ScriptFuncPush')
  assert_match('<SNR>\d*_ScriptFuncPush.*'
        \ .. 'localbool = true.*'
        \ .. ' PUSH v:true.*'
        \ .. 'localspec = v:none.*'
        \ .. ' PUSH v:none.*'
        \ .. 'localblob = 0z1234.*'
        \ .. ' PUSHBLOB 0z1234.*'
        \, res)
  if has('float')
  assert_match('<SNR>\d*_ScriptFuncPush.*'
        \ .. 'localfloat = 1.234.*'
        \ .. ' PUSHF 1.234.*'
        \, res)
  endif
enddef

def s:ScriptFuncStore()
  let localnr = 1
  localnr = 2
  let localstr = 'abc'
  localstr = 'xyz'
  v:char = 'abc'
  s:scriptvar = 'sv'
  g:globalvar = 'gv'
  &tabstop = 8
  $ENVVAR = 'ev'
  @z = 'rv'
enddef

def Test_disassembleStore()
  let res = execute('disass s:ScriptFuncStore')
  assert_match('<SNR>\d*_ScriptFuncStore.*'
        \ .. 'localnr = 2.*'
        \ .. ' STORE 2 in $0.*'
        \ .. 'localstr = ''xyz''.*'
        \ .. ' STORE $1.*'
        \ .. 'v:char = ''abc''.*'
        \ .. 'STOREV v:char.*'
        \ .. 's:scriptvar = ''sv''.*'
        \ .. ' STORES s:scriptvar in .*test_vim9_disassemble.vim.*'
        \ .. 'g:globalvar = ''gv''.*'
        \ .. ' STOREG g:globalvar.*'
        \ .. '&tabstop = 8.*'
        \ .. ' STOREOPT &tabstop.*'
        \ .. '$ENVVAR = ''ev''.*'
        \ .. ' STOREENV $ENVVAR.*'
        \ .. '@z = ''rv''.*'
        \ .. ' STOREREG @z.*'
        \, res)
enddef

def s:ScriptFuncTry()
  try
    echo 'yes'
  catch /fail/
    echo 'no'
  finally
    echo 'end'
  endtry
enddef

def Test_disassembleTry()
  let res = execute('disass s:ScriptFuncTry')
  assert_match('<SNR>\d*_ScriptFuncTry.*'
        \ .. 'try.*'
        \ .. 'TRY catch -> \d\+, finally -> \d\+.*'
        \ .. 'catch /fail/.*'
        \ .. ' JUMP -> \d\+.*'
        \ .. ' PUSH v:exception.*'
        \ .. ' PUSHS "fail".*'
        \ .. ' COMPARESTRING =\~.*'
        \ .. ' JUMP_IF_FALSE -> \d\+.*'
        \ .. ' CATCH.*'
        \ .. 'finally.*'
        \ .. ' PUSHS "end".*'
        \ .. 'endtry.*'
        \ .. ' ENDTRY.*'
        \, res)
enddef

def s:ScriptFuncNew()
  let ll = [1, "two", 333]
  let dd = #{one: 1, two: "val"}
enddef

def Test_disassembleNew()
  let res = execute('disass s:ScriptFuncNew')
  assert_match('<SNR>\d*_ScriptFuncNew.*'
        \ .. 'let ll = \[1, "two", 333].*'
        \ .. 'PUSHNR 1.*'
        \ .. 'PUSHS "two".*'
        \ .. 'PUSHNR 333.*'
        \ .. 'NEWLIST size 3.*'
        \ .. 'let dd = #{one: 1, two: "val"}.*'
        \ .. 'PUSHS "one".*'
        \ .. 'PUSHNR 1.*'
        \ .. 'PUSHS "two".*'
        \ .. 'PUSHS "val".*'
        \ .. 'NEWDICT size 2.*'
        \, res)
enddef

def FuncWithArg(arg)
  echo arg
enddef

func UserFunc()
  echo 'nothing'
endfunc

func UserFuncWithArg(arg)
  echo a:arg
endfunc

def s:ScriptFuncCall(): string
  changenr()
  char2nr("abc")
  Test_disassembleNew()
  FuncWithArg(343)
  ScriptFuncNew()
  s:ScriptFuncNew()
  UserFunc()
  UserFuncWithArg("foo")
  let FuncRef = function("UserFunc")
  FuncRef()
  let FuncRefWithArg = function("UserFuncWithArg")
  FuncRefWithArg("bar")
  return "yes"
enddef

def Test_disassembleCall()
  let res = execute('disass s:ScriptFuncCall')
  assert_match('<SNR>\d\+_ScriptFuncCall.*'
        \ .. 'changenr().*'
        \ .. ' BCALL changenr(argc 0).*'
        \ .. 'char2nr("abc").*'
        \ .. ' PUSHS "abc".*'
        \ .. ' BCALL char2nr(argc 1).*'
        \ .. 'Test_disassembleNew().*'
        \ .. ' DCALL Test_disassembleNew(argc 0).*'
        \ .. 'FuncWithArg(343).*'
        \ .. ' PUSHNR 343.*'
        \ .. ' DCALL FuncWithArg(argc 1).*'
        \ .. 'ScriptFuncNew().*'
        \ .. ' DCALL <SNR>\d\+_ScriptFuncNew(argc 0).*'
        \ .. 's:ScriptFuncNew().*'
        \ .. ' DCALL <SNR>\d\+_ScriptFuncNew(argc 0).*'
        \ .. 'UserFunc().*'
        \ .. ' UCALL UserFunc(argc 0).*'
        \ .. 'UserFuncWithArg("foo").*'
        \ .. ' PUSHS "foo".*'
        \ .. ' UCALL UserFuncWithArg(argc 1).*'
        \ .. 'let FuncRef = function("UserFunc").*'
        \ .. 'FuncRef().*'
        \ .. ' LOAD $\d.*'
        \ .. ' PCALL (argc 0).*'
        \ .. 'let FuncRefWithArg = function("UserFuncWithArg").*'
        \ .. 'FuncRefWithArg("bar").*'
        \ .. ' PUSHS "bar".*'
        \ .. ' LOAD $\d.*'
        \ .. ' PCALL (argc 1).*'
        \ .. 'return "yes".*'
        \ .. ' PUSHS "yes".*'
        \ .. ' RETURN.*'
        \, res)
enddef

def HasEval()
  if has("eval")
    echo "yes"
  else
    echo "no"
  endif
enddef

def HasNothing()
  if has("nothing")
    echo "yes"
  else
    echo "no"
  endif
enddef

def HasSomething()
  if has("nothing")
    echo "nothing"
  elseif has("something")
    echo "something"
  elseif has("eval")
    echo "eval"
  elseif has("less")
    echo "less"
  endif
enddef

def Test_compile_const_expr()
  assert_equal("\nyes", execute('call HasEval()'))
  let instr = execute('disassemble HasEval')
  assert_match('HasEval.*'
        \ .. 'if has("eval").*'
        \ .. ' PUSHS "yes".*'
        \, instr)
  assert_notmatch('JUMP', instr)

  assert_equal("\nno", execute('call HasNothing()'))
  instr = execute('disassemble HasNothing')
  assert_match('HasNothing.*'
        \ .. 'if has("nothing").*'
        \ .. 'else.*'
        \ .. ' PUSHS "no".*'
        \, instr)
  assert_notmatch('PUSHS "yes"', instr)
  assert_notmatch('JUMP', instr)

  assert_equal("\neval", execute('call HasSomething()'))
  instr = execute('disassemble HasSomething')
  assert_match('HasSomething.*'
        \ .. 'if has("nothing").*'
        \ .. 'elseif has("something").*'
        \ .. 'elseif has("eval").*'
        \ .. ' PUSHS "eval".*'
        \ .. 'elseif has("less").*'
        \, instr)
  assert_notmatch('PUSHS "nothing"', instr)
  assert_notmatch('PUSHS "something"', instr)
  assert_notmatch('PUSHS "less"', instr)
  assert_notmatch('JUMP', instr)
enddef


" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
