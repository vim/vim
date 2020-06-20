" Test the :disassemble command, and compilation as a side effect

source check.vim

func NotCompiled()
  echo "not"
endfunc

let s:scriptvar = 4
let g:globalvar = 'g'
let b:buffervar = 'b'
let w:windowvar = 'w'
let t:tabpagevar = 't'

def s:ScriptFuncLoad(arg: string)
  let local = 1
  buffers
  echo arg
  echo local
  echo &lines
  echo v:version
  echo s:scriptvar
  echo g:globalvar
  echo b:buffervar
  echo w:windowvar
  echo t:tabpagevar
  echo &tabstop
  echo $ENVVAR
  echo @z
enddef

def Test_disassemble_load()
  assert_fails('disass NoFunc', 'E1061:')
  assert_fails('disass NotCompiled', 'E1062:')
  assert_fails('disass', 'E471:')
  assert_fails('disass [', 'E475:')
  assert_fails('disass 234', 'E475:')
  assert_fails('disass <XX>foo', 'E475:')

  let res = execute('disass s:ScriptFuncLoad')
  assert_match('<SNR>\d*_ScriptFuncLoad.*' ..
        'buffers.*' ..
        ' EXEC \+buffers.*' ..
        ' LOAD arg\[-1\].*' ..
        ' LOAD $0.*' ..
        ' LOADOPT &lines.*' ..
        ' LOADV v:version.*' ..
        ' LOADS s:scriptvar from .*test_vim9_disassemble.vim.*' ..
        ' LOADG g:globalvar.*' ..
        ' LOADB b:buffervar.*' ..
        ' LOADW w:windowvar.*' ..
        ' LOADT t:tabpagevar.*' ..
        ' LOADENV $ENVVAR.*' ..
        ' LOADREG @z.*',
        res)
enddef

def s:EditExpand()
  let filename = "file"
  let filenr = 123
  edit the`=filename``=filenr`.txt
enddef

def Test_disassemble_exec_expr()
  let res = execute('disass s:EditExpand')
  assert_match('<SNR>\d*_EditExpand.*' ..
        ' let filename = "file".*' ..
        '\d PUSHS "file".*' ..
        '\d STORE $0.*' ..
        ' let filenr = 123.*' ..
        '\d STORE 123 in $1.*' ..
        ' edit the`=filename``=filenr`.txt.*' ..
        '\d PUSHS "edit the".*' ..
        '\d LOAD $0.*' ..
        '\d LOAD $1.*' ..
        '\d 2STRING stack\[-1\].*' ..
        '\d PUSHS ".txt".*' ..
        '\d EXECCONCAT 4.*' ..
        '\d PUSHNR 0.*' ..
        '\d RETURN',
        res)
enddef

def s:ScriptFuncPush()
  let localbool = true
  let localspec = v:none
  let localblob = 0z1234
  if has('float')
    let localfloat = 1.234
  endif
enddef

def Test_disassemble_push()
  let res = execute('disass s:ScriptFuncPush')
  assert_match('<SNR>\d*_ScriptFuncPush.*' ..
        'localbool = true.*' ..
        ' PUSH v:true.*' ..
        'localspec = v:none.*' ..
        ' PUSH v:none.*' ..
        'localblob = 0z1234.*' ..
        ' PUSHBLOB 0z1234.*',
        res)
  if has('float')
    assert_match('<SNR>\d*_ScriptFuncPush.*' ..
          'localfloat = 1.234.*' ..
          ' PUSHF 1.234.*',
          res)
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
  b:buffervar = 'bv'
  w:windowvar = 'wv'
  t:tabpagevar = 'tv'
  &tabstop = 8
  $ENVVAR = 'ev'
  @z = 'rv'
enddef

def Test_disassemble_store()
  let res = execute('disass s:ScriptFuncStore')
  assert_match('<SNR>\d*_ScriptFuncStore.*' ..
        'let localnr = 1.*' ..
        'localnr = 2.*' ..
        ' STORE 2 in $0.*' ..
        'let localstr = ''abc''.*' ..
        'localstr = ''xyz''.*' ..
        ' STORE $1.*' ..
        'v:char = ''abc''.*' ..
        'STOREV v:char.*' ..
        's:scriptvar = ''sv''.*' ..
        ' STORES s:scriptvar in .*test_vim9_disassemble.vim.*' ..
        'g:globalvar = ''gv''.*' ..
        ' STOREG g:globalvar.*' ..
        'b:buffervar = ''bv''.*' ..
        ' STOREB b:buffervar.*' ..
        'w:windowvar = ''wv''.*' ..
        ' STOREW w:windowvar.*' ..
        't:tabpagevar = ''tv''.*' ..
        ' STORET t:tabpagevar.*' ..
        '&tabstop = 8.*' ..
        ' STOREOPT &tabstop.*' ..
        '$ENVVAR = ''ev''.*' ..
        ' STOREENV $ENVVAR.*' ..
        '@z = ''rv''.*' ..
        ' STOREREG @z.*',
        res)
enddef

def s:ScriptFuncStoreMember()
  let locallist: list<number> = []
  locallist[0] = 123
  let localdict: dict<number> = {}
  localdict["a"] = 456
enddef

