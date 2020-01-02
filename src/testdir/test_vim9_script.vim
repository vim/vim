" Test various aspects of the Vim9 script language.

" Check that "lines" inside ":def" results in an "error" message.
func CheckDefFailure(lines, error)
  call writefile(['def Func()'] + a:lines + ['enddef'], 'Xdef')
  call assert_fails('so Xdef', a:error, a:lines)
  call delete('Xdef')
endfunc

func CheckScriptFailure(lines, error)
  call writefile(a:lines, 'Xdef')
  call assert_fails('so Xdef', a:error, a:lines)
  call delete('Xdef')
endfunc

def Test_syntax()
  let var = 234
  let other: list<string> = ['asdf']
enddef

func Test_def_basic()
  def SomeFunc(): string
    return 'yes'
  enddef
  call assert_equal('yes', SomeFunc())
endfunc

def Test_assignment()
  let list1: list<string> = ['sdf', 'asdf']
  let list2: list<number> = [1, 2, 3]

  let dict1: dict<string> = #{key: 'value'}
  let dict2: dict<number> = #{one: 1, two: 2}
enddef

func Test_assignment_failure()
  call CheckDefFailure(['let var=234'], 'E1005:')
  call CheckDefFailure(['let var =234'], 'E1005:')
  call CheckDefFailure(['let var= 234'], 'E1005:')

  call CheckDefFailure(['let var: list<string> = [123]'], 'expected list<string> but got list<number>')
  call CheckDefFailure(['let var: list<number> = ["xx"]'], 'expected list<number> but got list<string>')

  call CheckDefFailure(['let var: dict<string> = #{key: 123}'], 'expected dict<string> but got dict<number>')
  call CheckDefFailure(['let var: dict<number> = #{key: "xx"}'], 'expected dict<number> but got dict<string>')
endfunc

func Test_const()
  call CheckDefFailure(['const var = 234', 'var = 99'], 'E1018:')
  call CheckDefFailure(['const one = 234', 'let one = 99'], 'E1017:')
  call CheckDefFailure(['const two'], 'E1021:')
endfunc

def Test_block()
  let outer = 1
  {
    let inner = 2
    assert_equal(1, outer)
    assert_equal(2, inner)
  }
  assert_equal(1, outer)
enddef

func Test_block_failure()
  call CheckDefFailure(['{', 'let inner = 1', '}', 'echo inner'], 'E1001:')
endfunc

def ReturnString(): string
  return 'string'
enddef

def ReturnNumber(): number
  return 123
enddef

def Test_return_string()
  assert_equal('string', ReturnString())
  assert_equal(123, ReturnNumber())
enddef

func Increment()
  let g:counter += 1
endfunc

def Test_call_ufunc_count()
  g:counter = 1
  Increment()
  Increment()
  Increment()
  assert_equal(4, g:counter)
  unlet g:counter
enddef

def Test_return_type_wrong()
  CheckScriptFailure(['def Func(): number', 'return "a"', 'enddef'], 'expected number but got string')
  CheckScriptFailure(['def Func(): string', 'return 1', 'enddef'], 'expected string but got number')
  CheckScriptFailure(['def Func(): void', 'return "a"', 'enddef'], 'expected void but got string')
  CheckScriptFailure(['def Func()', 'return "a"', 'enddef'], 'expected void but got string')
enddef


" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
