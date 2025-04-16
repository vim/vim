" Vim :*grep and :*grepadd commands
" VIM_TEST_SETUP hi link vimCmdSep Operator
" VIM_TEST_SETUP hi link vimGrepBarEscape Special


grep  'pat\|tern' foo.txt
lgrep 'pat\|tern' foo.txt

grep!  'pat\|tern' foo.txt
lgrep! 'pat\|tern' foo.txt

grepadd  'pat\|tern' foo.txt
lgrepadd 'pat\|tern' foo.txt

grepadd!  'pat\|tern' foo.txt
lgrepadd! 'pat\|tern' foo.txt


" special filename characters

grep  'pat\|tern' %
lgrep 'pat\|tern' %

grep!  'pat\|tern' %
lgrep! 'pat\|tern' %

grepadd!  'pat\|tern' %
lgrepadd! 'pat\|tern' %

grepadd!  'pat\|tern' %
lgrepadd! 'pat\|tern' %


" trailing bar, no tail comment

grep  'pat\|tern' foo.txt | echo "Foo"
lgrep 'pat\|tern' foo.txt | echo "Foo"

grep!  'pat\|tern' foo.txt | echo "Foo"
lgrep! 'pat\|tern' foo.txt | echo "Foo"

grepadd  'pat\|tern' foo.txt | echo "Foo"
lgrepadd 'pat\|tern' foo.txt | echo "Foo"

grepadd!  'pat\|tern' foo.txt | echo "Foo"
lgrepadd! 'pat\|tern' foo.txt | echo "Foo"