def Test_disassemble_store_member()
  let res = execute('disass s:ScriptFuncStoreMember')
  assert_match('<SNR>\d*_ScriptFuncStoreMember\_s*' ..
        'let locallist: list<number> = []\_s*' ..
        '\d NEWLIST size 0\_s*' ..
        '\d STORE $0\_s*' ..
        'locallist\[0\] = 123\_s*' ..
        '\d PUSHNR 123\_s*' ..
        '\d PUSHNR 0\_s*' ..
        '\d LOAD $0\_s*' ..
        '\d STORELIST\_s*' ..
        'let localdict: dict<number> = {}\_s*' ..
        '\d NEWDICT size 0\_s*' ..
        '\d STORE $1\_s*' ..
        'localdict\["a"\] = 456\_s*' ..
        '\d\+ PUSHNR 456\_s*' ..
        '\d\+ PUSHS "a"\_s*' ..
        '\d\+ LOAD $1\_s*' ..
        '\d\+ STOREDICT\_s*' ..
        '\d\+ PUSHNR 0\_s*' ..
        '\d\+ RETURN',
        res)
enddef

def s:ListAssign()
  let x: string
  let y: string
  let l: list<any>
  [x, y; l] = g:stringlist
enddef

def Test_disassemble_list_assign()
  let res = execute('disass s:ListAssign')
  assert_match('<SNR>\d*_ListAssign\_s*' ..
        'let x: string\_s*' ..
        '\d PUSHS "\[NULL\]"\_s*' ..
        '\d STORE $0\_s*' ..
        'let y: string\_s*' ..
        '\d PUSHS "\[NULL\]"\_s*' ..
        '\d STORE $1\_s*' ..
        'let l: list<any>\_s*' ..
        '\d NEWLIST size 0\_s*' ..
        '\d STORE $2\_s*' ..
        '\[x, y; l\] = g:stringlist\_s*' ..
        '\d LOADG g:stringlist\_s*' ..
        '\d CHECKTYPE list stack\[-1\]\_s*' ..
        '\d CHECKLEN >= 2\_s*' ..
        '\d\+ ITEM 0\_s*' ..
        '\d\+ CHECKTYPE string stack\[-1\]\_s*' ..
        '\d\+ STORE $0\_s*' ..
        '\d\+ ITEM 1\_s*' ..
        '\d\+ CHECKTYPE string stack\[-1\]\_s*' ..
        '\d\+ STORE $1\_s*' ..
        '\d\+ SLICE 2\_s*' ..
        '\d\+ STORE $2\_s*' ..
        '\d\+ PUSHNR 0\_s*' ..
        '\d\+ RETURN',
        res)
enddef

def s:ScriptFuncUnlet()
  g:somevar = "value"
  unlet g:somevar
  unlet! g:somevar
  unlet $SOMEVAR
enddef

def Test_disassemble_unlet()
  let res = execute('disass s:ScriptFuncUnlet')
  assert_match('<SNR>\d*_ScriptFuncUnlet\_s*' ..
        'g:somevar = "value"\_s*' ..
        '\d PUSHS "value"\_s*' ..
        '\d STOREG g:somevar\_s*' ..
        'unlet g:somevar\_s*' ..
        '\d UNLET g:somevar\_s*' ..
        'unlet! g:somevar\_s*' ..
        '\d UNLET! g:somevar\_s*' ..
        'unlet $SOMEVAR\_s*' ..
        '\d UNLETENV $SOMEVAR\_s*',
        res)
enddef

def s:ScriptFuncTry()
  try
    echo "yes"
  catch /fail/
    echo "no"
  finally
    throw "end"
  endtry
enddef

def Test_disassemble_try()
  let res = execute('disass s:ScriptFuncTry')
  assert_match('<SNR>\d*_ScriptFuncTry\_s*' ..
        'try\_s*' ..
        '\d TRY catch -> \d\+, finally -> \d\+\_s*' ..
        'echo "yes"\_s*' ..
        '\d PUSHS "yes"\_s*' ..
        '\d ECHO 1\_s*' ..
        'catch /fail/\_s*' ..
        '\d JUMP -> \d\+\_s*' ..
        '\d PUSH v:exception\_s*' ..
        '\d PUSHS "fail"\_s*' ..
        '\d COMPARESTRING =\~\_s*' ..
        '\d JUMP_IF_FALSE -> \d\+\_s*' ..
        '\d CATCH\_s*' ..
        'echo "no"\_s*' ..
        '\d\+ PUSHS "no"\_s*' ..
        '\d\+ ECHO 1\_s*' ..
        'finally\_s*' ..
        'throw "end"\_s*' ..
        '\d\+ PUSHS "end"\_s*' ..
        '\d\+ THROW\_s*' ..
        'endtry\_s*' ..
        '\d\+ ENDTRY',
        res)
enddef

def s:ScriptFuncNew()
  let ll = [1, "two", 333]
  let dd = #{one: 1, two: "val"}
enddef

def Test_disassemble_new()
  let res = execute('disass s:ScriptFuncNew')
  assert_match('<SNR>\d*_ScriptFuncNew\_s*' ..
        'let ll = \[1, "two", 333\]\_s*' ..
        '\d PUSHNR 1\_s*' ..
        '\d PUSHS "two"\_s*' ..
        '\d PUSHNR 333\_s*' ..
        '\d NEWLIST size 3\_s*' ..
        '\d STORE $0\_s*' ..
        'let dd = #{one: 1, two: "val"}\_s*' ..
        '\d PUSHS "one"\_s*' ..
        '\d PUSHNR 1\_s*' ..
        '\d PUSHS "two"\_s*' ..
        '\d PUSHS "val"\_s*' ..
        '\d NEWDICT size 2\_s*',
        res)
