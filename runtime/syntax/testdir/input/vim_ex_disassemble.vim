" Vim :disassemble command


disassemble         Foo
disassemble debug   Foo
disassemble profile Foo


disassemble Foo
disassemble s:Foo
disassemble g:Foo
disassemble Foo<string>
disassemble qualifier.Foo
disassemble qualifier#Foo


" Trailing bar and tail comments

disassemble         Foo | echo "..."
disassemble debug   Foo | echo "..."
disassemble profile Foo | echo "..."

disassemble         Foo " comment
disassemble debug   Foo " comment
disassemble profile Foo " comment


def Vim9Context()
  disassemble         Foo
  disassemble debug   Foo
  disassemble profile Foo


  disassemble Foo
  disassemble g:Foo
  disassemble Foo<string>
  disassemble qualifier.Foo
  disassemble qualifier#Foo


  # Trailing bar and tail comments

  disassemble         Foo | echo "..."
  disassemble debug   Foo | echo "..."
  disassemble profile Foo | echo "..."

  disassemble         Foo # comment
  disassemble debug   Foo # comment
  disassemble profile Foo # comment
enddef

