" Vim :echo commands


echo        "Answer = " 42
echon       "Answer = " 42
echomsg     "Answer = " 42
echowindow  "Answer = " 42
echoerr     "Answer = " 42
echoconsole "Answer = " 42


" Trailing bar vs OR operator

" OR operator
echo foo || bar
echo foo ||
      \ bar

" following command is :|
echo "Foo" | |

" invalid expression
echo "Foo" ||

echohl WarningMsg | echo "Don't panic!" | echohl None


" Line continuations

echo "Answer = "
     "\ comment
      \ 42

echo
      \ "Answer = "
     "\ comment
      \ 42


" Trailing bar and comments

" :echo without {expr}
echo| echo "Foo"

" trailing comment needs |
echo "Foo" | " comment


" Issue #9987 (parenthesised argument - not a function call)

let foo = {'end': 123}

if 123
	echo (foo.end)
else
	echo 'bar'
endif