enddef

def FuncWithArg(arg: any)
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
  Test_disassemble_new()
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

def Test_disassemble_call()
  let res = execute('disass s:ScriptFuncCall')
  assert_match('<SNR>\d\+_ScriptFuncCall\_s*' ..
        'changenr()\_s*' ..
        '\d BCALL changenr(argc 0)\_s*' ..
        '\d DROP\_s*' ..
        'char2nr("abc")\_s*' ..
        '\d PUSHS "abc"\_s*' ..
        '\d BCALL char2nr(argc 1)\_s*' ..
        '\d DROP\_s*' ..
        'Test_disassemble_new()\_s*' ..
        '\d DCALL Test_disassemble_new(argc 0)\_s*' ..
        '\d DROP\_s*' ..
        'FuncWithArg(343)\_s*' ..
        '\d\+ PUSHNR 343\_s*' ..
        '\d\+ DCALL FuncWithArg(argc 1)\_s*' ..
        '\d\+ DROP\_s*' ..
        'ScriptFuncNew()\_s*' ..
        '\d\+ DCALL <SNR>\d\+_ScriptFuncNew(argc 0)\_s*' ..
        '\d\+ DROP\_s*' ..
        's:ScriptFuncNew()\_s*' ..
        '\d\+ DCALL <SNR>\d\+_ScriptFuncNew(argc 0)\_s*' ..
        '\d\+ DROP\_s*' ..
        'UserFunc()\_s*' ..
        '\d\+ UCALL UserFunc(argc 0)\_s*' ..
        '\d\+ DROP\_s*' ..
        'UserFuncWithArg("foo")\_s*' ..
        '\d\+ PUSHS "foo"\_s*' ..
        '\d\+ UCALL UserFuncWithArg(argc 1)\_s*' ..
        '\d\+ DROP\_s*' ..
        'let FuncRef = function("UserFunc")\_s*' ..
        '\d\+ PUSHS "UserFunc"\_s*' ..
        '\d\+ BCALL function(argc 1)\_s*' ..
        '\d\+ STORE $0\_s*' ..
        'FuncRef()\_s*' ..
        '\d\+ LOAD $\d\_s*' ..
        '\d\+ PCALL (argc 0)\_s*' ..
        '\d\+ DROP\_s*' ..
        'let FuncRefWithArg = function("UserFuncWithArg")\_s*' ..
        '\d\+ PUSHS "UserFuncWithArg"\_s*' ..
        '\d\+ BCALL function(argc 1)\_s*' ..
        '\d\+ STORE $1\_s*' ..
        'FuncRefWithArg("bar")\_s*' ..
        '\d\+ PUSHS "bar"\_s*' ..
        '\d\+ LOAD $\d\_s*' ..
        '\d\+ PCALL (argc 1)\_s*' ..
        '\d\+ DROP\_s*' ..
        'return "yes"\_s*' ..
        '\d\+ PUSHS "yes"\_s*' ..
        '\d\+ RETURN',
        res)
enddef

def s:CreateRefs()
  let local = 'a'
  def Append(arg: string)
    local ..= arg
  enddef
  g:Append = Append
  def Get(): string
    return local
  enddef
  g:Get = Get
enddef

def Test_disassemble_closure()
  CreateRefs()
  let res = execute('disass g:Append')
  assert_match('<lambda>\d\_s*' ..
        'local ..= arg\_s*' ..
        '\d LOADOUTER $0\_s*' ..
        '\d LOAD arg\[-1\]\_s*' ..
        '\d CONCAT\_s*' ..
        '\d STOREOUTER $0\_s*' ..
        '\d PUSHNR 0\_s*' ..
        '\d RETURN',
        res)

  res = execute('disass g:Get')
  assert_match('<lambda>\d\_s*' ..
        'return local\_s*' ..
        '\d LOADOUTER $0\_s*' ..
        '\d RETURN',
        res)

  unlet g:Append
  unlet g:Get
enddef


def EchoArg(arg: string): string
  return arg
enddef
def RefThis(): func
  return function('EchoArg')
enddef
def s:ScriptPCall()
  RefThis()("text")
enddef

def Test_disassemble_pcall()
  let res = execute('disass s:ScriptPCall')
  assert_match('<SNR>\d\+_ScriptPCall\_s*' ..
        'RefThis()("text")\_s*' ..
        '\d DCALL RefThis(argc 0)\_s*' ..
        '\d PUSHS "text"\_s*' ..
        '\d PCALL top (argc 1)\_s*' ..
        '\d PCALL end\_s*' ..
        '\d DROP\_s*' ..
        '\d PUSHNR 0\_s*' ..
        '\d RETURN',
        res)
enddef


def s:FuncWithForwardCall(): string
  return g:DefinedLater("yes")
enddef

def DefinedLater(arg: string): string
  return arg
enddef

def Test_disassemble_update_instr()
  let res = execute('disass s:FuncWithForwardCall')
  assert_match('FuncWithForwardCall\_s*' ..
        'return g:DefinedLater("yes")\_s*' ..
        '\d PUSHS "yes"\_s*' ..
        '\d DCALL DefinedLater(argc 1)\_s*' ..
        '\d RETURN',
        res)

  " Calling the function will change UCALL into the faster DCALL
  assert_equal('yes', FuncWithForwardCall())

  res = execute('disass s:FuncWithForwardCall')
  assert_match('FuncWithForwardCall\_s*' ..
        'return g:DefinedLater("yes")\_s*' ..
        '\d PUSHS "yes"\_s*' ..
        '\d DCALL DefinedLater(argc 1)\_s*' ..
        '\d RETURN',
        res)
