" Vim :uniq command
" VIM_TEST_SETUP hi link vimCmdSep Operator


uniq 
uniq!

uniq  ilu
uniq! ilu

uniq  /pa\%(tt\)ern/ ilu
uniq! /pa\%(tt\)ern/ ilu

uniq  /pa\%(tt\)ern/ rilu
uniq! /pa\%(tt\)ern/ rilu

uniq  ilu /pa\%(tt\)ern/
uniq! ilu /pa\%(tt\)ern/

uniq  rilu /pa\%(tt\)ern/
uniq! rilu /pa\%(tt\)ern/

uniq  | echo "Foo"
uniq! | echo "Foo"

uniq  /pa\%(t|t\)ern/ rilu | echo "Foo"
uniq! /pa\%(t|t\)ern/ rilu | echo "Foo"

uniq  rilu /pa\%(t|t\)ern/ | echo "Foo"
uniq! rilu /pa\%(t|t\)ern/ | echo "Foo"

