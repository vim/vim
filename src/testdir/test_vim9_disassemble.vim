" Test the :disassemble command, and compilation as a side effect

source check.vim

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
        ' LOADV v:version.*' ..
        ' LOADS s:scriptvar from .*test_vim9_disassemble.vim.*' ..
        ' LOADG g:globalvar.*' ..
        ' LOADENV $ENVVAR.*' ..
        ' LOADREG @z.*',
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
        '&tabstop = 8.*' ..
        ' STOREOPT &tabstop.*' ..
        '$ENVVAR = ''ev''.*' ..
        ' STOREENV $ENVVAR.*' ..
        '@z = ''rv''.*' ..
        ' STOREREG @z.*',
        res)
enddef

def s:ScriptFuncTry()
  try
    echo 'yes'
  catch /fail/
    echo 'no'
  finally
    throw 'end'
  endtry
enddef

def Test_disassemble_try()
  let res = execute('disass s:ScriptFuncTry')
  assert_match('<SNR>\d*_ScriptFuncTry.*' ..
        'try.*' ..
        'TRY catch -> \d\+, finally -> \d\+.*' ..
        'catch /fail/.*' ..
        ' JUMP -> \d\+.*' ..
        ' PUSH v:exception.*' ..
        ' PUSHS "fail".*' ..
        ' COMPARESTRING =\~.*' ..
        ' JUMP_IF_FALSE -> \d\+.*' ..
        ' CATCH.*' ..
        'finally.*' ..
        ' PUSHS "end".*' ..
        ' THROW.*' ..
        'endtry.*' ..
        ' ENDTRY.*',
        res)
enddef

def s:ScriptFuncNew()
  let ll = [1, "two", 333]
  let dd = #{one: 1, two: "val"}
enddef

def Test_disassemble_new()
  let res = execute('disass s:ScriptFuncNew')
  assert_match('<SNR>\d*_ScriptFuncNew.*' ..
        'let ll = \[1, "two", 333].*' ..
        'PUSHNR 1.*' ..
        'PUSHS "two".*' ..
        'PUSHNR 333.*' ..
        'NEWLIST size 3.*' ..
        'let dd = #{one: 1, two: "val"}.*' ..
        'PUSHS "one".*' ..
        'PUSHNR 1.*' ..
        'PUSHS "two".*' ..
        'PUSHS "val".*' ..
        'NEWDICT size 2.*',
        res)
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
  assert_match('<SNR>\d\+_ScriptFuncCall.*' ..
        'changenr().*' ..
        ' BCALL changenr(argc 0).*' ..
        'char2nr("abc").*' ..
        ' PUSHS "abc".*' ..
        ' BCALL char2nr(argc 1).*' ..
        'Test_disassemble_new().*' ..
        ' DCALL Test_disassemble_new(argc 0).*' ..
        'FuncWithArg(343).*' ..
        ' PUSHNR 343.*' ..
        ' DCALL FuncWithArg(argc 1).*' ..
        'ScriptFuncNew().*' ..
        ' DCALL <SNR>\d\+_ScriptFuncNew(argc 0).*' ..
        's:ScriptFuncNew().*' ..
        ' DCALL <SNR>\d\+_ScriptFuncNew(argc 0).*' ..
        'UserFunc().*' ..
        ' UCALL UserFunc(argc 0).*' ..
        'UserFuncWithArg("foo").*' ..
        ' PUSHS "foo".*' ..
        ' UCALL UserFuncWithArg(argc 1).*' ..
        'let FuncRef = function("UserFunc").*' ..
        'FuncRef().*' ..
        ' LOAD $\d.*' ..
        ' PCALL (argc 0).*' ..
        'let FuncRefWithArg = function("UserFuncWithArg").*' ..
        'FuncRefWithArg("bar").*' ..
        ' PUSHS "bar".*' ..
        ' LOAD $\d.*' ..
        ' PCALL (argc 1).*' ..
        'return "yes".*' ..
        ' PUSHS "yes".*' ..
        ' RETURN.*',
        res)
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
  assert_match('<SNR>\d\+_ScriptPCall.*' ..
        'RefThis()("text").*' ..
        '\d DCALL RefThis(argc 0).*' ..
        '\d PUSHS "text".*' ..
        '\d PCALL top (argc 1).*' ..
        '\d PCALL end.*' ..
        '\d DROP.*' ..
        '\d PUSHNR 0.*' ..
        '\d RETURN.*',
        res)
enddef