enddef


def FuncWithDefault(arg: string = 'default'): string
  return arg
enddef

def Test_disassemble_call_default()
  let res = execute('disass FuncWithDefault')
  assert_match('FuncWithDefault\_s*' ..
        '\d PUSHS "default"\_s*' ..
        '\d STORE arg\[-1]\_s*' ..
        'return arg\_s*' ..
        '\d LOAD arg\[-1]\_s*' ..
        '\d RETURN',
        res)
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

def Test_disassemble_const_expr()
  assert_equal("\nyes", execute('call HasEval()'))
  let instr = execute('disassemble HasEval')
  assert_match('HasEval\_s*' ..
        'if has("eval")\_s*' ..
        'echo "yes"\_s*' ..
        '\d PUSHS "yes"\_s*' ..
        '\d ECHO 1\_s*' ..
        'else\_s*' ..
        'echo "no"\_s*' ..
        'endif\_s*',
        instr)
  assert_notmatch('JUMP', instr)

  assert_equal("\nno", execute('call HasNothing()'))
  instr = execute('disassemble HasNothing')
  assert_match('HasNothing\_s*' ..
        'if has("nothing")\_s*' ..
        'echo "yes"\_s*' ..
        'else\_s*' ..
        'echo "no"\_s*' ..
        '\d PUSHS "no"\_s*' ..
        '\d ECHO 1\_s*' ..
        'endif',
        instr)
  assert_notmatch('PUSHS "yes"', instr)
  assert_notmatch('JUMP', instr)

  assert_equal("\neval", execute('call HasSomething()'))
  instr = execute('disassemble HasSomething')
  assert_match('HasSomething.*' ..
        'if has("nothing")\_s*' ..
        'echo "nothing"\_s*' ..
        'elseif has("something")\_s*' ..
        'echo "something"\_s*' ..
        'elseif has("eval")\_s*' ..
        'echo "eval"\_s*' ..
        '\d PUSHS "eval"\_s*' ..
        '\d ECHO 1\_s*' ..
        'elseif has("less").*' ..
        'echo "less"\_s*' ..
        'endif',
        instr)
  assert_notmatch('PUSHS "nothing"', instr)
  assert_notmatch('PUSHS "something"', instr)
  assert_notmatch('PUSHS "less"', instr)
  assert_notmatch('JUMP', instr)
enddef

def ReturnInIf(): string
  if g:cond
    return "yes"
  else
    return "no"
  endif
enddef

def Test_disassemble_return_in_if()
  let instr = execute('disassemble ReturnInIf')
  assert_match('ReturnInIf\_s*' ..
        'if g:cond\_s*' ..
        '0 LOADG g:cond\_s*' ..
        '1 JUMP_IF_FALSE -> 4\_s*' ..
        'return "yes"\_s*' ..
        '2 PUSHS "yes"\_s*' ..
        '3 RETURN\_s*' ..
        'else\_s*' ..
        ' return "no"\_s*' ..
        '4 PUSHS "no"\_s*' ..
        '5 RETURN$',
        instr)
enddef

def WithFunc()
  let Funky1: func
  let Funky2: func = function("len")
  let Party2: func = funcref("UserFunc")
enddef

def Test_disassemble_function()
  let instr = execute('disassemble WithFunc')
  assert_match('WithFunc\_s*' ..
        'let Funky1: func\_s*' ..
        '0 PUSHFUNC "\[none]"\_s*' ..
        '1 STORE $0\_s*' ..
        'let Funky2: func = function("len")\_s*' ..
        '2 PUSHS "len"\_s*' ..
        '3 BCALL function(argc 1)\_s*' ..
        '4 STORE $1\_s*' ..
        'let Party2: func = funcref("UserFunc")\_s*' ..
        '\d PUSHS "UserFunc"\_s*' ..
        '\d BCALL funcref(argc 1)\_s*' ..
        '\d STORE $2\_s*' ..
        '\d PUSHNR 0\_s*' ..
        '\d RETURN',
        instr)
enddef

if has('channel')
  def WithChannel()
    let job1: job
    let job2: job = job_start("donothing")
    let chan1: channel
  enddef
endif

def Test_disassemble_channel()
  CheckFeature channel

  let instr = execute('disassemble WithChannel')
  assert_match('WithChannel\_s*' ..
        'let job1: job\_s*' ..
        '\d PUSHJOB "no process"\_s*' ..
        '\d STORE $0\_s*' ..
        'let job2: job = job_start("donothing")\_s*' ..
        '\d PUSHS "donothing"\_s*' ..
        '\d BCALL job_start(argc 1)\_s*' ..
        '\d STORE $1\_s*' ..
        'let chan1: channel\_s*' ..
        '\d PUSHCHANNEL 0\_s*' ..
        '\d STORE $2\_s*' ..
        '\d PUSHNR 0\_s*' ..
        '\d RETURN',
        instr)
enddef

def WithLambda(): string
  let F = {a -> "X" .. a .. "X"}
  return F("x")
enddef

