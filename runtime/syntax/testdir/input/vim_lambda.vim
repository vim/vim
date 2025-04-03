" Vim lambda expressions
" VIM_TEST_SETUP hi link vimLambdaOperator Todo
" VIM_TEST_SETUP hi link vimLambdaBrace    Todo
" VIM_TEST_SETUP hi link vimFuncParam	   Identifier


let expr = 42

let Foo = {-> expr}
let Foo = {_ -> expr}
let Foo = {... -> expr}
let Foo = {x -> expr}
let Foo = {x, _ -> expr}
let Foo = {x, ... -> expr}
let Foo = {x, y -> expr}


" line continuations

let Foo = {->
      "\ comment
      \ expr
      \ }
let Foo = {_ ->
      "\ comment
      \ expr
      \ }
let Foo = {... ->
      "\ comment
      \ expr
      \ }
let Foo = {x ->
      \ expr
      "\ comment
      \ }
let Foo = {x, y ->
      "\ comment
      \ expr
      \ }

let Foo = {
      \ ->
      "\ comment
      \ expr
      \ }
let Foo = {x
      \ ->
      "\ comment
      \ expr
      \ }
let Foo = {x, y
      \ ->
      "\ comment
      \ expr
      \ }

let Foo = {x,
      \ y,
      \ z -> expr}

let Foo = {
      "\ comment
      \ x,
      "\ comment
      \ y,
      "\ comment
      \ z
      "\ comment
      \ ->
      "\ comment
      \ expr
      "\ comment
      \ }

let Foo = {-> [
      \ 42,
      \ 83
      \]}

let Foo = {-> {
      \ 'a': 42,
      \ 'b': 83
      \}}

let Foo = {-> #{
      \ a: 42,
      \ b: 83
      \}}

let Foo = {-> {->[
      \ 42,
      \ 83
      \]}}

let Foo = {-> {-> {
      \ 'a': 42,
      \ 'b': 83
      \}}}

let Foo = {-> {-> #{
      \ a: 42,
      \ b: 83
      \}}}


" :help lambda

:let F = {arg1, arg2 -> arg1 - arg2}
:echo F(5, 2)

:let F = {-> 'error function'}
:echo F('ignored')

:function Foo(arg)
:  let i = 3
:  return {x -> x + i - a:arg}
:endfunction
:let Bar = Foo(4)
:echo Bar(6)

:echo map([1, 2, 3], {idx, val -> val + 1})
" [2, 3, 4]  

:echo sort([3,7,2,1,4], {a, b -> a - b})
" [1, 2, 3, 4, 7]
:let timer = timer_start(500,
		\ {-> execute("echo 'Handler called'", "")},
		\ {'repeat': 3})

