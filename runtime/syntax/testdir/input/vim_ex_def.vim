" Vim :def command


" List

def
def Foo
def /Foo.*

def | echo "Foo"
def " comment
def Foo | echo "Foo"
def Foo " comment


" Definition

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


" Return types

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


" Parameters

def Foo(x: bool, y = 42, z: string = "zed")
enddef

def Foo(
    x: bool,
    y = 42,
    z: string = "zed")
enddef

" Issue #16243 (Vim script def parameters syntax highlight is wrong)

def Test(lines: list<number> = [line('.'), line('.')]): void
enddef


" Comments

def Foo()
  # Vim9-script comment
  "useless string"
enddef


" Leading command separator

echo "Foo" | def Foo()
enddef


" Command modifiers

silent! def Foo()
enddef