def Test_disassemble_lambda()
  assert_equal("XxX", WithLambda())
  let instr = execute('disassemble WithLambda')
  assert_match('WithLambda\_s*' ..
        'let F = {a -> "X" .. a .. "X"}\_s*' ..
        '\d FUNCREF <lambda>\d\+ $1\_s*' ..
        '\d STORE $0\_s*' ..
        'return F("x")\_s*' ..
        '\d PUSHS "x"\_s*' ..
        '\d LOAD $0\_s*' ..
        '\d PCALL (argc 1)\_s*' ..
        '\d RETURN',
        instr)
enddef

def AndOr(arg: any): string
  if arg == 1 && arg != 2 || arg == 4
    return 'yes'
  endif
  return 'no'
enddef

def Test_disassemble_and_or()
  assert_equal("yes", AndOr(1))
  assert_equal("no", AndOr(2))
  assert_equal("yes", AndOr(4))
  let instr = execute('disassemble AndOr')
  assert_match('AndOr\_s*' ..
        'if arg == 1 && arg != 2 || arg == 4\_s*' ..
        '\d LOAD arg\[-1]\_s*' ..
        '\d PUSHNR 1\_s*' ..
        '\d COMPAREANY ==\_s*' ..
        '\d JUMP_AND_KEEP_IF_FALSE -> \d\+\_s*' ..
        '\d LOAD arg\[-1]\_s*' ..
        '\d PUSHNR 2\_s*' ..
        '\d COMPAREANY !=\_s*' ..
        '\d JUMP_AND_KEEP_IF_TRUE -> \d\+\_s*' ..
        '\d LOAD arg\[-1]\_s*' ..
        '\d\+ PUSHNR 4\_s*' ..
        '\d\+ COMPAREANY ==\_s*' ..
        '\d\+ JUMP_IF_FALSE -> \d\+',
        instr)
enddef

def ForLoop(): list<number>
  let res: list<number>
  for i in range(3)
    res->add(i)
  endfor
  return res
enddef

def Test_disassemble_for_loop()
  assert_equal([0, 1, 2], ForLoop())
  let instr = execute('disassemble ForLoop')
  assert_match('ForLoop\_s*' ..
        'let res: list<number>\_s*' ..
        '\d NEWLIST size 0\_s*' ..
        '\d STORE $0\_s*' ..
        'for i in range(3)\_s*' ..
        '\d STORE -1 in $1\_s*' ..
        '\d PUSHNR 3\_s*' ..
        '\d BCALL range(argc 1)\_s*' ..
        '\d FOR $1 -> \d\+\_s*' ..
        '\d STORE $2\_s*' ..
        'res->add(i)\_s*' ..
        '\d LOAD $0\_s*' ..
        '\d LOAD $2\_s*' ..
        '\d\+ BCALL add(argc 2)\_s*' ..
        '\d\+ DROP\_s*' ..
        'endfor\_s*' ..
        '\d\+ JUMP -> \d\+\_s*' ..
        '\d\+ DROP',
        instr)
enddef

let g:number = 42

def Computing()
  let nr = 3
  let nrres = nr + 7
  nrres = nr - 7
  nrres = nr * 7
  nrres = nr / 7
  nrres = nr % 7

  let anyres = g:number + 7
  anyres = g:number - 7
  anyres = g:number * 7
  anyres = g:number / 7
  anyres = g:number % 7

  if has('float')
    let fl = 3.0
    let flres = fl + 7.0
    flres = fl - 7.0
    flres = fl * 7.0
    flres = fl / 7.0
  endif
enddef

def Test_disassemble_computing()
  let instr = execute('disassemble Computing')
  assert_match('Computing.*' ..
        'let nr = 3.*' ..
        '\d STORE 3 in $0.*' ..
        'let nrres = nr + 7.*' ..
        '\d LOAD $0.*' ..
        '\d PUSHNR 7.*' ..
        '\d OPNR +.*' ..
        '\d STORE $1.*' ..
        'nrres = nr - 7.*' ..
        '\d OPNR -.*' ..
        'nrres = nr \* 7.*' ..
        '\d OPNR \*.*' ..
        'nrres = nr / 7.*' ..
        '\d OPNR /.*' ..
        'nrres = nr % 7.*' ..
        '\d OPNR %.*' ..
        'let anyres = g:number + 7.*' ..
        '\d LOADG g:number.*' ..
        '\d PUSHNR 7.*' ..
        '\d OPANY +.*' ..
        '\d STORE $2.*' ..
        'anyres = g:number - 7.*' ..
        '\d OPANY -.*' ..
        'anyres = g:number \* 7.*' ..
        '\d OPANY \*.*' ..
        'anyres = g:number / 7.*' ..
        '\d OPANY /.*' ..
        'anyres = g:number % 7.*' ..
        '\d OPANY %.*',
        instr)
  if has('float')
    assert_match('Computing.*' ..
        'let fl = 3.0.*' ..
        '\d PUSHF 3.0.*' ..
        '\d STORE $3.*' ..
        'let flres = fl + 7.0.*' ..
        '\d LOAD $3.*' ..
        '\d PUSHF 7.0.*' ..
        '\d OPFLOAT +.*' ..
        '\d STORE $4.*' ..
        'flres = fl - 7.0.*' ..
        '\d OPFLOAT -.*' ..
        'flres = fl \* 7.0.*' ..
        '\d OPFLOAT \*.*' ..
        'flres = fl / 7.0.*' ..
        '\d OPFLOAT /.*',
        instr)
  endif
enddef

