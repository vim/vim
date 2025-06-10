" Vim :*make commands
" VIM_TEST_SETUP highlight link vimCmdSep Operator
" VIM_TEST_SETUP highlight link vimMakeBarEscape Special


make
lmake

make!
lmake!

make  -options target
lmake -options target

make!  -options target
lmake! -options target


" special filename characters

make  %
lmake %


" trailing bar, no tail comment

make  tar\|get | echo "Foo"
lmake tar\|get | echo "Foo"

make!  tar\|get | echo "Foo"
lmake! tar\|get | echo "Foo"

make  | echo "Foo"
lmake | echo "Foo"

make!  | echo "Foo"
lmake! | echo "Foo"

