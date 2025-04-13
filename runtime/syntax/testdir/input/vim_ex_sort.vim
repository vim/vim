" Vim :sort command
" VIM_TEST_SETUP highlight link vimCmdSep Operator


sort 
sort!

sort  ilu
sort! ilu

sort  /pa\%(tt\)ern/ ilu
sort! /pa\%(tt\)ern/ ilu

sort  /pa\%(tt\)ern/ rilu
sort! /pa\%(tt\)ern/ rilu

sort  nilu
sort! nilu
sort  filu
sort! filu
sort  xilu
sort! xilu
sort  oilu
sort! oilu
sort  bilu
sort! bilu

sort  /pa\%(tt\)ern/ nilu
sort! /pa\%(tt\)ern/ nilu
sort  /pa\%(tt\)ern/ filu
sort! /pa\%(tt\)ern/ filu
sort  /pa\%(tt\)ern/ xilu
sort! /pa\%(tt\)ern/ xilu
sort  /pa\%(tt\)ern/ oilu
sort! /pa\%(tt\)ern/ oilu
sort  /pa\%(tt\)ern/ bilu
sort! /pa\%(tt\)ern/ bilu

sort  /pa\%(tt\)ern/ rnilu
sort! /pa\%(tt\)ern/ rnilu
sort  /pa\%(tt\)ern/ rfilu
sort! /pa\%(tt\)ern/ rfilu
sort  /pa\%(tt\)ern/ rxilu
sort! /pa\%(tt\)ern/ rxilu
sort  /pa\%(tt\)ern/ roilu
sort! /pa\%(tt\)ern/ roilu
sort  /pa\%(tt\)ern/ rbilu
sort! /pa\%(tt\)ern/ rbilu

sort  | echo "Foo"
sort! | echo "Foo"

sort  /pa\%(t|t\)ern/ rilu | echo "Foo"
sort! /pa\%(t|t\)ern/ rilu | echo "Foo"

