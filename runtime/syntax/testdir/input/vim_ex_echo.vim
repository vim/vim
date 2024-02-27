" Vim :echo commands

echo        "Answer = " 42
echon       "Answer = " 42
echomsg     "Answer = " 42
echowindow  "Answer = " 42
echoerr     "Answer = " 42
echoconsole "Answer = " 42

echo "following command is :|" | |

echohl WarningMsg | echo "Don't panic!" | echohl None

echo "Answer = "
     "\ comment
      \ 42

" :echo without {expr}
echo| echo "Foo"

