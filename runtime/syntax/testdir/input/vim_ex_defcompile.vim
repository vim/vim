" Vim :defcompile command


defcompile         Foo
defcompile debug   Foo
defcompile profile Foo


defcompile Foo
defcompile g:Foo
defcompile s:Foo
defcompile Foo<string>
defcompile qualifier.Foo
defcompile qualifier#Foo


" Trailing bar and tail comments

defcompile         Foo | echo "..."
defcompile debug   Foo | echo "..."
defcompile profile Foo | echo "..."

defcompile         Foo " comment
defcompile debug   Foo " comment
defcompile profile Foo " comment


def Vim9Context()
  defcompile         Foo
  defcompile debug   Foo
  defcompile profile Foo


  defcompile Foo
  defcompile g:Foo
  defcompile Foo<string>
  defcompile qualifier.Foo
  defcompile qualifier#Foo


  # Trailing bar and tail comments

  defcompile         Foo | echo "..."
  defcompile debug   Foo | echo "..."
  defcompile profile Foo | echo "..."

  defcompile         Foo # comment
  defcompile debug   Foo # comment
  defcompile profile Foo # comment
enddef

