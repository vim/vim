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

" following command is :|"
execute "call Foo()" | |

execute "call"
      "\ comment
      \ "Foo()"

execute
      \ "call"
      "\ comment
      \ "Foo()"

" :execute without {expr}
execute| echo "Foo"

" trailing comment needs |
execute "foo" | " comment


" Issue #9987 (parenthesised argument - not a function call)

" FIXME: execute is ex command not builtin function
let foo = {'arg': "call Foo()"}
execute (foo.arg)
