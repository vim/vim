" Vim :function command


" List

function
function Foo
function /Foo.*

function | echo "Foo"
function " comment
function Foo | echo "Foo"
function Foo " comment


" Definition

" empty definition
function Foo()
endfunction

" curly-brace names
function {"F"}oo()
endfunction

function F{"o"}o()
endfunction

function Fo{"o"}()
endfunction

function {"F"}o{"o"}()
endfunction

function {"F"}{"o"}{"o"}()
endfunction

function Foo()
  return 42
endfunction

" trailing whitespace
function Foo()  
  return 42
endfunction

function Foo() " comment
  return 42
endfunction

function! Foo()
  return 42
endfunction

function g:Foo()
  return 42
endfunction

function s:Foo()
  return 42
endfunction

function <SID>Foo()
  return 42
endfunction

function b:dict.Foo()
  return 42
endfunction

function w:dict.Foo()
  return 42
endfunction

function t:dict.Foo()
  return 42
endfunction

function g:dict.Foo()
  return 42
endfunction

function s:dict.Foo()
  return 42
endfunction

function v:dict.Foo()
  return 42
endfunction

function Foo(arg)
  let l:dict = {}
  function l:dict.BAR()
    return 42
  endfunction
  function a:arg.BAR()
    return 42
  endfunction
endfunction

function foo#bar#Foo()
  return 42
endfunction

function g:foo#bar#Foo()
  return 42
endfunction

" same name as an Ex command
function s:ls()
endfunction


" Modifiers

function Foo() range
endfunction

function Foo() range " comment
endfunction

function Foo() range
  return 42
endfunction

function Foo() abort
  return 42
endfunction

function Foo() dict
  return 42
endfunction

function Foo() closure
  return 42
endfunction

function Foo() range abort dict closure
  return 42
endfunction

function! Foo() range
  return 42
endfunction

function! Foo() abort
  return 42
endfunction

function! Foo() dict
  return 42
endfunction

function! Foo() closure
  return 42
endfunction

function! Foo() range abort dict closure
  return 42
endfunction


" :endfunction trailing

function Foo()
  return 42
  " trailing whitespace
endfunction  

function Foo()
  return 42
endfunction | echo "Foo"

function Foo()
  return 42
endfunction " comment


" Parameters

function Foo(x, y, z, ...)
  return 42
endfunction

function Foo(
      \ x,
      \ y,
      \ z,
      \ ...)
  return 42
endfunction

function Foo(
        x,
        y,
        z,
        ...)
  return 42
endfunction

function Foo(x, y = 42, z = "zed")
  return 42
endfunction

function Foo(
      \ x,
      \ y = 42,
      \ z = "zed")
  return 42
endfunction

function Foo(
        x,
        y = 42,
        z = "zed")
  return 42
endfunction


" Arguments

function Foo(a, b, c)
  echo a:a a:b a:c
endfunction

function Foo(...)
  echo a:000
  echo a:0
  echo a:1 a:2 a:3 a:4 a:5 a:6 a:7 a:8 a:9 a:10 a:11 a:12 a:13 a:14 a:15 a:16 a:17 a:18 a:19 a:20
endfunction


" Issue #16243 (Vim script def parameters syntax highlight is wrong)

function Test(lines = [line('.'), line('.')])
endfunction


" Comments

function Foo()
  " Legacy-script comment
  # 42 " comment
  return 42
endfunction


" Command modifiers

silent! function Foo()
endfunction


" Leading command separator

echo "Foo" | function Foo()
endfunction


" Issue https://github.com/vim/vim/pull/17420#issuecomment-2927798687
" (function named /s:fu%\[nction]/)

func! s:func(_, func)
    return a:func
endfunc