def AddListBlob()
  let reslist = [1, 2] + [3, 4]
  let resblob = 0z1122 + 0z3344
enddef

def Test_disassemble_add_list_blob()
  let instr = execute('disassemble AddListBlob')
  assert_match('AddListBlob.*' ..
        'let reslist = \[1, 2] + \[3, 4].*' ..
        '\d PUSHNR 1.*' ..
        '\d PUSHNR 2.*' ..
        '\d NEWLIST size 2.*' ..
        '\d PUSHNR 3.*' ..
        '\d PUSHNR 4.*' ..
        '\d NEWLIST size 2.*' ..
        '\d ADDLIST.*' ..
        '\d STORE $.*.*' ..
        'let resblob = 0z1122 + 0z3344.*' ..
        '\d PUSHBLOB 0z1122.*' ..
        '\d PUSHBLOB 0z3344.*' ..
        '\d ADDBLOB.*' ..
        '\d STORE $.*',
        instr)
enddef

let g:aa = 'aa'
def ConcatString(): string
  let res = g:aa .. "bb"
  return res
enddef

def Test_disassemble_concat()
  let instr = execute('disassemble ConcatString')
  assert_match('ConcatString.*' ..
        'let res = g:aa .. "bb".*' ..
        '\d LOADG g:aa.*' ..
        '\d PUSHS "bb".*' ..
        '\d 2STRING stack\[-2].*' ..
        '\d CONCAT.*' ..
        '\d STORE $.*',
        instr)
  assert_equal('aabb', ConcatString())
enddef

def ListIndex(): number
  let l = [1, 2, 3]
  let res = l[1]
  return res
enddef

def Test_disassemble_list_index()
  let instr = execute('disassemble ListIndex')
  assert_match('ListIndex\_s*' ..
        'let l = \[1, 2, 3]\_s*' ..
        '\d PUSHNR 1\_s*' ..
        '\d PUSHNR 2\_s*' ..
        '\d PUSHNR 3\_s*' ..
        '\d NEWLIST size 3\_s*' ..
        '\d STORE $0\_s*' ..
        'let res = l\[1]\_s*' ..
        '\d LOAD $0\_s*' ..
        '\d PUSHNR 1\_s*' ..
        '\d INDEX\_s*' ..
        '\d STORE $1\_s*',
        instr)
  assert_equal(2, ListIndex())
enddef

def DictMember(): number
  let d = #{item: 1}
  let res = d.item
  res = d["item"]
  return res
enddef

def Test_disassemble_dict_member()
  let instr = execute('disassemble DictMember')
  assert_match('DictMember\_s*' ..
        'let d = #{item: 1}\_s*' ..
        '\d PUSHS "item"\_s*' ..
        '\d PUSHNR 1\_s*' ..
        '\d NEWDICT size 1\_s*' ..
        '\d STORE $0\_s*' ..
        'let res = d.item\_s*' ..
        '\d\+ LOAD $0\_s*' ..
        '\d\+ MEMBER item\_s*' ..
        '\d\+ STORE $1\_s*' ..
        'res = d\["item"\]\_s*' ..
        '\d\+ LOAD $0\_s*' ..
        '\d\+ PUSHS "item"\_s*' ..
        '\d\+ MEMBER\_s*' ..
        '\d\+ STORE $1\_s*',
        instr)
  call assert_equal(1, DictMember())
enddef

def NegateNumber(): number
  let nr = 9
  let plus = +nr
  let res = -nr
  return res
enddef

def Test_disassemble_negate_number()
  let instr = execute('disassemble NegateNumber')
  assert_match('NegateNumber\_s*' ..
        'let nr = 9\_s*' ..
        '\d STORE 9 in $0\_s*' ..
        'let plus = +nr\_s*' ..
        '\d LOAD $0\_s*' ..
        '\d CHECKNR\_s*' ..
        '\d STORE $1\_s*' ..
        'let res = -nr\_s*' ..
        '\d LOAD $0\_s*' ..
        '\d NEGATENR\_s*' ..
        '\d STORE $2\_s*',
        instr)
  call assert_equal(-9, NegateNumber())
enddef

def InvertBool(): bool
  let flag = true
  let invert = !flag
  let res = !!flag
  return res
enddef

def Test_disassemble_invert_bool()
  let instr = execute('disassemble InvertBool')
  assert_match('InvertBool\_s*' ..
        'let flag = true\_s*' ..
        '\d PUSH v:true\_s*' ..
        '\d STORE $0\_s*' ..
        'let invert = !flag\_s*' ..
        '\d LOAD $0\_s*' ..
        '\d INVERT (!val)\_s*' ..
        '\d STORE $1\_s*' ..
        'let res = !!flag\_s*' ..
        '\d LOAD $0\_s*' ..
        '\d 2BOOL (!!val)\_s*' ..
        '\d STORE $2\_s*',
        instr)
  call assert_equal(true, InvertBool())
enddef

