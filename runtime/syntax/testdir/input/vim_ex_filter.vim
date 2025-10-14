" Vim :filter command
" VIM_TEST_SETUP hi link vimCmdSep Operator


filter  pa\%(tt\)ern  oldfiles
filter! pa\%(tt\)ern  oldfiles

filter  /pa\%(tt\)ern/ oldfiles
filter! /pa\%(tt\)ern/ oldfiles

filter  !pa\%(tt\)ern! oldfiles
filter! !pa\%(tt\)ern! oldfiles

filter  /pa\%(t|t\)ern/ oldfiles | echo "Foo"
filter! /pa\%(t|t\)ern/ oldfiles | echo "Foo"

