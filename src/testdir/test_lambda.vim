" Test for lambda and closure

import './util/vim9.vim' as v9

func Test_lambda_feature()
  call assert_equal(1, has('lambda'))
endfunc

func Test_lambda_with_filter()
  let s:x = 2
  call assert_equal([2, 3], filter([1, 2, 3], {i, v -> v >= s:x}))
  unlet s:x
endfunc

func Test_lambda_with_map()
  let s:x = 1
  call assert_equal([2, 3, 4], map([1, 2, 3], {i, v -> v + s:x}))
  unlet s:x
endfunc

func Test_lambda_with_sort()
  call assert_equal([1, 2, 3, 4, 7], sort([3,7,2,1,4], {a, b -> a - b}))
endfunc

func Test_lambda_with_timer()
  CheckFeature timers

  let s:n = 0
  let s:timer_id = 0
  func! s:Foo()
    let s:timer_id = timer_start(10, {-> execute("let s:n += 1 | echo s:n", "")}, {"repeat": -1})
  endfunc

  call s:Foo()
  " check timer works
  call WaitForAssert({-> assert_true(s:n > 0)})

  " do not collect lambda
  call test_garbagecollect_now()

  " check timer still works
  let m = s:n
  call WaitForAssert({-> assert_true(s:n > m)})

  call timer_stop(s:timer_id)
  unlet s:n s:timer_id
  delfunc s:Foo
endfunc

func Test_lambda_vim9cmd_linebreak()
  CheckFeature timers

  let lines =<< trim END
      vim9cmd call timer_start(10, (x) => {
          # comment
          g:result = 'done'
         })
  END
  call v9.CheckScriptSuccess(lines)
  call WaitForAssert({-> assert_equal('done', get(g:, 'result', ''))})
  unlet g:result

  let lines =<< trim END
      g:result = [0]->map((_, v) =>
          1 # inline comment
          +
          2
      )
      assert_equal([3], g:result)
  END
  call v9.CheckDefAndScriptSuccess(lines)
endfunc

def Test_lamba_compiled_linebreak()
  var lines =<< trim END
      vim9script

      def Echo(what: any)
        assert_equal('hello world', what)
      enddef
      def That()
        printf("hello ")
          ->((x) => x .. "world")()
          ->Echo()
      enddef
      That()
  END
  v9.CheckScriptSuccess(lines)
enddef

func Test_lambda_with_partial()
  let l:Cb = function({... -> ['zero', a:1, a:2, a:3]}, ['one', 'two'])
  call assert_equal(['zero', 'one', 'two', 'three'], l:Cb('three'))
endfunc

function Test_lambda_fails()
  call assert_equal(3, {a, b -> a + b}(1, 2))
  call assert_fails('echo {a, a -> a + a}(1, 2)', 'E853: Duplicate argument name: a')
  call assert_fails('echo {a, b -> a + b)}(1, 2)', 'E451: Expected }: )}(1, 2)')
  call assert_fails('echo 10->{a -> a + 2}', 'E107: Missing parentheses: lambda')

  call assert_fails('eval 0->(', "E110: Missing ')'")
  call assert_fails('eval 0->(3)()', "E1275: String or function required for ->(expr)")
  call assert_fails('eval 0->([3])()', "E1275: String or function required for ->(expr)")
  call assert_fails('eval 0->({"a": 3})()', "E1275: String or function required for ->(expr)")
  call assert_fails('eval 0->(xxx)()', "E121: Undefined variable: xxx")
endfunc

func Test_not_lamda()
  let x = {'>' : 'foo'}
  call assert_equal('foo', x['>'])
endfunc

func Test_lambda_capture_by_reference()
  let v = 1
  let F = {x -> x + v}
  let v = 2
  call assert_equal(12, F(10))
endfunc

func Test_lambda_side_effect()
  func! s:update_and_return(arr)
    let a:arr[1] = 5
    return a:arr
  endfunc

  func! s:foo(arr)
    return {-> s:update_and_return(a:arr)}
  endfunc

  let arr = [3,2,1]
  call assert_equal([3, 5, 1], s:foo(arr)())