def FuncWithForwardCall(): string
  return DefinedLater("yes")
enddef

def DefinedLater(arg: string): string
  return arg
enddef

def Test_disassemble_update_instr()
  let res = execute('disass FuncWithForwardCall')
  assert_match('FuncWithForwardCall.*' ..
        'return DefinedLater("yes").*' ..
        '\d PUSHS "yes".*' ..
        '\d UCALL DefinedLater(argc 1).*' ..
        '\d CHECKTYPE string stack\[-1].*' ..
        '\d RETURN.*',
        res)

  " Calling the function will change UCALL into the faster DCALL
  assert_equal('yes', FuncWithForwardCall())

  res = execute('disass FuncWithForwardCall')
  assert_match('FuncWithForwardCall.*' ..
        'return DefinedLater("yes").*' ..
        '\d PUSHS "yes".*' ..
        '\d DCALL DefinedLater(argc 1).*' ..
        '\d CHECKTYPE string stack\[-1].*' ..
        '\d RETURN.*',
        res)
enddef


def FuncWithDefault(arg: string = 'default'): string
  return arg
enddef

def Test_disassemble_call_default()
  let res = execute('disass FuncWithDefault')
  assert_match('FuncWithDefault.*' ..
        '\d PUSHS "default".*' ..
        '\d STORE arg\[-1].*' ..
        'return arg.*' ..
        '\d LOAD arg\[-1].*' ..
        '\d RETURN.*',
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
  assert_match('HasEval.*' ..
        'if has("eval").*' ..
        ' PUSHS "yes".*',
        instr)
  assert_notmatch('JUMP', instr)

  assert_equal("\nno", execute('call HasNothing()'))
  instr = execute('disassemble HasNothing')
  assert_match('HasNothing.*' ..
        'if has("nothing").*' ..
        'else.*' ..
        ' PUSHS "no".*',
        instr)
  assert_notmatch('PUSHS "yes"', instr)
  assert_notmatch('JUMP', instr)

  assert_equal("\neval", execute('call HasSomething()'))
  instr = execute('disassemble HasSomething')
  assert_match('HasSomething.*' ..
        'if has("nothing").*' ..
        'elseif has("something").*' ..
        'elseif has("eval").*' ..
        ' PUSHS "eval".*' ..
        'elseif has("less").*',
        instr)
  assert_notmatch('PUSHS "nothing"', instr)
  assert_notmatch('PUSHS "something"', instr)
  assert_notmatch('PUSHS "less"', instr)
  assert_notmatch('JUMP', instr)
enddef

def WithFunc()
  let Funky1: func
  let Funky2: func = function("len")
  let Party2: func = funcref("UserFunc")
enddef

def Test_disassemble_function()
  let instr = execute('disassemble WithFunc')
  assert_match('WithFunc.*' ..
        'let Funky1: func.*' ..
        '0 PUSHFUNC "\[none]".*' ..
        '1 STORE $0.*' ..
        'let Funky2: func = function("len").*' ..
        '2 PUSHS "len".*' ..
        '3 BCALL function(argc 1).*' ..
        '4 STORE $1.*' ..
        'let Party2: func = funcref("UserFunc").*' ..
        '\d PUSHS "UserFunc".*' ..
        '\d BCALL funcref(argc 1).*' ..
        '\d STORE $2.*' ..
        '\d PUSHNR 0.*' ..
        '\d RETURN.*',
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
  assert_match('WithChannel.*' ..
        'let job1: job.*' ..
        '\d PUSHJOB "no process".*' ..
        '\d STORE $0.*' ..
        'let job2: job = job_start("donothing").*' ..
        '\d PUSHS "donothing".*' ..
        '\d BCALL job_start(argc 1).*' ..
        '\d STORE $1.*' ..
        'let chan1: channel.*' ..
        '\d PUSHCHANNEL 0.*' ..
        '\d STORE $2.*' ..
        '\d PUSHNR 0.*' ..
        '\d RETURN.*',
        instr)
enddef

def WithLambda(): string
  let F = {a -> "X" .. a .. "X"}
  return F("x")
enddef

def Test_disassemble_lambda()
  assert_equal("XxX", WithLambda())
  let instr = execute('disassemble WithLambda')
  assert_match('WithLambda.*' ..
        'let F = {a -> "X" .. a .. "X"}.*' ..
        ' FUNCREF <lambda>\d\+.*' ..
        'PUSHS "x".*' ..
        ' LOAD $0.*' ..
        ' PCALL (argc 1).*' ..
        ' CHECKTYPE string stack\[-1].*',
        instr)
enddef

def AndOr(arg): string
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
  assert_match('AndOr.*' ..
        'if arg == 1 && arg != 2 || arg == 4.*' ..
        '\d LOAD arg\[-1].*' ..
        '\d PUSHNR 1.*' ..
        '\d COMPAREANY ==.*' ..
        '\d JUMP_AND_KEEP_IF_FALSE -> \d\+.*' ..
        '\d LOAD arg\[-1].*' ..
        '\d PUSHNR 2.*' ..
        '\d COMPAREANY !=.*' ..
        '\d JUMP_AND_KEEP_IF_TRUE -> \d\+.*' ..
        '\d LOAD arg\[-1].*' ..
        '\d PUSHNR 4.*' ..
        '\d COMPAREANY ==.*' ..
        '\d JUMP_IF_FALSE -> \d\+.*',
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
  assert_match('ForLoop.*' ..
        'let res: list<number>.*' ..
        ' NEWLIST size 0.*' ..
        '\d STORE $0.*' ..
        'for i in range(3).*' ..
        '\d STORE -1 in $1.*' ..
        '\d PUSHNR 3.*' ..
        '\d BCALL range(argc 1).*' ..
        '\d FOR $1 -> \d\+.*' ..
        '\d STORE $2.*' ..
        'res->add(i).*' ..
        '\d LOAD $0.*' ..
        '\d LOAD $2.*' ..
        '\d BCALL add(argc 2).*' ..
        '\d DROP.*' ..
        'endfor.*' ..
        '\d JUMP -> \d\+.*' ..
        '\d DROP.*',
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
  assert_match('ListIndex.*' ..
        'let l = \[1, 2, 3].*' ..
        '\d PUSHNR 1.*' ..
        '\d PUSHNR 2.*' ..
        '\d PUSHNR 3.*' ..
        '\d NEWLIST size 3.*' ..
        '\d STORE $0.*' ..
        'let res = l\[1].*' ..
        '\d LOAD $0.*' ..
        '\d PUSHNR 1.*' ..
        '\d INDEX.*' ..
        '\d STORE $1.*',
        instr)
  assert_equal(2, ListIndex())
enddef

def DictMember(): number
  let d = #{item: 1}
  let res = d.item
  return res
enddef

def Test_disassemble_dict_member()
  let instr = execute('disassemble DictMember')
  assert_match('DictMember.*' ..
        'let d = #{item: 1}.*' ..
        '\d PUSHS "item".*' ..
        '\d PUSHNR 1.*' ..
        '\d NEWDICT size 1.*' ..
        '\d STORE $0.*' ..
        'let res = d.item.*' ..
        '\d LOAD $0.*' ..
        '\d MEMBER item.*' ..
        '\d STORE $1.*',
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
  assert_match('NegateNumber.*' ..
        'let nr = 9.*' ..
        '\d STORE 9 in $0.*' ..
        'let plus = +nr.*' ..
        '\d LOAD $0.*' ..
        '\d CHECKNR.*' ..
        '\d STORE $1.*' ..
        'let res = -nr.*' ..
        '\d LOAD $0.*' ..
        '\d NEGATENR.*' ..
        '\d STORE $2.*',
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
  assert_match('InvertBool.*' ..
        'let flag = true.*' ..
        '\d PUSH v:true.*' ..
        '\d STORE $0.*' ..
        'let invert = !flag.*' ..
        '\d LOAD $0.*' ..
        '\d INVERT (!val).*' ..
        '\d STORE $1.*' ..
        'let res = !!flag.*' ..
        '\d LOAD $0.*' ..
        '\d 2BOOL (!!val).*' ..
        '\d STORE $2.*',
        instr)
  call assert_equal(true, InvertBool())
enddef

def Test_disassemble_compare()
  " TODO: COMPAREFUNC
  let cases = [
        ['true == false', 'COMPAREBOOL =='],
        ['true != false', 'COMPAREBOOL !='],
        ['v:none == v:null', 'COMPARESPECIAL =='],
        ['v:none != v:null', 'COMPARESPECIAL !='],

        ['111 == 222', 'COMPARENR =='],
        ['111 != 222', 'COMPARENR !='],
        ['111 > 222', 'COMPARENR >'],
        ['111 < 222', 'COMPARENR <'],
        ['111 >= 222', 'COMPARENR >='],
        ['111 <= 222', 'COMPARENR <='],
        ['111 =~ 222', 'COMPARENR =\~'],
        ['111 !~ 222', 'COMPARENR !\~'],

        ['"xx" != "yy"', 'COMPARESTRING !='],
        ['"xx" > "yy"', 'COMPARESTRING >'],
        ['"xx" < "yy"', 'COMPARESTRING <'],
        ['"xx" >= "yy"', 'COMPARESTRING >='],
        ['"xx" <= "yy"', 'COMPARESTRING <='],
        ['"xx" =~ "yy"', 'COMPARESTRING =\~'],
        ['"xx" !~ "yy"', 'COMPARESTRING !\~'],
        ['"xx" is "yy"', 'COMPARESTRING is'],
        ['"xx" isnot "yy"', 'COMPARESTRING isnot'],

        ['0z11 == 0z22', 'COMPAREBLOB =='],
        ['0z11 != 0z22', 'COMPAREBLOB !='],
        ['0z11 is 0z22', 'COMPAREBLOB is'],
        ['0z11 isnot 0z22', 'COMPAREBLOB isnot'],

        ['[1,2] == [3,4]', 'COMPARELIST =='],
        ['[1,2] != [3,4]', 'COMPARELIST !='],
        ['[1,2] is [3,4]', 'COMPARELIST is'],
        ['[1,2] isnot [3,4]', 'COMPARELIST isnot'],

        ['#{a:1} == #{x:2}', 'COMPAREDICT =='],
        ['#{a:1} != #{x:2}', 'COMPAREDICT !='],
        ['#{a:1} is #{x:2}', 'COMPAREDICT is'],
        ['#{a:1} isnot #{x:2}', 'COMPAREDICT isnot'],

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
  if has('float')
    cases->extend([
        ['1.1 == 2.2', 'COMPAREFLOAT =='],
        ['1.1 != 2.2', 'COMPAREFLOAT !='],
        ['1.1 > 2.2', 'COMPAREFLOAT >'],
        ['1.1 < 2.2', 'COMPAREFLOAT <'],
        ['1.1 >= 2.2', 'COMPAREFLOAT >='],
        ['1.1 <= 2.2', 'COMPAREFLOAT <='],
        ['1.1 =~ 2.2', 'COMPAREFLOAT =\~'],
        ['1.1 !~ 2.2', 'COMPAREFLOAT !\~'],
        ])
  endif

  let nr = 1
  for case in cases
    writefile(['def TestCase' .. nr .. '()',
             '  if ' .. case[0],
             '    echo 42'
             '  endif',
             'enddef'], 'Xdisassemble')
    source Xdisassemble
    let instr = execute('disassemble TestCase' .. nr)
    assert_match('TestCase' .. nr .. '.*' ..
        'if ' .. substitute(case[0], '[[~]', '\\\0', 'g') .. '.*' ..
        '\d \(PUSH\|FUNCREF\).*' ..
        '\d \(PUSH\|FUNCREF\|LOADG\).*' ..
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
  assert_match('\<SNR>\d*_Execute.*' ..
        "execute 'help vim9.txt'.*" ..
        '\d PUSHS "help vim9.txt".*' ..
        '\d EXECUTE 1.*' ..
        "let cmd = 'help vim9.txt'.*" ..
        '\d PUSHS "help vim9.txt".*' ..
        '\d STORE $0.*' ..
        'execute cmd.*' ..
        '\d LOAD $0.*' ..
        '\d EXECUTE 1.*' ..
        "let tag = 'vim9.txt'.*" ..
        '\d PUSHS "vim9.txt".*' ..
        '\d STORE $1.*' ..
        "execute 'help ' .. tag.*" ..
        '\d PUSHS "help ".*' ..
        '\d LOAD $1.*' ..
        '\d CONCAT.*' ..
        '\d EXECUTE 1.*' ..
        '\d PUSHNR 0.*' ..
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
  assert_match('.* def SomeStringArg(arg: string).*' ..
        '  echo arg.*' ..
        '  enddef',
        res1)

  let res2 = execute('function SomeAnyArg')
  assert_match('.* def SomeAnyArg(arg: any).*' ..
        '  echo arg.*' ..
        '  enddef',
        res2)

  let res3 = execute('function SomeStringArgAndReturn')
  assert_match('.* def SomeStringArgAndReturn(arg: string): string.*' ..
        '  return arg.*' ..
        '  enddef',
        res3)
enddef

" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