def Test_disassemble_compare()
  let cases = [
        ['true == isFalse', 'COMPAREBOOL =='],
        ['true != isFalse', 'COMPAREBOOL !='],
        ['v:none == isNull', 'COMPARESPECIAL =='],
        ['v:none != isNull', 'COMPARESPECIAL !='],

        ['111 == aNumber', 'COMPARENR =='],
        ['111 != aNumber', 'COMPARENR !='],
        ['111 > aNumber', 'COMPARENR >'],
        ['111 < aNumber', 'COMPARENR <'],
        ['111 >= aNumber', 'COMPARENR >='],
        ['111 <= aNumber', 'COMPARENR <='],
        ['111 =~ aNumber', 'COMPARENR =\~'],
        ['111 !~ aNumber', 'COMPARENR !\~'],

        ['"xx" != aString', 'COMPARESTRING !='],
        ['"xx" > aString', 'COMPARESTRING >'],
        ['"xx" < aString', 'COMPARESTRING <'],
        ['"xx" >= aString', 'COMPARESTRING >='],
        ['"xx" <= aString', 'COMPARESTRING <='],
        ['"xx" =~ aString', 'COMPARESTRING =\~'],
        ['"xx" !~ aString', 'COMPARESTRING !\~'],
        ['"xx" is aString', 'COMPARESTRING is'],
        ['"xx" isnot aString', 'COMPARESTRING isnot'],

        ['0z11 == aBlob', 'COMPAREBLOB =='],
        ['0z11 != aBlob', 'COMPAREBLOB !='],
        ['0z11 is aBlob', 'COMPAREBLOB is'],
        ['0z11 isnot aBlob', 'COMPAREBLOB isnot'],

        ['[1, 2] == aList', 'COMPARELIST =='],
        ['[1, 2] != aList', 'COMPARELIST !='],
        ['[1, 2] is aList', 'COMPARELIST is'],
        ['[1, 2] isnot aList', 'COMPARELIST isnot'],

        ['#{a: 1} == aDict', 'COMPAREDICT =='],
        ['#{a: 1} != aDict', 'COMPAREDICT !='],
        ['#{a: 1} is aDict', 'COMPAREDICT is'],
        ['#{a: 1} isnot aDict', 'COMPAREDICT isnot'],

        ['{->33} == {->44}', 'COMPAREFUNC =='],
        ['{->33} != {->44}', 'COMPAREFUNC !='],
        ['{->33} is {->44}', 'COMPAREFUNC is'],
        ['{->33} isnot {->44}', 'COMPAREFUNC isnot'],

        ['77 == g:xx', 'COMPAREANY =='],
        ['77 != g:xx', 'COMPAREANY !='],
        ['77 > g:xx', 'COMPAREANY >'],
        ['77 < g:xx', 'COMPAREANY <'],
        ['77 >= g:xx', 'COMPAREANY >='],
        ['77 <= g:xx', 'COMPAREANY <='],
        ['77 =~ g:xx', 'COMPAREANY =\~'],
        ['77 !~ g:xx', 'COMPAREANY !\~'],
        ['77 is g:xx', 'COMPAREANY is'],
        ['77 isnot g:xx', 'COMPAREANY isnot'],
        ]
  let floatDecl = ''
  if has('float')
    cases->extend([
        ['1.1 == aFloat', 'COMPAREFLOAT =='],
        ['1.1 != aFloat', 'COMPAREFLOAT !='],
        ['1.1 > aFloat', 'COMPAREFLOAT >'],
        ['1.1 < aFloat', 'COMPAREFLOAT <'],
        ['1.1 >= aFloat', 'COMPAREFLOAT >='],
        ['1.1 <= aFloat', 'COMPAREFLOAT <='],
        ['1.1 =~ aFloat', 'COMPAREFLOAT =\~'],
        ['1.1 !~ aFloat', 'COMPAREFLOAT !\~'],
        ])
    floatDecl = 'let aFloat = 2.2'
  endif

  let nr = 1
  for case in cases
    " declare local variables to get a non-constant with the right type
    writefile(['def TestCase' .. nr .. '()',
             '  let isFalse = false',
             '  let isNull = v:null',
             '  let aNumber = 222',
             '  let aString = "yy"',
             '  let aBlob = 0z22',
             '  let aList = [3, 4]',
             '  let aDict = #{x: 2}',
             floatDecl,
             '  if ' .. case[0],
             '    echo 42'
             '  endif',
             'enddef'], 'Xdisassemble')
    source Xdisassemble
    let instr = execute('disassemble TestCase' .. nr)
    assert_match('TestCase' .. nr .. '.*' ..
        'if ' .. substitute(case[0], '[[~]', '\\\0', 'g') .. '.*' ..
        '\d \(PUSH\|FUNCREF\).*' ..
        '\d \(PUSH\|FUNCREF\|LOAD\).*' ..
        '\d ' .. case[1] .. '.*' ..
        '\d JUMP_IF_FALSE -> \d\+.*',
        instr)

    nr += 1
  endfor

  delete('Xdisassemble')
enddef

def Test_disassemble_compare_const()
  let cases = [
        ['"xx" == "yy"', false],
        ['"aa" == "aa"', true],
        ['has("eval") ? true : false', true],
        ['has("asdf") ? true : false', false],
        ]

  let nr = 1
  for case in cases
    writefile(['def TestCase' .. nr .. '()',
             '  if ' .. case[0],
             '    echo 42'
             '  endif',
             'enddef'], 'Xdisassemble')
    source Xdisassemble
    let instr = execute('disassemble TestCase' .. nr)
    if case[1]
      " condition true, "echo 42" executed
      assert_match('TestCase' .. nr .. '.*' ..
          'if ' .. substitute(case[0], '[[~]', '\\\0', 'g') .. '.*' ..
          '\d PUSHNR 42.*' ..
          '\d ECHO 1.*' ..
          '\d PUSHNR 0.*' ..
          '\d RETURN.*',
          instr)
    else
      " condition false, function just returns
      assert_match('TestCase' .. nr .. '.*' ..
          'if ' .. substitute(case[0], '[[~]', '\\\0', 'g') .. '[ \n]*' ..
          'echo 42[ \n]*' ..
          'endif[ \n]*' ..
          '\s*\d PUSHNR 0.*' ..
          '\d RETURN.*',
          instr)
    endif

    nr += 1
  endfor

  delete('Xdisassemble')
