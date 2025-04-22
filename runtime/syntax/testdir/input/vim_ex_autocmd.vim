" Vim :autocmd command
" VIM_TEST_SETUP highlight vimAutoCmdGroup Todo
" VIM_TEST_SETUP highlight vimUserAutoEvent Title


" Define

autocmd foogroup BufRead            *.txt echo "Foo" | echo "Bar"
autocmd          BufRead            *.txt echo "Foo" | echo "Bar"
autocmd          BufRead,BufNewFile *.txt echo "Foo" | echo "Bar"
autocmd          User FooEvent      *.txt echo "Foo" | echo "Bar"

autocmd foogroup BufRead            *.txt ++once echo "Foo" | echo "Bar"
autocmd          BufRead            *.txt ++once echo "Foo" | echo "Bar"
autocmd          BufRead,BufNewFile *.txt ++once echo "Foo" | echo "Bar"
autocmd          User FooEvent      *.txt ++once echo "Foo" | echo "Bar"

autocmd foogroup BufRead            *.txt ++nested echo "Foo" | echo "Bar"
autocmd          BufRead            *.txt ++nested echo "Foo" | echo "Bar"
autocmd          BufRead,BufNewFile *.txt ++nested echo "Foo" | echo "Bar"
autocmd          User FooEvent      *.txt ++nested echo "Foo" | echo "Bar"

autocmd foogroup BufRead            *.txt ++once ++nested echo "Foo" | echo "Bar"
autocmd          BufRead            *.txt ++once ++nested echo "Foo" | echo "Bar"
autocmd          BufRead,BufNewFile *.txt ++once ++nested echo "Foo" | echo "Bar"
autocmd          User FooEvent      *.txt ++once ++nested echo "Foo" | echo "Bar"

autocmd foogroup BufRead            <buffer>      ++once ++nested echo "Foo" | echo "Bar"
autocmd foogroup BufRead            <buffer=42>   ++once ++nested echo "Foo" | echo "Bar"
autocmd foogroup BufRead            <buffer=abuf> ++once ++nested echo "Foo" | echo "Bar"
autocmd          BufRead            <buffer>      ++once ++nested echo "Foo" | echo "Bar"
autocmd          BufRead            <buffer=42>   ++once ++nested echo "Foo" | echo "Bar"
autocmd          BufRead            <buffer=abuf> ++once ++nested echo "Foo" | echo "Bar"
autocmd          BufRead,BufNewFile <buffer>      ++once ++nested echo "Foo" | echo "Bar"
autocmd          BufRead,BufNewFile <buffer=42>   ++once ++nested echo "Foo" | echo "Bar"
autocmd          BufRead,BufNewFile <buffer=abuf> ++once ++nested echo "Foo" | echo "Bar"
autocmd          User FooEvent      <buffer>      ++once ++nested echo "Foo" | echo "Bar"
autocmd          User FooEvent      <buffer=42>   ++once ++nested echo "Foo" | echo "Bar"
autocmd          User FooEvent      <buffer=abuf> ++once ++nested echo "Foo" | echo "Bar"

autocmd foogroup BufRead            f<buffer>oo   ++once ++nested echo "Foo" | echo "Bar"

autocmd BufRead *.txt {
  echo "Foo"
  echo "Bar"
}
autocmd BufRead,BufNewFile *.txt {
  echo "Foo"
  echo "Bar"
}
autocmd User FooEvent *.txt {
  echo "Foo"
  echo "Bar"
}
autocmd foogroup BufRead *.txt {
  echo "Foo"
  echo "Bar"
}


" Multiline {cmd} arg

autocmd BufRead *.txt echo "Foo"
      \| echo "Bar"
      \| echo "Baz"

autocmd BufRead *.txt echo "Foo" |
      \ echo "Bar" |
      \ echo "Baz"

autocmd BufRead *.txt
      "\ comment
      \ echo "Foo" |
      "\ comment
      \ echo "Bar"
      "\ comment
      \| echo "Baz"

autocmd BufRead,BufNewFile *.txt
      "\ comment
      \ echo "Foo" |
      "\ comment
      \ echo "Bar"
      "\ comment
      \| echo "Baz"
autocmd User FooEvent *.txt
      "\ comment
      \ echo "Foo" |
      "\ comment
      \ echo "Bar"
      "\ comment
      \| echo "Baz"
autocmd foogroup BufRead *.txt
      "\ comment
      \ echo "Foo" |
      "\ comment
      \ echo "Bar"
      "\ comment
      \| echo "Baz"


" Multiple patterns

autocmd BufRead *.txt,*.vim,*.c      echo "Foo" | echo "Bar"
autocmd BufRead <buffer>,*.vim,*.c   echo "Foo" | echo "Bar"
autocmd BufRead *.txt,<buffer>,*.c   echo "Foo" | echo "Bar"
autocmd BufRead *.txt,*.vim,<buffer> echo "Foo" | echo "Bar"

