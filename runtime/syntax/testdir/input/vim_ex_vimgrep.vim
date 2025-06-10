" Vim :*vimgrep and :*vimgrepadd commands
" VIM_TEST_SETUP highlight link vimVimgrepFile Todo
" VIM_TEST_SETUP highlight link vimCmdSep Operator
" VIM_TEST_SETUP highlight link vimVimgrepBarEscape Special


vimgrep pa\%(tt\)ern   foo.txt
vimgrep /pa\%(tt\)ern/ foo.txt

vimgrep! pa\%(tt\)ern   foo.txt
vimgrep! /pa\%(tt\)ern/ foo.txt

vimgrep /pa\%(tt\)ern/gjf foo.txt

" trailing bar, no tail comment
vimgrep /pa\%(t|t\)ern/ b\|a\|r.txt | echo "Foo"
vimgrep /pa\%(t"t\)ern/ b"a"r.txt   | echo "Foo"

vimgrepadd pa\%(tt\)ern   foo.txt
vimgrepadd /pa\%(tt\)ern/ foo.txt

vimgrepadd! pa\%(tt\)ern   foo.txt
vimgrepadd! /pa\%(tt\)ern/ foo.txt

vimgrepadd /pa\%(tt\)ern/gjf foo.txt

" trailing bar, no tail comment
vimgrepadd /pa\%(t|t\)ern/ b\|a\|r.txt | echo "Foo"
vimgrepadd /pa\%(t"t\)ern/ b"a"r.txt   | echo "Foo"

lvimgrep pa\%(tt\)ern   foo.txt
lvimgrep /pa\%(tt\)ern/ foo.txt

lvimgrep! pa\%(tt\)ern   foo.txt
lvimgrep! /pa\%(tt\)ern/ foo.txt

lvimgrep /pa\%(tt\)ern/gjf foo.txt

" trailing bar, no tail comment
lvimgrep /pa\%(t|t\)ern/ b\|a\|r.txt | echo "Foo"
lvimgrep /pa\%(t"t\)ern/ b"a"r.txt   | echo "Foo"

lvimgrepadd pa\%(tt\)ern   foo.txt
lvimgrepadd /pa\%(tt\)ern/ foo.txt

lvimgrepadd! pa\%(tt\)ern   foo.txt
lvimgrepadd! /pa\%(tt\)ern/ foo.txt

lvimgrepadd /pa\%(tt\)ern/gjf foo.txt

" trailing bar, no tail comment
lvimgrepadd /pa\%(t|t\)ern/ b\|a\|r.txt | echo "Foo"
lvimgrepadd /pa\%(t"t\)ern/ b"a"r.txt   | echo "Foo"


" file args

" multiple files
vimgrep pa\%(tt\)ern   foo.txt bar.txt
vimgrep /pa\%(tt\)ern/ foo.txt bar.txt

" wildcard patterns
vimgrep pa\%(tt\)ern   **/*.txt
vimgrep /pa\%(tt\)ern/ **/*.txt

" special filename characters
vimgrep pa\%(tt\)ern   %
vimgrep /pa\%(tt\)ern/ %