enddef

def s:Execute()
  execute 'help vim9.txt'
  let cmd = 'help vim9.txt'
  execute cmd
  let tag = 'vim9.txt'
  execute 'help ' .. tag
enddef

def Test_disassemble_execute()
  let res = execute('disass s:Execute')
  assert_match('\<SNR>\d*_Execute\_s*' ..
        "execute 'help vim9.txt'\\_s*" ..
        '\d PUSHS "help vim9.txt"\_s*' ..
        '\d EXECUTE 1\_s*' ..
        "let cmd = 'help vim9.txt'\\_s*" ..
        '\d PUSHS "help vim9.txt"\_s*' ..
        '\d STORE $0\_s*' ..
        'execute cmd\_s*' ..
        '\d LOAD $0\_s*' ..
        '\d EXECUTE 1\_s*' ..
        "let tag = 'vim9.txt'\\_s*" ..
        '\d PUSHS "vim9.txt"\_s*' ..
        '\d STORE $1\_s*' ..
        "execute 'help ' .. tag\\_s*" ..
        '\d\+ PUSHS "help "\_s*' ..
        '\d\+ LOAD $1\_s*' ..
        '\d\+ CONCAT\_s*' ..
        '\d\+ EXECUTE 1\_s*' ..
        '\d\+ PUSHNR 0\_s*' ..
        '\d\+ RETURN',
        res)
enddef

def s:Echomsg()
  echomsg 'some' 'message'
  echoerr 'went' .. 'wrong'
enddef

def Test_disassemble_echomsg()
  let res = execute('disass s:Echomsg')
  assert_match('\<SNR>\d*_Echomsg\_s*' ..
        "echomsg 'some' 'message'\\_s*" ..
        '\d PUSHS "some"\_s*' ..
        '\d PUSHS "message"\_s*' ..
        '\d ECHOMSG 2\_s*' ..
        "echoerr 'went' .. 'wrong'\\_s*" ..
        '\d PUSHS "wentwrong"\_s*' ..
        '\d ECHOERR 1\_s*' ..
        '\d PUSHNR 0\_s*' ..
        '\d RETURN',
        res)
enddef

def SomeStringArg(arg: string)
  echo arg
enddef

def SomeAnyArg(arg: any)
  echo arg
enddef

def SomeStringArgAndReturn(arg: string): string
  return arg
enddef

def Test_display_func()
  let res1 = execute('function SomeStringArg')
  assert_match('.* def SomeStringArg(arg: string)\_s*' ..
        '\d *echo arg.*' ..
        ' *enddef',
        res1)

  let res2 = execute('function SomeAnyArg')
  assert_match('.* def SomeAnyArg(arg: any)\_s*' ..
        '\d *echo arg\_s*' ..
        ' *enddef',
        res2)

  let res3 = execute('function SomeStringArgAndReturn')
  assert_match('.* def SomeStringArgAndReturn(arg: string): string\_s*' ..
        '\d *return arg\_s*' ..
        ' *enddef',
        res3)
enddef

def Test_vim9script_forward_func()
  let lines =<< trim END
    vim9script
    def FuncOne(): string
      return FuncTwo()
    enddef
    def FuncTwo(): string
      return 'two'
    enddef
    g:res_FuncOne = execute('disass FuncOne')
  END
  writefile(lines, 'Xdisassemble')
  source Xdisassemble

  " check that the first function calls the second with DCALL
  assert_match('\<SNR>\d*_FuncOne\_s*' ..
        'return FuncTwo()\_s*' ..
        '\d DCALL <SNR>\d\+_FuncTwo(argc 0)\_s*' ..
        '\d RETURN',
        g:res_FuncOne)

  delete('Xdisassemble')
  unlet g:res_FuncOne
enddef

def s:ConcatStrings(): string
  return 'one' .. 'two' .. 'three'
enddef

def s:ComputeConst(): number
  return 2 + 3 * 4 / 6 + 7
enddef

def s:ComputeConstParen(): number
  return ((2 + 4) * (8 / 2)) / (3 + 4)
enddef

def Test_simplify_const_expr()
  let res = execute('disass s:ConcatStrings')
  assert_match('<SNR>\d*_ConcatStrings\_s*' ..
        "return 'one' .. 'two' .. 'three'\\_s*" ..
        '\d PUSHS "onetwothree"\_s*' ..
        '\d RETURN',
        res)

  res = execute('disass s:ComputeConst')
  assert_match('<SNR>\d*_ComputeConst\_s*' ..
        'return 2 + 3 \* 4 / 6 + 7\_s*' ..
        '\d PUSHNR 11\_s*' ..
        '\d RETURN',
        res)

  res = execute('disass s:ComputeConstParen')
  assert_match('<SNR>\d*_ComputeConstParen\_s*' ..
        'return ((2 + 4) \* (8 / 2)) / (3 + 4)\_s*' ..
        '\d PUSHNR 3\>\_s*' ..
        '\d RETURN',
        res)
enddef

" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
