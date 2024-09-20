" Vim :function command
" VIM_TEST_SETUP let g:vimsyn_folding = "f"
" VIM_TEST_SETUP setl fdc=2 fdl=999 fdm=syntax


" list

function
function Foo
function /Foo.*

function | echo "Foo"
function " comment
function Foo | echo "Foo"
function Foo " comment


" definition

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

function foo#bar#Foo()
  return 42
endfunction

" same name as an Ex command
function s:ls()
endfunction


" modifiers

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


" parameters

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

function Foo(x, y = 42, z = "zed")
  return 42
endfunction

function Foo(
      \ x,
      \ y = 42,
      \ z = "zed")
  return 42
endfunction


" comments

function Foo()
  " Legacy-script comment
  # 42 " comment
  return 42
endfunction


" command modifiers

silent! function Foo()
endfunction


" leading command separator

echo "Foo" | function Foo()
endfunction


" delete function

delfunction Foo
delfunction foo.bar
delfunction! Foo
delfunction foo.bar


" fold-region ending

function Foo()
  " endfunction
endfunction

function Foo()
  echo "endfunction"
endfunction

function Foo()
  let x =<< trim END
    endfunction
  END
endfunction

function Foo()
  append
    endfunction
.
endfunction

function Foo()
  change
    endfunction
.

endfunction

function Foo()
  insert
    endfunction
.
endfunction

:function Foo()
:endfunction


" Issue #15671
" No recognition of :fun or :def bodies commencing with empty lines if
" g:vimsyn_folding contains "f"

fun FA1()


    return
endfun

fun FA2()
    return
endfun

fun FB1() abort

    return
endfun

fun FB2() abort
    return
endfun

fun FC1(_)

    return
endfun

fun FC2(_)
    return
endfun

fun FD1(_) abort

    return
endfun

fun FD2(_) abort
    return
endfun