endfunc

func Test_lambda_refer_local_variable_from_other_scope()
  func! s:foo(X)
    return a:X() " refer l:x in s:bar()
  endfunc

  func! s:bar()
    let x = 123
    return s:foo({-> x})
  endfunc

  call assert_equal(123, s:bar())
endfunc

func Test_lambda_do_not_share_local_variable()
  func! s:define_funcs()
    let One = {-> split(execute("let a = 'abc' | echo a"))[0]}
    let Two = {-> exists("a") ? a : "no"}
    return [One, Two]
  endfunc

  let F = s:define_funcs()

  call assert_equal('no', F[1]())
  call assert_equal('abc', F[0]())
  call assert_equal('no', F[1]())
endfunc

func Test_lambda_closure_counter()
  func! s:foo()
    let x = 0
    return {-> [execute("let x += 1"), x][-1]}
  endfunc

  let F = s:foo()
  call test_garbagecollect_now()
  call assert_equal(1, F())
  call assert_equal(2, F())
  call assert_equal(3, F())
  call assert_equal(4, F())
endfunc

func Test_lambda_with_a_var()
  func! s:foo()
    let x = 2
    return {... -> a:000 + [x]}
  endfunc
  func! s:bar()
    return s:foo()(1)
  endfunc

  call assert_equal([1, 2], s:bar())
endfunc

func Test_lambda_call_lambda_from_lambda()
  func! s:foo(x)
    let F1 = {-> {-> a:x}}
    return {-> F1()}
  endfunc

  let F = s:foo(1)
  call assert_equal(1, F()())
endfunc

func Test_lambda_delfunc()
  func! s:gen()
    let pl = l:
    let l:Foo = {-> get(pl, "Foo", get(pl, "Bar", {-> 0}))}
    let l:Bar = l:Foo
    delfunction l:Foo
    return l:Bar
  endfunc

  let l:F = s:gen()
  call assert_fails(':call l:F()', 'E933: Function was deleted: <lambda>')
endfunc

func Test_lambda_scope()
  func! s:NewCounter()
    let c = 0
    return {-> [execute('let c += 1'), c][-1]}
  endfunc

  func! s:NewCounter2()
    return {-> [execute('let c += 100'), c][-1]}
  endfunc

  let C = s:NewCounter()
  let D = s:NewCounter2()

  call assert_equal(1, C())
  call assert_fails(':call D()', 'E121: Undefined variable: c')
  call assert_equal(2, C())
endfunc

func Test_lambda_share_scope()
  func! s:New()
    let c = 0
    let Inc0 = {-> [execute('let c += 1'), c][-1]}
    let Dec0 = {-> [execute('let c -= 1'), c][-1]}
    return [Inc0, Dec0]
  endfunc

  let [Inc, Dec] = s:New()

  call assert_equal(1, Inc())
  call assert_equal(2, Inc())
  call assert_equal(1, Dec())
endfunc

func Test_lambda_circular_reference()
  func! s:Foo()
    let d = {}
    let d.f = {-> d}
    return d.f
  endfunc

  " the lambda keeps the context alive, the dict refers to the lambda
  let F = s:Foo()
  call test_garbagecollect_now()
  let d = F()
  call assert_true(d.f is F)
  call assert_true(d.f() is d)
  let i = 0 | while i < 10000 | call s:Foo() | let i+= 1 | endwhile
  call test_garbagecollect_now()
endfunc

func Test_lambda_combination()
  call assert_equal(2, {x -> {x -> x}}(1)(2))
  call assert_equal(10, {y -> {x -> x(y)(10)}({y -> y})}({z -> z}))
  call assert_equal(5.0, {x -> {y -> x / y}}(10)(2.0))
  call assert_equal(6, {x -> {y -> {z -> x + y + z}}}(1)(2)(3))

  call assert_equal(6, {x -> {f -> f(x)}}(3)({x -> x * 2}))
  call assert_equal(6, {f -> {x -> f(x)}}({x -> x * 2})(3))

  " Z combinator
  let Z = {f -> {x -> f({y -> x(x)(y)})}({x -> f({y -> x(x)(y)})})}
  let Fact = {f -> {x -> x == 0 ? 1 : x * f(x - 1)}}
  call assert_equal(120, Z(Fact)(5))
