" Vim :execute command


" :help :execute

execute "buffer" nextbuf
execute "normal" count .. "w"
execute '!ls' | echo "theend"
execute "normal ixxx\<Esc>"
execute "e " .. fnameescape(filename)
execute "!ls " .. shellescape(filename, 1)
if 0
 execute 'while i > 5'
  echo "test"
 endwhile
endif
execute 'while i < 5 | echo i | let i = i + 1 | endwhile'


" Trailing bar vs OR operator

" OR operator
execute foo || bar ? "Foo" : "NotFoo"
execute foo ||
      \ bar ? "Foo" : "NotFoo"

" following command is :|"
execute "Foo" | |

" invalid expression
execute "Foo" ||


" Line continuations

execute "call"
      "\ comment
      \ "Foo()"

execute
      \ "call"
      "\ comment
      \ "Foo()"


" Trailing bar and comments

" :execute without {expr}
execute| echo "Foo"

" trailing comment needs |
execute "Foo" | " comment

def Vim9Context()
  # trailing comment allowed
  execute "Foo" # comment
enddef


" Issue #9987 (parenthesised argument - not a function call)

let foo = {'arg': "call Foo()"}
execute (foo.arg)

