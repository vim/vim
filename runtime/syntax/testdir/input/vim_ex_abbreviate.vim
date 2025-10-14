" Vim :abbreviate commands
" VIM_TEST_SETUP hi link vimMapLhs Identifier
" VIM_TEST_SETUP hi link vimMapRhs Todo
" VIM_TEST_SETUP hi link vimMapRhsContinue Todo


abbrev <buffer> foo foobar
cabbrev <buffer> cfoo cfoobar
iabbrev <buffer> ifoo ifoobar

abbrev <expr> <buffer> foo foobar
cabbrev <expr> <buffer> cfoo cfoobar
iabbrev <expr> <buffer> ifoo ifoobar

noreabbrev <buffer> foo foobar
cnoreabbrev <buffer> cfoo cfoobar
inoreabbrev <buffer> ifoo ifoobar

abbrev <expr> <buffer> foo foobar
cabbrev <expr> <buffer> cfoo cfoobar
iabbrev <expr> <buffer> ifoo ifoobar

unabbrev <buffer> foo
cunabbrev <buffer> cfoo
iunabbrev <buffer> ifoo

abclear <buffer>
cabclear <buffer>
iabclear <buffer>


" Multiline RHS

abbrev foo
      \ foobar

abbrev foo
      \
      \ foobar

abbrev foo
      "\ comment
      \ foobar

abbrev foo
      "\ comment
      \ foo
      "\ comment
      \bar

abbrev lhs
  "\ comment (matches as RHS but harmless)
echo "clear"

