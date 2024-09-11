" Vim :def command
" VIM_TEST_SETUP let g:vimsyn_folding = "f"
" VIM_TEST_SETUP setl fdc=2 fdl=999 fdm=syntax


" list

def
def Foo
def /Foo.*

def | echo "Foo"
def " comment
def Foo | echo "Foo"
def Foo " comment


" definition

" empty definition
def Foo()
enddef

def Foo(): number
  return 42
enddef

" trailing whitespace
def Foo(): number  
  return 42
enddef

def Foo() # comment
enddef

def Foo(): number # comment
  return 42
enddef

def! Foo(): number
  return 42
enddef

def g:Foo(): number
  return 42
enddef

def s:Foo(): number
  return 42
enddef

def <SID>Foo(): number
  return 42
enddef

def foo#bar#Foo(): number
  return 42
enddef

" same name as an Ex command
def s:ls()
enddef


" return types

def Foo(): void
enddef

def Foo(): void # comment
enddef

def Foo(): list<dict<number>>
enddef

def Foo(): func(dict<list<number>>, func, bool, func(number, list<number>)): bool
enddef


" :enddef trailing

def Foo()
  # trailing whitespace
enddef  

def Foo()
enddef | echo "Foo"

def Foo()
enddef # comment


" parameters

def Foo(x: bool, y = 42, z: string = "zed")
enddef

def Foo(
    x: bool,
    y = 42,
    z: string = "zed")
enddef


" comments

def Foo()
  # Vim9-script comment
  "useless string"
enddef


" command modifiers

silent! def Foo()
enddef


" leading command separator

echo "Foo" | def Foo()
enddef


" fold-region ending

def Foo()
  # enddef
enddef

def Foo()
  echo "enddef"
enddef

def Foo()
  var x =<< trim END
    endfunction
  END
enddef

:def Foo()
:enddef


" Issue #15671
" No recognition of :fun or :def bodies commencing with empty lines if
" g:vimsyn_folding contains "f"

def MA1()


    return
enddef

def MA2()
    return
enddef

def MB1(): void

    return
enddef

def MB2(): void
    return
enddef

def MC1(_: any)

    return
enddef

def MC2(_: any)
    return
enddef

def MD1(_: any): void

    return
enddef

def MD2(_: any): void
    return
enddef