autocmd BufRead <buffer=1>,<buffer=2>,<buffer=3> echo "Foo" | echo "Bar"


" FIXME: "BufRead" and "*" are valid group names, however, :help :augroup
" explicitly directs the user NOT to shadow event names with group names
autocmd BufRead BufRead *.txt ++once ++nested echo "Foo"
autocmd *       BufRead *.txt ++once ++nested echo "Foo"


" Remove

autocmd! foogroup BufRead       *.txt ++once ++nested echo "Foo" | echo "Bar"
autocmd!          BufRead       *.txt ++once ++nested echo "Foo" | echo "Bar"
autocmd! foogroup User FooEvent *.txt ++once ++nested echo "Foo" | echo "Bar"
autocmd!          User FooEvent *.txt ++once ++nested echo "Foo" | echo "Bar"

autocmd! foogroup BufRead       *.txt
autocmd!          BufRead       *.txt
autocmd! foogroup User FooEvent *.txt
autocmd!          User FooEvent *.txt

autocmd! foogroup * *.txt
autocmd!          * *.txt

autocmd! foogroup BufRead
autocmd!          BufRead
autocmd! foogroup User FooEvent
autocmd!          User FooEvent

autocmd! foogroup
autocmd!

" command -> bang -> group "!foogroup!"
autocmd!!foogroup!
" command -> bang -> group "foogroup"
autocmd!foogroup
" command -> bang -> event
autocmd!BufRead
" command -> bang -> user event
autocmd!User FooEvent

" FIXME: "*" and "BufRead" are valid group names, however, :help :augroup
" explicitly directs the user NOT to shadow event names
" command -> group "*" -> event glob -> pattern
autocmd!* * *.txt
" command -> group "BufRead" -> event "BufRead" -> pattern
autocmd!BufRead BufRead *.txt


" List

autocmd foogroup BufRead       *.txt
autocmd          BufRead       *.txt
autocmd foogroup User FooEvent *.txt
autocmd          User FooEvent *.txt

autocmd foogroup * *.txt
autocmd          * *.txt

autocmd foogroup BufRead
autocmd          BufRead
autocmd foogroup User FooEvent
autocmd          User FooEvent

autocmd foogroup
autocmd


" :doautoall

doautoall BufRead

doautoall BufRead *.txt
doautoall foogroup BufRead
doautoall <nomodeline> BufRead

doautoall <nomodeline> foogroup BufRead *.txt

doautoall User FooEvent

doautoall User FooEvent *.txt
doautoall foogroup User FooEvent
doautoall <nomodeline> User FooEvent

doautoall <nomodeline> foogroup User FooEvent *.txt

doautoall <nomodeline> foogroup BufRead *.txt | echo "Foo"
doautoall <nomodeline> foogroup BufRead *.txt " comment


" :doautocmd

doautocmd BufRead

doautocmd BufRead *.txt
doautocmd foogroup BufRead
doautocmd <nomodeline> BufRead

doautocmd <nomodeline> foogroup BufRead *.txt

doautocmd User FooEvent

doautocmd User FooEvent *.txt
doautocmd foogroup User FooEvent
doautocmd <nomodeline> User FooEvent

doautocmd <nomodeline> foogroup User FooEvent *.txt

doautocmd <nomodeline> foogroup BufRead *.txt | echo "Foo"
doautocmd <nomodeline> foogroup BufRead *.txt | " comment


" patterns

au BufRead */*.txt

au BufRead */*.*
au BufRead */*.???
au BufRead */*.[t][x]t

au BufRead */*.[a-z][a-z]t
au BufRead */*.[[:alpha:][:alpha:]]t

au BufRead */*.[tx]\\\{2\}t
au BufRead */*.[a-z]\\\{2\}t
au BufRead */*.[[:alpha:]]\\\{2\}t
au BufRead */*.[^[:punct:]]\\\{2\}t

au BufRead */*.[]]xt
au BufRead */*.[^]]xt

au BufRead */*.[t\]]xt
au BufRead */*.[^t\]]xt

au BufRead */*.[[]xt
au BufRead */*.[^[]xt

au BufRead */*.[-]xt
au BufRead */*.[^-]xt

au BufRead */*.[-t-]xt
au BufRead */*.[^-t-]xt

au BufRead */*.[\^]xt
au BufRead */*.[^^]xt

au BufRead */*.txt,*/*.vim
au BufRead */*.{txt,vim}
au BufRead */*.{t{x,t},v{i,m}}

" literal
au BufRead */*.[]xt
au BufRead */*.[\]xt
au BufRead */*.[^]xt
au BufRead */*.[^\]xt

