" Vim :def command
" VIM_TEST_SETUP let g:vimsyn_folding = "f" | set fdm=syntax


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


" fold-region ending

def Foo()
  # enddef
enddef

def Foo()
  echo "enddef"
enddef

def Foo()
  let x =<< END
    endfunction
  END
enddef

:def Foo()
:enddef