endfunc

func Test_closure_counter()
  func! s:foo()
    let x = 0
    func! s:bar() closure
      let x += 1
      return x
    endfunc
    return function('s:bar')
  endfunc

  let F = s:foo()
  call test_garbagecollect_now()
  call assert_equal(1, F())
  call assert_equal(2, F())
  call assert_equal(3, F())
  call assert_equal(4, F())

  call assert_match("^\n   function <SNR>\\d\\+_bar() closure"
  \              .. "\n1        let x += 1"
  \              .. "\n2        return x"
  \              .. "\n   endfunction$", execute('func s:bar'))
endfunc

func Test_closure_unlet()
  func! s:foo()
    let x = 1
    func! s:bar() closure
      unlet x
    endfunc
    call s:bar()
    return l:
  endfunc

  call assert_false(has_key(s:foo(), 'x'))
  call test_garbagecollect_now()
endfunc

func LambdaFoo()
  let x = 0
  func! LambdaBar() closure
    let x += 1
    return x
  endfunc
  return function('LambdaBar')
endfunc

func Test_closure_refcount()
  let g:Count = LambdaFoo()
  call test_garbagecollect_now()
  call assert_equal(1, g:Count())
  let g:Count2 = LambdaFoo()
  call test_garbagecollect_now()
  call assert_equal(1, g:Count2())
  call assert_equal(2, g:Count())
  call assert_equal(3, g:Count2())

  delfunc LambdaFoo
  delfunc LambdaBar
endfunc

func Test_named_function_closure()
  func! Afoo()
    let x = 14
    func! s:Abar() closure
      return x
    endfunc
    call assert_equal(14, s:Abar())
  endfunc
  call Afoo()
  call assert_equal(14, s:Abar())
  call test_garbagecollect_now()
  call assert_equal(14, s:Abar())
  delfunc Afoo
  delfunc s:Abar
endfunc

func Test_closure_outer_locked_or_deleted()
  " a closure cannot change a locked or const variable of the outer scope
  func! s:LockedOuter()
    let x = 1
    lockvar x
    const y = 2
    func! s:SetX() closure
      let x = 3
    endfunc
    func! s:SetY() closure
      let y = 4
    endfunc
    call assert_fails('call s:SetX()', 'E741: Value is locked: x')
    call assert_fails('call s:SetY()', 'E741: Value is locked: y')
    return [x, y]
  endfunc
  call assert_equal([1, 2], s:LockedOuter())

  " the context stays valid after the outer function is deleted or redefined
  func! s:DelOuter()
    let s = 'outer'
    return {-> s}
  endfunc
  let F = s:DelOuter()
  delfunc s:DelOuter
  call test_garbagecollect_now()
  call assert_equal('outer', F())
  func! s:DelOuter()
    return {-> 'new'}
  endfunc
  call assert_equal('outer', F())

  delfunc s:LockedOuter
  delfunc s:SetX
  delfunc s:SetY
  delfunc s:DelOuter
endfunc

func Test_lambda_with_index()
  let List = {x -> [x]}
  let Extract = {-> function(List, ['foobar'])()[0]}
  call assert_equal('foobar', Extract())
endfunc

func Test_lambda_error()
  " This was causing a crash
  call assert_fails('ec{@{->{d->()()', 'E451: Expected }: ')
endfunc

func Test_closure_error()
  let l =<< trim END
    func F1() closure
      return 1
    endfunc
  END
  call writefile(l, 'Xscript', 'D')
  call assert_fails('source Xscript', 'E932: Closure function should not be at top level: F1')
endfunc

" vim: shiftwidth=2 sts=2 expandtab
